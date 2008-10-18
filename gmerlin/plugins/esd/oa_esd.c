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
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <esd.h>

typedef struct
  {
  int esd_socket;
  char * hostname;
  int bytes_per_sample;
  int esd_format;
  int samplerate;
  } esd_t;

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "esd_host",
      .long_name = TRS("Host (empty: local)"),
      .type =      BG_PARAMETER_STRING,
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

static int open_esd(void * data, gavl_audio_format_t * format)
  {
  esd_t * e = (esd_t *)data;
  e->bytes_per_sample = 1;

  format->interleave_mode = GAVL_INTERLEAVE_ALL;
    
  e->esd_format = ESD_STREAM | ESD_PLAY;
  
  /* Delete unwanted channels */

  switch(format->num_channels)
    {
    case 1:
      e->esd_format |= ESD_MONO;
      format->num_channels = 1;
      format->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      break;
    default:
      e->bytes_per_sample *= 2;
      e->esd_format |= ESD_STEREO;
      format->num_channels = 2;
      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      break;
    }
  /* Set bits */
  
  if(gavl_bytes_per_sample(format->sample_format)==1)
    {
    format->sample_format = GAVL_SAMPLE_U8;
    e->esd_format |= ESD_BITS8;
    }
  else
    {
    format->sample_format = GAVL_SAMPLE_S16;
    e->esd_format |= ESD_BITS16;
    e->bytes_per_sample *= 2;
    }

  e->samplerate = format->samplerate;
  
  return 1;
  }

// static int stream_count = 0;

static int start_esd(void * p)
  {
  esd_t * e = (esd_t *)p;
  e->esd_socket = esd_play_stream(e->esd_format,
                                  e->samplerate,
                                  e->hostname,
                                  "gmerlin output");
  if(e->esd_socket < 0)
    return 0;
  return 1;
  }

static void write_esd(void * p, gavl_audio_frame_t * f)
  {
  esd_t * e = (esd_t*)(p);
  write(e->esd_socket, f->channels.s_8[0], f->valid_samples *
        e->bytes_per_sample);
  }

static void close_esd(void * p)
  {

  }

static void stop_esd(void * p)
  {
  esd_t * e = (esd_t*)(p);
  esd_close(e->esd_socket);
  }

static const bg_parameter_info_t *
get_parameters_esd(void * priv)
  {
  return parameters;
  }

const bg_oa_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "oa_esd",
      .long_name =     TRS("EsounD output driver"),
      .description =   TRS("EsounD output driver"),
      .type =          BG_PLUGIN_OUTPUT_AUDIO,
      .flags =         BG_PLUGIN_PLAYBACK,
      .priority =      BG_PLUGIN_PRIORITY_MIN,
      .create =        create_esd,
      .destroy =       destroy_esd,

      .get_parameters = get_parameters_esd,
      .set_parameter =  set_parameter_esd
    },
    .open =          open_esd,
    .start =         start_esd,
    .write_audio =   write_esd,
    .stop =          stop_esd,
    .close =         close_esd,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
