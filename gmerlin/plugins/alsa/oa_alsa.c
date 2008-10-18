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

#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "oa_alsa"

#include "alsa_common.h"

/* Playback modes */

#define PLAYBACK_NONE       0
#define PLAYBACK_GENERIC    1
#define PLAYBACK_USER       2
#define PLAYBACK_SURROUND40 3
#define PLAYBACK_SURROUND41 4
#define PLAYBACK_SURROUND50 5
#define PLAYBACK_SURROUND51 6


static const bg_parameter_info_t global_parameters[] =
  {
    {
      .name =        "surround40",
      .long_name =   TRS("Enable 4.0 Surround"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Use the surround 4.0 (aka quadrophonic) device")
    },
    {
      .name =        "surround41",
      .long_name =   TRS("Enable 4.1 Surround"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Use the surround 4.1 device")
    },
    {
      .name =        "surround50",
      .long_name =   TRS("Enable 5.0 Surround"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Use the surround 5.0 device")
    },
    {
      .name =        "surround51",
      .long_name =   TRS("Enable 5.1 Surround"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Use the surround 5.1 device")
    },
    {
      .name =        "user_device",
      .long_name =   TRS("User device"),
      .type =        BG_PARAMETER_STRING,
      .help_string = TRS("Enter a custom device to use for playback. Leave empty to use the\
 settings above"),
    },
    {
      .name =        "buffer_time",
      .long_name =   TRS("Buffer time"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 10    },
      .val_max =     { .val_i = 10000 },
      .val_default = { .val_i = 1000  },
      .help_string = TRS("Set the buffer time (in milliseconds). Larger values \
improve playback performance on slow systems under load. Smaller values \
decrease the latency of the volume control."),
    },
  };

static const int num_global_parameters =
  sizeof(global_parameters)/sizeof(global_parameters[0]);

typedef struct
  {
  bg_parameter_info_t * parameters;
  gavl_audio_format_t format;
  snd_pcm_t * pcm;

  /* Configuration stuff */

  int enable_surround40;
  int enable_surround41;
  int enable_surround50;
  int enable_surround51;
  
  //  int card_index;
  char * card;
  
  char * user_device;
  int convert_4_3;
  uint8_t * convert_buffer;
  int convert_buffer_alloc;
  
  gavl_time_t buffer_time;
  } alsa_t;

static void convert_4_to_3(alsa_t * priv, gavl_audio_frame_t * frame)
  {
  int i, imax;
  uint8_t * src, * dst;
  
  imax = frame->valid_samples * priv->format.num_channels;

  if(imax * 3 < priv->convert_buffer_alloc)
    {
    priv->convert_buffer_alloc = (imax+1024) * 3;
    priv->convert_buffer = realloc(priv->convert_buffer, priv->convert_buffer_alloc);
    }

  dst = priv->convert_buffer;
  src = frame->samples.u_8;
  
  for(i = 0; i < imax; i++)
    {
#ifndef WORDS_BIGENDIAN
    dst[0] = src[1];
    dst[1] = src[2];
    dst[2] = src[3];
#else
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
#endif
    }
  }

static void * create_alsa()
  {
  int i;
  alsa_t * ret = calloc(1, sizeof(*ret));
  
  ret->parameters = calloc(num_global_parameters+2, sizeof(*ret->parameters));
  
  bg_alsa_create_card_parameters(ret->parameters, 0);
  
  for(i = 0; i < num_global_parameters; i++)
    {
    bg_parameter_info_copy(&(ret->parameters[i+1]), &global_parameters[i]);
    }
  
  return ret;
  }

static int start_alsa(void * data)
  {
  alsa_t * priv = (alsa_t*)(data);

  if(snd_pcm_prepare(priv->pcm) < 0)
    return 0;
  snd_pcm_start(priv->pcm);
  return 1;
  }

static void stop_alsa(void * data)
  {
  alsa_t * priv = (alsa_t*)(data);
  snd_pcm_drop(priv->pcm);

  }

static int open_alsa(void * data, gavl_audio_format_t * format)
  {
  int playback_mode;
  int num_front_channels;
  int num_rear_channels;
  int num_lfe_channels;
  const char * card = (char*)0;
  alsa_t * priv = (alsa_t*)(data);

  //  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Opening");
  
  /* Figure out the right channel setup */
  
  num_front_channels = gavl_front_channels(format);
  num_rear_channels = gavl_rear_channels(format);
  num_lfe_channels = gavl_lfe_channels(format);

  if(priv->user_device)
    {
    playback_mode = PLAYBACK_USER;
    }
  else
    {
    playback_mode = PLAYBACK_NONE;
  
    if(num_front_channels > 2)
      {
      if(num_lfe_channels)
        {
        if(priv->enable_surround51)
          playback_mode = PLAYBACK_SURROUND51;
        else if(priv->enable_surround50)
          playback_mode = PLAYBACK_SURROUND50;
        }
      else if(priv->enable_surround50)
        playback_mode = PLAYBACK_SURROUND50;
      }
  
    else if((playback_mode == PLAYBACK_NONE) && num_rear_channels)
      {
      if(num_lfe_channels)
        {
        if(priv->enable_surround41)
          playback_mode = PLAYBACK_SURROUND41;
        else if(priv->enable_surround40)
          playback_mode = PLAYBACK_SURROUND40;
        }
      else if(priv->enable_surround40)
        playback_mode = PLAYBACK_SURROUND40;
      }

    if(playback_mode == PLAYBACK_NONE)
      playback_mode = PLAYBACK_GENERIC;
    }
  
  switch(playback_mode)
    {
    case PLAYBACK_GENERIC:
      if(format->num_channels > 2)
        format->num_channels = 2;
      format->channel_locations[0] = GAVL_CHID_NONE;
      gavl_set_channel_setup(format);

      card = priv->card;


      break;
    case PLAYBACK_USER:
      format->channel_locations[0] = GAVL_CHID_NONE;
      gavl_set_channel_setup(format);
      card = priv->user_device;
      break;
    case PLAYBACK_SURROUND40:
      format->num_channels = 4;

      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;

      card = bg_sprintf("surround40");

      
      break;
    case PLAYBACK_SURROUND41:
      format->num_channels = 5;

      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
      format->channel_locations[4] = GAVL_CHID_LFE;

      card = bg_sprintf("surround41");

      
      break;
    case PLAYBACK_SURROUND50:
      format->num_channels = 5;

      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
      format->channel_locations[4] = GAVL_CHID_FRONT_CENTER;

      card = bg_sprintf("surround50");
      
      break;
    case PLAYBACK_SURROUND51:
      format->num_channels = 6;

      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
      format->channel_locations[4] = GAVL_CHID_FRONT_CENTER;
      format->channel_locations[5] = GAVL_CHID_LFE;

      card = bg_sprintf("surround51");
      break;
    }

  if(!card)
    card = "default";
  
  priv->pcm = bg_alsa_open_write(card, format,
                                 priv->buffer_time, &priv->convert_4_3);
  
  if(!priv->pcm)
    return 0;
  gavl_audio_format_copy(&(priv->format), format);
  return 1;
  }

static void close_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);

  //  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Closing");

  if(priv->pcm)
    {
    snd_pcm_close(priv->pcm);
    priv->pcm = NULL;
    }
  }

static void write_frame_alsa(void * p, gavl_audio_frame_t * f)
  {
  int result = -EPIPE;
  alsa_t * priv = (alsa_t*)(p);

  if(priv->convert_4_3)
    {
    convert_4_to_3(priv, f);
    while(result == -EPIPE)
      {
      result = snd_pcm_writei(priv->pcm,
                              priv->convert_buffer,
                              f->valid_samples);

      if(result == -EPIPE)
        {
        //    snd_pcm_drop(priv->pcm);
        if(snd_pcm_prepare(priv->pcm) < 0)
          return;
        }
      }
    }
  else
    {
    while(result == -EPIPE)
      {
      result = snd_pcm_writei(priv->pcm,
                              f->samples.s_8,
                              f->valid_samples);

      if(result == -EPIPE)
        {
        //    snd_pcm_drop(priv->pcm);
        if(snd_pcm_prepare(priv->pcm) < 0)
          return;
        }
      }
    }
  
  
  if(result < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_write returned %s", snd_strerror(result));
    }
  }

static void destroy_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  close_alsa(priv);

  if(priv->parameters)
    bg_parameter_info_destroy_array(priv->parameters);
  if(priv->user_device)
    free(priv->user_device);
  if(priv->card)
    free(priv->card);
  free(priv);
  }

static const bg_parameter_info_t *
get_parameters_alsa(void * p)
  {
  alsa_t * priv = (alsa_t*)(p);
  return priv->parameters;
  }


static int get_delay_alsa(void * p)
  {
  int result;
  snd_pcm_sframes_t frames;
  alsa_t * priv;
  priv = (alsa_t*)(p); 
  result = snd_pcm_delay(priv->pcm, &frames);
  if(!result)
    return frames;
  return 0;
  }
/* Set parameter */

static void
set_parameter_alsa(void * p, const char * name,
                   const bg_parameter_value_t * val)
  {
  alsa_t * priv = (alsa_t*)(p);
  if(!name)
    return;

  if(!strcmp(name, "surround40"))
    {
    priv->enable_surround40 = val->val_i;
    }
  else if(!strcmp(name, "surround41"))
    {
    priv->enable_surround41 = val->val_i;
    }
  else if(!strcmp(name, "surround50"))
    {
    priv->enable_surround50 = val->val_i;
    }
  else if(!strcmp(name, "surround51"))
    {
    priv->enable_surround51 = val->val_i;
    }
  else if(!strcmp(name, "user_device"))
    {
    priv->user_device = bg_strdup(priv->user_device, val->val_str);
    }
  else if(!strcmp(name, "buffer_time"))
    {
    priv->buffer_time = val->val_i;
    priv->buffer_time *= (GAVL_TIME_SCALE/1000);
    }
  else if(!strcmp(name, "card"))
    {
    priv->card = bg_strdup(priv->card, val->val_str);
    }
  }

const bg_oa_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "oa_alsa",
      .long_name =     TRS("Alsa"),
      .description =   TRS("Alsa output plugin with support for channel configurations up to 5.1"),
      .type =          BG_PLUGIN_OUTPUT_AUDIO,
      .flags =         BG_PLUGIN_PLAYBACK,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        create_alsa,
      .destroy =       destroy_alsa,
      
      .get_parameters = get_parameters_alsa,
      .set_parameter =  set_parameter_alsa,
    },

    .open =          open_alsa,
    .start =         start_alsa,
    .write_audio =   write_frame_alsa,
    .stop =          stop_alsa,
    .close =         close_alsa,
    .get_delay =     get_delay_alsa,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
