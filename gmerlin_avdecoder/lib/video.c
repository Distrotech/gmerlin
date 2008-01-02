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

  stream->out_position = 0;
  stream->in_position = 0;
  stream->time_scaled = 0;

  if(!stream->timescale)
    {
    stream->timescale = stream->data.video.format.timescale;
    }
  
  result = dec->init(stream);
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

  result = stream->data.video.decoder->decoder->decode(stream, frame);
  /* Set the final timestamp for the frame */
  
  if(frame && result)
    {
    frame->timestamp = stream->data.video.last_frame_time;
    frame->duration  = stream->data.video.last_frame_duration;
                
    /* Yes, this sometimes happens due to rounding errors */
    if(frame->timestamp < 0)
      frame->timestamp = 0;
    }
  stream->out_position++;
  
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
  if(s->data.video.decoder &&
     s->data.video.decoder->decoder->resync)
    s->data.video.decoder->decoder->resync(s);
  
  }

int bgav_video_skipto(bgav_stream_t * s, int64_t * time, int scale)
  {
  //  gavl_time_t stream_time;
  gavl_time_t next_frame_time;
  int result;
  int64_t time_scaled;
  char tmp_string1[128];
  char tmp_string2[128];
  
  time_scaled = gavl_time_rescale(scale, s->data.video.format.timescale, *time);
  
  if(s->data.video.next_frame_time > time_scaled)
    {
    sprintf(tmp_string1, "%" PRId64, s->time_scaled);
    sprintf(tmp_string2, "%" PRId64, time_scaled);
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN, 
             "Cannot skip backwards, stream_time: %s, sync_time: %s",
             tmp_string1, tmp_string2);
    return 1;
    }

  else if(s->data.video.next_frame_time + s->data.video.next_frame_duration >
          time_scaled)
    {
    /* Do nothing but update the time */
    *time = gavl_time_rescale(s->data.video.format.timescale,
                              scale,
                              s->data.video.next_frame_time);
    s->time_scaled = gavl_time_rescale(scale, s->timescale, *time);
    return 1;
    }
  
  do{
    result = bgav_video_decode(s, (gavl_video_frame_t*)0);

    if(!result)
      return 0;

    next_frame_time = s->data.video.last_frame_time +
      s->data.video.last_frame_duration;
    } while((next_frame_time < time_scaled) && result);

  *time = gavl_time_rescale(s->data.video.format.timescale, scale, next_frame_time);
  s->time_scaled = gavl_time_rescale(scale, s->timescale, *time);


  
  return 1;
  }

