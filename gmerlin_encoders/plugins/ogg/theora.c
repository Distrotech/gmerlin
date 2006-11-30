/*****************************************************************
 
  theora.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <stdlib.h>

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

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "chroma_mode",
      long_name:   "Chroma subsampling mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "420" },
      multi_names:  (char*[]){ "420",   "422",   "444",   (char*)0 },
      multi_labels: (char*[]){ "4:2:0", "4:2:2", "4:4:4", (char*)0 },
    },
    {
      name:        "cbr",
      long_name:   "Use constant bitrate",
      type:        BG_PARAMETER_CHECKBUTTON,
      help_string: "For constant bitrate, choose a target bitrate. For variable bitrate, choose a nominal quality.",
    },
    {
      name:        "target_bitrate",
      long_name:   "Target bitrate (kbps)",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 45 },
      val_max:     { val_i: 2000 },
      val_default: { val_i: 250 },
      help_string: "Target bitrate (in kbps)",
    },
    {
      name:      "quality",
      long_name: "Nominal quality",
      type:      BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 63 },
      val_default: { val_i: 10 },
      num_digits:  1,
      help_string: "Quality for VBR mode\n\
63: best (largest output file)\n\
0:  worst (smallest output file",
    },
    {
      name:      "keyframe_auto_p",
      long_name: "Automatic keyframe insertion",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:      "keyframe_frequency",
      long_name: "Keyframe frequency",
      type:      BG_PARAMETER_INT,
      val_default: { val_i: 64 },
    },
    {
      name:      "keyframe_frequency_force",
      long_name: "Keyframe frequency force",
      type:      BG_PARAMETER_INT,
      val_default: { val_i: 64 },
    },
    {
      name:      "keyframe_auto_threshold",
      long_name: "Keyframe auto threshold",
      type:      BG_PARAMETER_INT,
      val_default: { val_i: 80 },
    },
    {
      name:      "keyframe_mindistance",
      long_name: "Keyframe minimum distance",
      type:      BG_PARAMETER_INT,
      val_default: { val_i: 8 },
    },
    {
      name:      "quick_encode",
      long_name: "Quick encode",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:      "sharpness",
      long_name: "Sharpness",
      type:      BG_PARAMETER_SLIDER_INT,
      val_default: { val_i: 0 },
      val_min:     { val_i: 0 },
      val_max:     { val_i: 2 },
    },
    {
      name:      "noise_sensitivity",
      long_name: "Noise Sensitivity",
      type:      BG_PARAMETER_SLIDER_INT,
      val_default: { val_i: 0 },
      val_min:     { val_i: 0 },
      val_max:     { val_i: 7 },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_theora()
  {
  return parameters;
  }

static void set_parameter_theora(void * data, char * name,
                                 bg_parameter_value_t * v)
  {
  theora_t * theora;
  theora = (theora_t*)data;
  
  if(!name)
    {
    return;
    }
  else if(!strcmp(name, "chroma_mode"))
    {
    if(!strcmp(v->val_str, "420"))
      {
      theora->ti.pixelformat = OC_PF_420;
      }
    else if(!strcmp(v->val_str, "422"))
      {
      theora->ti.pixelformat = OC_PF_422;
      }
    else if(!strcmp(v->val_str, "444"))
      {
      theora->ti.pixelformat = OC_PF_444;
      }
    }
  else if(!strcmp(name, "target_bitrate"))
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
  
  switch(theora->ti.pixelformat)
    {
    case OC_PF_420:
      format->pixelformat = GAVL_YUV_420_P;
      break;
    case OC_PF_422:
      format->pixelformat = GAVL_YUV_422_P;
      break;
    case OC_PF_444:
      format->pixelformat = GAVL_YUV_444_P;
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
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,  "Warning: Got no Theora ID page\n");
      return 0;
      }
    }
  theora_encode_comment(&theora->tc, &op);
  ogg_stream_packetin(&theora->os,&op);
  
  theora_encode_tables(&theora->ts, &op);
  ogg_stream_packetin(&theora->os,&op);

  return 1;
  }

static void flush_header_pages_theora(void*data)
  {
  theora_t * theora;
  theora = (theora_t*)data;
  bg_ogg_flush(&theora->os, theora->output, 1);
  }


static void write_video_frame_theora(void * data, gavl_video_frame_t * frame)
  {
  theora_t * theora;
  int sub_h, sub_v;
  yuv_buffer yuv;
  ogg_packet op;
  
  theora = (theora_t*)data;

  if(theora->have_packet)
    {
    if(!theora_encode_packetout(&theora->ts, 0, &op))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Warning: theora encoder produced no packet");
      return;
      }
    ogg_stream_packetin(&theora->os,&op);
    bg_ogg_flush(&theora->os, theora->output, 0);
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
  theora_encode_YUVin(&theora->ts, &yuv);
  theora->have_packet = 1;
  }

static void close_theora(void * data)
  {
  theora_t * theora;
  ogg_packet op;
  theora = (theora_t*)data;

  if(theora->have_packet)
    {
    if(!theora_encode_packetout(&theora->ts, 1, &op))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Warning: theora encoder produced no packet\n");
      return;
      }
    ogg_stream_packetin(&theora->os,&op);
    bg_ogg_flush(&theora->os, theora->output, 1);
    theora->have_packet = 0;
    }
  
  ogg_stream_clear(&theora->os);
  theora_comment_clear(&theora->tc);
  theora_info_clear(&theora->ti);
  theora_clear(&theora->ts);
  free(theora);
  }


bg_ogg_codec_t bg_theora_codec =
  {
    name:      "theora",
    long_name: "Theora encoder",
    create: create_theora,

    get_parameters: get_parameters_theora,
    set_parameter:  set_parameter_theora,
    
    init_video:     init_theora,
    
    //  int (*init_video)(void*, gavl_video_format_t * format);
  
    flush_header_pages: flush_header_pages_theora,
    
    encode_video: write_video_frame_theora,
    close: close_theora,
  };
