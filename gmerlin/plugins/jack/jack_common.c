/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <string.h>

#include "jack_common.h"

#define LOG_DOMAIN "jack"


static const bg_parameter_info_t parameters[] =
  {
    {
      .name = "connect_ports",
      .long_name = TRS("Connect ports"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Autoconnect ports"),
    },
    { /* End */ },
  };

const bg_parameter_info_t *
bg_jack_get_parameters(void * p)
  {
  return parameters;
  }

/* Set parameter */

void
bg_jack_set_parameter(void * p, const char * name,
                      const bg_parameter_value_t * val)
  {
  jack_t * priv = p;
  if(!name)
    return;

  if(!strcmp(name, "connect_ports"))
    priv->connect_ports = val->val_i;
  }

void * bg_jack_create()
  {
  jack_t * ret = calloc(1, sizeof(*ret));
  
  pthread_mutex_init(&ret->running_mutex, NULL);
  pthread_mutex_init(&ret->active_mutex, NULL);
  
  return ret;
  }

int bg_jack_start(void * data)
  {
  jack_t * priv = data;
  
  pthread_mutex_lock(&priv->active_mutex);
  priv->active = 1;
  pthread_mutex_unlock(&priv->active_mutex);
  
  return 1;
  }

void bg_jack_stop(void * data)
  {
  jack_t * priv = data;
  pthread_mutex_lock(&priv->active_mutex);
  priv->active = 0;
  pthread_mutex_unlock(&priv->active_mutex);
  }

static void create_channel_map(jack_t * priv, int output)
  {
  int i;

  int flags = JackPortIsPhysical;

  if(output)
    flags |= JackPortIsInput;
  else
    flags |= JackPortIsOutput;

  priv->ext_ports =
    jack_get_ports(priv->client, NULL, NULL, flags);
  
  /* Count ports */
  priv->num_ports = 0;
  while(priv->ext_ports[priv->num_ports])
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

  flags = JackPortIsTerminal;

  if(output)
    flags |= JackPortIsOutput;
  else
    flags |= JackPortIsInput;
  
  /* Set ports and buffers */
  for(i = 0; i < priv->num_ports; i++)
    {
    priv->ports[i].ext_name = priv->ext_ports[i];

    priv->ports[i].int_port =
      jack_port_register(priv->client,
                         gavl_channel_id_to_string(priv->ports[i].channel_id),
                         JACK_DEFAULT_AUDIO_TYPE,
                         flags, 0);
    priv->ports[i].buffer =
      jack_ringbuffer_create(sizeof(float)*priv->samples_per_frame*4);
    }
  
  }

static void connect_ports(jack_t * priv, int output)
  {
  int i;
  const char * src;
  const char * dst;

  for(i = 0; i < priv->num_ports; i++)
    {
    if(output)
      {
      src = jack_port_name(priv->ports[i].int_port);
      dst = priv->ports[i].ext_name;
      }
    else
      {
      src = priv->ports[i].ext_name;
      dst = jack_port_name(priv->ports[i].int_port);
      }
    
    if(jack_connect(priv->client, src, dst))
      bg_log(BG_LOG_WARNING, LOG_DOMAIN,
             "Connecting %s with %s failed", src, dst);
    }
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

int bg_jack_open_client(jack_t * priv, int output,
                        int (*process)(jack_nframes_t, void *))
  {
  priv->client = jack_client_open((output ?
                                   "gmerlin-output" : "gmerlin-input"),
                                  0, NULL);
  
  if(!priv->client)
    return 0;
  /* Set callbacks */
  jack_set_process_callback(priv->client, process, priv);
  jack_on_shutdown(priv->client, jack_shutdown, priv);

  priv->samples_per_frame = jack_get_buffer_size(priv->client);
  priv->samplerate = jack_get_sample_rate(priv->client);
  
  create_channel_map(priv, output);

  if(jack_activate(priv->client))
    return 0;
  
  pthread_mutex_lock(&priv->running_mutex);
  priv->running = 1;
  pthread_mutex_unlock(&priv->running_mutex);
  
  if(priv->connect_ports)
    {
    connect_ports(priv, output);
    }
  
  return 1;
  }

static void destroy_channel_map(jack_t * priv)
  {
  int i;
  for(i = 0; i < priv->num_ports; i++)
    {
    jack_port_unregister(priv->client,
                         priv->ports[i].int_port);
    jack_ringbuffer_free(priv->ports[i].buffer);
    }

  if(priv->ports)
    free(priv->ports);
  if(priv->ext_ports)
    free(priv->ext_ports);
  
  }


int bg_jack_close_client(jack_t * priv)
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

void bg_jack_destroy(void * p)
  {
  jack_t * priv = p;
  
  /* Stop thread and close client connection */
  if(priv->client)
    bg_jack_close_client(priv);
  
  pthread_mutex_destroy(&priv->running_mutex);
  pthread_mutex_destroy(&priv->active_mutex);
  
  free(priv);
  }
