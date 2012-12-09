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
#define LOG_DOMAIN "i_alsa"

#include <gavl/metatags.h>

#include "alsa_common.h"

static const bg_parameter_info_t static_parameters[] =
  {
    {
      .name =        "channel_mode",
      .long_name =   TRS("Channel Mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "stereo" },
      .multi_names =   (char const *[]){ "mono", "stereo", NULL },
      .multi_labels =  (char const *[]){ TRS("Mono"), TRS("Stereo"), NULL },
    },
    {
      .name =        "bits",
      .long_name =   TRS("Bits"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "16" },
      .multi_names =     (char const *[]){ "8", "16",NULL },
    },
    {
      .name =        "samplerate",
      .long_name =   TRS("Samplerate [Hz]"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 44100 },
      .val_min =     { .val_i =  8000 },
      .val_max =     { .val_i = 96000 },
    },
    {
      .name =        "buffer_time",
      .long_name =   TRS("Buffer time"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 10    },
      .val_max =     { .val_i = 10000 },
      .val_default = { .val_i = 1000  },
      .help_string = TRS("Set the buffer time (in milliseconds). Larger values \
improve recording performance on slow systems under load."),
    },
    {
      .name =        "user_device",
      .long_name =   TRS("User device"),
      .type =        BG_PARAMETER_STRING,
      .help_string = TRS("Enter a custom device to use for recording. Leave empty to use the\
 settings above"),
    },
  };

static const int num_static_parameters =
  sizeof(static_parameters)/sizeof(static_parameters[0]);

typedef struct
  {
  bg_parameter_info_t * parameters;
  
  gavl_audio_format_t format;
  int num_channels;
  int bytes_per_sample;
  int samplerate;
  char * card;
  
  snd_pcm_t * pcm;
  
  gavl_time_t buffer_time;
  char * user_device;

  int64_t samples_read;

  gavl_audio_source_t * src; 
  } alsa_t;

static const bg_parameter_info_t *
get_parameters_alsa(void * p)
  {
  int i;
  alsa_t * priv = p;
  if(!priv->parameters)
    {
    priv->parameters = calloc(num_static_parameters + 2,
                              sizeof(*(priv->parameters)));
    
    bg_alsa_create_card_parameters(priv->parameters, 1);
    
    for(i = 0; i < num_static_parameters; i++)
      {
      bg_parameter_info_copy(&priv->parameters[i+1], &static_parameters[i]);
      }
    }
  return priv->parameters;
  }

static void
set_parameter_alsa(void * p, const char * name,
                   const bg_parameter_value_t * val)
  {
  alsa_t * priv = p;
  
  if(!name)
    return;
  
  if(!strcmp(name, "channel_mode"))
    {
    if(!strcmp(val->val_str, "mono"))
      priv->num_channels = 1;
    else if(!strcmp(val->val_str, "stereo"))
      priv->num_channels = 2;
    }
  else if(!strcmp(name, "bits"))
    {
    if(!strcmp(val->val_str, "8"))
      priv->bytes_per_sample = 1;
    else if(!strcmp(val->val_str, "16"))
      priv->bytes_per_sample = 2;
    }
  else if(!strcmp(name, "buffer_time"))
    {
    priv->buffer_time = val->val_i;
    priv->buffer_time *= (GAVL_TIME_SCALE/1000);
    }
  else if(!strcmp(name, "samplerate"))
    {
    priv->samplerate = val->val_i;
    }
  else if(!strcmp(name, "card"))
    {
    priv->card = bg_strdup(priv->card, val->val_str);
    }
  else if(!strcmp(name, "user_device"))
    {
    priv->user_device = bg_strdup(priv->user_device, val->val_str);
    }
  }



static void * create_alsa()
  {
  alsa_t * ret = calloc(1, sizeof(*ret));
    
  return ret;
  }

static gavl_source_status_t read_func_alsa(void * p, gavl_audio_frame_t ** frame)
  {
  int result = 0;
  alsa_t * priv = p;

  gavl_audio_frame_t * f = *frame;
  
  while(1)
    {
    if(priv->format.interleave_mode == GAVL_INTERLEAVE_ALL)
      {
      result = snd_pcm_readi(priv->pcm,
                             f->samples.s_8,
                             priv->format.samples_per_frame);
      }
    else if(priv->format.interleave_mode == GAVL_INTERLEAVE_NONE)
      {
      result = snd_pcm_readn(priv->pcm,
                             (void**)(f->channels.s_8),
                             priv->format.samples_per_frame);
      }
    
    if(result > 0)
      {
      f->valid_samples = result;
      return GAVL_SOURCE_OK;
      }
    else if(result == -EPIPE)
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Dropping samples");
      snd_pcm_drop(priv->pcm);
      if(snd_pcm_prepare(priv->pcm) < 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_prepare failed");
        return GAVL_SOURCE_EOF;
        }
      snd_pcm_start(priv->pcm);
      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Unknown error");
      break;
      }
    }
  return GAVL_SOURCE_EOF;
  }

