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

static struct
  {
  gavl_sample_format_t gavl_format;
  snd_pcm_format_t     alsa_format;
  }
sampleformats[] =
  {
    { GAVL_SAMPLE_NONE,     SND_PCM_FORMAT_UNKNOWN  },
    { GAVL_SAMPLE_S8,       SND_PCM_FORMAT_S8       },
    { GAVL_SAMPLE_U8,       SND_PCM_FORMAT_U8       },
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
    { GAVL_SAMPLE_S16,      SND_PCM_FORMAT_S16_LE   },
    { GAVL_SAMPLE_U16,      SND_PCM_FORMAT_U16_LE   },
    { GAVL_SAMPLE_S32,      SND_PCM_FORMAT_S32_LE   },
    { GAVL_SAMPLE_FLOAT,    SND_PCM_FORMAT_FLOAT_LE },
#else  
    { GAVL_SAMPLE_S16,      SND_PCM_FORMAT_S16_BE   },
    { GAVL_SAMPLE_U16,      SND_PCM_FORMAT_U16_BE   },
    { GAVL_SAMPLE_S32,      SND_PCM_FORMAT_S32_BE   },
    { GAVL_SAMPLE_FLOAT,    SND_PCM_FORMAT_FLOAT_BE },
#endif

#if 0
    { SND_PCM_FORMAT_S24_LE             },
    { SND_PCM_FORMAT_S24_BE             },
    { SND_PCM_FORMAT_U24_LE             },
    { SND_PCM_FORMAT_U24_BE             },
    { SND_PCM_FORMAT_S32_BE             },
    { SND_PCM_FORMAT_U32_LE             },
    { SND_PCM_FORMAT_U32_BE             },
    { SND_PCM_FORMAT_FLOAT64_LE         },
    { SND_PCM_FORMAT_FLOAT64_BE         },
    { SND_PCM_FORMAT_IEC958_SUBFRAME_LE },
    { SND_PCM_FORMAT_IEC958_SUBFRAME_BE },
    { SND_PCM_FORMAT_MU_LAW             },
    { SND_PCM_FORMAT_A_LAW              },
    { SND_PCM_FORMAT_IMA_ADPCM          },
    { SND_PCM_FORMAT_MPEG               },
    { SND_PCM_FORMAT_GSM                },
    { SND_PCM_FORMAT_SPECIAL            },
    { SND_PCM_FORMAT_S24_3LE            },
    { SND_PCM_FORMAT_S24_3BE            },
    { SND_PCM_FORMAT_U24_3LE            },
    { SND_PCM_FORMAT_U24_3BE            },
    { SND_PCM_FORMAT_S20_3LE            },
    { SND_PCM_FORMAT_S20_3BE            },
    { SND_PCM_FORMAT_U20_3LE            },
    { SND_PCM_FORMAT_U20_3BE            },
    { SND_PCM_FORMAT_S18_3LE            },
    { SND_PCM_FORMAT_S18_3BE            },
    { SND_PCM_FORMAT_U18_3LE            },
    { SND_PCM_FORMAT_U18_3BE            }
#endif
  };

static snd_pcm_format_t sample_format_gavl_2_alsa(gavl_sample_format_t f)
  {
  int i;
  for(i = 0; i < sizeof(sampleformats)/sizeof(sampleformats[0]); i++)
    {
    if(sampleformats[i].gavl_format == f)
      return sampleformats[i].alsa_format;
    }
  return SND_PCM_FORMAT_UNKNOWN;
  }

static gavl_sample_format_t sample_format_alsa_2_gavl(snd_pcm_format_t f)
  {
  int i;
  for(i = 0; i < sizeof(sampleformats)/sizeof(sampleformats[0]); i++)
    {
    if(sampleformats[i].alsa_format == f)
      return sampleformats[i].gavl_format;
    }
  return GAVL_SAMPLE_NONE;
  }

