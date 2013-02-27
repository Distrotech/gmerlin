/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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
#include <stdlib.h>
#include <errno.h>

#include <faac.h>

#include <config.h>

#include <gmerlin_encoders.h>

#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>

#include <gavl/metatags.h>


#define LOG_DOMAIN "faac"

#include "faac_codec.h"

#define FAAC_DELAY 1024

struct bg_faac_s
  {
  faacEncHandle enc;
  faacEncConfigurationPtr enc_config;
  
  gavl_audio_sink_t * asink;
  gavl_packet_sink_t * psink;

  gavl_audio_frame_t * frame;

  gavl_packet_t p;
  
  gavl_audio_format_t fmt;

  int64_t in_pts;
  int64_t out_pts;
  
  /* Config stuff */
  unsigned int mpegVersion;
  unsigned int aacObjectType;
  unsigned int allowMidside;
  unsigned int useTns;
  unsigned long bitRate;
  unsigned long quantqual;
  int shortctl;
   
  };


bg_faac_t * bg_faac_create(void)
  {
  bg_faac_t * ret = calloc(1, sizeof(*ret));

  

  return ret;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "basic",
      .long_name =   TRS("Basic options"),
      .type =        BG_PARAMETER_SECTION
    },
    {
      .name =        "object_type",
      .long_name =   TRS("Object type"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str =  "mpeg4_main" },
      .multi_names = (char const *[]){ "mpeg2_main",
                              "mpeg2_lc",
                              "mpeg4_main",
                              "mpeg4_lc",
                              "mpeg4_ltp",
                              NULL },
      .multi_labels = (char const *[]){ TRS("MPEG-2 Main profile"),
                               TRS("MPEG-2 Low Complexity profile (LC)"),
                               TRS("MPEG-4 Main profile"),
                               TRS("MPEG-4 Low Complexity profile (LC)"),
                               TRS("MPEG-4 Long Term Prediction (LTP)"),
                               NULL },
    },
    {
      .name =        "bitrate",
      .long_name =   TRS("Bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0    },
      .val_max =     { .val_i = 1000 },
      .help_string = TRS("Average bitrate (0: VBR based on quality)"),
    },
    {
      .name =        "quality",
      .long_name =   TRS("Quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 10 },
      .val_max =     { .val_i = 500 },
      .val_default = { .val_i = 100 },
      .help_string = TRS("Quantizer quality"),
    },
    {
      .name =        "advanced",
      .long_name =   TRS("Advanced options"),
      .type =        BG_PARAMETER_SECTION
    },
    {
      .name =        "block_types",
      .long_name =   TRS("Block types"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str =  "Both" },
      .multi_names = (char const *[]){ "Both",
                              "No short",
                              "No long",
                              NULL },
      .multi_labels = (char const *[]){ TRS("Both"),
                               TRS("No short"),
                               TRS("No long"),
                               NULL },
    },
    {
      .name =        "tns",
      .type =        BG_PARAMETER_CHECKBUTTON,
      .long_name =   TRS("Use temporal noise shaping"),
      .val_default = { .val_i = 0 }
    },
    {
      .name =        "no_midside",
      .long_name =   TRS("Don\'t use mid/side coding"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 }
    },
    { /* End of parameters */ }
  };

const bg_parameter_info_t * bg_faac_get_parameters(bg_faac_t * ctx)
  {
  return parameters;
  }

void bg_faac_set_parameter(bg_faac_t * ctx, const char * name,
                           const bg_parameter_value_t * v)
  {
  if(!name)
    {
    return;
    }
  else if(!strcmp(name, "object_type"))
    {
    if(!strcmp(v->val_str, "mpeg2_main"))
      {
      ctx->mpegVersion = MPEG2;
      ctx->aacObjectType = MAIN;
      }
    else if(!strcmp(v->val_str, "mpeg2_lc"))
      {
      ctx->mpegVersion = MPEG2;
      ctx->aacObjectType = LOW;
      }
    else if(!strcmp(v->val_str, "mpeg4_main"))
      {
      ctx->mpegVersion = MPEG4;
      ctx->aacObjectType = MAIN;
      }
    else if(!strcmp(v->val_str, "mpeg4_lc"))
      {
      ctx->mpegVersion = MPEG4;
      ctx->aacObjectType = LOW;
      }
    else if(!strcmp(v->val_str, "mpeg4_ltp"))
      {
      ctx->mpegVersion = MPEG4;
      ctx->aacObjectType = LTP;
      }
    }
  else if(!strcmp(name, "bitrate"))
    {
    ctx->bitRate = v->val_i * 1000;
    }
  else if(!strcmp(name, "quality"))
    {
    ctx->quantqual = v->val_i;
    }
  else if(!strcmp(name, "block_types"))
    {
    if(!strcmp(v->val_str, "Both"))
      {
      ctx->shortctl = SHORTCTL_NORMAL;
      }
    else if(!strcmp(v->val_str, "No short"))
      {
      ctx->shortctl = SHORTCTL_NOSHORT;
      }
    else if(!strcmp(v->val_str, "No long"))
      {
      ctx->shortctl = SHORTCTL_NOLONG;
      }
    }
  else if(!strcmp(name, "tns"))
    {
    ctx->useTns = v->val_i;
    }
  else if(!strcmp(name, "no_midside"))
    {
    ctx->allowMidside = !v->val_i;
    }

  }

static int flush_audio(bg_faac_t * ctx)
  {
  int i, imax;
  int bytes_encoded;
  int num_samples;
  
  gavl_packet_reset(&ctx->p);
  
  /* First, we must scale the samples to -32767 .. 32767 */

  imax = ctx->frame->valid_samples * ctx->fmt.num_channels;
  
  for(i = 0; i < imax; i++)
    ctx->frame->samples.f[i] *= 32767.0;
  
  /* Encode the stuff */


  num_samples = ctx->frame->valid_samples ?
    (ctx->fmt.samples_per_frame * ctx->fmt.num_channels) : 0;

  //fprintf(stderr, "Encode %d\n", num_samples);
  
  bytes_encoded = faacEncEncode(ctx->enc,
                                (int32_t*)ctx->frame->samples.f,
                                num_samples,
                                ctx->p.data, ctx->p.data_alloc);

  ctx->p.data_len = bytes_encoded;

  /* Mute sets valid samples to samples_per_frame!! */
  gavl_audio_frame_mute(ctx->frame, &ctx->fmt);
  
  ctx->frame->valid_samples = 0;
  
  /* Write this to the file */

  if(bytes_encoded)
    {
    
    
    ctx->p.pts = ctx->out_pts;

    ctx->p.duration = ctx->fmt.samples_per_frame;

    if(ctx->p.pts + ctx->p.duration > ctx->in_pts)
      ctx->p.duration = ctx->in_pts - ctx->p.pts;
    
    ctx->out_pts += ctx->p.duration;
    
//    fprintf(stderr, "Got AAC packet\n");
//    gavl_packet_dump(&ctx->p);
    
    if(gavl_packet_sink_put_packet(ctx->psink, &ctx->p) != GAVL_SINK_OK)
      return -1;
    }
  return bytes_encoded;
  }


static gavl_sink_status_t
write_audio_func_faac(void * data, gavl_audio_frame_t * frame)
  {
  int samples_done = 0;
  int samples_copied;
  bg_faac_t * ctx = data;

  if(ctx->in_pts == GAVL_TIME_UNDEFINED)
    {
    ctx->in_pts = frame->timestamp;
    ctx->out_pts = ctx->in_pts - FAAC_DELAY;
    }
  
  while(samples_done < frame->valid_samples)
    {

    /* Copy frame into our buffer */
    
    samples_copied =
      gavl_audio_frame_copy(&ctx->fmt,
                            ctx->frame,                                                 /* dst */
                            frame,                                                       /* src */
                            ctx->frame->valid_samples,                                  /* dst_pos */
                            samples_done,                                                /* src_pos */
                            ctx->fmt.samples_per_frame - ctx->frame->valid_samples, /* dst_size */
                            frame->valid_samples - samples_done);                        /* src_size */
    
    samples_done += samples_copied;
    ctx->frame->valid_samples += samples_copied;
    
    /* Encode buffer */

    if(ctx->frame->valid_samples == ctx->fmt.samples_per_frame)
      {
      if(flush_audio(ctx) < 0)
        return GAVL_SINK_ERROR;
      }
    }
  
  ctx->in_pts += ctx->frame->valid_samples;
  return GAVL_SINK_OK;
  }


gavl_audio_sink_t * bg_faac_open(bg_faac_t * ctx,
                                 gavl_compression_info_t * ci,
                                 gavl_audio_format_t * fmt,
                                 gavl_metadata_t * m)
  {
  unsigned long input_samples;
  unsigned long output_bytes;
    
  /* Create encoder handle and get configuration */

  ctx->enc = faacEncOpen(fmt->samplerate,
                         fmt->num_channels,
                         &input_samples,
                         &output_bytes);
  
  ctx->enc_config = faacEncGetCurrentConfiguration(ctx->enc);
  ctx->enc_config->inputFormat = FAAC_INPUT_FLOAT;

  /* Decide output format: If ci == NULL we output ADTS */

  if(ci)
    ctx->enc_config->outputFormat = 0; // raw
  else      
    ctx->enc_config->outputFormat = 1; // ADTS

  /* Set config */

  ctx->enc_config->mpegVersion   = ctx->mpegVersion;
  ctx->enc_config->aacObjectType = ctx->aacObjectType;
  ctx->enc_config->allowMidside  = ctx->allowMidside;
  ctx->enc_config->useTns        = ctx->useTns;
  ctx->enc_config->bitRate       = ctx->bitRate / fmt->num_channels;
  ctx->enc_config->quantqual     = ctx->quantqual;
  ctx->enc_config->shortctl      = ctx->shortctl;
  
  /* Copy and adjust format */

  fmt->interleave_mode = GAVL_INTERLEAVE_ALL;
  fmt->sample_format   = GAVL_SAMPLE_FLOAT;
  fmt->samples_per_frame = input_samples / fmt->num_channels;

  switch(fmt->num_channels)
    {
    case 1:
      fmt->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      break;
    case 2:
      fmt->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      fmt->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      break;
    case 3:
      fmt->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      fmt->channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      fmt->channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      break;
    case 4:
      fmt->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      fmt->channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      fmt->channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      fmt->channel_locations[3] = GAVL_CHID_REAR_CENTER;
      break;
    case 5:
      fmt->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      fmt->channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      fmt->channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      fmt->channel_locations[3] = GAVL_CHID_REAR_LEFT;
      fmt->channel_locations[4] = GAVL_CHID_REAR_RIGHT;
      break;
    case 6:
      fmt->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      fmt->channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      fmt->channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      fmt->channel_locations[3] = GAVL_CHID_REAR_LEFT;
      fmt->channel_locations[4] = GAVL_CHID_REAR_RIGHT;
      fmt->channel_locations[5] = GAVL_CHID_LFE;
      break;

    }
  
  gavl_packet_alloc(&ctx->p, output_bytes);

  if(!faacEncSetConfiguration(ctx->enc, ctx->enc_config))
    { 
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "faacEncSetConfiguration failed");
    return NULL;
    }
    
  gavl_audio_format_copy(&ctx->fmt, fmt);
  ctx->frame = gavl_audio_frame_create(&ctx->fmt);
  
  ctx->asink =
    gavl_audio_sink_create(NULL, write_audio_func_faac, ctx, &ctx->fmt);

  /* Initialize compression info */
  if(ci)
    {
    unsigned long SizeOfDecoderSpecificInfo;
    ci->id = GAVL_CODEC_ID_AAC;

    faacEncGetDecoderSpecificInfo(ctx->enc, &ci->global_header,
                                  &SizeOfDecoderSpecificInfo);
    ci->global_header_len = SizeOfDecoderSpecificInfo;
    ci->pre_skip = FAAC_DELAY;
    gavl_metadata_set_nocpy(m, GAVL_META_SOFTWARE,
                            bg_sprintf("libfaac %s", ctx->enc_config->name));
    
    }
  
  ctx->in_pts = GAVL_TIME_UNDEFINED;
  ctx->out_pts = GAVL_TIME_UNDEFINED;
  return ctx->asink;
  }

void bg_faac_set_packet_sink(bg_faac_t * ctx,
                             gavl_packet_sink_t * psink)
  {
  ctx->psink = psink;
  }

void bg_faac_destroy(bg_faac_t * ctx)
  {
  int result;
  /* Flush remaining audio data */

  if(ctx->enc)
    {
    while(1)
      {
      result = flush_audio(ctx);
      if(result <= 0)
        break;
      }
    }
  
  if(ctx->enc)
    {
    faacEncClose(ctx->enc);
    ctx->enc = NULL;
    }

  gavl_packet_free(&ctx->p);
  
  if(ctx->frame)
    gavl_audio_frame_destroy(ctx->frame);
  
  if(ctx->asink)
    {
    gavl_audio_sink_destroy(ctx->asink);
    ctx->asink = NULL;
    }
    
  free(ctx);
  }
