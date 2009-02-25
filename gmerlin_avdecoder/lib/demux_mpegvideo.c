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

#include <avdec_private.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <parser.h>

#define LOG_DOMAIN "mpegvideo"

/* Trivial demuxer for MPEG-1/2 video streams */

#define BUFFER_LEN 1024

#define MPEG12_SEQUENCE_HEADER 

typedef struct
  {
  bgav_video_parser_t * parser;
  uint8_t * buffer;
  int buffer_alloc;
  
  int64_t last_position;
  int eof;
  } mpegvideo_priv_t;

static int detect_type(bgav_input_context_t * input)
  {
  char * pos;
  uint32_t header_32;
  uint64_t header_64;
  if(!bgav_input_get_32_be(input, &header_32))
    return 0;

  /* MPEG-1/2 Video */
  if(header_32 == 0x000001b3)
    return BGAV_MK_FOURCC('m', 'p', 'g', 'v');
  else if(header_32 == 0x000001b0)
    return BGAV_MK_FOURCC('C', 'A', 'V', 'S');
  else if(header_32 == 0x0000010f)
    return BGAV_MK_FOURCC('V', 'C', '-', '1');
  
  /* H.264 */
  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(pos && !strcasecmp(pos, ".h264"))
      return BGAV_MK_FOURCC('H', '2', '6', '4');
    }

  /* MPEG-4 */
  if(!bgav_input_get_64_be(input, &header_64))
    return 0;

  /* Check for video_object_start_code followed by
     video_object_layer_start_code */
#if 0
  fprintf(stderr, "Test header: %016lx %016lx\n",
          header_64, header_64 & 0xFFFFFFE0FFFFFFF0LL);
#endif
  if((header_64 & 0xFFFFFFE0FFFFFFF0LL) == 0x0000010000000120LL)
    return BGAV_MK_FOURCC('m', 'p', '4', 'v');
  
  return 0;
  }

static int probe_mpegvideo(bgav_input_context_t * input)
  {
  return detect_type(input) ? 1 : 0;
  }

static int parse(bgav_demuxer_context_t * ctx, int code)
  {
  int state;
  mpegvideo_priv_t * priv;
  int64_t pos;
  int bytes_read;
  
  priv = (mpegvideo_priv_t *)(ctx->priv);
  
  while(1)
    {
    state = bgav_video_parser_parse(priv->parser);

    if(state == code)
      return 1;
    else if(state == PARSER_EOF)
      {
      priv->eof = 1;
      return 0;
      }
    else if(state == PARSER_NEED_DATA)
      {
      pos = ctx->input->position;
      
      if(priv->buffer_alloc < BUFFER_LEN)
        {
        priv->buffer_alloc = BUFFER_LEN;
        priv->buffer = realloc(priv->buffer, priv->buffer_alloc);
        }
      
      bytes_read = bgav_input_read_data(ctx->input, priv->buffer,
                                        BUFFER_LEN);
      
      if(!bytes_read)
        bgav_video_parser_set_eof(priv->parser);
      else
        bgav_video_parser_add_data(priv->parser,
                                   priv->buffer, bytes_read, pos);
      }
    }
  /* Never get here */
  return 0;
  }

static void next_packet_fi(bgav_demuxer_context_t * ctx)
  {
  int bytes_to_read, state, bytes_read;
  mpegvideo_priv_t * priv;
  int64_t pos, end_pos;
  bgav_packet_t * p;
  bgav_stream_t * s;

  
  priv = (mpegvideo_priv_t *)(ctx->priv);
  s = ctx->tt->cur->video_streams;
  
  pos = ctx->input->position;

  //  fprintf(stderr, "Next packet pos: %ld\n", ctx->next_packet_pos);

  end_pos = ctx->next_packet_pos;
  if(end_pos > ctx->input->total_bytes)
    end_pos = ctx->input->total_bytes;
  
  bytes_to_read = end_pos - pos;

  if(priv->buffer_alloc < bytes_to_read)
    {
    priv->buffer_alloc = bytes_to_read + 1024;
    priv->buffer = realloc(priv->buffer, priv->buffer_alloc);
    }

  bytes_read = bgav_input_read_data(ctx->input, priv->buffer,
                                    bytes_to_read);
  bgav_video_parser_add_data(priv->parser,
                             priv->buffer, bytes_read, pos);

  while(1)
    {
    state = bgav_video_parser_parse(priv->parser);
    if(state == PARSER_HAVE_PACKET)
      {
      p = bgav_stream_get_packet_write(s);
      bgav_video_parser_get_packet(priv->parser, p);
      bgav_packet_done_write(p);
      }
    else if(state == PARSER_NEED_DATA)
      break;
    }
  
  /* Flush last packets */
  if(ctx->next_packet_pos > ctx->input->total_bytes)
    {
    bgav_video_parser_set_eof(priv->parser);

    while(1)
      {
      state = bgav_video_parser_parse(priv->parser);
      if(state == PARSER_HAVE_PACKET)
        {
        p = bgav_stream_get_packet_write(s);
        bgav_video_parser_get_packet(priv->parser, p);
        bgav_packet_done_write(p);
        }
      else if(state == PARSER_EOF)
        break;
      }
    }
  
  }

