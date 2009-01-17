/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <stdlib.h>
#include <string.h>


#include <avdec_private.h>
#include <videoparser.h>
#include <videoparser_priv.h>
#include <mpv_header.h>
#include <h264_header.h>

/* H.264 */

#define H264_NEED_NAL_START 0
#define H264_NEED_NAL_END   1
#define H264_HAVE_NAL       2
#define H264_NEED_SPS       3
#define H264_NEED_PPS       4

typedef struct
  {
  /* Sequence header */
  bgav_h264_sps_t sps;
  
  uint8_t * sps_buffer;
  int sps_len;
  uint8_t * pps_buffer;
  int pps_len;

  int have_sps;
  
  int state;

  int nal_len;

  uint8_t * rbsp;
  int rbsp_alloc;
  int rbsp_len;
  
  int has_picture_start;
  } h264_priv_t;

static void get_rbsp(bgav_video_parser_t * parser, uint8_t * pos, int len)
  {
  h264_priv_t * priv = parser->priv;
  if(priv->rbsp_alloc < priv->nal_len)
    {
    priv->rbsp_alloc = priv->nal_len + 1024;
    priv->rbsp = realloc(priv->rbsp, priv->rbsp_alloc);
    }
  priv->rbsp_len = bgav_h264_decode_nal_rbsp(pos, len, priv->rbsp);
  }

static void handle_sei(bgav_video_parser_t * parser)
  {
  int sei_type, sei_size;
  uint8_t * ptr;
  int header_len;
  int pic_struct;
  h264_priv_t * priv = parser->priv;
  
  ptr = priv->rbsp;

  while(ptr - priv->rbsp < priv->rbsp_len - 2)
    {
    header_len = bgav_h264_decode_sei_message_header(ptr,
                                                     priv->rbsp_len -
                                                     (ptr - priv->rbsp),
                                                     &sei_type, &sei_size);
    ptr += header_len;
    switch(sei_type)
      {
      case 0:
        //        fprintf(stderr, "Got SEI buffering_period\n");
        break;
      case 1:
        bgav_h264_decode_sei_pic_timing(ptr, priv->rbsp_len -
                                        (ptr - priv->rbsp),
                                        &priv->sps,
                                        &pic_struct);
        fprintf(stderr, "Got SEI pic_timing, pic_struct: %d\n", pic_struct);

        switch(pic_struct)
          {
          case 0: // frame
            break;
          case 1: // top field
          case 2: // bottom field
            parser->cache[parser->cache_size-1].field_pic = 1;
            break;
          case 3: // top field, bottom field, in that order 
            break;
          case 4: // bottom field, top field, in that order 
            break;
          case 5: // top field, bottom field, top field repeated, in that order
            break;
          case 6: // bottom field, top field, bottom field 
            break;
          case 7: // frame doubling                         
            break;
          case 8: // frame tripling
            break;
          }

        break;
      case 2:
        //        fprintf(stderr, "Got SEI pan_scan_rect\n");
        break;
      case 3:
        // fprintf(stderr, "Got SEI filler_payload\n");
        break;
      case 4:
        // fprintf(stderr, "Got SEI user_data_registered_itu_t_t35\n");
        break;
      case 5:
        // fprintf(stderr, "Got SEI user_data_unregistered\n");
        break;
      case 6:
        // fprintf(stderr, "Got SEI recovery_point\n");
        break;
      case 7:
        // fprintf(stderr, "Got SEI dec_ref_pic_marking_repetition\n");
        break;
      case 8:
        // fprintf(stderr, "Got SEI spare_pic\n");
        break;
      case 9:
        // fprintf(stderr, "Got SEI scene_info\n");
        break;
      case 10:
        // fprintf(stderr, "Got SEI sub_seq_info\n");
        break;
      case 11:
        // fprintf(stderr, "Got SEI sub_seq_layer_characteristics\n");
        break;
      case 12:
        // fprintf(stderr, "Got SEI sub_seq_characteristics\n");
        break;
      case 13:
        // fprintf(stderr, "Got SEI full_frame_freeze\n");
        break;
      case 14:
        // fprintf(stderr, "Got SEI full_frame_freeze_release\n");
        break;
      case 15:
        // fprintf(stderr, "Got SEI full_frame_snapshot\n");
        break;
      case 16:
        // fprintf(stderr, "Got SEI progressive_refinement_segment_start\n");
        break;
      case 17:
        // fprintf(stderr, "Got SEI progressive_refinement_segment_end\n");
        break;
      case 18:
        // fprintf(stderr, "Got SEI motion_constrained_slice_group_set\n");
        break;
      case 19:
        // fprintf(stderr, "Got SEI film_grain_characteristics\n");
        break;
      case 20:
        // fprintf(stderr, "Got SEI deblocking_filter_display_preference\n");
        break;
      case 21:
        // fprintf(stderr, "Got SEI stereo_video_info\n");
        break;
      case 22:
        // fprintf(stderr, "Got SEI post_filter_hint\n");
        break;
      case 23:
        // fprintf(stderr, "Got SEI tone_mapping_info\n");
        break;
      case 24:
        // fprintf(stderr, "Got SEI scalability_info\n"); /* specified in Annex G */
        break;
      case 25:
        // fprintf(stderr, "Got SEI sub_pic_scalable_layer\n"); /* specified in Annex G */
        break;
      case 26:
        // fprintf(stderr, "Got SEI non_required_layer_rep\n"); /* specified in Annex G */
        break;
      case 27:
        // fprintf(stderr, "Got SEI priority_layer_info\n"); /* specified in Annex G */
        break;
      case 28:
        // fprintf(stderr, "Got SEI layers_not_present\n"); /* specified in Annex G */
        break;
      case 29:
        // fprintf(stderr, "Got SEI layer_dependency_change\n"); /* specified in Annex G */
        break;
      case 30:
        // fprintf(stderr, "Got SEI scalable_nesting\n"); /* specified in Annex G */
        break;
      case 31:
        // fprintf(stderr, "Got SEI base_layer_temporal_hrd\n"); /* specified in Annex G */
        break;
      case 32:
        // fprintf(stderr, "Got SEI quality_layer_integrity_check\n"); /* specified in Annex G */
        break;
      case 33:
        // fprintf(stderr, "Got SEI redundant_pic_property\n"); /* specified in Annex G */
        break;
      case 34:
        // fprintf(stderr, "Got SEI tl0_picture_index\n"); /* specified in Annex G */
        break;
      case 35:
        // fprintf(stderr, "Got SEI tl_switching_point\n"); /* specified in Annex G */
        break;

      
      }
    ptr += sei_size; 
    }
  }

