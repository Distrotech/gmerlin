/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
#include <bsf.h>
#include <mpeg4_header.h>

// #define DUMP_TIMESTAMPS

#define LOG_DOMAIN "video"

int bgav_num_video_streams(bgav_t *  bgav, int track)
  {
  return bgav->tt->tracks[track].num_video_streams;
  }

const gavl_video_format_t * bgav_get_video_format(bgav_t * bgav, int stream)
  {
  return &bgav->tt->cur->video_streams[stream].data.video.format;
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
  
  if((s->flags & (STREAM_PARSE_FULL|STREAM_PARSE_FRAME)) &&
     !s->data.video.parser)
    {
    
    s->data.video.parser = bgav_video_parser_create(s);
    if(!s->data.video.parser)
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
    
    /* Get the first packet to garantuee that the parser is fully initialized */
    if(!bgav_stream_peek_packet_read(s, 1))
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "EOF while initializing video parser");
      return 0;
      }
    s->index_mode = INDEX_MODE_SIMPLE;
    }
  /* Packet timer */
  if((s->flags & (STREAM_NO_DURATIONS|STREAM_WRONG_B_TIMESTAMPS)) &&
     !s->pt)
    {
    s->pt = bgav_packet_timer_create(s);

    if(!bgav_stream_peek_packet_read(s, 1))
      {
      bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "EOF while initializing packet timer");
      return 0;
      }
    s->index_mode = INDEX_MODE_SIMPLE;
    }
  
  if(s->flags & STREAM_NEED_START_TIME)
    {
    bgav_packet_t * p;
    char tmp_string[128];
    p = bgav_stream_peek_packet_read(s, 1);
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
    s->flags &= ~STREAM_NEED_START_TIME;
    }

  if((s->action == BGAV_STREAM_PARSE) &&
     ((s->data.video.format.framerate_mode == GAVL_FRAMERATE_VARIABLE) ||
      (s->data.video.format.interlace_mode == GAVL_INTERLACE_MIXED)))
    {
    s->data.video.ft = bgav_video_format_tracker_create(s);
    }
  
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

  if(s->data.video.format.interlace_mode == GAVL_INTERLACE_UNKNOWN)
    s->data.video.format.interlace_mode = GAVL_INTERLACE_NONE;
  if(s->data.video.format.framerate_mode == GAVL_FRAMERATE_UNKNOWN)
    s->data.video.format.framerate_mode = GAVL_FRAMERATE_CONSTANT;
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
  return bgav_video_decode(&b->tt->cur->video_streams[s], frame);
  }

void bgav_video_dump(bgav_stream_t * s)
  {
  bgav_dprintf("  Depth:             %d\n", s->data.video.depth);
  bgav_dprintf("Format:\n");
  gavl_video_format_dump(&s->data.video.format);
  }

void bgav_video_stop(bgav_stream_t * s)
  {
  if(s->data.video.parser)
    {
    bgav_video_parser_destroy(s->data.video.parser);
    s->data.video.parser = NULL;
    }
  if(s->pt)
    {
    bgav_packet_timer_destroy(s->pt);
    s->pt = NULL;
    }
  if(s->data.video.ft)
    {
    bgav_video_format_tracker_destroy(s->data.video.ft);
    s->data.video.ft = NULL;
    }
  if(s->data.video.decoder)
    {
    s->data.video.decoder->decoder->close(s);
    free(s->data.video.decoder);
    s->data.video.decoder = NULL;
    }
  /* Clear still mode flag (it will be set during reinit) */
  s->flags &= ~(STREAM_STILL_MODE | STREAM_STILL_SHOWN  | STREAM_HAVE_PICTURE);
  
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
    bgav_video_parser_reset(s->data.video.parser,
                            BGAV_TIMESTAMP_UNDEFINED, s->out_time);
    }

  if(s->pt)
    {
    bgav_packet_timer_reset(s->pt);
    }

  /* If the stream has keyframes, skip until the next one */

  if(!(s->flags & (STREAM_INTRA_ONLY|STREAM_STILL_MODE)))
    {
    bgav_packet_t * p;
    while(1)
      {
      /* Skip pictures until we have the next keyframe */
      p = bgav_stream_peek_packet_read(s, 1);

      if(!p)
        return;

      if(PACKET_GET_KEYFRAME(p))
        {
        s->out_time = p->pts;
        break;
        }
      /* Skip this packet */
      bgav_log(s->opt, BGAV_LOG_DEBUG, LOG_DOMAIN, "Skipping packet while waiting for keyframe");
      p = bgav_stream_get_packet_read(s);
      bgav_stream_done_packet_read(s, p);
      }
    }
  
  if(s->data.video.decoder->decoder->resync)
    s->data.video.decoder->decoder->resync(s);
  }

