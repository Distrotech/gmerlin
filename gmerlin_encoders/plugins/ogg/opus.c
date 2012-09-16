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

#include <string.h>
#include <stdlib.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "oggopus"

#include <gavl/metatags.h>

#include <ogg/ogg.h>
#include "ogg_common.h"

#include <opus.h>
#include <opus_multistream.h>

#define BITRATE_VBR   0
#define BITRATE_CVBR  1
#define BITRATE_CBR   2

typedef struct
  {
  uint8_t  version;
  uint8_t  channel_count;
  uint16_t pre_skip;
  uint32_t samplerate;
  int16_t  output_gain;
  uint8_t  channel_mapping;
  
  struct
    {
    uint8_t stream_count;
    uint8_t coupled_count;

    uint8_t map[256]; // 255 actually
    
    } chtab; // Channel mapping table
  } opus_header_t;

static void setup_header(opus_header_t * h, gavl_audio_format_t * format);
static void header_to_packet(opus_header_t * h, ogg_packet * op);

typedef struct
  {
  /* Common stuff */
  ogg_stream_state enc_os;
  bg_ogg_encoder_t * output;
  long serialno;

  /* Config */
  int application;
  int bitrate_mode;
  int complexity;
  int fec;
  int dtx;
  int loss_perc;
  int bandwidth;
  int max_bandwidth;
  int bitrate;
  
  /* Encoder */

  OpusMSEncoder * enc;
  opus_header_t h;
  
  } opus_t;

static void * create_opus(bg_ogg_encoder_t * output, long serialno)
  {
  opus_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->serialno = serialno;
  ret->output = output;
  return ret;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "application",
      .long_name =   TRS("Application"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "audio" },
      .multi_names =  (char const *[]){ "audio", "voip", NULL },
      .multi_labels = (char const *[]){ TRS("Audio"), TRS("VOIP"),
                                        NULL },
    },
    {
      .name =        "bitrate_mode",
      .long_name =   TRS("Bitrate mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "vbr" },
      .multi_names =  (char const *[]){ "vbr", "cvbr", "cbr", NULL },
      .multi_labels = (char const *[]){ TRS("VBR"), TRS("Constrained VBR"),
                                        TRS("CBR"),
                                        NULL },
      .help_string = TRS("Bitrate mode")
    },
    {
      .name =        "bitrate",
      .long_name =   TRS("Bitrate"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 512000 },
      .val_default = { .val_i = 0 },
      .help_string = TRS("Bitrate (in bps). 0 means auto."),
    },
    {
      .name =      "complexity",
      .long_name = TRS("Encoding complexity"),
      .type =      BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0  },
      .val_max =     { .val_i = 10 },
      .val_default = { .val_i = 10 },
    },
    {
      .name =        "dtx",
      .long_name =   TRS("Enable discontinuous transmission"),
      .type =        BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name =        "inband_fec",
      .long_name =   TRS("Enable inband forward error correction"),
      .type =        BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name =        "bandwidth",
      .long_name =   TRS("Bandwidth"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "auto" },
      .multi_names =  (char const *[]){ "auto", "narrow", "medium", "wide", "superwide", "full", NULL },
      .multi_labels = (char const *[]){ TRS("Automatic"),
                                        TRS("Narrow (4 kHz)"),
                                        TRS("Medium (6 kHz)"),
                                        TRS("Wide (8 kHz)"),
                                        TRS("Superwide (12 kHz)"),
                                        TRS("Full (20 kHz)"),
                                        NULL },
    },
    {
      .name =        "max_bandwidth",
      .long_name =   TRS("Maximum bandwidth"),
      .type =        BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "full" },
      .multi_names =  (char const *[]){ "narrow", "medium", "wide", "superwide", "full", NULL },
      .multi_labels = (char const *[]){ TRS("Narrow (4 kHz)"),
                                        TRS("Medium (6 kHz)"),
                                        TRS("Wide (8 kHz)"),
                                        TRS("Superwide (12 kHz)"),
                                        TRS("Full (20 kHz)"),
                                        NULL },
    },
    {
      .name =      "loss_perc",
      .long_name = TRS("Loss percentage"),
      .type =      BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0  },
      .val_max =     { .val_i = 100 },
      .val_default = { .val_i = 0 },
      
    },
    { /* End */ },
  };

static const bg_parameter_info_t * get_parameters_opus()
  {
  return parameters;
  }