static char * pcm_card_name(snd_pcm_t * pcm)
  {
  int card;
  snd_pcm_info_t *info;
  char * name = NULL;
  
  snd_pcm_info_malloc(&info);
  if(snd_pcm_info (pcm, info) == 0)
    {
    card = snd_pcm_info_get_card(info);
    if(card >= 0)
      {
      if(snd_card_get_name(card, &name) &&
         snd_card_get_longname(card, &name))
        {
        snd_pcm_info_free(info);
        return NULL;
        }
      }
    }
  snd_pcm_info_free(info);
  return name;
  }

static int open_alsa(void * data,
                     gavl_audio_format_t * format,
                     gavl_video_format_t * video_format, gavl_metadata_t * m)
  {
  const char * card = NULL;
  alsa_t * priv = data;
  
  if(priv->user_device)
    card = priv->user_device;
  else
    card = priv->card;
  
  priv->samples_read = 0;
  
  if(!card)
    card = "default";
  
  memset(format, 0, sizeof(*format));
  
  format->num_channels      = priv->num_channels;
  format->samples_per_frame = 1024;
  
  if(priv->bytes_per_sample == 1)
    format->sample_format = GAVL_SAMPLE_U8;
  else if(priv->bytes_per_sample == 2)
    format->sample_format = GAVL_SAMPLE_S16;
  format->samplerate = priv->samplerate;
    
  priv->pcm = bg_alsa_open_read(card, format, priv->buffer_time);
  
  if(!priv->pcm)
    return 0;

  /* Get the name of the soundcard */
  gavl_metadata_set_nocpy(m, GAVL_META_DEVICE, pcm_card_name(priv->pcm));
  
  gavl_audio_format_copy(&priv->format, format);

  if(snd_pcm_prepare(priv->pcm) < 0)
    return 0;
  snd_pcm_start(priv->pcm);

  priv->src = gavl_audio_source_create(read_func_alsa, priv, 0, format);
  
  return 1;
  }

static void close_alsa(void * p)
  {
  alsa_t * priv = p;
  if(priv->pcm)
    {
    snd_pcm_close(priv->pcm);
    priv->pcm = NULL;
    }
  if(priv->src)
    {
    gavl_audio_source_destroy(priv->src);
    priv->src = NULL;
    }
  }

static int read_samples_alsa(void * p, gavl_audio_frame_t * f,
                           int stream,
                           int num_samples)
  {
  alsa_t * priv = p;
  return gavl_audio_source_read_samples(priv->src, f, num_samples);
  }

static gavl_audio_source_t * get_audio_source_alsa(void * p)
  {
  alsa_t * priv = p;
  return priv->src;
  }

static void destroy_alsa(void * p)
  {
  alsa_t * priv = p;
  if(priv->parameters)
    bg_parameter_info_destroy_array(priv->parameters);
  snd_config_update_free_global();

  free(priv);
  }

const bg_recorder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_alsa",
      .long_name =     TRS("Alsa"),
      .description =   TRS("Alsa recorder"),
      .type =          BG_PLUGIN_RECORDER_AUDIO,
      .flags =         BG_PLUGIN_RECORDER | BG_PLUGIN_DEVPARAM,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        create_alsa,
      .destroy =       destroy_alsa,

      .get_parameters = get_parameters_alsa,
      .set_parameter =  set_parameter_alsa,
    },

    .open =             open_alsa,
    .get_audio_source = get_audio_source_alsa,
    .read_audio =       read_samples_alsa,
    .close =            close_alsa,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