/* Skipping to a specified time can happen in 3 ways:
   
   1. For intra-only streams, we can just skip the packets, completely
      bypassing the codec.

   2. For codecs with keyframes but without delay, we call the decode
      method until the next packet (as seen by
      bgav_stream_peek_packet_read()) is the right one.

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
      p = bgav_stream_peek_packet_read(s, 1);

      if(!p)
        return 0;
      
      if(p->pts + p->duration > time_scaled)
        {
        s->out_time = p->pts;
        return 1;
        }
      p = bgav_stream_get_packet_read(s);
      bgav_stream_done_packet_read(s, p);
      }
    *time = gavl_time_rescale(s->data.video.format.timescale, scale, s->out_time);
    return 1;
    }

  /* Fast path: Skip to next keyframe */
#if 0
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
        p = bgav_stream_peek_packet_read(s, 1);
        
        if(p->pts >= next_key_frame)
          break;

        // fprintf(stderr, "Skipping to next keyframe %ld %ld\n", p->pts, next_key_frame);
        
        p = bgav_stream_get_packet_read(s);
        bgav_stream_done_packet_read(s, p);
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
      p = bgav_stream_peek_packet_read(s, 1);
      
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
  if(bgav_stream_peek_packet_read(s, 0))
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

  for(i = 0; i < fi->tt.num_entries; i++)
    {
    gavl_frame_table_append_timecode(ret,
                                     fi->tt.entries[i].pts,
                                     fi->tt.entries[i].timecode);
    }
  
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

  /* Maybe we have timecodes in the timecode table */

  if(s->timecode_table)
    {
    for(i = 0; i < s->timecode_table->num_entries; i++)
      {
      gavl_frame_table_append_timecode(ret,
                                       s->timecode_table->entries[i].pts,
                                       s->timecode_table->entries[i].timecode);
      }
    }
  
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

static uint32_t png_fourccs[] =
  {
    BGAV_MK_FOURCC('p', 'n', 'g', ' '),
    BGAV_MK_FOURCC('M', 'P', 'N', 'G'),
    0x00
  };

static uint32_t jpeg_fourccs[] =
  {
    BGAV_MK_FOURCC('j', 'p', 'e', 'g'),
    0x00
  };

static uint32_t tiff_fourccs[] =
  {
    BGAV_MK_FOURCC('t', 'i', 'f', 'f'),
    0x00
  };

static uint32_t tga_fourccs[] =
  {
    BGAV_MK_FOURCC('t', 'g', 'a', ' '),
    0x00
  };

static uint32_t mpeg1_fourccs[] =
  {
    BGAV_MK_FOURCC('m', 'p', 'v', '1'),
    0x00
  };

static uint32_t mpeg2_fourccs[] =
  {
    BGAV_MK_FOURCC('m', 'p', 'v', '2'),
    0x00
  };

static uint32_t theora_fourccs[] =
  {
    BGAV_MK_FOURCC('T','H','R','A'),
    0x00
  };

static uint32_t dirac_fourccs[] =
  {
    BGAV_MK_FOURCC('d','r','a','c'),
    0x00
  };

static uint32_t h264_fourccs[] =
  {
    BGAV_MK_FOURCC('H','2','6','4'),
    0x00
  };

static uint32_t avc1_fourccs[] =
  {
    BGAV_MK_FOURCC('a','v','c','1'),
    0x00
  };

static uint32_t mpeg4_fourccs[] =
  {
    BGAV_MK_FOURCC('m','p','4','v'),
    0x00
  };

static uint32_t d10_fourccs[] =
  {
    BGAV_MK_FOURCC('m', 'x', '5', 'p'),
    BGAV_MK_FOURCC('m', 'x', '4', 'p'),
    BGAV_MK_FOURCC('m', 'x', '3', 'p'),
    BGAV_MK_FOURCC('m', 'x', '5', 'n'),
    BGAV_MK_FOURCC('m', 'x', '4', 'n'),
    BGAV_MK_FOURCC('m', 'x', '3', 'n'),
    0x00,
  };

