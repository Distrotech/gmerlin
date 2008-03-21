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
#include <avdec_private.h>
#include <stdio.h>

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

int bgav_video_start(bgav_stream_t * stream)
  {
  int result;
  bgav_video_decoder_t * dec;
  bgav_video_decoder_context_t * ctx;
  bgav_packet_t * p;
  
  dec = bgav_find_video_decoder(stream);
  if(!dec)
    {
    bgav_log(stream->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "No audio decoder found for fourcc %c%c%c%c (0x%08x)",
             (stream->fourcc & 0xFF000000) >> 24,
             (stream->fourcc & 0x00FF0000) >> 16,
             (stream->fourcc & 0x0000FF00) >> 8,
             (stream->fourcc & 0x000000FF),
             stream->fourcc);
    return 0;
    }
  ctx = calloc(1, sizeof(*ctx));
  stream->data.video.decoder = ctx;
  stream->data.video.decoder->decoder = dec;

  stream->out_time = 0;
  stream->in_position = 0;
  stream->in_time = 0;

  if(!stream->timescale)
    {
    stream->timescale = stream->data.video.format.timescale;
    }

  switch(stream->data.video.frametime_mode)
    {
    case BGAV_FRAMETIME_PACKET:
    case BGAV_FRAMETIME_PTS:
      p = bgav_demuxer_peek_packet_read(stream->demuxer, stream, 1);
      if(!p)
        stream-> data.video.next_frame_duration = 0;
      else
        stream-> data.video.next_frame_duration =
          gavl_time_rescale(stream-> timescale, stream-> data.video.format.timescale,
                            p->duration);
      break;
    case BGAV_FRAMETIME_CODEC:
    case BGAV_FRAMETIME_CONSTANT:
      /* later */
      break;
    }
  
  result = dec->init(stream);
  if(!result)
    return 0;
  
  switch(stream-> data.video.frametime_mode)
    {
    case BGAV_FRAMETIME_CONSTANT:
      stream->data.video.last_frame_duration = stream->data.video.format.frame_duration;
      stream->data.video.next_frame_duration = stream->data.video.format.frame_duration;
      break;
    case BGAV_FRAMETIME_PACKET:
    case BGAV_FRAMETIME_PTS:
      break;
    case BGAV_FRAMETIME_CODEC:
      /* Codec will set everything */
      break;
    }
  
  return result;
  }

const char * bgav_get_video_description(bgav_t * b, int s)
  {
  return b->tt->cur->video_streams[s].description;
  }

static int bgav_video_decode(bgav_stream_t * stream,
                             gavl_video_frame_t* frame)
  {
  int result;
  bgav_packet_t * p;

  if(stream->eof)
    return 0;
  
  result = stream->data.video.decoder->decoder->decode(stream, frame);
  if(!result)
    {
    stream->eof = 1;
    return result;
    }
  
  /* Update time */
  switch(stream->data.video.frametime_mode)
    {
    case BGAV_FRAMETIME_CONSTANT:
      stream->data.video.last_frame_time = stream->out_time;
      stream->out_time += stream->data.video.format.frame_duration;
      break;
    case BGAV_FRAMETIME_PACKET:
    case BGAV_FRAMETIME_PTS:
      stream->data.video.last_frame_time = stream->out_time;
      
      p = bgav_demuxer_peek_packet_read(stream->demuxer, stream, 1);
      if(!p)
        fprintf(stderr, "bgav_demuxer_peek_packet_read failed\n");
      stream->out_time = gavl_time_rescale(stream->timescale,
                                           stream->data.video.format.timescale, p->pts);

      stream->data.video.last_frame_duration = stream->data.video.next_frame_duration;
      stream->data.video.next_frame_duration =
        gavl_time_rescale(stream->timescale,
                          stream->data.video.format.timescale, p->duration);
      break;
    case BGAV_FRAMETIME_CODEC:
    case BGAV_FRAMETIME_CODEC_PTS:
      /* Codec already set everything */
      break;
    }

  /* Set the final timestamp for the frame */
  
  if(frame)
    {
    frame->timestamp = stream->data.video.last_frame_time;
    
    if(stream->demuxer->demux_mode == DEMUX_MODE_FI)
      frame->timestamp += stream->first_timestamp;
    
    frame->duration  = stream->data.video.last_frame_duration;
    
    /* Yes, this sometimes happens due to rounding errors */
    if(frame->timestamp < 0)
      frame->timestamp = 0;
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
  if(s->data.video.decoder)
    {
    s->data.video.decoder->decoder->close(s);
    free(s->data.video.decoder);
    s->data.video.decoder = (bgav_video_decoder_context_t*)0;
    }
  /* Clear still mode flag (it will be set during reinit */
  s->data.video.still_mode = 0;
  }

void bgav_video_resync(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  switch(s->data.video.frametime_mode)
    {
    case BGAV_FRAMETIME_CONSTANT:
      break;
    case BGAV_FRAMETIME_PACKET:
    case BGAV_FRAMETIME_PTS:
      p = bgav_demuxer_peek_packet_read(s->demuxer, s, 1);
      if(!p)
        s->data.video.next_frame_duration = 0;
      else
        s->data.video.next_frame_duration =
          gavl_time_rescale(s->timescale, s->data.video.format.timescale,
                            p->duration);
      break;
    case BGAV_FRAMETIME_CODEC:
      /* Codec will set everything */
      break;
    }
  
  if(s->data.video.decoder->decoder->resync)
    s->data.video.decoder->decoder->resync(s);
  
  }

int bgav_video_skipto(bgav_stream_t * s, int64_t * time, int scale)
  {
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

  else if(s->out_time + s->data.video.next_frame_duration >
          time_scaled)
    {
    /* Do nothing but update the time */
    *time = gavl_time_rescale(s->data.video.format.timescale,
                              scale,
                              s->out_time);
    //    s->in_time = gavl_time_rescale(scale, s->timescale, *time);
    return 1;
    }
  //  fprintf(stderr, "bgav_video_skipto %ld %ld -> %ld\n", s->out_time, s->data.video.next_frame_duration, time_scaled);
  do{
    result = bgav_video_decode(s, (gavl_video_frame_t*)0);
    if(!result)
      return 0;
    //    fprintf(stderr, "%ld %ld\n", s->out_time, s->data.video.next_frame_duration);
    } while(s->out_time + s->data.video.next_frame_duration <= time_scaled);
  //  fprintf(stderr, "%ld %ld\n", s->out_time, s->data.video.next_frame_duration);
  *time = gavl_time_rescale(s->data.video.format.timescale, scale, s->out_time);
  s->in_time = gavl_time_rescale(scale, s->timescale, *time);


  
  return 1;
  }

