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

#include <config.h>
#include <avdec_private.h>
#include <videoparser.h>
#include <mpv_header.h>

#define MAX_B_FRAMES 16

typedef void (*init_func)(bgav_video_parser_t*);

typedef int (*parse_func)(bgav_video_parser_t*);
typedef void (*cleanup_func)(bgav_video_parser_t*);
typedef void (*reset_func)(bgav_video_parser_t*);

typedef struct
  {
  int coding_type;
  int size;
  int duration;
  int64_t pts;
  int64_t position;
  
  int skip;
  } cache_t;

struct bgav_video_parser_s
  {
  int raw;
  bgav_bytebuffer_t buf;
  
  int flags;
  int pos;
  
  parse_func parse;
  cleanup_func cleanup;
  reset_func reset;

  const bgav_options_t * opt;
  
  /* Extradata */
  uint8_t * header;
  int header_len;
  
  /* Private data for parsers */
  void * priv;
  
  /* Raw byte offset of the start of the parser buffer */
  int64_t raw_position;
  
  /* Timescales */
  int in_scale;

  int timescale;
  int frame_duration;

  /* Cache */
  cache_t cache[MAX_B_FRAMES];
  int cache_size;
  
  int low_delay;
  
  int64_t timestamp;

  int eof;
  
  int non_b_count;
  };

/* Parse functions (come below) */

static void init_h264(bgav_video_parser_t*);
static void init_mpeg12(bgav_video_parser_t*);

static const struct
  {
  uint32_t fourcc;
  init_func func;
  }
parsers[] =
  {
    { BGAV_MK_FOURCC('H', '2', '6', '4'), init_h264 },
    { BGAV_MK_FOURCC('m', 'p', 'g', 'v'), init_mpeg12 },
  };

bgav_video_parser_t * bgav_video_parser_create(uint32_t fourcc, int timescale,
                                               const bgav_options_t * opt)
  {
  int i;
  bgav_video_parser_t * ret;
  /* First find a parse function */
  
  init_func func = NULL;
  for(i = 0; i < sizeof(parsers)/sizeof(parsers[0]); i++)
    {
    if(parsers[i].fourcc == fourcc)
      {
      func = parsers[i].func;
      break;
      }
    }

  if(!func)
    return NULL;
  
  ret = calloc(1, sizeof(*ret));
  ret->in_scale = timescale;
  
  ret->raw_position = -1;
  func(ret);
  return ret;
  }


static void extract_header(bgav_video_parser_t * parser)
  {
  parser->header_len = parser->pos;
  parser->header = malloc(parser->header_len);
  memcpy(parser->header, parser->buf.buffer, parser->header_len);
  }

static void update_previous_size(bgav_video_parser_t * parser)
  {
  int i, total;
  if((parser->cache_size) && !parser->cache[parser->cache_size-1].size)
    {
    total = 0;
    for(i = 0; i < parser->cache_size-1; i++)
      total += parser->cache[i].size;
    parser->cache[parser->cache_size-1].size = parser->pos - total;
    }
  }

static void set_coding_type(bgav_video_parser_t * parser, int type)
  {
  parser->cache[parser->cache_size-1].coding_type = type;
  if(type != BGAV_CODING_TYPE_B)
    parser->non_b_count++;
  else if(parser->non_b_count < 2)
    parser->cache[parser->cache_size-1].skip = 1;
  }

static void set_picture_position(bgav_video_parser_t * parser)
  {
  if(parser->raw)
    {
    int offset = 0, i;

    for(i = 0; i < parser->cache_size-1; i++)
      offset += parser->cache[i].size;
    parser->cache[parser->cache_size-1].position =
      parser->raw_position + offset;
    }
  }
     
