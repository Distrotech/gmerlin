#include <parameter.h>
#include <utils.h>
#include "alsa_common.h"

static struct
  {
  snd_pcm_format_t     alsa_format;
  gavl_sample_format_t gavl_format;
  }
sampleformats[] =
  {
    { GAVL_SAMPLE_NONE,     SND_PCM_FORMAT_UNKNOWN            },
    { GAVL_SAMPLE_S8,       SND_PCM_FORMAT_S8                 },
    { GAVL_SAMPLE_U8,       SND_PCM_FORMAT_U8                 },
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
    { GAVL_SAMPLE_S16,      SND_PCM_FORMAT_S16_LE             },
    { GAVL_SAMPLE_U16,      SND_PCM_FORMAT_U16_LE             },
    { GAVL_SAMPLE_S32,      SND_PCM_FORMAT_S32_LE             },
    { GAVL_SAMPLE_FLOAT,    SND_PCM_FORMAT_FLOAT_LE           },
#else  
    { GAVL_SAMPLE_S16,      SND_PCM_FORMAT_S16_BE             },
    { GAVL_SAMPLE_U16,      SND_PCM_FORMAT_U16_BE             },
    { GAVL_SAMPLE_S32,      SND_PCM_FORMAT_S32_BE             },
    { GAVL_SAMPLE_FLOAT,    SND_PCM_FORMAT_FLOAT_BE           },
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

static snd_pcm_t * bg_alsa_open(const char * card, gavl_audio_format_t * format,
                                snd_pcm_stream_t stream)
  {
  snd_pcm_format_t alsa_format;
  snd_pcm_hw_params_t *hw_params = (snd_pcm_hw_params_t *)0;
  snd_pcm_t *ret                 = (snd_pcm_t *)0;

  if(snd_pcm_open(&ret,
                  card,
                  stream, // SND_PCM_STREAM_PLAYBACK SND_PCM_STREAM_CAPTURE
                  0      //  SND_PCM_NONBLOCK SND_PCM_ASYNC
                  ) < 0)
    {
    ret = (snd_pcm_t *)0;
    fprintf(stderr, "bg_alsa_open: snd_pcm_open failed\n");
    goto fail;
    }

  if(snd_pcm_hw_params_malloc(&hw_params) < 0)
    {
    fprintf(stderr, "bg_alsa_open: snd_pcm_hw_params_malloc failed\n");
    goto fail;
    }

  if(snd_pcm_hw_params_any(ret, hw_params) < 0)
    {
    fprintf(stderr, "bg_alsa_open: snd_pcm_hw_params_any failed\n");
    goto fail;
    }

  /* Interleave mode */
  
  if(format->interleave_mode != GAVL_INTERLEAVE_ALL)
    {
    if(snd_pcm_hw_params_set_access(ret, hw_params,
                                    SND_PCM_ACCESS_RW_NONINTERLEAVED) < 0)
      {
      if(snd_pcm_hw_params_set_access(ret, hw_params,
                                      SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
        {
        fprintf(stderr, "bg_alsa_open: snd_pcm_hw_params_set_access failed\n");
        goto fail;
        }
      else
        format->interleave_mode = GAVL_INTERLEAVE_ALL;
      }
    else
      format->interleave_mode = GAVL_INTERLEAVE_NONE;
    }
  else
    if(snd_pcm_hw_params_set_access(ret, hw_params,
                                    SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
      {
      fprintf(stderr, "bg_alsa_open: snd_pcm_hw_params_set_access failed\n");
      goto fail;
      }

  /* Sample format */

  alsa_format = sample_format_gavl_2_alsa(format->sample_format);
  if(alsa_format == SND_PCM_FORMAT_UNKNOWN)
    alsa_format = sample_format_gavl_2_alsa(GAVL_SAMPLE_S16);
  
  if(snd_pcm_hw_params_set_format(ret, hw_params,
                                  alsa_format) < 0)
    {
    alsa_format = sample_format_gavl_2_alsa(GAVL_SAMPLE_S16);

    if(snd_pcm_hw_params_set_format(ret, hw_params,
                                    alsa_format) < 0)
      {
      if(gavl_bytes_per_sample(format->sample_format) <= 8)
        alsa_format = SND_PCM_FORMAT_U8;
      else
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        alsa_format = SND_PCM_FORMAT_S16_LE;
#else
      alsa_format = SND_PCM_FORMAT_S16_BE;
#endif

      if(snd_pcm_hw_params_set_format(ret, hw_params,
                                      alsa_format) < 0)
        {
        fprintf(stderr, "bg_alsa_open: snd_pcm_hw_params_set_format failed\n");
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
    fprintf(stderr, "bg_alsa_open: snd_pcm_hw_params_set_rate_near failed\n");
    goto fail;
    }

  if(snd_pcm_hw_params_set_channels(ret, hw_params,
                                    format->num_channels) < 0)
    {
    fprintf(stderr, "bg_alsa_open: snd_pcm_hw_params_set_channels failed\n");
    goto fail;
    }

  if(snd_pcm_hw_params (ret, hw_params) < 0)
    {
    fprintf(stderr, "bg_alsa_open: snd_pcm_hw_params failed\n");
    goto fail;
    }
  snd_pcm_hw_params_free(hw_params);
  return ret;
  fail:
  if(ret)
    snd_pcm_close(ret);
  if(hw_params)
    snd_pcm_hw_params_free(hw_params);
  return (snd_pcm_t *)0;
  }

snd_pcm_t * bg_alsa_open_read(const char * card, gavl_audio_format_t * format)
  {
  return bg_alsa_open(card, format, SND_PCM_STREAM_CAPTURE);
  }

snd_pcm_t * bg_alsa_open_write(const char * card, gavl_audio_format_t * format)
  {
  return bg_alsa_open(card, format, SND_PCM_STREAM_PLAYBACK);
  }

void bg_alsa_create_card_parameters(bg_parameter_info_t * ret,
                                    bg_parameter_info_t * per_card_params)
  {
  int i, j;
  int num_parameters;
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

  ret->name = "card";
  ret->long_name = "Card";
  ret->type = BG_PARAMETER_MULTI_MENU;
  
  ret->multi_names        = calloc(num_cards+1, sizeof(*ret->multi_names));
  //  ret->multi_labels       = calloc(num_cards+1, sizeof(*ret->multi_labels));
  ret->multi_descriptions = calloc(num_cards+1, sizeof(*ret->multi_descriptions));

  num_parameters = 0;

  if(per_card_params)
    {
    while(per_card_params[num_parameters].name)
      num_parameters++;
    }
  if(num_parameters)
    ret->multi_parameters   = calloc(num_cards+1, sizeof(*ret->multi_parameters));
    
  for(i = 0; i < num_cards; i++)
    {
    snd_card_get_name(i, &c_tmp);
    ret->multi_names[i] = bg_strdup(NULL, c_tmp);

    if(!i)
      ret->val_default.val_str = bg_strdup(NULL, c_tmp);
    
    snd_card_get_longname(i, &c_tmp);
    ret->multi_descriptions[i] = bg_strdup(NULL, c_tmp);

    if(num_parameters)
      {
      ret->multi_parameters[i] =
        calloc(num_parameters+1, sizeof(*(ret->multi_parameters[i])));
      for(j = 0; j < num_parameters; j++)
        {
        bg_parameter_info_copy(&(ret->multi_parameters[i][j]),
                               &(per_card_params[j]));
        }
      }
    }
  }
