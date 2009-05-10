/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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


#include <string.h>
#include <stdlib.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "oggtheora"


#include <theora/theora.h>
#include "ogg_common.h"

typedef struct
  {
  /* Ogg theora stuff */
    
  ogg_stream_state os;
  
  theora_info    ti;
  theora_comment tc;
  theora_state   ts;
  
  long serialno;
  FILE * output;

  gavl_video_format_t * format;
  int have_packet;
  int cbr;
  } theora_t;

static void * create_theora(FILE * output, long serialno)
  {
  theora_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->serialno = serialno;
  ret->output = output;
  theora_info_init(&ret->ti);

  return ret;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "cbr",
      .long_name =   TRS("Use constant bitrate"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .help_string = TRS("For constant bitrate, choose a target bitrate. For variable bitrate, choose a nominal quality."),
    },
    {
      .name =        "target_bitrate",
      .long_name =   TRS("Target bitrate (kbps)"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 45 },
      .val_max =     { .val_i = 2000 },
      .val_default = { .val_i = 250 },
    },
    {
      .name =      "quality",
      .long_name = TRS("Nominal quality"),
      .type =      BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 63 },
      .val_default = { .val_i = 10 },
      .num_digits =  1,
      .help_string = TRS("Quality for VBR mode\n\
63: best (largest output file)\n\
0:  worst (smallest output file)"),
    },
    {
      .name =      "keyframe_auto_p",
      .long_name = TRS("Automatic keyframe insertion"),
      .type =      BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
    },
    {
      .name =      "keyframe_frequency",
      .long_name = TRS("Keyframe frequency"),
      .type =      BG_PARAMETER_INT,
      .val_default = { .val_i = 64 },
    },
    {
      .name =      "keyframe_frequency_force",
      .long_name = TRS("Keyframe frequency force"),
      .type =      BG_PARAMETER_INT,
      .val_default = { .val_i = 64 },
    },
    {
      .name =      "keyframe_auto_threshold",
      .long_name = TRS("Keyframe auto threshold"),
      .type =      BG_PARAMETER_INT,
      .val_default = { .val_i = 80 },
    },
    {
      .name =      "keyframe_mindistance",
      .long_name = TRS("Keyframe minimum distance"),
      .type =      BG_PARAMETER_INT,
      .val_default = { .val_i = 8 },
    },
    {
      .name =      "quick_encode",
      .long_name = TRS("Quick encode"),
      .type =      BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
    },
    {
      .name =      "sharpness",
      .long_name = TRS("Sharpness"),
      .type =      BG_PARAMETER_SLIDER_INT,
      .val_default = { .val_i = 0 },
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 2 },
    },
    {
      .name =      "noise_sensitivity",
      .long_name = TRS("Noise Sensitivity"),
      .type =      BG_PARAMETER_SLIDER_INT,
      .val_default = { .val_i = 0 },
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 7 },
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_theora()
  {
  return parameters;
  }

static void set_parameter_theora(void * data, const char * name,
                                 const bg_parameter_value_t * v)
  {
  theora_t * theora;
  theora = (theora_t*)data;
  
  if(!name)
    {
    return;
    }
  if(!strcmp(name, "target_bitrate"))
    {
    theora->ti.target_bitrate = v->val_i * 1000;
    }
  else if(!strcmp(name, "quality"))
    {
    theora->ti.quality = v->val_i;
    }
  else if(!strcmp(name, "quick_encode"))
    {
    theora->ti.quick_p = v->val_i;
    }
  else if(!strcmp(name, "keyframe_auto_p"))
    {
    theora->ti.keyframe_auto_p = v->val_i;
    }
  else if(!strcmp(name, "cbr"))
    {
    theora->cbr = v->val_i;
    }
  else if(!strcmp(name, "sharpness"))
    {
    theora->ti.sharpness = v->val_i;
    }
  else if(!strcmp(name, "noise_sensitivity"))
    {
    theora->ti.noise_sensitivity = v->val_i;
    }
  else if(!strcmp(name, "keyframe_frequency"))
    {
    theora->ti.keyframe_frequency = v->val_i;
    }
  else if(!strcmp(name, "keyframe_frequency_force"))
    {
    theora->ti.keyframe_frequency_force = v->val_i;
    }
  else if(!strcmp(name, "keyframe_auto_threshold"))
    {
    theora->ti.keyframe_auto_threshold = v->val_i;
    }
  else if(!strcmp(name, "keyframe_mindistance"))
    {
    theora->ti.keyframe_mindistance = v->val_i;
    }
  
  }