static void calc_timestamps(bgav_video_parser_t * parser)
  {
  int i;

  if(!parser->cache_size ||
     parser->cache[0].pts != BGAV_TIMESTAMP_UNDEFINED)
    return;

  /* Need at least 2 frames */
  if((parser->cache_size < 2) && !(parser->eof))
    return;

  /* Low delay or last frame */  
  if(parser->low_delay || (parser->cache_size == 1))
    {
    parser->cache[0].pts = parser->timestamp;
    parser->timestamp += parser->cache[0].duration;
    return;
    }

  /* I P */
  if((parser->cache[0].coding_type != BGAV_CODING_TYPE_B) && 
     ((parser->cache[1].coding_type != BGAV_CODING_TYPE_B) ||
      parser->cache[1].skip))
    {
    parser->cache[0].pts = parser->timestamp;
    parser->timestamp += parser->cache[0].duration;
    return;
    }
  
  /* P B B P */
  
  if((parser->cache[0].coding_type != BGAV_CODING_TYPE_B) && 
     (parser->cache[1].coding_type == BGAV_CODING_TYPE_B) &&
     ((parser->cache[parser->cache_size-1].coding_type !=
       BGAV_CODING_TYPE_B) || parser->eof))
    {
    i = 1;
    
    while(parser->cache[i].coding_type == BGAV_CODING_TYPE_B)
      {
      if(!parser->cache[i].skip)
        {
        parser->cache[i].pts = parser->timestamp;
        parser->timestamp += parser->cache[i].duration;
        }
      i++;
      }
    parser->cache[0].pts = parser->timestamp;
    parser->timestamp += parser->cache[0].duration;
    //    fprintf(stderr, "** PTS: %ld\n", parser->cache[0].pts);
    }
  
  }

static int check_output(bgav_video_parser_t * parser)
  {
  if(!parser->cache_size)
    return 0;

  calc_timestamps(parser);
  
  if(((parser->cache[0].pts != BGAV_TIMESTAMP_UNDEFINED) ||
      parser->cache[0].skip) && parser->cache[0].size)
    return 1;
  else
    return 0;
  }

void bgav_video_parser_destroy(bgav_video_parser_t * parser)
  {
  if(parser->cleanup)
    parser->cleanup(parser);
  if(parser->header)
    free(parser->header);
  bgav_bytebuffer_free(&parser->buf);
  free(parser);
  }

void bgav_video_parser_reset(bgav_video_parser_t * parser, int64_t pts)
  {
  bgav_bytebuffer_flush(&parser->buf);
  parser->flags = 0;
  parser->raw_position = -1;
  parser->cache_size = 0;
  parser->eof = 0;
  parser->timestamp = pts;
  parser->pos = 0;
  parser->non_b_count = 0;
  if(parser->reset)
    parser->reset(parser);
  }

int bgav_video_parser_get_out_scale(bgav_video_parser_t * parser)
  {
  return parser->timescale;
  }

const uint8_t * bgav_video_parser_get_header(bgav_video_parser_t * parser, int * header_len)
  {
  *header_len = parser->header_len;
  return parser->header;
  }

void bgav_video_parser_set_eof(bgav_video_parser_t * parser)
  {
  fprintf(stderr, "EOF buf: %d %d %d\n", parser->buf.size, parser->pos,
          parser->cache_size);
  /* Set size of last frame */
  parser->pos = parser->buf.size;
  update_previous_size(parser);
  parser->eof = 1;
  }

int bgav_video_parser_parse(bgav_video_parser_t * parser)
  {
  if(parser->eof && !parser->cache_size)
    return PARSER_EOF;
  
  if(check_output(parser))
    return PARSER_HAVE_PACKET;
  
  if(!parser->buf.size)
    return PARSER_NEED_DATA;
  return parser->parse(parser);
  }

void bgav_video_parser_add_packet(bgav_video_parser_t * parser,
                                  bgav_packet_t * p)
  {

  }

void bgav_video_parser_add_data(bgav_video_parser_t * parser,
                                uint8_t * data, int len, int64_t position)
  {
  parser->raw = 1;
  bgav_bytebuffer_append_data(&parser->buf, data, len, 0);
  if(parser->raw_position < 0)
    parser->raw_position = position;
  }

static void parser_flush(bgav_video_parser_t * parser, int bytes)
  {
  bgav_bytebuffer_remove(&parser->buf, bytes);
  parser->pos -= bytes;
  if(parser->raw)
    parser->raw_position += bytes;
  }

void bgav_video_parser_get_packet(bgav_video_parser_t * parser,
                                  bgav_packet_t * p)
  {
  cache_t * c;
  c = &parser->cache[0];
  
  bgav_packet_alloc(p, c->size);
  memcpy(p->data, parser->buf.buffer, c->size);
  p->data_size = c->size;
    
  parser_flush(parser, c->size);

  p->pts = c->pts;
  p->duration = c->duration;
  p->keyframe = (c->coding_type == BGAV_CODING_TYPE_I) ? 1 : 0;
  p->position = c->position;
  
  parser->cache_size--;
  if(parser->cache_size)
    memmove(&parser->cache[0], &parser->cache[1],
            sizeof(parser->cache[0]) * parser->cache_size);
  
  }


