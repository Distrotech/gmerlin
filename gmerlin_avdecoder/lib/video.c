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
#include <parser.h>

// #define DUMP_TIMESTAMPS

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

  if(!s->timescale && s->data.video.format.timescale)
    s->timescale = s->data.video.format.timescale;
  
  if(s->flags & (STREAM_PARSE_FULL|STREAM_PARSE_FRAME))
    {
    int result, done = 0;
    bgav_packet_t * p;
    bgav_video_parser_t * parser;
    const uint8_t * header;
    int header_len;
    
    parser = bgav_video_parser_create(s->fourcc, s->timescale,
                                      s->opt, s->flags);
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

    /* Set the format, as far as known by the demuxer */
    bgav_video_parser_set_format(parser, &s->data.video.format);
    
    /* Very few formats pass the extradata out-of-band, but still need
       parsing */
    if(s->ext_data)
      {
      if(!bgav_video_parser_set_header(parser, s->ext_data, s->ext_size))
        {
        bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Video parser doesn't support out-of-band header");
        }
      }
    
    /* Start the parser and extract the header */
    
    while(!s->ext_data)
      {
      result = bgav_video_parser_parse(parser);
      switch(result)
        {
        case PARSER_NEED_DATA:
          p = bgav_demuxer_get_packet_read(s->demuxer, s);

          if(!p)
            {
            bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                     "EOF while initializing video parser");
            return 0;
            }
          bgav_video_parser_add_packet(parser, p);
          bgav_demuxer_done_packet_read(s->demuxer, p);
          break;
        case PARSER_HAVE_HEADER:
          done = 1;
          
          header = bgav_video_parser_get_header(parser, &header_len);
          
          //          fprintf(stderr, "Got extradata %d bytes\n", header_len);
          //          bgav_hexdump(header, header_len, 16);
          
          s->ext_size = header_len;
          s->ext_data = malloc(s->ext_size);
          
          memcpy(s->ext_data, header, s->ext_size);

          s->flags |= STREAM_HEADER_FROM_PARSER;
          
          break;
        }
      }
    
    s->data.video.max_ref_frames = bgav_video_parser_max_ref_frames(parser);
    
    s->data.video.parser = parser;
    s->parsed_packet = bgav_packet_create();
    s->index_mode = INDEX_MODE_SIMPLE;
    }

  if(s->flags & STREAM_START_TIME)
    {
    bgav_packet_t * p;
    char tmp_string[128];
    p = bgav_demuxer_peek_packet_read(s->demuxer, s, 1);
    if(!p)
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "EOF while getting start time");
      }
    s->start_time = p->pts;
    s->out_time = s->start_time;

    sprintf(tmp_string, "%" PRId64, s->out_time);
    bgav_log(s->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Got initial video timestamp: %s",
             tmp_string);
    }
  
  s->in_position = 0;
  
  if(s->action == BGAV_STREAM_DECODE)
    {
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

    result = dec->init(s);
    if(!result)
      return 0;
    }
  
  return 1;
  }

const char * bgav_get_video_description(bgav_t * b, int s)
  {
  return b->tt->cur->video_streams[s].description;
  }


static int bgav_video_decode(bgav_stream_t * s,
                             gavl_video_frame_t* frame)
  {
  int result;

  result = s->data.video.decoder->decoder->decode(s, frame);
  if(!result)
    return result;
  
  /* Set the final timestamp for the frame */
  
  if(frame)
    {
#ifdef DUMP_TIMESTAMPS
    bgav_dprintf("Video timestamp: %"PRId64"\n", frame->timestamp);
#endif    
    s->out_time = frame->timestamp + frame->duration;
    
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
    
    }

  s->flags &= ~STREAM_HAVE_PICTURE;
  
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

    if(s->flags & STREAM_HEADER_FROM_PARSER)
      {
      free(s->ext_data);
      s->ext_data = NULL;
      }
    }
  
  if(s->data.video.decoder)
    {
    s->data.video.decoder->decoder->close(s);
    free(s->data.video.decoder);
    s->data.video.decoder = (bgav_video_decoder_context_t*)0;
    }
  /* Clear still mode flag (it will be set during reinit */
  s->flags &= ~(STREAM_STILL_MODE | STREAM_STILL_SHOWN  | STREAM_HAVE_PICTURE);

  if(s->data.video.parser)
    bgav_video_parser_destroy(s->data.video.parser);

  if(s->data.video.kft)
    {
    bgav_keyframe_table_destroy(s->data.video.kft);
    s->data.video.kft = NULL;
    }
  }

