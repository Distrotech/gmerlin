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

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/parameter.h>
#include <gmerlin/utils.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "alsa_common"

#include "alsa_common.h"

static snd_pcm_t * bg_alsa_open(const char * card,
                                gavl_audio_format_t * format,
                                snd_pcm_stream_t stream,
                                gavl_time_t buffer_time,
                                int * convert_4_3)
  {
  unsigned int i_tmp;
  int dir, err;
  snd_pcm_hw_params_t *hw_params = NULL;
  //  snd_pcm_sw_params_t *sw_params = NULL;
  snd_pcm_t *ret                 = NULL;
  
  snd_pcm_uframes_t buffer_size;
  snd_pcm_uframes_t period_size;
  
  snd_pcm_uframes_t period_size_min = 0; 
  snd_pcm_uframes_t period_size_max = 0; 
  snd_pcm_uframes_t buffer_size_min = 0;
  snd_pcm_uframes_t buffer_size_max = 0;
  /* We open in non blocking mode so our process won't hang if the card is
     busy */
  
  if((err = snd_pcm_open(&ret,
                         card,
                         stream,  // SND_PCM_STREAM_PLAYBACK SND_PCM_STREAM_CAPTURE
                         SND_PCM_NONBLOCK  //   SND_PCM_ASYNC
                         ) < 0))
    {
    ret = NULL;
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_open failed for device %s (%s)",
           card, snd_strerror(err));
    goto fail;
    }

  /* Now, set blocking mode */

  snd_pcm_nonblock(ret, 0);
  
  if(snd_pcm_hw_params_malloc(&hw_params) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_malloc failed");
    goto fail;
    }
  if(snd_pcm_hw_params_any(ret, hw_params) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_any failed");
    goto fail;
    }

  /* Interleave mode */
  
  if(snd_pcm_hw_params_set_access(ret, hw_params,
                                  SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_set_access failed");
    goto fail;
    }
  else
    format->interleave_mode = GAVL_INTERLEAVE_ALL;
  
  /* Sample format */

  switch(format->sample_format)
    {
    case GAVL_SAMPLE_S8:
    case GAVL_SAMPLE_U8:
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S8) < 0)
        //  if(1)
          {
        /* Soundcard support no 8-bit, try 16 */
        if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S16) < 0)
          {
          /* Hopeless */
          bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_set_format failed");
          goto fail;
          }
        else
          format->sample_format = GAVL_SAMPLE_S16;
        }
        else
        format->sample_format = GAVL_SAMPLE_S8;
      break;
    case GAVL_SAMPLE_S16:
    case GAVL_SAMPLE_U16:
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S16) < 0)
        {
        /* Hopeless */
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_set_format failed");
        goto fail;
        }
      else
        format->sample_format = GAVL_SAMPLE_S16;
      break;
    case GAVL_SAMPLE_S32:
#ifndef WORDS_BIGENDIAN
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S32_LE) < 0)
#else
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S32_BE) < 0)
#endif
        {
        /* Soundcard supports no 32-bit, try 24
           (more probably supported but needs conversion) */
#ifndef WORDS_BIGENDIAN
        if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S24_3LE) < 0)
#else
        if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S24_3BE) < 0)
#endif
          {
          /* No 24 bit, try 16 */
          if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S16) < 0)
            {
            /* Hopeless */
            bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_set_format failed");
            goto fail;
            }
          else
            format->sample_format = GAVL_SAMPLE_S16;
          }
        else
          {
          format->sample_format = GAVL_SAMPLE_S32;
          if(convert_4_3)
            *convert_4_3 = 1;
          }
        }
      else
        format->sample_format = GAVL_SAMPLE_S32;
      break;
    case GAVL_SAMPLE_FLOAT:
    case GAVL_SAMPLE_DOUBLE:
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_FLOAT) < 0)
       {

#ifndef WORDS_BIGENDIAN
        if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S32_LE) < 0)
#else
        if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S32_BE) < 0)