static void build_comment(theora_comment * vc, bg_metadata_t * metadata)
  {
  char * tmp_string;
  
  theora_comment_init(vc);
  
  if(metadata->artist)
    theora_comment_add_tag(vc, "ARTIST", metadata->artist);
  
  if(metadata->title)
    theora_comment_add_tag(vc, "TITLE", metadata->title);

  if(metadata->album)
    theora_comment_add_tag(vc, "ALBUM", metadata->album);
    
  if(metadata->genre)
    theora_comment_add_tag(vc, "GENRE", metadata->genre);

  if(metadata->date)
    theora_comment_add_tag(vc, "DATE", metadata->date);
  
  if(metadata->copyright)
    theora_comment_add_tag(vc, "COPYRIGHT", metadata->copyright);

  if(metadata->track)
    {
    tmp_string = bg_sprintf("%d", metadata->track);
    theora_comment_add_tag(vc, "TRACKNUMBER", tmp_string);
    free(tmp_string);
    }

  if(metadata->comment)
    theora_comment_add(vc, metadata->comment);
  }

static const gavl_pixelformat_t supported_pixelformats[] =
  {
    GAVL_YUV_420_P,
    //    GAVL_YUV_422_P,
    //    GAVL_YUV_444_P,
    GAVL_PIXELFORMAT_NONE,
  };

static int init_theora(void * data, gavl_video_format_t * format, bg_metadata_t * metadata)
  {
  ogg_packet op;

  theora_t * theora = (theora_t *)data;

  theora->format = format;
  
  /* Set video format */
  theora->ti.width  = ((format->image_width  + 15)/16*16);
  theora->ti.height = ((format->image_height + 15)/16*16);
  theora->ti.frame_width = format->image_width;
  theora->ti.frame_height = format->image_height;
  
  theora->ti.fps_numerator      = format->timescale;
  theora->ti.fps_denominator    = format->frame_duration;
  theora->ti.aspect_numerator   = format->pixel_width;
  theora->ti.aspect_denominator = format->pixel_height;

  format->interlace_mode = GAVL_INTERLACE_NONE;
  format->framerate_mode = GAVL_FRAMERATE_CONSTANT;
  format->frame_width  = theora->ti.width;
  format->frame_height = theora->ti.height;

  /* TODO: Configure these */

  if(theora->cbr)
    theora->ti.quality = 0;
  else
    theora->ti.target_bitrate = 0;
  
  theora->ti.colorspace=OC_CS_UNSPECIFIED;
  theora->ti.dropframes_p=0;
  theora->ti.keyframe_frequency=64;
  theora->ti.keyframe_frequency_force=64;
  theora->ti.keyframe_data_target_bitrate=theora->ti.keyframe_data_target_bitrate*1.5;
  theora->ti.keyframe_auto_threshold=80;
  theora->ti.keyframe_mindistance=8;

  format->pixelformat =
    gavl_pixelformat_get_best(format->pixelformat,
                              supported_pixelformats, NULL);
    
  switch(format->pixelformat)
    {
    case GAVL_YUV_420_P:
      theora->ti.pixelformat = OC_PF_420;
      break;
    case GAVL_YUV_422_P:
      theora->ti.pixelformat = OC_PF_422;
      break;
    case GAVL_YUV_444_P:
      theora->ti.pixelformat = OC_PF_444;
      break;
    default:
      return 0;
    }
  
  ogg_stream_init(&theora->os, theora->serialno);

  /* Initialize encoder */
  if(theora_encode_init(&theora->ts, &theora->ti))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "theora_encode_init failed");
    return 0;
    }
  /* Build comment (comments are UTF-8, good for us :-) */

  build_comment(&theora->tc, metadata);

  /* Encode initial packets */
  if(theora_encode_header(&theora->ts, &op))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "theora_encode_header failed");
    return 0;
    }
  else
    {
    /* And stream them out */
    ogg_stream_packetin(&theora->os,&op);
    if(!bg_ogg_flush_page(&theora->os, theora->output, 1))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Got no Theora ID page");
      return 0;
      }
    }
  theora_encode_comment(&theora->tc, &op);
  ogg_stream_packetin(&theora->os,&op);
  
  theora_encode_tables(&theora->ts, &op);
  ogg_stream_packetin(&theora->os,&op);

  return 1;
  }

