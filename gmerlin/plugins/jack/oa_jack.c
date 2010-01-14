/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include "jack_common.h"

#include <gmerlin/utils.h>


#define LOG_DOMAIN "oa_jack"

static void mute_port(jack_t * priv, jack_nframes_t nframes, int port)
  {
  char *out;
  out = jack_port_get_buffer(priv->ports[port].int_port, nframes);
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

  pthread_mutex_unlock(&priv->active_mutex);
  
  for(i = 0; i < priv->num_ports; i++)
    {
    if(priv->ports[i].active)
      {
      bytes_read = 0;
      out = jack_port_get_buffer(priv->ports[i].int_port, nframes);
      
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

  return 0;
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
    bg_jack_open_client(priv, 1, jack_process);

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

static int get_delay_jack(void * p)
  {
  jack_t * priv;
  priv = p;
  
  return jack_port_get_latency(priv->ports[0].int_port);
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
      .create =        bg_jack_create,
      .destroy =       bg_jack_destroy,
      
      .get_parameters = bg_jack_get_parameters,
      .set_parameter =  bg_jack_set_parameter,
    },

    .open =          open_jack,
    .start =         bg_jack_start,
    .write_audio =   write_frame_jack,
    .stop =          bg_jack_stop,
    .close =         close_jack,
    .get_delay =     get_delay_jack,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