static int handle_nal(bgav_video_parser_t * parser)
  {
  bgav_h264_nal_header_t nh;
  bgav_h264_slice_header_t sh;
  int header_len;
  int primary_pic_type;
  h264_priv_t * priv = parser->priv;
  
  header_len =
    bgav_h264_decode_nal_header(parser->buf.buffer + parser->pos,
                                priv->nal_len, &nh);
  //  fprintf(stderr, "Got NAL: %d (%d bytes)\n", nh.unit_type,
  //          priv->nal_len);

  switch(nh.unit_type)
    {
    case H264_NAL_NON_IDR_SLICE:
    case H264_NAL_IDR_SLICE:
    case H264_NAL_SLICE_PARTITION_A:
      fprintf(stderr, "Got slice\n");
      /* Decode slice header if necessary */
      if(priv->have_sps)
        {
        /* has_picture_start is also set if the sps was found, so we must check for
           coding_type as well */
        if(!priv->has_picture_start || !parser->cache[parser->cache_size-1].coding_type)
          {
          get_rbsp(parser, parser->buf.buffer + parser->pos + header_len,
                   priv->nal_len - header_len);
                
          bgav_h264_slice_header_parse(priv->rbsp, priv->rbsp_len,
                                       &priv->sps,
                                       &sh);
          bgav_h264_slice_header_dump(&priv->sps,
                                      &sh);

          if(!priv->has_picture_start &&
             !bgav_video_parser_set_picture_start(parser))
            return PARSER_ERROR;
          
          switch(sh.slice_type)
            {
            case 2:
            case 7:
              bgav_video_parser_set_coding_type(parser, BGAV_CODING_TYPE_I);
              break;
            case 0:
            case 5:
              bgav_video_parser_set_coding_type(parser, BGAV_CODING_TYPE_P);
              break;
            case 1:
            case 6:
              bgav_video_parser_set_coding_type(parser, BGAV_CODING_TYPE_B);
              break;
            default: /* Assume the worst */
              fprintf(stderr, "Unknown slice type %d\n", sh.slice_type);
                    
            }

          if(sh.field_pic_flag)
            parser->cache[parser->cache_size-1].field_pic = 1;
          
          }
        priv->has_picture_start = 0;
        }
      else
        {
        /* Skip slices before the first SPS */
        fprintf(stderr, "Skipping slice before SPS %d\n", parser->pos);
        bgav_video_parser_flush(parser, parser->pos + priv->nal_len);
        priv->state = H264_NEED_NAL_END;
        priv->has_picture_start = 0;
        return PARSER_CONTINUE;
        }
            
      break;
    case H264_NAL_SLICE_PARTITION_B:
    case H264_NAL_SLICE_PARTITION_C:
      break;
    case H264_NAL_SEI:
      fprintf(stderr, "Got SEI\n");
      if(!priv->has_picture_start)
        {
        if(!bgav_video_parser_set_picture_start(parser))
          return PARSER_ERROR;
        priv->has_picture_start = 1;
        }
            
      get_rbsp(parser, parser->buf.buffer + parser->pos + header_len,
               priv->nal_len - header_len);
      handle_sei(parser);
      break;
    case H264_NAL_SPS:
      fprintf(stderr, "Got SPS\n");
      if(!priv->sps_buffer)
        {
        // fprintf(stderr, "Got SPS %d bytes\n", priv->nal_len);
        // bgav_hexdump(parser->buf.buffer + parser->pos,
        //              priv->nal_len, 16);
              
        get_rbsp(parser,
                 parser->buf.buffer + parser->pos + header_len,
                 priv->nal_len - header_len);
        bgav_h264_sps_parse(parser->opt,
                            &priv->sps,
                            priv->rbsp, priv->rbsp_len);
        bgav_h264_sps_dump(&priv->sps);
              
        priv->sps_len = priv->nal_len;
        priv->sps_buffer = malloc(priv->sps_len);
        memcpy(priv->sps_buffer,
               parser->buf.buffer + parser->pos, priv->sps_len);

        parser->timescale = priv->sps.vui.time_scale;
        parser->frame_duration = priv->sps.vui.num_units_in_tick;

        /* If we have field pictures, half the framerate */
        if(!priv->sps.frame_mbs_only_flag)
          parser->frame_duration *= 2;
        }
      priv->have_sps = 1;

      /* Also set picture start if it didn't already happen */
      if(!priv->has_picture_start)
        {
        if(!bgav_video_parser_set_picture_start(parser))
          return PARSER_ERROR;
        priv->has_picture_start = 1;
        }
      break;
    case H264_NAL_PPS:
      fprintf(stderr, "Got PPS\n");
      if(!priv->pps_buffer)
        {
        priv->pps_len = priv->nal_len;
        priv->pps_buffer = malloc(priv->sps_len);
        memcpy(priv->pps_buffer,
               parser->buf.buffer + parser->pos, priv->pps_len);
        }
      break;
    case H264_NAL_ACCESS_UNIT_DEL:
      primary_pic_type =
        parser->buf.buffer[parser->pos + header_len] >> 5;
      fprintf(stderr, "Got access unit delimiter, pic_type: %d\n",
              primary_pic_type);
      if(!priv->has_picture_start)
        {
        if(!bgav_video_parser_set_picture_start(parser))
          return PARSER_ERROR;
        priv->has_picture_start = 1;
        }
      switch(primary_pic_type)
        {
        case 0:
          bgav_video_parser_set_coding_type(parser, BGAV_CODING_TYPE_I);
          break;
        case 1:
          bgav_video_parser_set_coding_type(parser, BGAV_CODING_TYPE_P);
          break;
        default: /* Assume the worst */
          bgav_video_parser_set_coding_type(parser, BGAV_CODING_TYPE_B);
          break;
        }
      break;
    case H264_NAL_END_OF_SEQUENCE:
      break;
    case H264_NAL_END_OF_STREAM:
      break;
    case H264_NAL_FILLER_DATA:
      break;
    }
  
  //        bgav_video_parser_flush(parser, priv->nal_len);
  parser->pos += priv->nal_len;
  priv->state = H264_NEED_NAL_END;

  if(!parser->header && priv->pps_buffer && priv->sps_buffer)
    {
    parser->header_len = priv->sps_len + priv->pps_len;
    parser->header = malloc(parser->header_len);
    memcpy(parser->header, priv->sps_buffer, priv->sps_len);
    memcpy(parser->header + priv->sps_len, priv->pps_buffer, priv->pps_len);
    return PARSER_HAVE_HEADER;
    }
  
  return PARSER_CONTINUE;
  }