void bgav_video_resync(bgav_stream_t * s)
  {
  if(s->out_time == BGAV_TIMESTAMP_UNDEFINED)
    {
    s->out_time =
      gavl_time_rescale(s->timescale,
                        s->data.video.format.timescale,
                        STREAM_GET_SYNC(s));
    }

  s->flags &= ~STREAM_HAVE_PICTURE;
  
  if(s->data.video.parser)
    {
    if(s->parsed_packet)
      s->parsed_packet->valid = 0;
    bgav_video_parser_reset(s->data.video.parser,
                            BGAV_TIMESTAMP_UNDEFINED, s->out_time);
    }
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

int bgav_video_skipto(bgav_stream_t * s, int64_t * time, int scale,
                      int exact)
  {
  bgav_packet_t * p; 
  //  gavl_time_t stream_time;
  int result;
  int64_t time_scaled;
  int64_t next_key_frame;
  
  time_scaled =
    gavl_time_rescale(scale, s->data.video.format.timescale, *time);
  
  if(s->flags & STREAM_STILL_MODE)
    {
    /* Nothing to do */
    return 1;
    }
  else if(s->out_time > time_scaled)
    {
    if(exact)
      {
      char tmp_string1[128];
      char tmp_string2[128];
      char tmp_string3[128];
      sprintf(tmp_string1, "%" PRId64, s->out_time);
      sprintf(tmp_string2, "%" PRId64, time_scaled);
      sprintf(tmp_string3, "%" PRId64, time_scaled - s->out_time);
      
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Cannot skip backwards: Stream time: %s skip time: %s difference: %s",
               tmp_string1, tmp_string2, tmp_string3);
      }
    return 1;
    }
  
  /* Easy case: Intra only streams */
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
    *time = gavl_time_rescale(s->data.video.format.timescale, scale, s->out_time);
    return 1;
    }

  /* Fast path: Skip to next keyframe */
#if 1
  if(s->data.video.decoder->decoder->resync)
    {
    next_key_frame = BGAV_TIMESTAMP_UNDEFINED;
    if(!exact)
      next_key_frame = bgav_video_stream_keyframe_after(s, time_scaled);
    
    if(next_key_frame == BGAV_TIMESTAMP_UNDEFINED)
      next_key_frame = bgav_video_stream_keyframe_before(s, time_scaled);
    
    if((next_key_frame != BGAV_TIMESTAMP_UNDEFINED) &&
       (next_key_frame > s->out_time) &&
       ((next_key_frame <= time_scaled) ||
        (!exact &&
         (gavl_time_unscale(s->data.video.format.timescale, next_key_frame - time_scaled) <
          GAVL_TIME_SCALE/20))))
      {
      while(1)
        {
        p = bgav_demuxer_peek_packet_read(s->demuxer, s, 1);
        
        if(p->pts >= next_key_frame)
          break;

        // fprintf(stderr, "Skipping to next keyframe %ld %ld\n", p->pts, next_key_frame);
        
        p = bgav_demuxer_get_packet_read(s->demuxer, s);
        bgav_demuxer_done_packet_read(s->demuxer, p);
        }
      s->data.video.decoder->decoder->resync(s);
      }
    }
#endif
  
  if(s->data.video.decoder->decoder->skipto)
    {
    if(!s->data.video.decoder->decoder->skipto(s, time_scaled, exact))
      return 0;
    }
  else
    {
    while(1)
      {
      p = bgav_demuxer_peek_packet_read(s->demuxer, s, 1);
      
      if(!p)
        {
        s->out_time = BGAV_TIMESTAMP_UNDEFINED;
        return 0;
        }

      //      fprintf(stderr, "Peek packet: %ld %ld %ld\n",
      //              p->pts, p->duration, time_scaled);
      
      if(p->pts + p->duration > time_scaled)
        {
        s->out_time = p->pts;
        return 1;
        }
      result = bgav_video_decode(s, (gavl_video_frame_t*)0);
      if(!result)
        {
        s->out_time = BGAV_TIMESTAMP_UNDEFINED;
        return 0;
        }
      }
    }

  *time = gavl_time_rescale(s->data.video.format.timescale, scale, s->out_time);
  
  return 1;
  }

