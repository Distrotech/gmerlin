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
    return 0;
  ctx = calloc(1, sizeof(*ctx));
  stream->data.video.decoder = ctx;
  stream->data.video.decoder->decoder = dec;

  stream->position = 0;
  stream->time = 0;

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
  stream->time = GAVL_TIME_UNDEFINED;
  
  result = stream->data.video.decoder->decoder->decode(stream,
                                                       frame);
  if(stream->time == GAVL_TIME_UNDEFINED)
    {
    stream->time = gavl_frames_to_time(stream->data.video.format.framerate_num,
                                      stream->data.video.format.framerate_den,
                                      stream->position);
    }
  if(frame)
    frame->time = stream->time;
  stream->position++;
  return result;
  }


int bgav_read_video(bgav_t * b, gavl_video_frame_t * frame, int s)
  {
  return bgav_video_decode(&(b->tt->current_track->video_streams[s]), frame);
  }

void bgav_video_dump(bgav_stream_t * s)
  {
  fprintf(stderr, "Depth:             %d\n", s->data.video.depth);
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

void bgav_video_skipto(bgav_stream_t * s, gavl_time_t time)
  {
  if(s->time >= time)
    return;
  do
    {
    bgav_video_decode(s, (gavl_video_frame_t*)0);
    }while(s->time < time);
  }