static int flush_header_pages_theora(void*data)
  {
  theora_t * theora;
  theora = (theora_t*)data;
  if(bg_ogg_flush(&theora->os, theora->output, 1) <= 0)
    return 0;
  return 1;
  }


static int write_video_frame_theora(void * data, gavl_video_frame_t * frame)
  {
  theora_t * theora;
  int sub_h, sub_v;
  int result;
  yuv_buffer yuv;
  ogg_packet op;
  
  theora = (theora_t*)data;

  if(theora->have_packet)
    {
    if(!theora_encode_packetout(&theora->ts, 0, &op))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Theora encoder produced no packet");
      fprintf(stderr, "Packetout failed\n");
      
      return 0;
      }
    
    ogg_stream_packetin(&theora->os,&op);
    if(bg_ogg_flush(&theora->os, theora->output, 0) < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Writing theora packet failed");
      return 0;
      }
    theora->have_packet = 0;
    }
  
  gavl_pixelformat_chroma_sub(theora->format->pixelformat, &sub_h, &sub_v);
  
  yuv.y_width  = theora->format->frame_width;
  yuv.y_height = theora->format->frame_height;
  yuv.y_stride = frame->strides[0];

  yuv.uv_width  = theora->format->frame_width / sub_h;
  yuv.uv_height = theora->format->frame_height / sub_v;
  yuv.uv_stride = frame->strides[1];

  yuv.y = frame->planes[0];
  yuv.u = frame->planes[1];
  yuv.v = frame->planes[2];
  result = theora_encode_YUVin(&theora->ts, &yuv);
  theora->have_packet = 1;
  return 1;
  }

static int close_theora(void * data)
  {
  int ret = 1;
  theora_t * theora;
  ogg_packet op;
  theora = (theora_t*)data;

  if(theora->have_packet)
    {
    if(!theora_encode_packetout(&theora->ts, 1, &op))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Theora encoder produced no packet");
      ret = 0;
      }

    if(ret)
      {
      ogg_stream_packetin(&theora->os,&op);
      if(bg_ogg_flush(&theora->os, theora->output, 1) <= 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing packet failed");
        ret = 0;
        }
      }
    theora->have_packet = 0;
    }
  
  ogg_stream_clear(&theora->os);
  theora_comment_clear(&theora->tc);
  theora_info_clear(&theora->ti);
  theora_clear(&theora->ts);
  free(theora);
  return ret;
  }


const bg_ogg_codec_t bg_theora_codec =
  {
    .name =      "theora",
    .long_name = TRS("Theora encoder"),
    .create = create_theora,

    .get_parameters = get_parameters_theora,
    .set_parameter =  set_parameter_theora,
    
    .init_video =     init_theora,
    
    //  int (*init_video)(void*, gavl_video_format_t * format);
  
    .flush_header_pages = flush_header_pages_theora,
    
    .encode_video = write_video_frame_theora,
    .close = close_theora,
  };