#endif
          {
            /* Soundcard supports no 32-bit, try 24
               (more probably supported but needs conversion) */
#ifndef WORDS_BIGENDIAN
          if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S24_3LE) < 0)
#else
          if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S24_3BE) < 0)
#endif
            {
            /* No 24 bit, try 16 */
            if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S16) < 0)
              {
              /* Hopeless */
              bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_set_format failed");
              goto fail;
              }
            else
              format->sample_format = GAVL_SAMPLE_S16;
            }
          else
            {
            format->sample_format = GAVL_SAMPLE_S32;
            if(convert_4_3)
              *convert_4_3 = 1;
            }
          }
        else
          format->sample_format = GAVL_SAMPLE_S32;
       }
      else
        format->sample_format = GAVL_SAMPLE_FLOAT;
      break;
    case GAVL_SAMPLE_NONE:
      goto fail;
      break;
    }

  /* Channels */
  if(snd_pcm_hw_params_set_channels(ret, hw_params,
                                    format->num_channels) < 0)
    {
    if(format->num_channels == 1) /* Mono doesn't work, try stereo */
      {
      if(snd_pcm_hw_params_set_channels(ret, hw_params,
                                        2) < 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,
               "snd_pcm_hw_params_set_channels failed (Format has %d channels)",
               format->num_channels);
        goto fail;
        }
      else
        {
        format->num_channels = 2;
        format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        }
      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "snd_pcm_hw_params_set_channels failed (Format has %d channels)",
             format->num_channels);
      goto fail;
      }
    }
  
  /* Switch off driver side resampling */
#if SND_LIB_VERSION >= 0x010009
  snd_pcm_hw_params_set_rate_resample(ret, hw_params, 0);
#endif
  
  /* Samplerate */
  i_tmp = format->samplerate;
  if(snd_pcm_hw_params_set_rate_near(ret, hw_params,
                                     &i_tmp,
                                     0) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_set_rate_near failed");
    goto fail;
    }
  if(format->samplerate != i_tmp)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Samplerate %d not supported by device %s, using %d",
           format->samplerate, card, i_tmp);

    }
  format->samplerate = i_tmp;
  
  dir = 0;

  /* Buffer size */

  snd_pcm_hw_params_get_buffer_size_min(hw_params, &buffer_size_min);
  snd_pcm_hw_params_get_buffer_size_max(hw_params, &buffer_size_max);
  dir=0;
  snd_pcm_hw_params_get_period_size_min(hw_params, &period_size_min,&dir);
  dir=0;
  snd_pcm_hw_params_get_period_size_max(hw_params, &period_size_max,&dir);
  
  buffer_size = gavl_time_to_samples(format->samplerate, buffer_time);

  
  if(buffer_size > buffer_size_max)
    buffer_size = buffer_size_max;
  if(buffer_size < buffer_size_min)
    buffer_size = buffer_size_min;

  period_size = buffer_size / 8;
  buffer_size = period_size * 8;

  dir = 0;
  if(snd_pcm_hw_params_set_period_size_near(ret, hw_params, &period_size, &dir) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_set_period_size failed");
    goto fail;
    }
  dir = 0;
  snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir);

  dir = 0;
  if(snd_pcm_hw_params_set_buffer_size_near(ret, hw_params, &buffer_size) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params_set_buffer_size failed");
    goto fail;
    }
  
  snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);

  
  format->samples_per_frame = period_size;
  
  /* Write params to card */
  if(snd_pcm_hw_params (ret, hw_params) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_pcm_hw_params failed");
    goto fail;
    }
  snd_pcm_hw_params_free(hw_params);
  
  gavl_set_channel_setup(format);
  //  gavl_audio_format_dump(format);
  
  return ret;
  fail:
  
  bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Alsa initialization failed");
  if(ret)
    snd_pcm_close(ret);
  if(hw_params)
    snd_pcm_hw_params_free(hw_params);
  return NULL;
  }

