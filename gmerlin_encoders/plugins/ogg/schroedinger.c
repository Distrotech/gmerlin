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

#include <gavl/metatags.h>
#include <gavl/numptr.h>


#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "schroedinger"

#include <schroedinger/schro.h>
// #include <schroedinger/schrodebug.h>
#include <schroedinger/schrovideoformat.h>
#ifdef HAVE_SCHROEDINGER_SCHROVERSION_H
#include <schroedinger/schroversion.h>
#else
#define SCHRO_CHECK_VERSION(a,b,c) 0
#endif

#include <ogg/ogg.h>
#include "ogg_common.h"


typedef struct
  {
  gavl_packet_sink_t * psink;
  SchroEncoder * enc;
  SchroFrameFormat  frame_format;
  
  gavl_video_frame_t * gavl_frame;
  gavl_video_format_t * gavl_format;

  uint32_t pic_num_max;
  
  gavl_packet_t pkt;

  bg_encoder_framerate_t fr;

  /* Granulepos calculation, used only for Ogg streams */
  
  int64_t decode_frame_number;
  int distance_from_sync;
  
  } schro_t;

static void set_packet_sink(void * data, gavl_packet_sink_t * psink)
  {
  schro_t * s = data;
  s->psink = psink;
  }

static void * create_schro()
  {
  schro_t * ret;

  schro_init();

  ret = calloc(1, sizeof(*ret));
  ret->enc = schro_encoder_new();
  ret->gavl_frame = gavl_video_frame_create(NULL);
  return ret;
  }

