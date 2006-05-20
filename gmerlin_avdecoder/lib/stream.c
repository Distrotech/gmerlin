
/*****************************************************************
 
  stream.c
 
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

#include <avdec_private.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int bgav_stream_start(bgav_stream_t * stream)
  {
  int result = 1;
  if(stream->action == BGAV_STREAM_DECODE)
    {
    switch(stream->type)
      {
      case BGAV_STREAM_VIDEO:
        result = bgav_video_start(stream);
        break;
      case BGAV_STREAM_AUDIO:
        result = bgav_audio_start(stream);
        break;
      case BGAV_STREAM_SUBTITLE_OVERLAY:
      case BGAV_STREAM_SUBTITLE_TEXT:
        result = bgav_subtitle_start(stream);
        break;
      default:
        break;
      }
    }
  return result;
  }

void bgav_stream_stop(bgav_stream_t * stream)
  {
  if(stream->action == BGAV_STREAM_DECODE)
    {
    switch(stream->type)
      {
      case BGAV_STREAM_VIDEO:
        bgav_video_stop(stream);
        break;
      case BGAV_STREAM_AUDIO:
        bgav_audio_stop(stream);
        break;
      case BGAV_STREAM_SUBTITLE_TEXT:
      case BGAV_STREAM_SUBTITLE_OVERLAY:
        bgav_subtitle_stop(stream);
      default:
        break;
      }
    }
  if(stream->packet_buffer)
    bgav_packet_buffer_clear(stream->packet_buffer);
  }

void bgav_stream_create_packet_buffer(bgav_stream_t * stream)
  {
  stream->packet_buffer = bgav_packet_buffer_create();
  }

void bgav_stream_init(bgav_stream_t * stream, const bgav_options_t * opt)
  {
  memset(stream, 0, sizeof(*stream));
  stream->time_scaled = -1;
  stream->first_index_position = INT_MAX;

  /* need to set this to -1 so we know, if this stream has packets at all */
  stream->last_index_position = -1; 
  stream->index_position = -1;
  stream->opt = opt;
  }


void bgav_stream_free(bgav_stream_t * s)
  {
  if(s->description)
    free(s->description);
  if(s->info)
    free(s->info);
  
  if(s->packet_buffer)
    bgav_packet_buffer_destroy(s->packet_buffer);
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

    case BGAV_STREAM_SUBTITLE_TEXT:
      fprintf(stderr, "=========== Text subtitles ===========\n");
      break;
    case BGAV_STREAM_SUBTITLE_OVERLAY:
      fprintf(stderr, "========= Overlay subtitles ===========\n");
      break;
      
    case BGAV_STREAM_UNKNOWN:
      return;
    }

  if(s->language[0] != '\0')
    fprintf(stderr, "  Language:          %s\n", bgav_lang_name(s->language));
  if(s->info)
    fprintf(stderr, "  Info:              %s\n", s->info);
  
  fprintf(stderr, "  Type:              %s\n",
          (s->description ? s->description : "Not specified"));
  fprintf(stderr, "  Fourcc:            ");
  bgav_dump_fourcc(s->fourcc);
  fprintf(stderr, "\n");
  
  fprintf(stderr, "  Stream ID:         %d (0x%x)\n",
          s->stream_id,
          s->stream_id);
  fprintf(stderr, "  Codec bitrate:     ");
  if(s->codec_bitrate)
    fprintf(stderr, "%d\n", s->codec_bitrate);
  else
    fprintf(stderr, "Unspecified\n");

  fprintf(stderr, "  Container bitrate: ");
  if(s->container_bitrate)
    fprintf(stderr, "%d\n", s->container_bitrate);
  else
    fprintf(stderr, "Unspecified\n");

  fprintf(stderr, "  Timescale:         %d\n", s->timescale);
  fprintf(stderr, "  Private data:      %p\n", s->priv);
  }

void bgav_stream_clear(bgav_stream_t * s)
  {
  if(s->packet_buffer)
    bgav_packet_buffer_clear(s->packet_buffer);
  s->packet = (bgav_packet_t*)0;
  s->position = -1;
  s->time_scaled = -1;
  }

void bgav_stream_resync_decoder(bgav_stream_t * s)
  {
  if(s->action != BGAV_STREAM_DECODE)
    return;
  
  switch(s->type)
    {
    case BGAV_STREAM_AUDIO:
      bgav_audio_resync(s);
      break;
    case BGAV_STREAM_VIDEO:
      bgav_video_resync(s);
      break;
    case BGAV_STREAM_SUBTITLE_TEXT:
    case BGAV_STREAM_SUBTITLE_OVERLAY:
      bgav_subtitle_resync(s);
      break;
    case BGAV_STREAM_UNKNOWN:
      break;
    }
  }

int bgav_stream_skipto(bgav_stream_t * s, gavl_time_t * time)
  {
  
  if(s->action != BGAV_STREAM_DECODE)
    return 1;
  
  switch(s->type)
    {
    case BGAV_STREAM_AUDIO:
      return bgav_audio_skipto(s, time);
      break;
    case BGAV_STREAM_VIDEO:
      return bgav_video_skipto(s, time);
      break;
    case BGAV_STREAM_SUBTITLE_TEXT:
    case BGAV_STREAM_SUBTITLE_OVERLAY:
      return bgav_subtitle_skipto(s, time);
      break;
    case BGAV_STREAM_UNKNOWN:
      break;
    }
  return 0;
  }