static snd_pcm_t * bg_alsa_open(const char * card,
                                gavl_audio_format_t * format,
                                snd_pcm_stream_t stream,
                                unsigned int buffer_time,
                                unsigned int period_time,
                                char ** error_msg)
  {
  int dir;
  snd_pcm_format_t alsa_format;
  snd_pcm_hw_params_t *hw_params = (snd_pcm_hw_params_t *)0;
  snd_pcm_t *ret                 = (snd_pcm_t *)0;
  
  snd_pcm_sframes_t buffer_size;
  snd_pcm_sframes_t period_size;

  /* We open in non blocking mode so our process won't hang if the card is
     busy */
  
  if(snd_pcm_open(&ret,
                  card,
                  stream,           // SND_PCM_STREAM_PLAYBACK SND_PCM_STREAM_CAPTURE
                  SND_PCM_NONBLOCK  //   SND_PCM_ASYNC
                  ) < 0)
    {
    ret = (snd_pcm_t *)0;
    if(error_msg) *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_open failed");
    goto fail;
    }

  /* Now, set blocking mode */

  snd_pcm_nonblock(ret, 0);
  
  if(snd_pcm_hw_params_malloc(&hw_params) < 0)
    {
    if(error_msg) *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params_malloc failed");
    goto fail;
    }
  if(snd_pcm_hw_params_any(ret, hw_params) < 0)
    {
    if(error_msg) *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params_any failed");
    goto fail;
    }

  /* Interleave mode */
  
#if 0
  if(format->interleave_mode != GAVL_INTERLEAVE_ALL)
    {
    if(snd_pcm_hw_params_set_access(ret, hw_params,
                                    SND_PCM_ACCESS_RW_NONINTERLEAVED) < 0)
      {
      fprintf(stderr, "Non interleaved access doesn't work, trying interleaved\n");
#endif
      if(snd_pcm_hw_params_set_access(ret, hw_params,
                                      SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
        {
        if(error_msg)
          *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params_set_access failed");
        goto fail;
        }
      else
        format->interleave_mode = GAVL_INTERLEAVE_ALL;
#if 0
      }
    else
      format->interleave_mode = GAVL_INTERLEAVE_NONE;
    }
  else
    if(snd_pcm_hw_params_set_access(ret, hw_params,
                                    SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
      {
      if(error_msg)
        *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params_set_access failed");
      goto fail;
      }
#endif
  
  /* Sample format */
  
  alsa_format = sample_format_gavl_2_alsa(format->sample_format);
  if(alsa_format == SND_PCM_FORMAT_UNKNOWN)
    alsa_format = sample_format_gavl_2_alsa(GAVL_SAMPLE_S16);
  
  //  if(error_msg) *error_msg = bgav_sprintf("Trying 1 %s\n", snd_pcm_format_name(alsa_format)); 
  if(snd_pcm_hw_params_set_format(ret, hw_params,
                                  alsa_format) < 0)
    {
    alsa_format = sample_format_gavl_2_alsa(GAVL_SAMPLE_S16);
    //    if(error_msg) *error_msg = bgav_sprintf("Trying 2 %s\n", snd_pcm_format_name(alsa_format)); 

    if(snd_pcm_hw_params_set_format(ret, hw_params,
                                    alsa_format) < 0)
      {
      if(gavl_bytes_per_sample(format->sample_format) == 1)
        alsa_format = SND_PCM_FORMAT_U8;
      else
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        alsa_format = SND_PCM_FORMAT_S16_LE;
#else
      alsa_format = SND_PCM_FORMAT_S16_BE;
#endif
      //      fprintf(stderr, "Trying 3 %s\n", snd_pcm_format_name(alsa_format)); 
      if(snd_pcm_hw_params_set_format(ret, hw_params,
                                      alsa_format) < 0)
        {
        if(error_msg) *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params_set_format failed");
        goto fail;
        }
      }
    }
  format->sample_format = sample_format_alsa_2_gavl(alsa_format);
  
  /* Samplerate */
  
  if(snd_pcm_hw_params_set_rate_near(ret, hw_params,
                                     &(format->samplerate),
                                     0) < 0)
    {
    if(error_msg) *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params_set_rate_near failed");
    goto fail;
    }

  /* Channels */
  
  if(snd_pcm_hw_params_set_channels(ret, hw_params,
                                    format->num_channels) < 0)
    {
    if(error_msg)
      *error_msg =
        bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params_set_channels failed (Format has %d channels)",
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

  dir = 0;
  /* Buffer time */
  if(snd_pcm_hw_params_set_buffer_time_near(ret, hw_params,
                                            &buffer_time, &dir) < 0)
    {
    if(error_msg)
      *error_msg =
        bg_sprintf(*error_msg, "bg_alsa_open: snd_pcm_hw_params_set_buffer_time_near failed");
    goto fail;
    }

  /* Buffer size */
  if(snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size) < 0)
    {
    if(error_msg)
      *error_msg = bg_sprintf(*error_msg, "bg_alsa_open: snd_pcm_hw_params_get_buffer_size failed");
    goto fail;
    }
  //  fprintf(stderr, "*** Buffer size: %d\n", (int)buffer_size);
  /* Period time */
  if(snd_pcm_hw_params_set_period_time_near(ret, hw_params, &period_time, &dir) < 0)
    {
    if(error_msg)
      *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params_set_period_time_near failed");
    goto fail;
    }
  /* Period size */
  
  if(snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir) < 0)
    {
    if(error_msg) *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params_get_period_size failed");
    goto fail;
    }
  //  fprintf(stderr, "*** Period size: %d\n", (int)period_size);

  format->samples_per_frame = period_size;
  
  /* Write params to card */
  if(snd_pcm_hw_params (ret, hw_params) < 0)
    {
    if(error_msg)
      *error_msg = bg_sprintf(*error_msg, "alsa: snd_pcm_hw_params failed");
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
                      30000, 10000, error_msg);
  }

snd_pcm_t * bg_alsa_open_write(const char * card, gavl_audio_format_t * format,
                              char ** error_msg)
  {
  return bg_alsa_open(card, format, SND_PCM_STREAM_PLAYBACK,
                      350000, 30000, error_msg);
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