/* Parse functions */

/* MPEG-1/2 */

#define MPEG_NEED_SYNC                   0
#define MPEG_NEED_STARTCODE              1
#define MPEG_HAS_PICTURE_CODE            2
#define MPEG_HAS_PICTURE_HEADER          3
#define MPEG_HAS_PICTURE_EXT_CODE        4
#define MPEG_HAS_PICTURE_EXT_HEADER      5
#define MPEG_GOP_CODE                    6
#define MPEG_HAS_SEQUENCE_CODE           7
#define MPEG_HAS_SEQUENCE_HEADER         8
#define MPEG_HAS_SEQUENCE_EXT_CODE       9
#define MPEG_HAS_SEQUENCE_EXT_HEADER     10

typedef struct
  {
  /* Sequence header */
  bgav_mpv_sequence_header_t sh;
  int have_sh;
  
  int state;
  } mpeg12_priv_t;

#define MPEG_CODE_SEQUENCE     1
#define MPEG_CODE_SEQUENCE_EXT 2
#define MPEG_CODE_PICTURE      3
#define MPEG_CODE_PICTURE_EXT  4
#define MPEG_CODE_GOP          5

static int get_code_mpeg12(const uint8_t * data)
  {
  if(bgav_mpv_sequence_header_probe(data))
    {
    fprintf(stderr, "MPEG_SEQUENCE\n");
    return MPEG_CODE_SEQUENCE;
    }
  else if(bgav_mpv_sequence_extension_probe(data))
    {
    fprintf(stderr, "MPEG_SEQUENCE_EXT\n");
    return MPEG_CODE_SEQUENCE_EXT;
    }
  else if(bgav_mpv_gop_header_probe(data))
    {
    //    fprintf(stderr, "MPEG_GOP\n");
    return MPEG_CODE_GOP;
    }
  else if(bgav_mpv_picture_header_probe(data))
    {
    //    fprintf(stderr, "MPEG_PICTURE\n");
    return MPEG_CODE_PICTURE;
    }
  else if(bgav_mpv_picture_extension_probe(data))
    {
    //    fprintf(stderr, "MPEG_PICTURE_EXT\n");
    return MPEG_CODE_PICTURE_EXT;
    }
  else
    return 0;
  }

static void reset_mpeg12(bgav_video_parser_t * parser)
  {
  mpeg12_priv_t * priv = parser->priv;
  priv->state = MPEG_NEED_SYNC;
  }