void bgav_skip_video(bgav_t * bgav, int stream,
                     int64_t * time, int scale,
                     int exact)
  {
  bgav_stream_t * s;
  s = &bgav->tt->cur->video_streams[stream];
  bgav_video_skipto(s, time, scale, exact);
  }

int bgav_video_has_still(bgav_t * bgav, int stream)
  {
  bgav_stream_t * s;
  s = &bgav->tt->cur->video_streams[stream];

  if(s->flags & STREAM_HAVE_PICTURE)
    return 1;
  if(bgav_packet_buffer_peek_packet_read(s->packet_buffer, 0))
    return 1;
  else if(s->flags & STREAM_EOF_D)
    return 1;
  
  return 0;
  }

static void frame_table_append_frame(gavl_frame_table_t * t,
                                     int64_t time,
                                     int64_t * last_time)
  {
  if(*last_time != BGAV_TIMESTAMP_UNDEFINED)
    gavl_frame_table_append_entry(t, time - *last_time);
  *last_time = time;
  }

/* Create frame table from file index */

static gavl_frame_table_t * create_frame_table_fi(bgav_stream_t * s)
  {
  gavl_frame_table_t * ret;
  int i;
  int last_non_b_index = -1;
  bgav_file_index_t * fi = s->file_index;
  
  int64_t last_time = BGAV_TIMESTAMP_UNDEFINED;
  
  ret = gavl_frame_table_create();
  ret->offset = s->start_time;
  
  for(i = 0; i < fi->num_entries; i++)
    {
    if((fi->entries[i].flags & 0xff) == BGAV_CODING_TYPE_B)
      {
      frame_table_append_frame(ret,
                               fi->entries[i].pts,
                               &last_time);
      }
    else
      {
      if(last_non_b_index >= 0)
        frame_table_append_frame(ret,
                                 fi->entries[last_non_b_index].pts,
                                 &last_time);
      last_non_b_index = i;
      }
    }

  /* Flush last non B-frame */
  
  if(last_non_b_index >= 0)
    {
    frame_table_append_frame(ret,
                             fi->entries[last_non_b_index].pts,
                             &last_time);
    }

  /* Flush last frame */
  gavl_frame_table_append_entry(ret, s->duration - last_time);
  
  return ret;
  }

/* Create frame table from superindex */

static gavl_frame_table_t *
create_frame_table_si(bgav_stream_t * s, bgav_superindex_t * si)
  {
  int i;
  gavl_frame_table_t * ret;
  int last_non_b_index = -1;
  
  ret = gavl_frame_table_create();

  for(i = 0; i < si->num_entries; i++)
    {
    if(si->entries[i].stream_id == s->stream_id)
      {
      if((si->entries[i].flags & 0xff) == BGAV_CODING_TYPE_B)
        {
        gavl_frame_table_append_entry(ret, si->entries[i].duration);
        }
      else
        {
        if(last_non_b_index >= 0)
          gavl_frame_table_append_entry(ret, si->entries[last_non_b_index].duration);
        last_non_b_index = i;
        }
      }
    }

  if(last_non_b_index >= 0)
    gavl_frame_table_append_entry(ret, si->entries[last_non_b_index].duration);
  
  return ret;
  }

gavl_frame_table_t * bgav_get_frame_table(bgav_t * bgav, int stream)
  {
  bgav_stream_t * s;
  s = &bgav->tt->cur->video_streams[stream];

  if(s->file_index)
    {
    return create_frame_table_fi(s);
    }
  else if(bgav->demuxer->si)
    {
    return create_frame_table_si(s, bgav->demuxer->si);
    }
  else
    return NULL;
  }
