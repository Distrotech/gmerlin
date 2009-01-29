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
#include <stdio.h>
#include <string.h>

#include <avdec_private.h>
#include <videoparser.h>

#define LOG_DOMAIN "video"

int bgav_num_video_streams(bgav_t *  bgav, int track)
  {
  return bgav->tt->tracks[track].num_video_streams;
  }

const gavl_video_format_t * bgav_get_video_format(bgav_t * bgav, int stream)
  {
  return &(bgav->tt->cur->video_streams[stream].data.video.format);
  }

int bgav_set_video_stream(bgav_t * b, int stream, bgav_stream_action_t action)
  {
  if((stream >= b->tt->cur->num_video_streams) ||
     (stream < 0))
    return 0;
  b->demuxer->tt->cur->video_streams[stream].action = action;
  return 1;
  }

int bgav_video_start(bgav_stream_t * s)
  {
  int result;
  bgav_video_decoder_t * dec;
  bgav_video_decoder_context_t * ctx;
  
  if(s->flags & STREAM_PARSE_FULL)
    {
    int result, done = 0;
    bgav_packet_t * p;
    const gavl_video_format_t * format;
    bgav_video_parser_t * parser;
    const uint8_t * header;
    int header_len;
    
    parser = bgav_video_parser_create(s->fourcc,
                                      s->timescale,
                                      s->opt);
    if(!parser)
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "No video parser found for fourcc %c%c%c%c (0x%08x)",
               (s->fourcc & 0xFF000000) >> 24,
               (s->fourcc & 0x00FF0000) >> 16,
               (s->fourcc & 0x0000FF00) >> 8,
               (s->fourcc & 0x000000FF),
               s->fourcc);
      return 0;
      }

    /* Start the parser and extract the header */

    while(!done)
      {
      result = bgav_video_parser_parse(parser);
      switch(result)
        {
        case PARSER_NEED_DATA:
          p = bgav_demuxer_get_packet_read(s->demuxer, s);
          bgav_video_parser_add_packet(parser, p);
          bgav_demuxer_done_packet_read(s->demuxer, p);
          break;
        case PARSER_HAVE_HEADER:
          done = 1;

          format = bgav_video_parser_get_format(parser);
          gavl_video_format_copy(&s->data.video.format, format);

          header = bgav_video_parser_get_header(parser, &header_len);
          
          //          fprintf(stderr, "Got extradata %d bytes\n", header_len);
          //          bgav_hexdump(header, header_len, 16);
          
          s->ext_size = header_len;
          s->ext_data = malloc(s->ext_size);
          memcpy(s->ext_data, header, s->ext_size);
          
          break;
        }
      }
    s->data.video.parser = parser;
    s->data.video.parsed_packet = bgav_packet_create();
    s->index_mode = INDEX_MODE_SIMPLE;
    }
  
  dec = bgav_find_video_decoder(s);
  if(!dec)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "No video decoder found for fourcc %c%c%c%c (0x%08x)",
             (s->fourcc & 0xFF000000) >> 24,
             (s->fourcc & 0x00FF0000) >> 16,
             (s->fourcc & 0x0000FF00) >> 8,
             (s->fourcc & 0x000000FF),
             s->fourcc);
    return 0;
    }
  ctx = calloc(1, sizeof(*ctx));
  s->data.video.decoder = ctx;
  s->data.video.decoder->decoder = dec;
  
  s->out_time = 0;
  s->in_position = 0;
  s->in_time = 0;

  if(!s->timescale)
    {
    s->timescale = s->data.video.format.timescale;
    }
  
  result = dec->init(s);
  if(!result)
    return 0;
  
  return result;
  }

const char * bgav_get_video_description(bgav_t * b, int s)
  {
  return b->tt->cur->video_streams[s].description;
  }