static int next_packet_mpegvideo(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  mpegvideo_priv_t * priv;

  priv = (mpegvideo_priv_t *)(ctx->priv);
  
  s = ctx->tt->cur->video_streams;

  /* Sample accurate: Read data and output as many
     packets as available */
  
  if(ctx->next_packet_pos)
    {
    next_packet_fi(ctx);
    return 1;
    }
  
  if(!parse(ctx, PARSER_HAVE_PACKET))
    return 0;
  
  p = bgav_stream_get_packet_write(s);
  bgav_video_parser_get_packet(priv->parser, p);
  bgav_packet_done_write(p);
  
  return 1;
  }

static int open_mpegvideo(bgav_demuxer_context_t * ctx)
  {
  mpegvideo_priv_t * priv;
  bgav_stream_t * s;
  const uint8_t * header;
  int header_len;
  const gavl_video_format_t * format;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  /* Create track */

  ctx->tt = bgav_track_table_create(1);
  
  s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
  
  /*
   *  We just set the fourcc, everything else will
   *  be set by the parser
   */

  s->fourcc = detect_type(ctx->input);
  s->flags |= (STREAM_PARSE_FULL|STREAM_B_FRAMES);
  priv->parser = bgav_video_parser_create(s->fourcc, 0, ctx->opt, s->flags);
  if(!priv->parser)
    return 0;

  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;

  s->data.video.frametime_mode = BGAV_FRAMETIME_CODEC;

  if(!parse(ctx, PARSER_HAVE_HEADER))
    return 0;

  format = bgav_video_parser_get_format(priv->parser);

  gavl_video_format_copy(&s->data.video.format, format);
                         
  s->timescale = s->data.video.format.timescale;

  header = bgav_video_parser_get_header(priv->parser, &header_len);

  fprintf(stderr, "Got extradata %d bytes\n", header_len);
  bgav_hexdump(header, header_len, 16);

  s->ext_size = header_len;
  s->ext_data = malloc(s->ext_size);
  memcpy(s->ext_data, header, s->ext_size);

  
  
  ctx->tt->cur->duration = GAVL_TIME_UNDEFINED;
  
  ctx->stream_description = bgav_sprintf("Elementary video stream");


  ctx->index_mode = INDEX_MODE_SIMPLE;
  
  return 1;

  }

static void close_mpegvideo(bgav_demuxer_context_t * ctx)
  {
  mpegvideo_priv_t * priv;
  priv = (mpegvideo_priv_t *)(ctx->priv);
  if(priv->buffer)
    free(priv->buffer);
  bgav_video_parser_destroy(priv->parser);
  free(priv);
  }

static void resync_mpegvideo(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  mpegvideo_priv_t * priv;
  priv = (mpegvideo_priv_t *)(ctx->priv);
  bgav_video_parser_reset(priv->parser, BGAV_TIMESTAMP_UNDEFINED, STREAM_GET_SYNC(s));
  //  fprintf(stderr, "resync: %ld\n", s->in_time);
  priv->eof = 0;
  }

static int select_track_mpegvideo(bgav_demuxer_context_t * ctx, int track)
  {
  mpegvideo_priv_t * priv;
  priv = (mpegvideo_priv_t *)(ctx->priv);
  bgav_video_parser_reset(priv->parser, 0, 0);
  priv->eof = 0;
  return 1;
  }


const bgav_demuxer_t bgav_demuxer_mpegvideo =
  {
    .probe =       probe_mpegvideo,
    .open =        open_mpegvideo,
    .next_packet = next_packet_mpegvideo,
    //    .seek =        seek_mpegvideo,
    .close =       close_mpegvideo,
    .resync =      resync_mpegvideo,
    .select_track =      select_track_mpegvideo
  };

