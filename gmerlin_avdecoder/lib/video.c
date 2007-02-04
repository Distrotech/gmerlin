/*****************************************************************
 
  video.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

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
             "No audio decoder found for fourcc %c%c%c%c (0x08x)",
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
    frame->time_scaled = stream->data.video.last_frame_time;
    //    fprintf(stderr, "Video time: %f\n",
    //            (float)(frame->time_scaled) /
    //            (float)(stream->data.video.format.timescale));
                
    /* Yes, this sometimes happens due to rounding errors */
    if(frame->time_scaled < 0)
      frame->time_scaled = 0;
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
  }

void bgav_video_resync(bgav_stream_t * s)
  {
  if(s->data.video.decoder &&
     s->data.video.decoder->decoder->resync)
    s->data.video.decoder->decoder->resync(s);
  
  }

int bgav_video_skipto(bgav_stream_t * s, gavl_time_t * time)
  {
  //  gavl_time_t stream_time;
  gavl_time_t next_frame_time;
  int result;
  int64_t time_scaled;
    
  time_scaled = gavl_time_scale(s->data.video.format.timescale, *time);
  
  if(s->data.video.next_frame_time > time_scaled)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN, 
             "Cannot skip backwards, stream_time: %lld, sync_time: %lld",
             s->time_scaled, time_scaled);
    return 1;
    }

  else if(s->data.video.next_frame_time + s->data.video.next_frame_duration >
          time_scaled)
    {
    /* Do nothing but update the time */
    *time = gavl_time_unscale(s->data.video.format.timescale,
                              s->data.video.next_frame_time);
    s->time_scaled = gavl_time_scale(s->timescale, *time);
    return 1;
    }
  
  do{
    result = bgav_video_decode(s, (gavl_video_frame_t*)0);

    if(!result)
      return 0;

    next_frame_time = s->data.video.last_frame_time +
      s->data.video.last_frame_duration;
    } while((next_frame_time < time_scaled) && result);

  *time = gavl_time_unscale(s->data.video.format.timescale, next_frame_time);
  s->time_scaled = gavl_time_scale(s->timescale, *time);


  
  return 1;
  }