static void set_parameter_opus(void * data, const char * name,
                               const bg_parameter_value_t * v)
  {
  opus_t * opus = data;
  
  if(!name)
    return;
  else if(!strcmp(name, "application"))
    {
    if(!strcmp(v->val_str, "audio"))
      opus->application = OPUS_APPLICATION_AUDIO;
    else if(!strcmp(v->val_str, "voip"))
      opus->application = OPUS_APPLICATION_VOIP;
    }
  else if(!strcmp(name, "bitrate_mode"))
    {
    if(!strcmp(v->val_str, "vbr"))
      opus->bitrate_mode = BITRATE_VBR;
    else if(!strcmp(v->val_str, "cvbr"))
      opus->bitrate_mode = BITRATE_CVBR;
    else if(!strcmp(v->val_str, "cbr"))
      opus->bitrate_mode = BITRATE_CBR;
    }
  else if(!strcmp(name, "bitrate"))
    {
    opus->bitrate = v->val_i;
    }
  else if(!strcmp(name, "complexity"))
    {
    opus->complexity = v->val_i; 
    }
  else if(!strcmp(name, "dtx"))
    {
    opus->dtx = v->val_i; 
    }
  else if(!strcmp(name, "inband_fec"))
    {
    opus->fec = v->val_i; 
    }
  else if(!strcmp(name, "bandwidth"))
    {
    if(!strcmp(v->val_str, "narrow"))
      opus->bandwidth = OPUS_BANDWIDTH_NARROWBAND;
    else if(!strcmp(v->val_str, "medium"))
      opus->bandwidth = OPUS_BANDWIDTH_MEDIUMBAND;
    else if(!strcmp(v->val_str, "wide"))
      opus->bandwidth = OPUS_BANDWIDTH_WIDEBAND;
    else if(!strcmp(v->val_str, "superwide"))
      opus->bandwidth = OPUS_BANDWIDTH_SUPERWIDEBAND;
    else if(!strcmp(v->val_str, "full"))
      opus->bandwidth = OPUS_BANDWIDTH_FULLBAND;
    else if(!strcmp(v->val_str, "auto"))
      opus->bandwidth = OPUS_AUTO;
    }
  else if(!strcmp(name, "loss_perc"))
    {
    opus->loss_perc = v->val_i; 
    }
  
  }

static int init_opus(void * data, gavl_audio_format_t * format,
                     gavl_metadata_t * metadata)
  {
  int err;
  opus_t * opus = data;

  /* Setup header */

  setup_header(&opus->h, format);
  
  /* Create encoder */

  opus->enc = opus_multistream_encoder_create(format->samplerate,
                                              opus->h.channel_count,
                                              opus->h.chtab.stream_count,
                                              opus->h.chtab.coupled_count,
                                              opus->h.chtab.map,
                                              opus->application,
                                              &err);
  
  /* Apply config */

  switch(opus->bitrate_mode)
    {
    case BITRATE_VBR:
      opus_multistream_encoder_ctl(opus->enc, OPUS_SET_VBR(1));
      opus_multistream_encoder_ctl(opus->enc, OPUS_SET_VBR_CONSTRAINT(0));
      break;
    case BITRATE_CVBR:
      opus_multistream_encoder_ctl(opus->enc, OPUS_SET_VBR(1));
      opus_multistream_encoder_ctl(opus->enc, OPUS_SET_VBR_CONSTRAINT(1));
      break;
    case BITRATE_CBR:
      opus_multistream_encoder_ctl(opus->enc, OPUS_SET_VBR(0));
      opus_multistream_encoder_ctl(opus->enc, OPUS_SET_VBR_CONSTRAINT(0));
      break;
    }

  opus_multistream_encoder_ctl(opus->enc, OPUS_SET_BITRATE(opus->bitrate));
  opus_multistream_encoder_ctl(opus->enc, OPUS_SET_COMPLEXITY(opus->complexity));
  opus_multistream_encoder_ctl(opus->enc, OPUS_SET_DTX(opus->dtx));
  opus_multistream_encoder_ctl(opus->enc, OPUS_SET_INBAND_FEC(opus->fec));
  opus_multistream_encoder_ctl(opus->enc, OPUS_SET_PACKET_LOSS_PERC(opus->loss_perc));
  opus_multistream_encoder_ctl(opus->enc, OPUS_SET_BANDWIDTH(opus->bandwidth));
  opus_multistream_encoder_ctl(opus->enc, OPUS_SET_MAX_BANDWIDTH(opus->max_bandwidth));
  
  
  return 1;
  }

static int flush_header_pages_opus(void*data)
  {
  opus_t * opus = data;
  }

static int write_audio_frame_opus(void * data, gavl_audio_frame_t * frame)
  {
  opus_t * opus = data;
  
  }

static int close_opus(void * data)
  {
  opus_t * opus = data;

  /* Flush */

  /* Cleanup */
  ogg_stream_clear(&opus->enc_os);

  opus_multistream_encoder_destroy(opus->enc);
  free(opus);
  }

const bg_ogg_codec_t bg_opus_codec =
  {
    .name =      "opus",
    .long_name = TRS("Opus encoder"),
    .create = create_opus,

    .get_parameters = get_parameters_opus,
    .set_parameter =  set_parameter_opus,
    
    .init_audio =     init_opus,
    
    //  int (*init_video)(void*, gavl_video_format_t * format);
  
    .flush_header_pages = flush_header_pages_opus,
    
    .encode_audio = write_audio_frame_opus,
    .close = close_opus,
  };


/* Header stuff */

static void setup_header(opus_header_t * h, gavl_audio_format_t * format)
  {
  
  }

static void header_to_packet(opus_header_t * h, ogg_packet * op)
  {
  
  }