static uint32_t dv_fourccs[] =
  {
    BGAV_MK_FOURCC('d', 'v', 's', 'd'), 
    BGAV_MK_FOURCC('D', 'V', 'S', 'D'), 
    BGAV_MK_FOURCC('d', 'v', 'h', 'd'), 
    BGAV_MK_FOURCC('d', 'v', 's', 'l'), 
    BGAV_MK_FOURCC('d', 'v', '2', '5'),
    /* Generic DV */
    BGAV_MK_FOURCC('D', 'V', ' ', ' '),

    BGAV_MK_FOURCC('d', 'v', 'c', 'p') , /* DV PAL */
    BGAV_MK_FOURCC('d', 'v', 'c', ' ') , /* DV NTSC */
    BGAV_MK_FOURCC('d', 'v', 'p', 'p') , /* DVCPRO PAL produced by FCP */
    BGAV_MK_FOURCC('d', 'v', '5', 'p') , /* DVCPRO50 PAL produced by FCP */
    BGAV_MK_FOURCC('d', 'v', '5', 'n') , /* DVCPRO50 NTSC produced by FCP */
    BGAV_MK_FOURCC('A', 'V', 'd', 'v') , /* AVID DV */
    BGAV_MK_FOURCC('A', 'V', 'd', '1') , /* AVID DV */
    BGAV_MK_FOURCC('d', 'v', 'h', 'q') , /* DVCPRO HD 720p50 */
    BGAV_MK_FOURCC('d', 'v', 'h', 'p') , /* DVCPRO HD 720p60 */
    BGAV_MK_FOURCC('d', 'v', 'h', '5') , /* DVCPRO HD 50i produced by FCP */
    BGAV_MK_FOURCC('d', 'v', 'h', '6') , /* DVCPRO HD 60i produced by FCP */
    BGAV_MK_FOURCC('d', 'v', 'h', '3') , /* DVCPRO HD 30p produced by FCP */
    0x00,
  };

static int check_fourcc(uint32_t fourcc, uint32_t * fourccs)
  {
  int i = 0;
  while(fourccs[i])
    {
    if(fourccs[i] == fourcc)
      return 1;
    else
      i++;
    }
  return 0;
  }

