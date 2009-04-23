/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <stdlib.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "oa_jack"

// #include "alsa_common.h"


static const bg_parameter_info_t parameters[] =
  {
    {
      .name = "connect_ports",
      .long_name = TRS("Connect ports"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Connect the available ports to output channels of the sondcard at startup"),
    },
    { /* End */ },
  };

typedef struct
  {
  gavl_channel_id_t channel_id;
  const char * dst_name;
  jack_ringbuffer_t * buffer;
  jack_port_t * src_port;
  int active;
  int index; /* In format */
  } port_t;

typedef struct
  {
  jack_client_t *client;
  
  gavl_audio_format_t format;

  const char ** dst_ports;

  /* Thread is running */
  int running;
  pthread_mutex_t running_mutex;

  /* Do stop */
  int active;
  pthread_mutex_t active_mutex;
  
  int num_ports;
  port_t * ports;

  int samples_per_frame;
  int samplerate;

  int connect_ports;
  
  } jack_t;

static void mute_port(jack_t * priv, jack_nframes_t nframes, int port)
  {
  char *out;
  out = jack_port_get_buffer(priv->ports[port].src_port, nframes);
  memset(out, 0, nframes * sizeof(float));
  }

static int find_port(jack_t * priv, gavl_channel_id_t id)
  {
  int i;
  for(i = 0; i < priv->num_ports; i++)
    {
    if(priv->ports[i].channel_id == id)
      return i;
    }
  return -1;
  }

static int jack_process(jack_nframes_t nframes, void *arg)
  {
  int i;
  jack_t * priv = arg;
  char *out;
  int bytes_read, bytes_to_read, result;
  gavl_time_t delay_time;
  
  //  fprintf(stderr, "jack_process %d\n", nframes);
  
  bytes_to_read = nframes * sizeof(float);

  pthread_mutex_lock(&priv->active_mutex);

  if(!priv->active)
    {
    for(i = 0; i < priv->num_ports; i++)
      mute_port(priv, nframes, i);
    pthread_mutex_unlock(&priv->active_mutex);
    return 0;
    }
  
  for(i = 0; i < priv->num_ports; i++)
    {
    if(priv->ports[i].active)
      {
      bytes_read = 0;
      out = jack_port_get_buffer(priv->ports[i].src_port, nframes);
      
      while(bytes_read < bytes_to_read)
        {
        result = jack_ringbuffer_read(priv->ports[i].buffer, out + bytes_read,
                                      bytes_to_read - bytes_read);
        
        if(result < bytes_to_read - bytes_read)
          {
          fprintf(stderr, "Underflow\n");
          
          delay_time = gavl_time_unscale(priv->format.samplerate,
                                         bytes_to_read - bytes_read - result) / 2;
          gavl_time_delay(&delay_time);
          }
        bytes_read += result;
        }
      }
    else
      {
      mute_port(priv, nframes, i);
      }
    }
  
  pthread_mutex_unlock(&priv->active_mutex);

  return 0;
  }

static void jack_shutdown (void *arg)
  {
  jack_t * priv = arg;
  fprintf(stderr, "Jack shutdown\n");

  pthread_mutex_lock(&priv->running_mutex);
  priv->running = 0;
  pthread_mutex_unlock(&priv->running_mutex);
  
  //  exit (1);
  }

static void create_channel_map(jack_t * priv)
  {
  int i;
  priv->dst_ports =
    jack_get_ports(priv->client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);

  /* Count ports */
  priv->num_ports = 0;
  while(priv->dst_ports[priv->num_ports])
    priv->num_ports++;

  priv->ports = calloc(priv->num_ports, sizeof(*priv->ports));
  
  /* Set channel map */
  
  if(priv->num_ports == 1)
    {
    priv->ports[0].channel_id = GAVL_CHID_FRONT_CENTER;
    }
  if(priv->num_ports >= 2)
    {
    priv->ports[0].channel_id = GAVL_CHID_FRONT_LEFT;
    priv->ports[1].channel_id = GAVL_CHID_FRONT_RIGHT;
    }
  if(priv->num_ports >= 4)
    {
    priv->ports[2].channel_id = GAVL_CHID_REAR_LEFT;
    priv->ports[3].channel_id = GAVL_CHID_REAR_RIGHT;
    }
  if(priv->num_ports >= 5)
    {
    priv->ports[4].channel_id = GAVL_CHID_FRONT_CENTER;
    }
  if(priv->num_ports >= 6)
    {
    priv->ports[5].channel_id = GAVL_CHID_LFE;
    }
  if(priv->num_ports >= 8)
    {
    priv->ports[6].channel_id = GAVL_CHID_SIDE_LEFT;
    priv->ports[7].channel_id = GAVL_CHID_SIDE_RIGHT;
    }

  /* Set ports and buffers */
  for(i = 0; i < priv->num_ports; i++)
    {
    priv->ports[i].dst_name = priv->dst_ports[i];

    priv->ports[i].src_port =
      jack_port_register(priv->client,
                         gavl_channel_id_to_string(priv->ports[i].channel_id),
                         JACK_DEFAULT_AUDIO_TYPE,
                         JackPortIsOutput|JackPortIsTerminal, 0);
    priv->ports[i].buffer =
      jack_ringbuffer_create(sizeof(float)*priv->samples_per_frame*4);
    }
  
  }

static void destroy_channel_map(jack_t * priv)
  {
  int i;
  for(i = 0; i < priv->num_ports; i++)
    {
    jack_port_unregister(priv->client,
                         priv->ports[i].src_port);
    jack_ringbuffer_free(priv->ports[i].buffer);
    }

  if(priv->ports)
    free(priv->ports);
  if(priv->dst_ports)
    free(priv->dst_ports);
  
  }

static void connect_ports(jack_t * priv)
  {
  int i;
  for(i = 0; i < priv->num_ports; i++)
    {
    if(priv->connect_ports)
      {
      if(jack_connect(priv->client,
                      jack_port_name(priv->ports[i].src_port),
                      priv->ports[i].dst_name))
        bg_log(BG_LOG_WARNING, LOG_DOMAIN,
               "Connecting %s with %s failed",
               jack_port_name(priv->ports[i].src_port),
               priv->ports[i].dst_name);
      }
    }
  
  }

static int open_client(jack_t * priv)
  {
  priv->client = jack_client_open("gmerlin-output",
                                 0, NULL);

  if(!priv->client)
    return 0;
  /* Set callbacks */
  jack_set_process_callback(priv->client, jack_process, priv);
  jack_on_shutdown(priv->client, jack_shutdown, priv);

  priv->samples_per_frame = jack_get_buffer_size(priv->client);
  priv->samplerate = jack_get_sample_rate(priv->client);
  
  create_channel_map(priv);

  if(jack_activate(priv->client))
    return 0;
  
  pthread_mutex_lock(&priv->running_mutex);
  priv->running = 1;
  pthread_mutex_unlock(&priv->running_mutex);
  
  if(priv->connect_ports)
    {
    connect_ports(priv);
    }
  
  return 1;

  }

static int close_client(jack_t * priv)
  {
  int running;
  gavl_time_t delay_time;
  delay_time = GAVL_TIME_SCALE / 100; /* 10 ms */

  jack_deactivate(priv->client);
  
  while(1)
    {
    pthread_mutex_lock(&priv->running_mutex);
    running = priv->running;
    pthread_mutex_unlock(&priv->running_mutex);

    if(running)
      gavl_time_delay(&delay_time);
    else
      break;
    }


  destroy_channel_map(priv);
  return 1;
  }

static void * create_jack()
  {
  jack_t * ret = calloc(1, sizeof(*ret));
  
  pthread_mutex_init(&ret->running_mutex, NULL);
  pthread_mutex_init(&ret->active_mutex, NULL);
  
  return ret;
  }

static int start_jack(void * data)
  {
  jack_t * priv = (jack_t*)(data);
  
  pthread_mutex_lock(&priv->active_mutex);
  priv->active = 1;
  pthread_mutex_unlock(&priv->active_mutex);
  
  return 1;
  }

static void stop_jack(void * data)
  {
  jack_t * priv = (jack_t*)(data);
  pthread_mutex_lock(&priv->active_mutex);
  priv->active = 0;
  pthread_mutex_unlock(&priv->active_mutex);
  }

static void setup_channel(jack_t * priv, gavl_channel_id_t id)
  {
  int index;

  index = find_port(priv, id);
  if(index < 0)
    return;
  
  priv->ports[index].index = priv->format.num_channels;
  priv->format.channel_locations[priv->format.num_channels] = id;
  priv->ports[index].active = 1;
  priv->format.num_channels++;

  jack_ringbuffer_reset(priv->ports[index].buffer);
  
  }

static int open_jack(void * data, gavl_audio_format_t * format)
  {
  int i;
  int num_front_channels;
  int num_rear_channels;
  int num_lfe_channels;
  
  jack_t * priv = data;

  if(!priv->client)
    open_client(priv);

  /* Copy format */
  gavl_audio_format_copy(&priv->format, format);
  
  priv->format.samplerate = priv->samplerate;
  priv->format.sample_format = GAVL_SAMPLE_FLOAT;
  priv->format.interleave_mode = GAVL_INTERLEAVE_NONE;
  priv->format.samples_per_frame = priv->samples_per_frame;
  
  /* Clear ports */
  for(i = 0; i < priv->num_ports; i++)
    priv->ports[i].active = 0;
  
  /* Setup ports */
  
  num_front_channels = gavl_front_channels(format);
  num_rear_channels = gavl_rear_channels(format);
  num_lfe_channels = gavl_lfe_channels(format);

  priv->format.num_channels = 0;

  switch(num_front_channels)
    {
    case 2:
      setup_channel(priv, GAVL_CHID_FRONT_LEFT); 
      setup_channel(priv, GAVL_CHID_FRONT_RIGHT);
      break;
    case 1:
    case 3:
      setup_channel(priv, GAVL_CHID_FRONT_CENTER);
      setup_channel(priv, GAVL_CHID_FRONT_LEFT); 
      setup_channel(priv, GAVL_CHID_FRONT_RIGHT);
      break;
    }
  
  if(num_rear_channels)
    {
    setup_channel(priv, GAVL_CHID_REAR_LEFT); 
    setup_channel(priv, GAVL_CHID_REAR_RIGHT);
    }
  if(num_lfe_channels)
    setup_channel(priv, GAVL_CHID_LFE);

  gavl_audio_format_copy(format, &priv->format);

  return 1;
  }

static void close_jack(void * p)
  {
  int i;
  jack_t * priv = (jack_t*)(p);
  
  //  stop_jack(p);
  
  }

static void write_frame_jack(void * p, gavl_audio_frame_t * f)
  {
  int i;
  int samples_written, result;
  gavl_time_t delay_time;
  jack_t * priv = (jack_t*)(p);
  int write_space;
  
  //  fprintf(stderr, "Write jack %d\n", f->valid_samples);
  
  for(i = 0; i < priv->num_ports; i++)
    {
    if(!priv->ports[i].active)
      continue;
    
    samples_written = 0;

    while(1)
      {
      write_space = jack_ringbuffer_write_space(priv->ports[i].buffer);
      if(write_space >= f->valid_samples * sizeof(float))
        break;
      
      delay_time = gavl_time_unscale(priv->format.samplerate,
                                     f->valid_samples - write_space / sizeof(float)) / 2;
      gavl_time_delay(&delay_time);
      }
    
    while(samples_written < f->valid_samples)
      {
      result =
        jack_ringbuffer_write(priv->ports[i].buffer,
                              (const char*)(f->channels.f[priv->ports[i].index] +
                                            samples_written),
                              (f->valid_samples - samples_written) *
                              sizeof(float));
#if 0
      if(i)
        fprintf(stderr, "Write %d %d %ld\n", samples_written,
                f->valid_samples - samples_written, result);

#endif
      result /= sizeof(float);

      samples_written += result;
      
      }
    
    }
  
  }

static void destroy_jack(void * p)
  {
  jack_t * priv = (jack_t*)(p);

  /* Stop thread and close client connection */
  if(priv->client)
    close_client(priv);
  
  pthread_mutex_destroy(&priv->running_mutex);
  pthread_mutex_destroy(&priv->active_mutex);

  
  
  free(priv);
  }

static const bg_parameter_info_t *
get_parameters_jack(void * p)
  {
  return parameters;
  }

static int get_delay_jack(void * p)
  {
  jack_t * priv;
  priv = p;
  
  return jack_port_get_latency(priv->ports[0].src_port);
  }
/* Set parameter */

static void
set_parameter_jack(void * p, const char * name,
                   const bg_parameter_value_t * val)
  {
  jack_t * priv = (jack_t*)(p);
  if(!name)
    return;

  if(!strcmp(name, "connect_ports"))
    priv->connect_ports = val->val_i;
  }

const bg_oa_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "oa_jack",
      .long_name =     TRS("Jack"),
      .description =   TRS("Jack output plugin"),
      .type =          BG_PLUGIN_OUTPUT_AUDIO,
      .flags =         BG_PLUGIN_PLAYBACK,
      .priority =      BG_PLUGIN_PRIORITY_MAX-1,
      .create =        create_jack,
      .destroy =       destroy_jack,
      
      .get_parameters = get_parameters_jack,
      .set_parameter =  set_parameter_jack,
    },

    .open =          open_jack,
    .start =         start_jack,
    .write_audio =   write_frame_jack,
    .stop =          stop_jack,
    .close =         close_jack,
    .get_delay =     get_delay_jack,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