static int parse_h264(bgav_video_parser_t * parser)
  {
  const uint8_t * sc;
  h264_priv_t * priv = parser->priv;
  
  switch(priv->state)
    {
    case H264_NEED_NAL_START:
      sc = bgav_h264_find_nal_start(parser->buf.buffer + parser->pos,
                                    parser->buf.size - parser->pos);
      if(!sc)
        return PARSER_NEED_DATA;
      bgav_video_parser_flush(parser, sc - parser->buf.buffer);
      parser->pos = 0;
      priv->state = H264_NEED_NAL_END;
      break;
    case H264_NEED_NAL_END:
      sc = bgav_h264_find_nal_start(parser->buf.buffer + parser->pos + 5,
                                    parser->buf.size - parser->pos - 5);
      if(!sc)
        return PARSER_NEED_DATA;
      priv->nal_len = sc - (parser->buf.buffer + parser->pos);
      // fprintf(stderr, "Got nal %d bytes\n", priv->nal_len);
      priv->state = H264_HAVE_NAL;
      break;
    case H264_HAVE_NAL:
      return handle_nal(parser);
      break;
    }
  return PARSER_CONTINUE;
  }

static void reset_h264(bgav_video_parser_t * parser)
  {
  h264_priv_t * priv = parser->priv;
  priv->state = H264_NEED_NAL_START;
  priv->have_sps = 0;
  priv->has_picture_start = 0;
  }


static void cleanup_h264(bgav_video_parser_t * parser)
  {
  h264_priv_t * priv = parser->priv;
  bgav_h264_sps_free(&priv->sps);
  if(priv->sps_buffer)
    free(priv->sps_buffer);
  if(priv->pps_buffer)
    free(priv->pps_buffer);
  free(priv);
  }

void bgav_video_parser_init_h264(bgav_video_parser_t * parser)
  {
  h264_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv = priv;
  parser->parse = parse_h264;
  parser->cleanup = cleanup_h264;
  parser->reset = reset_h264;
  
  }