static int parse_mpeg12(bgav_video_parser_t * parser)
  {
  const uint8_t * sc;
  int len;
  mpeg12_priv_t * priv = parser->priv;
  cache_t * c;
  bgav_mpv_picture_extension_t pe;
  bgav_mpv_picture_header_t    ph;
  int duration;
  int start_code;
  
  while(1)
    {
    switch(priv->state)
      {
      case MPEG_NEED_SYNC:
        sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                     parser->buf.buffer + parser->buf.size);
        if(!sc)
          return PARSER_NEED_DATA;
        parser_flush(parser, sc - parser->buf.buffer);
        parser->pos = 0;
        priv->state = MPEG_NEED_STARTCODE;
        break;
      case MPEG_NEED_STARTCODE:
        sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                     parser->buf.buffer + parser->buf.size);
        if(!sc)
          return PARSER_NEED_DATA;
        
        start_code = get_code_mpeg12(sc);
        parser->pos = sc - parser->buf.buffer;

        switch(start_code)
          {
          case MPEG_CODE_SEQUENCE:
            update_previous_size(parser);

            if(!priv->have_sh)
              priv->state = MPEG_HAS_SEQUENCE_CODE;
            else
              {
              priv->state = MPEG_NEED_STARTCODE;
              parser->pos+=4;
              }
            break;
          case MPEG_CODE_SEQUENCE_EXT:
            if(priv->have_sh && !priv->sh.mpeg2)
              priv->state = MPEG_HAS_SEQUENCE_EXT_CODE;
            else
              {
              priv->state = MPEG_NEED_STARTCODE;
              parser->pos+=4;
              }
            break;
          case MPEG_CODE_PICTURE:
            update_previous_size(parser);
            
            /* Reserve cache entry */
            parser->cache_size++;
            c = &parser->cache[parser->cache_size-1];
            memset(c, 0, sizeof(*c));
            c->duration = parser->frame_duration;
            c->pts = BGAV_TIMESTAMP_UNDEFINED;

            set_picture_position(parser);
            
            /* Need the picture header */
            priv->state = MPEG_HAS_PICTURE_CODE;

            if(!parser->header)
              {
              extract_header(parser);
              return PARSER_HAVE_HEADER;
              }
            break;
          case MPEG_CODE_PICTURE_EXT:
            /* Need picture extension */
            priv->state = MPEG_HAS_PICTURE_EXT_CODE;
            break;
          case MPEG_CODE_GOP:
            update_previous_size(parser);

            if(!parser->header)
              {
              extract_header(parser);
              parser->pos += 4;
              priv->state = MPEG_NEED_STARTCODE;
              return PARSER_HAVE_HEADER;
              }
            
            parser->pos += 4;
            priv->state = MPEG_NEED_STARTCODE;
            break;
          default:
            parser->pos += 4;
            priv->state = MPEG_NEED_STARTCODE;
            break;
          }
        
        break;
      case MPEG_HAS_PICTURE_CODE:
        /* Try to get the picture header */

        len = bgav_mpv_picture_header_parse(parser->opt,
                                            &ph,
                                            parser->buf.buffer + parser->pos,
                                            parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;

        set_coding_type(parser, ph.coding_type);
        
        //        fprintf(stderr, "Pic type: %c\n", ph.coding_type);
        parser->pos += len;
        priv->state = MPEG_NEED_STARTCODE;
        
        /* Now we know the coding type, we can check for output */
        if(check_output(parser))
          return PARSER_HAVE_PACKET;
        
        break;
      case MPEG_HAS_PICTURE_EXT_CODE:
        /* Try to get the picture extension */
        len = bgav_mpv_picture_extension_parse(parser->opt,
                                               &pe,
                                               parser->buf.buffer + parser->pos,
                                               parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;
        if(pe.repeat_first_field)
          {
          duration = 0;
          if(priv->sh.ext.progressive_sequence)
            {
            if(pe.top_field_first)
              duration = parser->frame_duration * 2;
            else
              duration = parser->frame_duration;
            }
          else if(pe.progressive_frame)
            duration = parser->frame_duration / 2;
          c->duration += duration;
          }
        parser->pos += len;
        priv->state = MPEG_NEED_STARTCODE;
        break;
      case MPEG_GOP_CODE:
        break;
      case MPEG_HAS_SEQUENCE_CODE:
        /* Try to get the sequence header */

        len = bgav_mpv_sequence_header_parse(parser->opt,
                                             &priv->sh,
                                             parser->buf.buffer + parser->pos,
                                             parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;

        parser->pos += len;
        priv->have_sh = 1;
        
        parser->timescale      = priv->sh.timescale;
        parser->frame_duration = priv->sh.frame_duration;
        priv->state = MPEG_NEED_STARTCODE;
        break;
      case MPEG_HAS_SEQUENCE_EXT_CODE:
        /* Try to get the sequence extension */
        len = bgav_mpv_sequence_extension_parse(parser->opt,
                                                &priv->sh.ext,
                                                parser->buf.buffer + parser->pos,
                                                parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;
        
        priv->sh.mpeg2 = 1;
        
        parser->timescale      *= (priv->sh.ext.timescale_ext+1);
        parser->frame_duration *= (priv->sh.ext.frame_duration_ext+1);
        parser->timescale      *= 2;
        parser->frame_duration *= 2;
        parser->pos += len;
        priv->state = MPEG_NEED_STARTCODE;
        break;
      }
    }
  }

static void cleanup_mpeg12(bgav_video_parser_t * parser)
  {
  free(parser->priv);
  }

static void init_mpeg12(bgav_video_parser_t * parser)
  {
  mpeg12_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv = priv;
  parser->parse = parse_mpeg12;
  parser->cleanup = cleanup_mpeg12;
  parser->reset = reset_mpeg12;
  }

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
  
  int state;

  int nal_len;

  uint8_t * rbsp;
  int rbsp_alloc;
  int rbsp_len;

  int has_access_units;
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
        fprintf(stderr, "Got SEI buffering_period\n");
        break;
      case 1:
        fprintf(stderr, "Got SEI pic_timing\n");
        break;
      case 2:
        fprintf(stderr, "Got SEI pan_scan_rect\n");
        break;
      case 3:
        fprintf(stderr, "Got SEI filler_payload\n");
        break;
      case 4:
        fprintf(stderr, "Got SEI user_data_registered_itu_t_t35\n");
        break;
      case 5:
        fprintf(stderr, "Got SEI user_data_unregistered\n");
        break;
      case 6:
        fprintf(stderr, "Got SEI recovery_point\n");
        break;
      case 7:
        fprintf(stderr, "Got SEI dec_ref_pic_marking_repetition\n");
        break;
      case 8:
        fprintf(stderr, "Got SEI spare_pic\n");
        break;
      case 9:
        fprintf(stderr, "Got SEI scene_info\n");
        break;
      case 10:
        fprintf(stderr, "Got SEI sub_seq_info\n");
        break;
      case 11:
        fprintf(stderr, "Got SEI sub_seq_layer_characteristics\n");
        break;
      case 12:
        fprintf(stderr, "Got SEI sub_seq_characteristics\n");
        break;
      case 13:
        fprintf(stderr, "Got SEI full_frame_freeze\n");
        break;
      case 14:
        fprintf(stderr, "Got SEI full_frame_freeze_release\n");
        break;
      case 15:
        fprintf(stderr, "Got SEI full_frame_snapshot\n");
        break;
      case 16:
        fprintf(stderr, "Got SEI progressive_refinement_segment_start\n");
        break;
      case 17:
        fprintf(stderr, "Got SEI progressive_refinement_segment_end\n");
        break;
      case 18:
        fprintf(stderr, "Got SEI motion_constrained_slice_group_set\n");
        break;
      case 19:
        fprintf(stderr, "Got SEI film_grain_characteristics\n");
        break;
      case 20:
        fprintf(stderr, "Got SEI deblocking_filter_display_preference\n");
        break;
      case 21:
        fprintf(stderr, "Got SEI stereo_video_info\n");
        break;
      case 22:
        fprintf(stderr, "Got SEI post_filter_hint\n");
        break;
      case 23:
        fprintf(stderr, "Got SEI tone_mapping_info\n");
        break;
      case 24:
        fprintf(stderr, "Got SEI scalability_info\n"); /* specified in Annex G */
        break;
      case 25:
        fprintf(stderr, "Got SEI sub_pic_scalable_layer\n"); /* specified in Annex G */
        break;
      case 26:
        fprintf(stderr, "Got SEI non_required_layer_rep\n"); /* specified in Annex G */
        break;
      case 27:
        fprintf(stderr, "Got SEI priority_layer_info\n"); /* specified in Annex G */
        break;
      case 28:
        fprintf(stderr, "Got SEI layers_not_present\n"); /* specified in Annex G */
        break;
      case 29:
        fprintf(stderr, "Got SEI layer_dependency_change\n"); /* specified in Annex G */
        break;
      case 30:
        fprintf(stderr, "Got SEI scalable_nesting\n"); /* specified in Annex G */
        break;
      case 31:
        fprintf(stderr, "Got SEI base_layer_temporal_hrd\n"); /* specified in Annex G */
        break;
      case 32:
        fprintf(stderr, "Got SEI quality_layer_integrity_check\n"); /* specified in Annex G */
        break;
      case 33:
        fprintf(stderr, "Got SEI redundant_pic_property\n"); /* specified in Annex G */
        break;
      case 34:
        fprintf(stderr, "Got SEI tl0_picture_index\n"); /* specified in Annex G */
        break;
      case 35:
        fprintf(stderr, "Got SEI tl_switching_point\n"); /* specified in Annex G */
        break;

      
      }
    ptr += sei_size; 
    }
  }

