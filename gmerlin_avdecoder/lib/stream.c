
/*****************************************************************
 
  stream.c
 
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

#include <avdec_private.h>
#include <stdlib.h>
#include <string.h>

void bgav_stream_start(bgav_stream_t * stream)
  {
  if((stream->action == BGAV_STREAM_DECODE) ||
     (stream->action == BGAV_STREAM_SYNC))
    {
    switch(stream->type)
      {
      case BGAV_STREAM_VIDEO:
        bgav_video_start(stream);
        break;
      case BGAV_STREAM_AUDIO:
        bgav_audio_start(stream);
        break;
      }
    }
  }

void bgav_stream_stop(bgav_stream_t * stream)
  {
  if(stream->packet_buffer)
    bgav_packet_buffer_destroy(stream->packet_buffer);
  
  if((stream->action == BGAV_STREAM_DECODE) ||
     (stream->action == BGAV_STREAM_SYNC))
    {
    switch(stream->type)
      {
      case BGAV_STREAM_VIDEO:
        bgav_video_stop(stream);
        break;
      case BGAV_STREAM_AUDIO:
        bgav_audio_stop(stream);
        break;
      }
    }
  }

void bgav_stream_alloc(bgav_stream_t * stream)
  {
  memset(stream, 0, sizeof(*stream));
  stream->packet_buffer = bgav_packet_buffer_create();
  }

void bgav_stream_free(bgav_stream_t * s)
  {
  if(s->description)
    free(s->description);
  }

void bgav_stream_dump(bgav_stream_t * s)
  {
  switch(s->type)
    {
    case BGAV_STREAM_AUDIO:
      fprintf(stderr, "============ Audio stream ============\n");
      break;
    case BGAV_STREAM_VIDEO:
      fprintf(stderr, "============ Video stream ============\n");
      break;
      
    }
  fprintf(stderr, "Type:            %s\n",
          (s->description ? s->description : "Not specified"));
  fprintf(stderr, "Fourcc:          ");
  bgav_dump_fourcc(s->fourcc);
  fprintf(stderr, "\n");
  
  fprintf(stderr, "Stream ID:    %d (0x%x)\n",
          s->stream_id,
          s->stream_id);
  fprintf(stderr, "Codec bitrate: ");
  if(s->codec_bitrate)
    fprintf(stderr, "%d\n", s->codec_bitrate);
  else
    fprintf(stderr, "Unspecified\n");

  fprintf(stderr, "Container bitrate: ");
  if(s->container_bitrate)
    fprintf(stderr, "%d\n", s->container_bitrate);
  else
    fprintf(stderr, "Unspecified\n");
  }
