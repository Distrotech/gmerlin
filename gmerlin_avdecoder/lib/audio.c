
/*****************************************************************
 
  audio.c
 
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

int bgav_num_audio_streams(bgav_t * bgav)
  {
  return bgav->demuxer->supported_audio_streams;
  }

gavl_audio_format_t * bgav_get_audio_format(bgav_t *  bgav, int stream)
  {
  return &(bgav->demuxer->audio_streams[bgav->demuxer->audio_stream_index[stream]].data.audio.format);
  }

int bgav_set_audio_stream(bgav_t * b, int stream, int action)
  {
  if((stream >= b->demuxer->num_audio_streams) ||
     (stream < 0))
    return 0;
  b->demuxer->audio_streams[b->demuxer->audio_stream_index[stream]].action = action;
  return 1;
  }

static int init_audio_stream(bgav_stream_t * stream)
  {
  int result;
  bgav_audio_decoder_t * dec;
  bgav_audio_decoder_context_t * ctx;
  
  dec = bgav_find_audio_decoder(stream);
  if(!dec)
    {
    fprintf(stderr, "No decoder found\n");
    return 0;
    }
  ctx = calloc(1, sizeof(*ctx));
  stream->data.audio.decoder = ctx;
  stream->data.audio.decoder->decoder = dec;
  fprintf(stderr, "Opening codec %s\n", dec->name);
  result = dec->init(stream);
  return result;
  }

int bgav_start_audio_decoders(bgav_t * b)
  {
  int i;
  
  for(i = 0; i < b->demuxer->num_audio_streams; i++)
    {
    switch(b->demuxer->audio_streams[i].action)
      {
      case BGAV_STREAM_SYNC:
      case BGAV_STREAM_DECODE:
        if(!init_audio_stream(&(b->demuxer->audio_streams[i])))
          {
          fprintf(stderr, "Setting audio stream failed\n");
          bgav_dump_fourcc(b->demuxer->audio_streams[i].fourcc);
          }
        break;
      default:
        break;
      }
    }
  return 1;
  }

void bgav_close_audio_decoders(bgav_t * b)
  {
  int i;
  for(i = 0; i < b->demuxer->num_audio_streams; i++)
    {
    if(b->demuxer->audio_streams[i].data.audio.decoder)
      b->demuxer->audio_streams[i].data.audio.decoder->decoder->close(&(b->demuxer->audio_streams[i]));
    if(b->demuxer->audio_streams[i].packet_buffer)
      bgav_packet_buffer_destroy(b->demuxer->audio_streams[i].packet_buffer);
    if(b->demuxer->audio_streams[i].data.audio.decoder)
      free(b->demuxer->audio_streams[i].data.audio.decoder);
    }
  }

int bgav_audio_decode(bgav_stream_t * s, gavl_audio_frame_t * frame,
                      int num_samples)
  {
  return
    s->data.audio.decoder->decoder->decode(s, frame, num_samples);
  
  }

int bgav_read_audio(bgav_t * b, gavl_audio_frame_t * frame,
                    int stream)
  {
  int num_samples;
  bgav_stream_t * s = &(b->demuxer->audio_streams[b->demuxer->audio_stream_index[stream]]);
  num_samples = s->data.audio.format.samples_per_frame;
  return bgav_audio_decode(s, frame, num_samples);
  }



void bgav_audio_dump(bgav_stream_t * s)
  {
  fprintf(stderr, "============ Audio stream ============\n");
  fprintf(stderr, "Fourcc:          ");
  bgav_dump_fourcc(s->fourcc);
  fprintf(stderr, "\n");
  fprintf(stderr, "Bits per sample: %d\n", s->data.audio.bits_per_sample);
  fprintf(stderr, "Block align:     %d\n", s->data.audio.block_align);
  fprintf(stderr, "Bitrate:         %d\n", s->data.audio.bitrate);
  fprintf(stderr, "Format:\n");
  gavl_audio_format_dump(&(s->data.audio.format));
  }

