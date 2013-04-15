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
#include <errno.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "i_oss"

#include "oss_common.h"

#define SAMPLES_PER_FRAME 1024

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "device",
      .long_name =   TRS("Device"),
      .type =        BG_PARAMETER_DEVICE,
      .val_default = { .val_str = "/dev/dsp" },
    },
    {
      .name =        "channel_mode",
      .long_name =   TRS("Channel Mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "Stereo" },
      .multi_names =     (char const *[]){ "Mono", "Stereo", NULL },
      .multi_labels =     (char const *[]){ TRS("Mono"), TRS("Stereo"), NULL },
    },
    {
      .name =        "bits",
      .long_name =   TRS("Bits"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "16" },
      .multi_names =     (char const *[]){ "8", "16", NULL },
    },
    {
      .name =        "samplerate",
      .long_name =   TRS("Samplerate [Hz]"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 44100 },
      .val_min =     { .val_i =  8000 },
      .val_max =     { .val_i = 96000 },
    },
    { /* End of parameters */ }
  };


typedef struct
  {
  char * device;
  int fd;

  gavl_audio_format_t format;
  
  /* Configuration vairables */
  
  int num_channels;
  int bytes_per_sample;
  int samplerate;

  /* Runtime variables */
    
  int block_align;
  gavl_audio_source_t * src;
  } oss_t;

static const bg_parameter_info_t *
get_parameters_oss(void * priv)
  {
  return parameters;
  }

static void
set_parameter_oss(void * p, const char * name, const bg_parameter_value_t * val)
  {
  oss_t * priv = p;
  char * pos;
  if(!name)
    return;
  if(!strcmp(name, "device"))
    {
    priv->device = gavl_strrep(priv->device, val->val_str);
    pos = strchr(priv->device, ' '); if(pos) *pos = '\0';
    }
  else if(!strcmp(name, "channel_mode"))
    {
    if(!strcmp(val->val_str, "Mono"))
      priv->num_channels = 1;
    else if(!strcmp(val->val_str, "Stereo"))
      priv->num_channels = 2;
    }
  else if(!strcmp(name, "bits"))
    {
    if(!strcmp(val->val_str, "8"))
      priv->bytes_per_sample = 1;
    else if(!strcmp(val->val_str, "16"))
      priv->bytes_per_sample = 2;
    }
  else if(!strcmp(name, "samplerate"))
    {
    priv->samplerate = val->val_i;
    }
  }



static void * create_oss()
  {
  oss_t * ret = calloc(1, sizeof(*ret));
  ret->fd = -1;
  return ret;
  }

static int read_samples_oss(void * p, gavl_audio_frame_t * f, int stream, int num_samples)
  {
  oss_t * priv = p;
  return gavl_audio_source_read_samples(priv->src, f, num_samples);
  }

static gavl_source_status_t read_func_oss(void * p, gavl_audio_frame_t ** frame)
  {
  oss_t * priv = p;
  gavl_audio_frame_t * f = *frame;
  
  f->valid_samples = read(priv->fd,
                          f->samples.s_8,
                          priv->format.samples_per_frame * priv->block_align);

  if(f->valid_samples <= 0)
    return GAVL_SOURCE_EOF;
  f->valid_samples /= priv->block_align;
  return GAVL_SOURCE_OK;
  }

static int open_oss(void * data,
                    gavl_audio_format_t * format,
                    gavl_video_format_t * video_format,
                    gavl_metadata_t * m)
  {
  gavl_sample_format_t sample_format;
  int test_value;
  oss_t * priv = data;

  priv->fd = -1;
  
  memset(format, 0, sizeof(*format));
  
  /* Set up the format */

  format->interleave_mode = GAVL_INTERLEAVE_ALL;
  format->samples_per_frame = 1024;

  switch(priv->bytes_per_sample)
    {
    case 1:
      format->sample_format = GAVL_SAMPLE_U8;
      break;
    case 2:
      format->sample_format = GAVL_SAMPLE_S16;
      break;
    default:
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid number of bits");
      return 0;
    }

  switch(priv->num_channels)
    {
    case 1:
      format->num_channels = 1;
      format->interleave_mode = GAVL_INTERLEAVE_NONE;
      break;
    case 2:
      format->num_channels = 2;
      format->interleave_mode = GAVL_INTERLEAVE_ALL;
      break;
    default:
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid number of channels");
      return 0;
    }
  
  format->samplerate = priv->samplerate;
  format->samples_per_frame = SAMPLES_PER_FRAME;
  format->channel_locations[0] = GAVL_CHID_NONE;
  gavl_set_channel_setup(format);
  
  
  priv->fd = open(priv->device, O_RDONLY, 0);
  if(priv->fd == -1)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s: %s", priv->device,
           strerror(errno));
    goto fail;
    }
  
  sample_format = bg_oss_set_sample_format(priv->fd,
                                           format->sample_format);
  
  if(sample_format == GAVL_SAMPLE_NONE)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot set sampleformat for %s", priv->device);
    goto fail;
    }

  test_value =
    bg_oss_set_channels(priv->fd, format->num_channels);
  if(test_value != format->num_channels)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Device %s doesn't support %d channel sound",
            priv->device, format->num_channels);
    goto fail;
    }

  test_value =
    bg_oss_set_samplerate(priv->fd, format->samplerate);
  if(test_value != format->samplerate)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Samplerate %f kHz not supported by device %s",
              format->samplerate / 1000.0,
              priv->device);
    goto fail;
    }

  priv->block_align = priv->bytes_per_sample * priv->num_channels;

  gavl_audio_format_copy(&priv->format, format);

  priv->src = gavl_audio_source_create(read_func_oss, priv, 0, format);
  
  return 1;
  fail:
  if(priv->fd != -1)
    close(priv->fd);
  return 0;
  }

static void close_oss(void * p)
  {
  oss_t * priv = p;
  
  if(priv->fd != -1)
    {
    close(priv->fd);
    priv->fd = -1;
    }
  if(priv->src)
    {
    gavl_audio_source_destroy(priv->src);
    priv->src = NULL;
    }
  }


static gavl_audio_source_t * get_audio_source_oss(void * p)
  {
  oss_t * priv = p;
  return priv->src;
  }

static void destroy_oss(void * p)
  {
  oss_t * priv = p;

  if(priv->device)
    free(priv->device);
  free(priv);
  }

const bg_recorder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_oss",
      .long_name =     TRS("OSS"),
      .description =   TRS("OSS Recorder"),
      .type =          BG_PLUGIN_RECORDER_AUDIO,
      .flags =         BG_PLUGIN_RECORDER,
      .priority =      5,
      .create =        create_oss,
      .destroy =       destroy_oss,

      .get_parameters = get_parameters_oss,
      .set_parameter =  set_parameter_oss
    },

    .open =             open_oss,
    .read_audio =       read_samples_oss,
    .close  =           close_oss,
    .get_audio_source = get_audio_source_oss,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
