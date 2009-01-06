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
  fprintf(stderr, "Got extradata %d bytes\n", parser->header_len);
  bgav_hexdump(parser->header, parser->header_len < 16 ? parser->header_len : 16, 16);
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

typedef struct
  {
  /* Sequence header */
  bgav_h264_sps_t sps;
  int have_sps;
  int have_pps;
  
  int state;
  } h264_priv_t;


static int parse_h264(bgav_video_parser_t * parser)
  {
  h264_priv_t * priv = parser->priv;
  
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
  priv = calloc(1, sizeof(priv));
  parser->priv = priv;
  parser->parse = parse_h264;
  parser->cleanup = cleanup_h264;
  
  }
