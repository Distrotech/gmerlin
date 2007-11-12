/*****************************************************************
 
  alsa_common.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <config.h>
#include <translation.h>

#include <parameter.h>
#include <utils.h>

#include <log.h>
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
  snd_pcm_hw_params_t *hw_params = (snd_pcm_hw_params_t *)0;
  //  snd_pcm_sw_params_t *sw_params = (snd_pcm_sw_params_t *)0;
  snd_pcm_t *ret                 = (snd_pcm_t *)0;
  
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
    ret = (snd_pcm_t *)0;
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
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S32_LE) < 0)
#else
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S32_BE) < 0)
#endif
        {
        /* Soundcard supports no 32-bit, try 24
           (more probably supported but needs conversion) */
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
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
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_FLOAT) < 0)
        {
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S32_LE) < 0)
#else
          if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S32_BE) < 0)
#endif
            {
            /* Soundcard supports no 32-bit, try 24
               (more probably supported but needs conversion) */
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
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
                                     &(i_tmp),
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
  return (snd_pcm_t *)0;
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

void bg_alsa_create_card_parameters(bg_parameter_info_t * ret)
  {
  int i, istart;
  int num_cards = 1; /* default */
  int card_index = -1;
  char * c_tmp;
  while(!snd_card_next(&card_index))
    {
    if(card_index > -1)
      num_cards++;
    else
      break;
    }

  ret->name      = bg_strdup((char*)0, "card");
  ret->long_name = bg_strdup((char*)0, TRS("Card"));
  ret->type = BG_PARAMETER_STRINGLIST;

  istart = 1;
  
  ret->multi_names   = calloc(num_cards+istart,
                              sizeof(*(ret->multi_names)));
  ret->multi_labels   = calloc(num_cards+istart,
                               sizeof(*(ret->multi_labels)));
  
  ret->val_default.val_str = bg_strdup((char*)0, "default");

  ret->multi_labels[0] = bg_strdup((char*)0, TRS("Default"));
  ret->multi_names[0]  = bg_strdup((char*)0, "default");
  
  for(i = istart; i < num_cards; i++)
    {
    snd_card_get_name(i-istart, &c_tmp);
    //    snd_card_get_longname(i, &c_tmp);
    ret->multi_labels[i] = bg_strdup(NULL, c_tmp);
    ret->multi_names[i] = bg_sprintf("hw:%d,0", i-1);
    free(c_tmp);
    }
  }