static int parse_h264(bgav_video_parser_t * parser)
  {
  int primary_pic_type;
  bgav_h264_nal_header_t nh;
  const uint8_t * sc;
  const uint8_t * ptr;
  int header_len;
  cache_t * c;
  
  h264_priv_t * priv = parser->priv;
  
  
  while(1)
    {
    switch(priv->state)
      {
      case H264_NEED_NAL_START:
        sc = bgav_h264_find_nal_start(parser->buf.buffer + parser->pos,
                                      parser->buf.size - parser->pos);
        if(!sc)
          return PARSER_NEED_DATA;
        parser_flush(parser, sc - parser->buf.buffer);
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
        header_len =
          bgav_h264_decode_nal_header(parser->buf.buffer + parser->pos,
                                      priv->nal_len, &nh);
        fprintf(stderr, "Got NAL: %d (%d bytes)\n", nh.unit_type,
                priv->nal_len);
        
        switch(nh.unit_type)
          {
          case H264_NAL_NON_IDR_SLICE:
            break;
          case H264_NAL_SLICE_PARTITION_A:
          case H264_NAL_SLICE_PARTITION_B:
          case H264_NAL_SLICE_PARTITION_C:
            break;
          case H264_NAL_IDR_SLICE:
            break;
          case H264_NAL_SEI:
            get_rbsp(parser, parser->buf.buffer + parser->pos + header_len,
                     priv->nal_len - header_len);
            handle_sei(parser);
            break;
          case H264_NAL_SPS:
            if(!priv->sps_buffer)
              {
              fprintf(stderr, "Got SPS %d bytes\n", priv->nal_len);
              bgav_hexdump(parser->buf.buffer + parser->pos,
                           priv->nal_len, 16);
              
              get_rbsp(parser, parser->buf.buffer + parser->pos + header_len, priv->nal_len - header_len);
              bgav_h264_sps_parse(parser->opt,
                                  &priv->sps,
                                  priv->rbsp, priv->rbsp_len);
              bgav_h264_sps_dump(&priv->sps);
              
              priv->sps_len = priv->nal_len;
              priv->sps_buffer = malloc(priv->sps_len);
              memcpy(priv->sps_buffer, parser->buf.buffer + parser->pos, priv->sps_len);
              }
            break;
          case H264_NAL_PPS:
            if(!priv->pps_buffer)
              {
              priv->pps_len = priv->nal_len;
              priv->pps_buffer = malloc(priv->sps_len);
              memcpy(priv->pps_buffer, parser->buf.buffer + parser->pos, priv->pps_len);
              }
            break;
          case H264_NAL_ACCESS_UNIT_DEL:
#if 0
            priv->has_access_units = 1;
            update_previous_size(parser);
            
            /* Reserve cache entry */
            parser->cache_size++;
            c = &parser->cache[parser->cache_size-1];
            memset(c, 0, sizeof(*c));
            c->duration = parser->frame_duration;
            c->pts = BGAV_TIMESTAMP_UNDEFINED;
            
            set_picture_position(parser);

            primary_pic_type =
              parser->buf.buffer[parser->pos + header_len] >> 5;
            
            switch(primary_pic_type)
              {
              case 0:
                set_coding_type(parser, BGAV_CODING_TYPE_I);
                break;
              case 1:
                set_coding_type(parser, BGAV_CODING_TYPE_P);
                break;
              default: /* Assume the worst */
                set_coding_type(parser, BGAV_CODING_TYPE_B);
                break;
              }
#endif
            
            break;
          case H264_NAL_END_OF_SEQUENCE:
            break;
          case H264_NAL_END_OF_STREAM:
            break;
          case H264_NAL_FILLER_DATA:
            break;
          }
                
        parser_flush(parser, priv->nal_len);
        parser->pos = 0;
        priv->state = H264_NEED_NAL_END;
#if 0
        if(!parser->header && priv->pps_buffer && priv->sps_buffer)
          {
          parser->header_len = priv->sps_len + priv->pps_len;
          parser->header = malloc(parser->header_len);
          memcpy(parser->header, priv->sps_buffer, priv->sps_len);
          memcpy(parser->header + priv->sps_len, priv->pps_buffer, priv->pps_len);
          return PARSER_HAVE_HEADER;
          }
#endif
        break;
      }
    }
  
  }

static void cleanup_h264(bgav_video_parser_t * parser)
  {
  h264_priv_t * priv = parser->priv;
  bgav_h264_sps_free(&priv->sps);
  free(priv);
  }

static void init_h264(bgav_video_parser_t * parser)
  {
  h264_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv = priv;
  parser->parse = parse_h264;
  parser->cleanup = cleanup_h264;
  
  }
