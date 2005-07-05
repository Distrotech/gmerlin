/*****************************************************************
 
  video.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

int bgav_num_video_streams(bgav_t *  bgav, int track)
  {
  return bgav->tt->tracks[track].num_video_streams;
  }

const gavl_video_format_t * bgav_get_video_format(bgav_t * bgav, int stream)
  {
  return &(bgav->tt->current_track->video_streams[stream].data.video.format);
  }

int bgav_set_video_stream(bgav_t * b, int stream, int action)
  {
  if((stream >= b->tt->current_track->num_video_streams) ||
     (stream < 0))
    return 0;
  b->demuxer->tt->current_track->video_streams[stream].action = action;
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
    fprintf(stderr, "No video decoder found for fourcc ");
    bgav_dump_fourcc(stream->fourcc);
    fprintf(stderr, "\n");
    return 0;
    }
  ctx = calloc(1, sizeof(*ctx));
  stream->data.video.decoder = ctx;
  stream->data.video.decoder->decoder = dec;

  stream->position = 0;
  stream->time_scaled = 0;

  if(!stream->timescale)
    {
    stream->timescale = stream->data.video.format.timescale;
    }
  
  //  fprintf(stderr, "Opening codec %s\n", dec->name);
  result = dec->init(stream);
  return result;
  }

const char * bgav_get_video_description(bgav_t * b, int s)
  {
  return b->tt->current_track->video_streams[s].description;
  }

static int bgav_video_decode(bgav_stream_t * stream, gavl_video_frame_t* frame)
  {
  int result;

  result = stream->data.video.decoder->decoder->decode(stream, frame);
  /* Set the final timestamp for the frame */
  
  if(frame && result)
    {
    frame->time_scaled = stream->data.video.last_frame_time;
    
    /* Yes, this sometimes happens due to rounding errors */
    if(frame->time_scaled < 0)
      frame->time_scaled = 0;
#if 0
    fprintf(stderr, "Video timestamps: %lld %d\n",
            frame->time_scaled, stream->data.video.format.timescale);
#endif
    }
  stream->position++;
  return result;
  }

int bgav_read_video(bgav_t * b, gavl_video_frame_t * frame, int s)
  {
  if(b->eof)
    return 0;
  return bgav_video_decode(&(b->tt->current_track->video_streams[s]), frame);
  }

void bgav_video_dump(bgav_stream_t * s)
  {
  fprintf(stderr, "  Depth:             %d\n", s->data.video.depth);
  fprintf(stderr, "Format:\n");
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
  gavl_time_t stream_time;
  int64_t codec_time_scaled;
  gavl_time_t next_frame_time;
  int result;
  
  stream_time = gavl_samples_to_time(s->timescale,
                                     s->time_scaled);
  
  if(stream_time >= *time)
    {
    fprintf(stderr, "video.c: cannot skip backwards\n");
    return 1;
    }

  codec_time_scaled = gavl_time_to_samples(s->data.video.format.timescale,
                                           *time);
  
  do{
    result = bgav_video_decode(s, (gavl_video_frame_t*)0);

    if(!result)
      return 0;

    next_frame_time = gavl_samples_to_time(s->data.video.format.timescale,
                                           s->data.video.last_frame_time +
                                           s->data.video.last_frame_duration);
    } while((next_frame_time < *time) && result);
#if 0  
  fprintf(stderr, "bgav_video_skipto: Time: %f, next frame time: %f\n",
          gavl_time_to_seconds(*time),
          gavl_time_to_seconds(next_frame_time));
#endif
  s->time_scaled = gavl_time_to_samples(s->timescale, *time);
  return 1;
  }