static const bg_parameter_info_t parameters[] =
  {
    { 
      .name =        "rc",
      .long_name =   TRS("Rate control"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
    .name = "schro_rate_control",
    .long_name = TRS("Rate control"),
    .type = BG_PARAMETER_STRINGLIST,
    .multi_names = (const char*[])
    {
      "constant_noise_threshold",
      "constant_bitrate",
      "low_delay",
      "lossless",
      "constant_lambda",
      "constant_error",
      "constant_quality",
      NULL
      },
    .multi_labels = (const char*[])
    {
      TRS("Constant noise threshold"),
      TRS("Constant bitrate"),
      TRS("Low delay"),
      TRS("Lossless"),
      TRS("Constant lambda"),
      TRS("Constant error"),
      TRS("Constant quality"),
      NULL
      },
    .val_default = { .val_str = "constant_quality" },
    },
    {
    .name = "schro_bitrate",
    .long_name = TRS("Bitrate"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 2147483647 },
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_max_bitrate",
    .long_name = TRS("Max bitrate"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 2147483647 },
    .val_default = { .val_i = 13824000 },
    },
    {
    .name = "schro_min_bitrate",
    .long_name = TRS("Min bitrate"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 2147483647 },
    .val_default = { .val_i = 13824000 },
    },
    {
    .name = "schro_buffer_size",
    .long_name = TRS("Buffer size"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 2147483647 },
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_buffer_level",
    .long_name = TRS("Buffer level"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 2147483647 },
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_quality",
    .long_name = TRS("Quality"),
    .type = BG_PARAMETER_SLIDER_FLOAT,
    .val_min = { .val_f = 0.000000 },
    .val_max = { .val_f = 10.000000 },
    .val_default = { .val_f = 5.000000 },
    .num_digits = 2,
    },
    {
    .name = "schro_noise_threshold",
    .long_name = TRS("Noise threshold"),
    .type = BG_PARAMETER_SLIDER_FLOAT,
    .val_min = { .val_f = 0.000000 },
    .val_max = { .val_f = 100.000000 },
    .val_default = { .val_f = 25.000000 },
    .num_digits = 2,
    },
    {
    .name = "schro_enable_rdo_cbr",
    .long_name = TRS("Enable rdo cbr"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 },
    },
    {
    .name = "schro_noise_threshold",
    .long_name = TRS("Noise threshold"),
    .type = BG_PARAMETER_SLIDER_FLOAT,
    .val_min = { .val_f = 0.000000 },
    .val_max = { .val_f = 100.000000 },
    .val_default = { .val_f = 25.000000 },
    .num_digits = 2,
    },
    { 
      .name =        "enc_frame_types",
      .long_name =   TRS("Frame types"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
    .name = "schro_gop_structure",
    .long_name = TRS("Gop structure"),
    .type = BG_PARAMETER_STRINGLIST,
    .multi_names = (const char*[])
    {
      "adaptive",
      "intra_only",
      "backref",
      "chained_backref",
      "biref",
      "chained_biref",
      NULL
      },
    .multi_labels = (const char*[])
    {
      TRS("Adaptive"),
      TRS("Intra only"),
      TRS("Backref"),
      TRS("Chained backref"),
      TRS("Biref"),
      TRS("Chained biref"),
      NULL
      },
    .val_default = { .val_str = "adaptive" },
    },
    {
    .name = "schro_au_distance",
    .long_name = TRS("Au distance"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 1 },
    .val_max = { .val_i = 2147483647 },
    .val_default = { .val_i = 120 },
    },
    {
    .name = "schro_open_gop",
    .long_name = TRS("Open gop"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 },
    },
    { 
      .name =        "enc_me",
      .long_name =   TRS("Motion estimation"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
    .name = "schro_mv_precision",
    .long_name = TRS("Mv precision"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 3 },
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_motion_block_size",
    .long_name = TRS("Motion block size"),
    .type = BG_PARAMETER_STRINGLIST,
    .multi_names = (const char*[])
    {
      "automatic",
      "small",
      "medium",
      "large",
      NULL
      },
    .multi_labels = (const char*[])
    {
      TRS("Automatic"),
      TRS("Small"),
      TRS("Medium"),
      TRS("Large"),
      NULL
      },
    .val_default = { .val_str = "automatic" },
    },
    {
    .name = "schro_motion_block_overlap",
    .long_name = TRS("Motion block overlap"),
    .type = BG_PARAMETER_STRINGLIST,
    .multi_names = (const char*[])
    {
      "automatic",
      "none",
      "partial",
      "full",
      NULL
      },
    .multi_labels = (const char*[])
    {
      TRS("Automatic"),
      TRS("None"),
      TRS("Partial"),
      TRS("Full"),
      NULL
      },
    .val_default = { .val_str = "automatic" },
    },
    {
    .name = "schro_enable_chroma_me",
    .long_name = TRS("Enable chroma me"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_enable_global_motion",
    .long_name = TRS("Enable global motion"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_enable_phasecorr_estimation",
    .long_name = TRS("Enable phasecorr estimation"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_enable_hierarchical_estimation",
    .long_name = TRS("Enable hierarchical estimation"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 },
    },
    {
    .name = "schro_enable_zero_estimation",
    .long_name = TRS("Enable zero estimation"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_enable_bigblock_estimation",
    .long_name = TRS("Enable bigblock estimation"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 },
    },
    {
    .name = "schro_enable_scene_change_detection",
    .long_name = TRS("Enable scene change detection"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 },
    },
    {
    .name = "schro_enable_deep_estimation",
    .long_name = TRS("Enable deep estimation"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 },
    },
    { 
      .name =        "enc_wavelets",
      .long_name =   TRS("Wavelets"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
    .name = "schro_intra_wavelet",
    .long_name = TRS("Intra wavelet"),
    .type = BG_PARAMETER_STRINGLIST,
    .multi_names = (const char*[])
    {
      "desl_dubuc_9_7",
      "le_gall_5_3",
      "desl_dubuc_13_7",
      "haar_0",
      "haar_1",
      "fidelity",
      "daub_9_7",
      NULL
      },
    .multi_labels = (const char*[])
    {
      TRS("Desl dubuc 9 7"),
      TRS("Le gall 5 3"),
      TRS("Desl dubuc 13 7"),
      TRS("Haar 0"),
      TRS("Haar 1"),
      TRS("Fidelity"),
      TRS("Daub 9 7"),
      NULL
      },
    .val_default = { .val_str = "desl_dubuc_9_7" },
    },
    {
    .name = "schro_inter_wavelet",
    .long_name = TRS("Inter wavelet"),
    .type = BG_PARAMETER_STRINGLIST,
    .multi_names = (const char*[])
    {
      "desl_dubuc_9_7",
      "le_gall_5_3",
      "desl_dubuc_13_7",
      "haar_0",
      "haar_1",
      "fidelity",
      "daub_9_7",
      NULL
      },
    .multi_labels = (const char*[])
    {
      TRS("Desl dubuc 9 7"),
      TRS("Le gall 5 3"),
      TRS("Desl dubuc 13 7"),
      TRS("Haar 0"),
      TRS("Haar 1"),
      TRS("Fidelity"),
      TRS("Daub 9 7"),
      NULL
      },
    .val_default = { .val_str = "desl_dubuc_9_7" },
    },
   
    { 
      .name =        "enc_filter",
      .long_name =   TRS("Filter"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
    .name = "schro_filtering",
    .long_name = TRS("Filtering"),
    .type = BG_PARAMETER_STRINGLIST,
    .multi_names = (const char*[])
    {
      "none",
      "center_weighted_median",
      "gaussian",
      "add_noise",
      "adaptive_gaussian",
      NULL
      },
    .multi_labels = (const char*[])
    {
      TRS("None"),
      TRS("Center weighted median"),
      TRS("Gaussian"),
      TRS("Add noise"),
      TRS("Adaptive gaussian"),
      NULL
      },
    .val_default = { .val_str = "none" },
    },
    {
    .name = "schro_filter_value",
    .long_name = TRS("Filter value"),
    .type = BG_PARAMETER_SLIDER_FLOAT,
    .val_min = { .val_f = 0.000000 },
    .val_max = { .val_f = 100.000000 },
    .val_default = { .val_f = 5.000000 },
    .num_digits = 2,
    },
    { 
      .name =        "enc_misc",
      .long_name =   TRS("Misc"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
    .name = "schro_force_profile",
    .long_name = TRS("Force profile"),
    .type = BG_PARAMETER_STRINGLIST,
    .multi_names = (const char*[])
    {
      "auto",
      "vc2_low_delay",
      "vc2_simple",
      "vc2_main",
      "main",
      NULL
      },
    .multi_labels = (const char*[])
    {
      TRS("Auto"),
      TRS("Vc2 low delay"),
      TRS("Vc2 simple"),
      TRS("Vc2 main"),
      TRS("Main"),
      NULL
      },
    .val_default = { .val_str = "auto" },
    },
    {
    .name = "schro_codeblock_size",
    .long_name = TRS("Codeblock size"),
    .type = BG_PARAMETER_STRINGLIST,
    .multi_names = (const char*[])
    {
      "automatic",
      "small",
      "medium",
      "large",
      "full",
      NULL
      },
    .multi_labels = (const char*[])
    {
      TRS("Automatic"),
      TRS("Small"),
      TRS("Medium"),
      TRS("Large"),
      TRS("Full"),
      NULL
      },
    .val_default = { .val_str = "automatic" },
    },
    {
    .name = "schro_enable_multiquant",
    .long_name = TRS("Enable multiquant"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_enable_dc_multiquant",
    .long_name = TRS("Enable dc multiquant"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_enable_noarith",
    .long_name = TRS("Enable noarith"),
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 0 },
    },
    {
    .name = "schro_downsample_levels",
    .long_name = TRS("Downsample levels"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 2 },
    .val_max = { .val_i = 8 },
    .val_default = { .val_i = 5 },
    },
    {
    .name = "schro_transform_depth",
    .long_name = TRS("Transform depth"),
    .type = BG_PARAMETER_INT,
    .val_min = { .val_i = 0 },
    .val_max = { .val_i = 6 },
    .val_default = { .val_i = 3 },
    },
    BG_ENCODER_FRAMERATE_PARAMS,
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_schro()
  {
  return parameters;
  }

static void set_parameter_schro(void * data, const char * name,
                                const bg_parameter_value_t * v)
  {
  int i;
  schro_t * s = data;
  double val = 0.0;
  const bg_parameter_info_t * info = NULL;

  if(!name)
    return;

  else if(bg_encoder_set_framerate_parameter(&s->fr, name, v))
    return;
  else if(strncmp(name, "schro_", 6))
    return;
  
  i = 0;
  while(parameters[i].name)
    {
    if(!strcmp(parameters[i].name, name))
      {
      info = &parameters[i];
      break;
      }
    i++;
    }

  if(!info)
    return;

  switch(info->type)
    {
    case BG_PARAMETER_CHECKBUTTON:
    case BG_PARAMETER_INT:
      val = (double)v->val_i;
      break;
    case BG_PARAMETER_FLOAT:
    case BG_PARAMETER_SLIDER_FLOAT:
      val = v->val_f;
      break;
    case BG_PARAMETER_STRINGLIST:
      {
      int j = 0, found = -1;
      while(info->multi_names[j])
        {
        if(!strcmp(v->val_str, info->multi_names[j]))
          {
          found = j;
          break;
          }
        j++;
        }
      if(found >= 0)
        val = (double)(found);
      else
        return;
      }
      break;
    default:
      return;
      break;
    }

  schro_encoder_setting_set_double(s->enc, name + 6, val);
  }

typedef struct
  {
  gavl_pixelformat_t pixelformat;
  SchroChromaFormat chroma_format;
  SchroFrameFormat  frame_format;
  SchroSignalRange  signal_range;
  int bits;
  } pixel_format_t;


static const pixel_format_t
pixel_format_map[] =
  {
    { GAVL_YUV_420_P, SCHRO_CHROMA_420, SCHRO_FRAME_FORMAT_U8_420, SCHRO_SIGNAL_RANGE_8BIT_VIDEO, 8 },
    { GAVL_YUV_422_P, SCHRO_CHROMA_422, SCHRO_FRAME_FORMAT_U8_422, SCHRO_SIGNAL_RANGE_8BIT_VIDEO, 8 },
    { GAVL_YUV_444_P, SCHRO_CHROMA_444, SCHRO_FRAME_FORMAT_U8_444, SCHRO_SIGNAL_RANGE_8BIT_VIDEO, 8 },
    { GAVL_YUVJ_420_P, SCHRO_CHROMA_420, SCHRO_FRAME_FORMAT_U8_420, SCHRO_SIGNAL_RANGE_8BIT_FULL, 8 },
    { GAVL_YUVJ_422_P, SCHRO_CHROMA_422, SCHRO_FRAME_FORMAT_U8_422, SCHRO_SIGNAL_RANGE_8BIT_FULL, 8 },
    { GAVL_YUVJ_444_P, SCHRO_CHROMA_444, SCHRO_FRAME_FORMAT_U8_444, SCHRO_SIGNAL_RANGE_8BIT_FULL, 8 },
  };

static const int num_pixel_formats = sizeof(pixel_format_map)/sizeof(pixel_format_map[0]);


static int get_best_pixelformat(gavl_pixelformat_t * pfmt)
  {
  int i;
  gavl_pixelformat_t * supported;

  supported = malloc((num_pixel_formats+1) * sizeof(*supported));

  for(i = 0; i < num_pixel_formats; i++)
    supported[i] = pixel_format_map[i].pixelformat;
  supported[num_pixel_formats] = GAVL_PIXELFORMAT_NONE;

  *pfmt = gavl_pixelformat_get_best(*pfmt, supported, NULL);
  free(supported);

  for(i = 0; i < num_pixel_formats; i++)
    {
    if(pixel_format_map[i].pixelformat == *pfmt)
      return i;
    }
  return -1; // Never happens
  }

static gavl_sink_status_t flush_data(schro_t * s)
  {
  SchroStateEnum  state;
  SchroBuffer * buf;
  int presentation_frame;
  gavl_sink_status_t st;
  
  while(1)
    {
    //    fprintf(stderr, "Flush data\n");
    
    state = schro_encoder_wait(s->enc);

    switch(state)
      {
      case SCHRO_STATE_HAVE_BUFFER:
        {
        int parse_code;

        buf = schro_encoder_pull(s->enc, &presentation_frame);
        
        parse_code = buf->data[4];

        if(SCHRO_PARSE_CODE_IS_PICTURE(parse_code))
          {
          uint32_t pic_num;
          gavl_packet_t * out_pkt;
          gavl_packet_t pkt;
          
          if(s->pkt.data_len) // Have data already
            {
            gavl_packet_alloc(&s->pkt, s->pkt.data_len + buf->length);
            memcpy(s->pkt.data + s->pkt.data_len, buf->data, buf->length);
            s->pkt.data_len += buf->length;
            out_pkt = &s->pkt;
            }
          else
            {
            gavl_packet_init(&pkt);
            pkt.data_len = buf->length;
            pkt.data = buf->data;
            out_pkt = &pkt;
            }
          
          pic_num = GAVL_PTR_2_32BE(buf->data + 13);
          
          if(SCHRO_PARSE_CODE_IS_INTRA(parse_code))
            {
            out_pkt->flags |= GAVL_PACKET_KEYFRAME;
            out_pkt->flags |= GAVL_PACKET_TYPE_I;
            s->pic_num_max = pic_num;
            }
          else if(pic_num < s->pic_num_max)
            out_pkt->flags |= GAVL_PACKET_TYPE_B;
          else
            {
            out_pkt->flags |= GAVL_PACKET_TYPE_P;
            s->pic_num_max = pic_num;
            }

          out_pkt->pts = (int64_t)pic_num * s->gavl_format->frame_duration;
          out_pkt->duration = s->gavl_format->frame_duration;
          
          fprintf(stderr, "Got picture\n");
          gavl_packet_dump(out_pkt);
          
          if((st = gavl_packet_sink_put_packet(s->psink, out_pkt)) != GAVL_SINK_OK)
            return st;
          
          gavl_packet_reset(&s->pkt);
          }
        else
          {
          gavl_packet_alloc(&s->pkt, s->pkt.data_len + buf->length);
          memcpy(s->pkt.data + s->pkt.data_len, buf->data, buf->length);
          s->pkt.data_len += buf->length;
          if(SCHRO_PARSE_CODE_IS_SEQ_HEADER(parse_code))
            s->pkt.header_size = s->pkt.data_len;
          }
        schro_buffer_unref(buf);
        }
        break;
      case SCHRO_STATE_END_OF_STREAM:
        buf = schro_encoder_pull(s->enc, &presentation_frame);
        fprintf(stderr, "Got EOS %d bytes\n", buf->length);
        schro_buffer_unref(buf);
        return GAVL_SINK_OK;
        break;
      case SCHRO_STATE_NEED_FRAME:
        //      fprintf(stderr, "Need frame\n");
        return GAVL_SINK_OK;
        break;
      case SCHRO_STATE_AGAIN:
        break;
      }
    }
  return GAVL_SINK_OK;
  }

static gavl_video_frame_t * get_frame(void * data)
  {
  int i;
  schro_t * s = data;
  SchroFrame * frame;
  frame = schro_frame_new_and_alloc(NULL,
                                    s->frame_format,
                                    s->gavl_format->image_width,
                                    s->gavl_format->image_height);

  for(i = 0; i < 3; i++)
    {
    s->gavl_frame->planes[i]    = frame->components[i].data;
    s->gavl_frame->strides[i] = frame->components[i].stride;
    }
  s->gavl_frame->user_data = frame;
  return s->gavl_frame;
  }

static gavl_sink_status_t put_frame(void * data, gavl_video_frame_t * f)
  {
  schro_t * s = data;
  // fprintf(stderr, "Put frame %ld\n", f->timestamp);
  schro_encoder_push_frame(s->enc, f->user_data);
  f->user_data = NULL;
  return flush_data(s);
  }

static gavl_video_sink_t *
init_schro(void * data, gavl_video_format_t * format,
           gavl_metadata_t * stream_metadata,
           gavl_compression_info_t * ci)
  {
  int idx;
  schro_t * s = data;
  SchroVideoFormat * fmt;
  SchroBuffer * buf;
  
  bg_encoder_set_framerate(&s->fr, format);
  
  idx = get_best_pixelformat(&format->pixelformat);
  
  fmt = schro_encoder_get_video_format(s->enc);
  fmt->width = format->image_width;
  fmt->height = format->image_height;
  fmt->clean_width = format->image_width;
  fmt->clean_height = format->image_height;
  fmt->left_offset = 0;
  fmt->top_offset = 0;

  fmt->frame_rate_numerator   = format->timescale;
  fmt->frame_rate_denominator = format->frame_duration;

  fmt->aspect_ratio_numerator   = format->pixel_width;  
  fmt->aspect_ratio_denominator = format->pixel_height;

  schro_video_format_set_std_signal_range(fmt, pixel_format_map[idx].signal_range);
  fmt->chroma_format = pixel_format_map[idx].chroma_format;
  s->frame_format = pixel_format_map[idx].frame_format;

  switch(format->interlace_mode)
    {
    case GAVL_INTERLACE_NONE:
      fmt->interlaced = 0;
      fmt->top_field_first = 0;
      break;
    case GAVL_INTERLACE_TOP_FIRST:
    case GAVL_INTERLACE_MIXED_TOP:
      fmt->interlaced = 0;
      fmt->top_field_first = 1;
      break;
    case GAVL_INTERLACE_MIXED_BOTTOM:
    case GAVL_INTERLACE_BOTTOM_FIRST:
      fmt->interlaced = 1;
      fmt->top_field_first = 0;
      break;
    case GAVL_INTERLACE_MIXED:
    case GAVL_INTERLACE_UNKNOWN:
      break;
    }
  
  schro_encoder_set_video_format(s->enc, fmt);
  schro_encoder_start(s->enc);
  
  ci->id = GAVL_CODEC_ID_DIRAC;

  /* TODO: Figure out the true values. Currently we assume the worst */
  ci->flags = GAVL_COMPRESSION_HAS_P_FRAMES | GAVL_COMPRESSION_HAS_B_FRAMES;
    
  /* Try to get the sequence header */
  buf = schro_encoder_encode_sequence_header(s->enc);

  ci->global_header_len = buf->length;
  ci->global_header = malloc(buf->length);
  memcpy(ci->global_header, buf->data, buf->length);
  schro_buffer_unref(buf);

  s->gavl_format = format;

  if(flush_data(s) != GAVL_SINK_OK)
    return NULL;

#ifdef HAVE_SCHROEDINGER_SCHROVERSION_H
  gavl_metadata_set_nocpy(stream_metadata, GAVL_META_SOFTWARE,
                          bg_sprintf("libschroedinger-%d.%d.%d",
                          SCHRO_VERSION_MAJOR,
                          SCHRO_VERSION_MINOR,
                                     SCHRO_VERSION_MICRO));
#else
  gavl_metadata_set_nocpy(stream_metadata, GAVL_META_SOFTWARE,
                          bg_sprintf("libschroedinger"));
#endif
  
  return  gavl_video_sink_create(get_frame, put_frame, s, format);
  
#if 0  
  if(!buf)
    fprintf(stderr, "Got no sequence header\n");
  else
    {
    fprintf(stderr, "Got sequence header\n");
    bg_hexdump(buf->data, buf->length, 16);
    }
#endif
  }

static int init_compressed_schro(bg_ogg_stream_t * s)
  {
  ogg_packet packet;
  schro_t * sch = s->codec_priv;

  memset(&packet, 0, sizeof(packet));

  /*
   *  Copy the sequence header verbatim.
   *  According to the mapping spec, we SHOULD inset
   *  an EOS unit after that, but noone else seems to
   *  do that either.
   */
  
  packet.packet = s->ci.global_header;
  packet.bytes  = s->ci.global_header_len;

  if(!bg_ogg_stream_write_header_packet(s, &packet))
    return 0;

  sch->decode_frame_number = -1;

  /* Flush stream after each packet as
     mandated by the Dirac mapping specification */  
  s->flags |= STREAM_FORCE_FLUSH;
  return 1;
  }

static void convert_packet(bg_ogg_stream_t * s, gavl_packet_t * src, ogg_packet * dst)
  {
  int64_t granulepos_hi;
  int64_t granulepos_low;

  int64_t pt, dt, dist, delay;
  
  int64_t presentation_frame_number;
  schro_t * sch = s->codec_priv;
  
  /* Convert the granulepos */
  
  presentation_frame_number = src->pts / sch->gavl_format->frame_duration;

  if(src->flags & GAVL_PACKET_KEYFRAME)
    sch->distance_from_sync = 0;
  else
    sch->distance_from_sync++;

  pt = presentation_frame_number * 2;
  dt = sch->decode_frame_number * 2;
  delay = pt - dt;
  dist = sch->distance_from_sync;

  granulepos_hi = ((pt - delay)<<9) | ((dist>>8));
  granulepos_low = (delay << 9) | (dist & 0xff);
  
  dst->granulepos = (granulepos_hi << 22) | (granulepos_low);
  
  sch->decode_frame_number++;
  }

static int close_schro(void * data)
  {
  int ret = 1;
  schro_t * s = data;

  fprintf(stderr, "close_schro\n");
  
  /* Flush stuff */
  schro_encoder_end_of_stream(s->enc);

  if(flush_data(s) != GAVL_SINK_OK)
    ret = 0;
  
  schro_encoder_free(s->enc);
  free(s);
  return ret;
  }

const bg_ogg_codec_t bg_schroedinger_codec =
  {
    .name =      "schroedinger",
    .long_name = TRS("Dirac encoder"),
    .create = create_schro,

    .get_parameters = get_parameters_schro,
    .set_parameter =  set_parameter_schro,
    //    .set_video_pass = set_video_pass_schro,
    .init_video =     init_schro,
    .init_video_compressed =     init_compressed_schro,

    .set_packet_sink = set_packet_sink,
    
    .convert_packet = convert_packet,
    .close = close_schro,
  };
