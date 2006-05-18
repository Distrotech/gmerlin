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

#include <parameter.h>
#include <utils.h>
#include "alsa_common.h"

static snd_pcm_t * bg_alsa_open(const char * card,
                                gavl_audio_format_t * format,
                                snd_pcm_stream_t stream,
                                gavl_time_t buffer_time,
                                char ** error_msg,
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
    if(error_msg)
      *error_msg = bg_sprintf("alsa: snd_pcm_open failed for device %s (%s)", 
                              card, snd_strerror(err));
    goto fail;
    }

  /* Now, set blocking mode */

  snd_pcm_nonblock(ret, 0);
  
  if(snd_pcm_hw_params_malloc(&hw_params) < 0)
    {
    if(error_msg) *error_msg = bg_sprintf("alsa: snd_pcm_hw_params_malloc failed");
    goto fail;
    }
  if(snd_pcm_hw_params_any(ret, hw_params) < 0)
    {
    if(error_msg) *error_msg = bg_sprintf("alsa: snd_pcm_hw_params_any failed");
    goto fail;
    }

  /* Interleave mode */
  
  if(snd_pcm_hw_params_set_access(ret, hw_params,
                                  SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
    {
    if(error_msg)
      *error_msg = bg_sprintf("alsa: snd_pcm_hw_params_set_access failed");
    goto fail;
    }
  else
    format->interleave_mode = GAVL_INTERLEAVE_ALL;
  
  /* Sample format */

  switch(format->sample_format)
    {
    case GAVL_SAMPLE_S8:
    case GAVL_SAMPLE_U8:
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_U8) < 0)
        {
        /* Soundcard support no 8-bit, try 16 */
        if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S16) < 0)
          {
          /* Hopeless */
          if(error_msg) *error_msg = bg_sprintf("alsa: snd_pcm_hw_params_set_format failed");
          goto fail;
          }
        else
          format->sample_format = GAVL_SAMPLE_S16;
        }
      else
        format->sample_format = GAVL_SAMPLE_U8;
      break;
    case GAVL_SAMPLE_S16:
    case GAVL_SAMPLE_U16:
      if(snd_pcm_hw_params_set_format(ret, hw_params, SND_PCM_FORMAT_S16) < 0)
        {
        /* Hopeless */
        if(error_msg) *error_msg = bg_sprintf("alsa: snd_pcm_hw_params_set_format failed");
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
            if(error_msg) *error_msg = bg_sprintf("alsa: snd_pcm_hw_params_set_format failed");
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
                  if(error_msg) *error_msg = bg_sprintf("alsa: snd_pcm_hw_params_set_format failed");
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
    if(error_msg)
      *error_msg =
        bg_sprintf("alsa: snd_pcm_hw_params_set_channels failed (Format has %d channels)",
                     format->num_channels);

    if(format->num_channels == 1) /* Mono doesn't work, try stereo */
      {
      if(snd_pcm_hw_params_set_channels(ret, hw_params,
                                        2) < 0)
        goto fail;
      else
        {
        format->num_channels = 2;
        format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
        format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
        }
      }
    else
      goto fail;
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
    if(error_msg) *error_msg = bg_sprintf("alsa: snd_pcm_hw_params_set_rate_near failed");
    goto fail;
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

  //  fprintf(stderr, "Buffer size: [%d..%d], Period Size: [%d..%d]\n",
  //          buffer_size_min, buffer_size_max, period_size_min, period_size_max);
  
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
    if(error_msg)
      *error_msg = bg_sprintf("bg_alsa_open: snd_pcm_hw_params_set_period_size failed");
    goto fail;
    }
  dir = 0;
  snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir);

  dir = 0;
  if(snd_pcm_hw_params_set_buffer_size_near(ret, hw_params, &buffer_size) < 0)
    {
    if(error_msg)
      *error_msg = bg_sprintf("bg_alsa_open: snd_pcm_hw_params_set_buffer_size failed");
    goto fail;
    }
  
  snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);

  //  fprintf(stderr, "Buffer size: %d, period_size: %d\n", buffer_size, period_size);
  
  format->samples_per_frame = period_size;
  
  /* Write params to card */
  if(snd_pcm_hw_params (ret, hw_params) < 0)
    {
    if(error_msg)
      *error_msg = bg_sprintf("alsa: snd_pcm_hw_params failed");
    goto fail;
    }
#if 0  
  if(error_msg)
    *error_msg = bg_sprintf("Test error");
  goto fail;
#endif
  snd_pcm_hw_params_free(hw_params);
  
  gavl_set_channel_setup(format);
  //  gavl_audio_format_dump(format);
  
  return ret;
  fail:

  fprintf(stderr, "Alsa initialization failed\n");
  if(ret)
    snd_pcm_close(ret);
  if(hw_params)
    snd_pcm_hw_params_free(hw_params);
  return (snd_pcm_t *)0;
  }

snd_pcm_t * bg_alsa_open_read(const char * card, gavl_audio_format_t * format,
                              char ** error_msg)
  {
  return bg_alsa_open(card, format, SND_PCM_STREAM_CAPTURE,
                      GAVL_TIME_SCALE/10, error_msg, NULL);
  }

snd_pcm_t * bg_alsa_open_write(const char * card, gavl_audio_format_t * format,
                               char ** error_msg, int * convert_4_3)
  {
  return bg_alsa_open(card, format, SND_PCM_STREAM_PLAYBACK,
                      GAVL_TIME_SCALE, error_msg, convert_4_3);
  }

void bg_alsa_create_card_parameters(bg_parameter_info_t * ret)
  {
  int i;
  int num_cards = 0;
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
  ret->long_name = bg_strdup((char*)0, "Card");
  ret->type = BG_PARAMETER_STRINGLIST;
  
  ret->multi_names   = calloc(num_cards+1,
                          sizeof(*(ret->multi_names)));
  for(i = 0; i < num_cards; i++)
    {
    snd_card_get_name(i, &c_tmp);
    //    snd_card_get_longname(i, &c_tmp);
    ret->multi_names[i] = bg_strdup(NULL, c_tmp);
    
    if(!i)
      ret->val_default.val_str = bg_strdup(NULL, c_tmp);

    free(c_tmp);
    }
  }
