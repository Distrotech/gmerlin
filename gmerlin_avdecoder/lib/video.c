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

int bgav_num_video_streams(bgav_t *  bgav)
  {
  return bgav->demuxer->supported_video_streams;
  }

gavl_video_format_t * bgav_get_video_format(bgav_t * bgav, int stream)
  {
  return &(bgav->demuxer->video_streams[bgav->demuxer->video_stream_index[stream]].data.video.format);
  }

int bgav_set_video_stream(bgav_t * b, int stream, int action)
  {
  if((stream >= b->demuxer->num_video_streams) ||
     (stream < 0))
    return 0;
  b->demuxer->video_streams[b->demuxer->video_stream_index[stream]].action = action;
  return 1;
  }

static int init_video_stream(bgav_stream_t * stream)
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
  fprintf(stderr, "Opening codec %s\n", dec->name);
  result = dec->init(stream);
  return result;
  }

int bgav_start_video_decoders(bgav_t * b)
  {
  int i;
  
  for(i = 0; i < b->demuxer->num_video_streams; i++)
    {
    switch(b->demuxer->video_streams[i].action)
      {
      case BGAV_STREAM_SYNC:
      case BGAV_STREAM_DECODE:
        if(!init_video_stream(&(b->demuxer->video_streams[i])))
          fprintf(stderr, "Setting video stream failed\n");
        break;
      default:
        break;
      }
    }
  return 1;
  }

void bgav_close_video_decoders(bgav_t * b)
  {
  int i;
  for(i = 0; i < b->demuxer->num_video_streams; i++)
    {
    if(b->demuxer->video_streams[i].data.video.decoder)
      b->demuxer->video_streams[i].data.video.decoder->decoder->close(&(b->demuxer->video_streams[i]));
    if(b->demuxer->video_streams[i].packet_buffer)
      bgav_packet_buffer_destroy(b->demuxer->video_streams[i].packet_buffer);
    if(b->demuxer->video_streams[i].data.video.decoder)
      free(b->demuxer->video_streams[i].data.video.decoder);
    }
  }

int bgav_read_video(bgav_t * b, gavl_video_frame_t * frame, int s)
  {
  int result;
  bgav_stream_t * stream;

  stream = &(b->demuxer->video_streams[b->demuxer->video_stream_index[s]]);
  
  stream->time = -1;
  
  result = stream->data.video.decoder->decoder->decode(stream,
                                                       frame);
  if(stream->time == -1)
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

void bgav_video_dump(bgav_stream_t * s)
  {
  fprintf(stderr, "============ Video stream ============\n");
  fprintf(stderr, "Fourcc: ");
  bgav_dump_fourcc(s->fourcc);
  fprintf(stderr, "\nDepth: %d\n", s->data.video.depth);
  fprintf(stderr, "Format:\n");
  gavl_video_format_dump(&(s->data.video.format));
  }



