/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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


#define LOG_DOMAIN "i_jack"

// #include "alsa_common.h"

static int jack_process(jack_nframes_t nframes, void *arg)
  {
  int i;
  int samples_written, result;
  jack_t * priv = arg;
  int write_space;
  char *out;

  pthread_mutex_lock(&priv->active_mutex);
  
  //  fprintf(stderr, "Jack Process %d\n", nframes);
  
  /* Check if there is enough space */
  for(i = 0; i < priv->num_ports; i++)
    {
    if(!priv->ports[i].active)
      continue;

    write_space = jack_ringbuffer_write_space(priv->ports[i].buffer);
    if(write_space < nframes * sizeof(float))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Dropping %d samples", nframes);

      pthread_mutex_unlock(&priv->active_mutex);
      return 0;
      }
    }
  
  for(i = 0; i < priv->num_ports; i++)
    {
    if(!priv->ports[i].active)
      continue;
    
    samples_written = 0;

    out = jack_port_get_buffer(priv->ports[i].int_port, nframes);
    
    while(samples_written < nframes)
      {
      result =
        jack_ringbuffer_write(priv->ports[i].buffer,
                              out + samples_written * sizeof(float),
                              (nframes - samples_written) *
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
  pthread_mutex_unlock(&priv->active_mutex);
  return 0;
  }

static void close_jack(void * p)
  {
  jack_t * priv = p;
  bg_jack_close_client(priv);
  priv->client = NULL;
  
  if(priv->src)
    {
    gavl_audio_source_destroy(priv->src);
    priv->src = NULL;
    }
  }

static gavl_source_status_t
read_func_jack(void * p, gavl_audio_frame_t ** frame)
  {
  int i;
  
  int samples_read, result;

  gavl_audio_frame_t * f = *frame;
  jack_t * priv = p;
  
  for(i = 0; i < priv->format.num_channels; i++)
    {
    samples_read = 0;
    while(samples_read < priv->format.samples_per_frame)
      {
      result = jack_ringbuffer_read(priv->ports[i].buffer,
                                    (char*)(f->channels.f[i] + samples_read),
                                    (priv->format.samples_per_frame -
                                     samples_read) * sizeof(float));
      samples_read += result / sizeof(float);
      
      if(samples_read < priv->format.num_channels)
        {
        gavl_time_t delay_time;
        delay_time = gavl_time_unscale(priv->format.samplerate,
                                       priv->format.samples_per_frame -
                                       samples_read);
        gavl_time_delay(&delay_time);
        }
      }
    }

  f->valid_samples = samples_read;
  return GAVL_SOURCE_OK;
  }


static int open_jack(void * data,
                     gavl_audio_format_t * format,
                     gavl_video_format_t * video_format)
  {
  int i;
  jack_t * priv = data;
  if(!priv->client)
    bg_jack_open_client(priv, 0, jack_process);

  /* Copy format */
  gavl_audio_format_copy(&priv->format, format);
  
  priv->format.samplerate = priv->samplerate;
  priv->format.sample_format = GAVL_SAMPLE_FLOAT;
  priv->format.interleave_mode = GAVL_INTERLEAVE_NONE;
  priv->format.samples_per_frame = priv->samples_per_frame;

  /* TODO: Make this configurable */
  priv->format.num_channels = 2;
  
  /* Clear ports */
  for(i = 0; i < priv->num_ports; i++)
    priv->ports[i].active = 0;
  
  /* Setup ports */
  for(i = 0; i < priv->num_ports; i++)
    {
    priv->ports[i].index = i;
    priv->format.channel_locations[i] = priv->ports[i].channel_id;
    priv->ports[i].active = 1;
    jack_ringbuffer_reset(priv->ports[i].buffer);
    }

  gavl_audio_format_copy(format, &priv->format);

  priv->src = gavl_audio_source_create(read_func_jack, priv, 0, format);
  
  priv->samples_read = 0;
  return 1;
  }


static gavl_audio_source_t * get_audio_source_jack(void * p)
  {
  jack_t * priv = p;
  return priv->src;
  }

static int read_samples_jack(void * p, gavl_audio_frame_t * f, int stream,
                             int num_samples)
  {
  jack_t * priv = p;
  return gavl_audio_source_read_samples(priv->src, f, num_samples);
  }


const bg_recorder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "i_jack",
      .long_name =      TRS("Jack"),
      .description =    TRS("Jack recorder"),
      .type =           BG_PLUGIN_RECORDER_AUDIO,
      .flags =          BG_PLUGIN_RECORDER,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .create =         bg_jack_create,
      .destroy =        bg_jack_destroy,

      .get_parameters = bg_jack_get_parameters,
      .set_parameter =  bg_jack_set_parameter,
    },

    .open =             open_jack,
    .get_audio_source = get_audio_source_jack,
    .read_audio =       read_samples_jack,
    .close =            close_jack,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