snd_pcm_t * bg_alsa_open_read(const char * card,
                              gavl_audio_format_t * format,
                              gavl_time_t buffer_time)
  {
  return bg_alsa_open(card, format, SND_PCM_STREAM_CAPTURE,
                      buffer_time, NULL);
  }

snd_pcm_t * bg_alsa_open_write(const char * card, gavl_audio_format_t * format,
                               gavl_time_t buffer_time,
                               int * convert_4_3)
  {
  return bg_alsa_open(card, format, SND_PCM_STREAM_PLAYBACK,
                      buffer_time, convert_4_3);
  }

static void append_card(bg_parameter_info_t * ret,
                        char * name, char * label)
  {
  int num = 0;

  if(ret->multi_names)
    {
    while(ret->multi_names[num])
      num++;
    }
  
  ret->multi_names_nc = realloc(ret->multi_names_nc,
                             sizeof(*ret->multi_names) * (num+2));

  ret->multi_labels_nc = realloc(ret->multi_labels_nc,
                             sizeof(*ret->multi_labels) * (num+2));

  ret->multi_names_nc[num]  = name;
  ret->multi_labels_nc[num] = label;

  ret->multi_names_nc[num+1] = NULL;
  ret->multi_labels_nc[num+1] = NULL;

  bg_parameter_info_set_const_ptrs(ret);
  }

void bg_alsa_create_card_parameters(bg_parameter_info_t * ret,
                                    int record)
  {
  snd_ctl_card_info_t *info;
  snd_ctl_t *handle;
  snd_pcm_info_t *pcminfo;
  snd_pcm_stream_t stream;
  int err;
  int card, dev;

  stream = record ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;
  
  ret->name      = bg_strdup(NULL, "card");
  ret->long_name = bg_strdup(NULL, TRS("Card"));
  ret->type = BG_PARAMETER_STRINGLIST;

  snd_ctl_card_info_malloc(&info);

  card = -1;
  if (snd_card_next(&card) < 0 || card < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No soundcards found");
    return;
    }
  
  /* Default is always supported */
  ret->val_default.val_str = bg_strdup(NULL, "default");
  append_card(ret, bg_strdup(NULL, "default"),
              bg_strdup(NULL, TRS("Default")));
  
  while (card >= 0)
    {
    char name[32];
    sprintf(name, "hw:%d", card);
    if ((err = snd_ctl_open(&handle, name, 0)) < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "control open failed (%i): %s",
             card, snd_strerror(err));
      goto next_card;
      }
    if ((err = snd_ctl_card_info(handle, info)) < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "control hardware info failed (%i): %s",
             card, snd_strerror(err));
      snd_ctl_close(handle);
      goto next_card;
      }
    dev = -1;

    while (1)
      {
      char * name, *label;
      snd_pcm_info_malloc(&pcminfo);
      if (snd_ctl_pcm_next_device(handle, &dev)<0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "snd_ctl_pcm_next_device failed");
        snd_pcm_info_free(pcminfo);
        break;
        }
      if (dev < 0)
        {
        snd_pcm_info_free(pcminfo);
        break;
        }
      snd_pcm_info_set_device(pcminfo, dev);
      snd_pcm_info_set_subdevice(pcminfo, 0);
      snd_pcm_info_set_stream(pcminfo, stream);
      if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0)
        {
        if (err != -ENOENT)
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,
                 "control digital audio info failed (%i): %s",
                 card, snd_strerror(err));
        snd_pcm_info_free(pcminfo);
        continue;
        }
      name = bg_sprintf("hw:%d,%d", card, dev);
      label = bg_strdup(NULL, snd_pcm_info_get_name(pcminfo));
      append_card(ret, name, label);
      snd_pcm_info_free(pcminfo);
      }
    snd_ctl_close(handle);
    
    next_card:
    if (snd_card_next(&card) < 0)
      break;
    }
  snd_ctl_card_info_free(info);
  
  }
