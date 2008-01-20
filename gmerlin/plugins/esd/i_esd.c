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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <config.h>
#include <translation.h>

#include <plugin.h>
#include <utils.h>

#include <esd.h>

#include <log.h>
#define LOG_DOMAIN "i_esd"


typedef struct
  {
  int esd_socket;
  char * hostname;
  int bytes_per_frame;

  int do_monitor;
  gavl_audio_frame_t * f;
  gavl_audio_format_t format;
  
  int last_frame_size;
  
  int64_t samples_read;
  } esd_t;

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "esd_host",
      .long_name = TRS("Host (empty: local)"),
      .type =      BG_PARAMETER_STRING,
    },
    {
      .name =        "input_mode",
      .long_name =   TRS("Input Mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "record" },
      .multi_names =     (char const *[]){ "record", "monitor", (char*)0 },
      .multi_labels =    (char const *[]){ TRS("Record"), TRS("Monitor"), (char*)0 },
    },
    { /* End of parameters */ }
  };

static void set_parameter_esd(void * data, const char * name,
                              const bg_parameter_value_t * val)
  {
  esd_t * e = (esd_t *)data;

  if(!name)
    return;
  if(!strcmp(name, "esd_host"))
    {
    e->hostname = bg_strdup(e->hostname, val->val_str);
    }
  else if(!strcmp(name, "input_mode"))
    {
    if(!strcmp(val->val_str, "monitor"))
      e->do_monitor = 1;
    else
      e->do_monitor = 0;
    }
  }

static void * create_esd()
  {
  esd_t * ret = calloc(1, sizeof(*ret));

  return ret;
  }

static void destroy_esd(void *data)
  {
  esd_t * e = (esd_t *)data;

  if(e->hostname)
    free(e->hostname);
  free(e);
  }

static const bg_parameter_info_t *
get_parameters_esd(void * priv)
  {
  return parameters;
  }

static int open_esd(void * data,
                    gavl_audio_format_t * format,
                    gavl_video_format_t * video_format)
  {
  int esd_format;
  const char * esd_host;
  char * name;
  char hostname[128];
  
  esd_t * e = (esd_t*)data;

  e->samples_read = 0;
  
  /* Set up format */

  memset(format, 0, sizeof(*format));
    
  format->interleave_mode = GAVL_INTERLEAVE_ALL;
  format->samplerate = 44100;
  format->sample_format = GAVL_SAMPLE_S16;
  format->samples_per_frame = ESD_BUF_SIZE / 4;

  format->num_channels = 2;
  gavl_set_channel_setup(format);
  
  gavl_audio_format_copy(&e->format, format);
  
  e->f = gavl_audio_frame_create(format);
    
  if(!e->hostname || (*(e->hostname) == '\0'))
    {
    esd_host = (const char*)0;
    }
  else
    esd_host = e->hostname;
    
  esd_format = ESD_STREAM | ESD_PLAY;
  if(e->do_monitor)
    esd_format |= ESD_MONITOR;
  else
    esd_format |= ESD_RECORD;
  
  esd_format |= (ESD_STEREO|ESD_BITS16);

  gethostname(hostname, 128);
    
  name = bg_sprintf("gmerlin@%s pid: %d", hostname, getpid());

    
  if(e->do_monitor)
    e->esd_socket = esd_monitor_stream(esd_format, format->samplerate,
                                       e->hostname, name);
  else
    e->esd_socket = esd_record_stream(esd_format, format->samplerate,
                                      e->hostname, name);
  free(name);
  if(e->esd_socket < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot connect to daemon");
    return 0;
    }
  e->bytes_per_frame = 4;
  return 1;
  }

static void close_esd(void * data)
  {
  esd_t * e = (esd_t*)(data);
  esd_close(e->esd_socket);
  gavl_audio_frame_destroy(e->f);
  }

static int read_frame_esd(void * p, gavl_audio_frame_t * f, int stream,
                           int num_samples)
  {
  int samples_read;
  int samples_copied;
  
  esd_t * priv = (esd_t*)(p);
  
  samples_read = 0;

  while(samples_read < num_samples)
    {
    if(!priv->f->valid_samples)
      {
      priv->f->valid_samples = read(priv->esd_socket,
                                    priv->f->samples.s_8,
                                    ESD_BUF_SIZE);
      priv->f->valid_samples /= priv->bytes_per_frame;
      priv->last_frame_size = priv->f->valid_samples;
      }
    
    samples_copied =
      gavl_audio_frame_copy(&priv->format,         /* format  */
                            f,                     /* dst     */
                            priv->f,               /* src     */
                            samples_read,          /* dst_pos */
                            priv->last_frame_size - priv->f->valid_samples, /* src_pos */
                            num_samples - samples_read, /* dst_size */
                            priv->f->valid_samples      /* src_size */ );
    
    priv->f->valid_samples -= samples_copied;
    samples_read += samples_copied;
    }

  if(f)
    {
    f->valid_samples = samples_read;
    f->timestamp = priv->samples_read;
    }
  priv->samples_read += samples_read;
  return samples_read;
  }

const bg_recorder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_esd",
      .long_name =     TRS("EsounD input driver"),
      .description =   TRS("EsounD input driver"),

      .type =          BG_PLUGIN_RECORDER_AUDIO,
      .flags =         BG_PLUGIN_RECORDER,
      .priority =      BG_PLUGIN_PRIORITY_MIN,
      .create =        create_esd,
      .destroy =       destroy_esd,

      .get_parameters = get_parameters_esd,
      .set_parameter =  set_parameter_esd
    },

    .open =                open_esd,
    .read_audio =          read_frame_esd,
    .close =               close_esd,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
