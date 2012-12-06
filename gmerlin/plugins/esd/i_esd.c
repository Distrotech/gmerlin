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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <esd.h>

#include <gmerlin/log.h>
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
  
  gavl_audio_source_t * src;
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
      .multi_names =     (char const *[]){ "record", "monitor", NULL },
      .multi_labels =    (char const *[]){ TRS("Record"), TRS("Monitor"), NULL },
    },
    { /* End of parameters */ }
  };

static void set_parameter_esd(void * data, const char * name,
                              const bg_parameter_value_t * val)
  {
  esd_t * e = data;

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
  esd_t * e = data;

  if(e->hostname)
    free(e->hostname);
  free(e);
  }

static const bg_parameter_info_t *
get_parameters_esd(void * priv)
  {
  return parameters;
  }

static int read_samples_esd(void * p, gavl_audio_frame_t * f, int stream,
                            int num_samples)
  {
  esd_t * priv = p;
  return gavl_audio_source_read_samples(priv->src, f, num_samples);
  }

static gavl_source_status_t
read_func_esd(void * p, gavl_audio_frame_t ** f)
  {
  esd_t * priv = p;

  (*f)->valid_samples = read(priv->esd_socket,
                             (*f)->samples.s_8,
                             ESD_BUF_SIZE);

  if((*f)->valid_samples < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "reading samples failed: %s",
           strerror(errno));
    return GAVL_SOURCE_EOF;
    }
  (*f)->valid_samples /= priv->bytes_per_frame;
  
  return ((*f)->valid_samples ? GAVL_SOURCE_OK : GAVL_SOURCE_EOF);
  }

static int open_esd(void * data,
                    gavl_audio_format_t * format,
                    gavl_video_format_t * video_format)
  {
  int esd_format;
  const char * esd_host;
  char * name;
  char hostname[128];
  
  esd_t * e = data;

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
    esd_host = NULL;
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
  e->src = gavl_audio_source_create(read_func_esd, e,
                                    GAVL_SOURCE_SRC_FRAMESIZE_MAX,
                                    format);
  return 1;
  }

static void close_esd(void * data)
  {
  esd_t * e = data;
  esd_close(e->esd_socket);
  gavl_audio_frame_destroy(e->f);
  if(e->src)
    {
    gavl_audio_source_destroy(e->src);
    e->src = NULL;
    }
  }

static gavl_audio_source_t * get_audio_source_esd(void * p)
  {
  esd_t * priv = p;
  return priv->src;
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
    .read_audio =          read_samples_esd,
    .close =               close_esd,
    .get_audio_source =    get_audio_source_esd,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