int bgav_get_video_compression_info(bgav_t * bgav, int stream,
                                    gavl_compression_info_t * info)
  {
  gavl_codec_id_t id;
  bgav_stream_t * s = &bgav->tt->cur->video_streams[stream];
  int need_bitrate = 0;
  
  bgav_track_get_compression(bgav->tt->cur);
  
  memset(info, 0, sizeof(*info));
  
  if(check_fourcc(s->fourcc, png_fourccs))
    id = GAVL_CODEC_ID_PNG;
  else if(check_fourcc(s->fourcc, jpeg_fourccs))
    id = GAVL_CODEC_ID_JPEG;
  else if(check_fourcc(s->fourcc, tiff_fourccs))
    id = GAVL_CODEC_ID_TIFF;
  else if(check_fourcc(s->fourcc, tga_fourccs))
    id = GAVL_CODEC_ID_TGA;
  else if(check_fourcc(s->fourcc, mpeg1_fourccs))
    id = GAVL_CODEC_ID_MPEG1;
  else if(check_fourcc(s->fourcc, mpeg2_fourccs))
    id = GAVL_CODEC_ID_MPEG2;
  else if(check_fourcc(s->fourcc, theora_fourccs))
    id = GAVL_CODEC_ID_THEORA;
  else if(check_fourcc(s->fourcc, dirac_fourccs))
    id = GAVL_CODEC_ID_DIRAC;
  else if(check_fourcc(s->fourcc, h264_fourccs))
    id = GAVL_CODEC_ID_H264;
  else if(check_fourcc(s->fourcc, mpeg4_fourccs))
    id = GAVL_CODEC_ID_MPEG4_ASP;
  else if(check_fourcc(s->fourcc, dv_fourccs))
    id = GAVL_CODEC_ID_DV;
  else if(check_fourcc(s->fourcc, avc1_fourccs))
    {
    id = GAVL_CODEC_ID_H264;
    s->flags |= STREAM_FILTER_PACKETS;
    }
  else if(check_fourcc(s->fourcc, d10_fourccs))
    {
    id = GAVL_CODEC_ID_MPEG2;
    need_bitrate = 1;
    }
  else if(bgav_video_is_divx4(s->fourcc))
    id = GAVL_CODEC_ID_MPEG4_ASP;
  else
    return 0;

  if(gavl_compression_need_pixelformat(id) &&
     s->data.video.format.pixelformat == GAVL_PIXELFORMAT_NONE)
    {
    bgav_log(&bgav->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Video compression format needs pixelformat for compressed output");
    return 0;
    }
  
  info->id = id;

  if(s->flags & STREAM_FILTER_PACKETS)
    {
    const uint8_t * header;
    int header_size;
    s->bsf = bgav_bsf_create(s);
    header = bgav_bsf_get_header(s->bsf, &header_size);

    if(header)
      {
      info->global_header = malloc(header_size);
      memcpy(info->global_header, header, header_size);
      info->global_header_len = header_size;
      }
    }
  else if(s->ext_size)
    {
    info->global_header = malloc(s->ext_size);
    memcpy(info->global_header, s->ext_data, s->ext_size);
    info->global_header_len = s->ext_size;
    if(bgav_video_is_divx4(s->fourcc))
      bgav_mpeg4_remove_packed_flag(info->global_header,
                                    &info->global_header_len,
                                    &info->global_header_len);
    }

  if(need_bitrate)
    {
    if(s->codec_bitrate)
      info->bitrate = s->codec_bitrate;
    else if(s->container_bitrate)
      info->bitrate = s->container_bitrate;
    else
      {
      bgav_log(&bgav->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Video compression format needs bitrate for compressed output");
      return 0;
      }
    }
  
  if(!(s->flags & STREAM_INTRA_ONLY))
    info->flags |= GAVL_COMPRESSION_HAS_P_FRAMES;
  if(s->flags & STREAM_B_FRAMES)
    info->flags |= GAVL_COMPRESSION_HAS_B_FRAMES;
  if(s->flags & STREAM_FIELD_PICTURES)
    info->flags |= GAVL_COMPRESSION_HAS_FIELD_PICTURES;
  
  return 1;
  }

static void copy_packet_fields(gavl_packet_t * p, bgav_packet_t * bp)
  {
  p->pts = bp->pts;
  p->duration = bp->duration;

  p->header_size   = bp->header_size;
  p->field2_offset = bp->field2_offset;
  
  p->sequence_end_pos = bp->sequence_end_pos;
  
  /* Set flags */

  p->flags = 0;
  switch(bp->flags & 0xff)
    {
    case BGAV_CODING_TYPE_I:
      p->flags |= GAVL_PACKET_TYPE_I;
      break;
    case BGAV_CODING_TYPE_P:
      p->flags |= GAVL_PACKET_TYPE_P;
      break;
    case BGAV_CODING_TYPE_B:
      p->flags |= GAVL_PACKET_TYPE_B;
      break;
    }

  if(PACKET_GET_KEYFRAME(bp))
    p->flags |= GAVL_PACKET_KEYFRAME;

  }

int bgav_read_video_packet(bgav_t * bgav, int stream, gavl_packet_t * p)
  {
  bgav_packet_t * bp;
  bgav_stream_t * s = &bgav->tt->cur->video_streams[stream];
  
  bp = bgav_stream_get_packet_read(s);
  if(!bp)
    return 0;

  //  fprintf(stderr, "bgav_read_video_packet\n");
  //  bgav_packet_dump(bp);
  
  if(s->flags & STREAM_FILTER_PACKETS)
    {
    bgav_packet_t tmp_packet;
    memset(&tmp_packet, 0, sizeof(tmp_packet));

    tmp_packet.data = p->data;
    tmp_packet.data_alloc = p->data_alloc;
    bgav_bsf_run(s->bsf, bp, &tmp_packet);

    p->data = tmp_packet.data;
    p->data_alloc = tmp_packet.data_alloc;
    p->data_len  = tmp_packet.data_size;

    copy_packet_fields(p, &tmp_packet);
    }
  else
    {
    gavl_packet_alloc(p, bp->data_size);
    memcpy(p->data, bp->data, bp->data_size);
    p->data_len = bp->data_size;

    copy_packet_fields(p, bp);
    }
  
  bgav_stream_done_packet_read(s, bp);
  
  return 1;
  }