static int bgav_video_decode(bgav_stream_t * s,
                             gavl_video_frame_t* frame)
  {
  int result;

  if(s->eof)
    return 0;
  
  result = s->data.video.decoder->decoder->decode(s, frame);
  if(!result)
    {
    s->eof = 1;
    return result;
    }

  s->out_time = frame->timestamp + frame->duration;
  
  /* Set the final timestamp for the frame */
  
  if(frame)
    {
    /* Set timecode */
    if(s->timecode_table)
      frame->timecode =
        bgav_timecode_table_get_timecode(s->timecode_table,
                                         frame->timestamp);
    else if(s->has_codec_timecode)
      {
      frame->timecode = s->codec_timecode;
      s->has_codec_timecode = 0;
      }
    else
      frame->timecode = GAVL_TIMECODE_UNDEFINED;
    
    if(s->demuxer->demux_mode == DEMUX_MODE_FI)
      frame->timestamp += s->first_timestamp;
    }
  
  return result;
  }

int bgav_read_video(bgav_t * b, gavl_video_frame_t * frame, int s)
  {
  if(b->eof)
    return 0;
  return bgav_video_decode(&(b->tt->cur->video_streams[s]), frame);
  }

void bgav_video_dump(bgav_stream_t * s)
  {
  bgav_dprintf("  Depth:             %d\n", s->data.video.depth);
  bgav_dprintf("Format:\n");
  gavl_video_format_dump(&(s->data.video.format));
  }

void bgav_video_stop(bgav_stream_t * s)
  {
  if(s->data.video.parser)
    {
    bgav_video_parser_destroy(s->data.video.parser);
    s->data.video.parser = NULL;
    }
  
  if(s->data.video.decoder)
    {
    s->data.video.decoder->decoder->close(s);
    free(s->data.video.decoder);
    s->data.video.decoder = (bgav_video_decoder_context_t*)0;
    }
  /* Clear still mode flag (it will be set during reinit */
  s->data.video.still_mode = 0;

  if(s->data.video.parser)
    bgav_video_parser_destroy(s->data.video.parser);
  if(s->data.video.parsed_packet)
    bgav_packet_destroy(s->data.video.parsed_packet);

  }

void bgav_video_resync(bgav_stream_t * s)
  {
  if(s->data.video.decoder->decoder->resync)
    s->data.video.decoder->decoder->resync(s);
  }

/* Skipping to a specified time can happen in 3 ways:
   
   1. For intra-only streams, we can just skip the packets, completely
      bypassing the codec.

   2. For codecs with keyframes but without delay, we call the decode
      method until the next packet (as seen by
      bgav_demuxer_peek_packet_read()) is the right one.

   3. Codecs with delay (i.e. with B-frames) must have a skipto method
      we'll call
*/

int bgav_video_skipto(bgav_stream_t * s, int64_t * time, int scale)
  {
  bgav_packet_t * p; 
  //  gavl_time_t stream_time;
  int result;
  int64_t time_scaled;
  char tmp_string1[128];
  char tmp_string2[128];
  time_scaled = gavl_time_rescale(scale, s->data.video.format.timescale, *time);
  
  if(s->out_time > time_scaled)
    {
    sprintf(tmp_string1, "%" PRId64, s->in_time);
    sprintf(tmp_string2, "%" PRId64, time_scaled);
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN, 
             "Cannot skip backwards, stream_time: %s, sync_time: %s",
             tmp_string1, tmp_string2);
    return 1;
    }
  else if(s->flags & STREAM_INTRA_ONLY)
    {
    while(1)
      {
      p = bgav_demuxer_peek_packet_read(s->demuxer, s, 1);

      if(!p)
        return 0;
      
      if(p->pts + p->duration > time_scaled)
        {
        s->out_time = p->pts;
        return 1;
        }
      p = bgav_demuxer_get_packet_read(s->demuxer, s);
      bgav_demuxer_done_packet_read(s->demuxer, p);
      }
    }
  else if(s->data.video.decoder->decoder->skipto)
    {
    if(!s->data.video.decoder->decoder->skipto(s, time_scaled))
      return 0;
    }
  else
    {
    p = bgav_demuxer_peek_packet_read(s->demuxer, s, 1);
    
    if(!p)
      return 0;
    
    if(p->pts + p->duration > time_scaled)
      return 1;

    result = bgav_video_decode(s, (gavl_video_frame_t*)0);
    if(!result)
      return 0;
    }
  
  *time = gavl_time_rescale(s->data.video.format.timescale, scale, s->out_time);
  //  s->in_time = gavl_time_rescale(scale, s->timescale, *time);
  
  return 1;
  }

