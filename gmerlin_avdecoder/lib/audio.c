
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

int bgav_num_audio_streams(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].num_audio_streams;
  }

const gavl_audio_format_t * bgav_get_audio_format(bgav_t *  bgav, int stream)
  {
  return &(bgav->tt->current_track->audio_streams[stream].data.audio.format);
  }

int bgav_set_audio_stream(bgav_t * b, int stream, int action)
  {
  if((stream >= b->tt->current_track->num_audio_streams) || (stream < 0))
    return 0;
  b->tt->current_track->audio_streams[stream].action = action;
  return 1;
  }

int bgav_audio_start(bgav_stream_t * stream)
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
  //  fprintf(stderr, "Opening codec %s\n", dec->name);
  result = dec->init(stream);
  return result;
  }

void bgav_audio_stop(bgav_stream_t * s)
  {
  if(s->data.audio.decoder)
    {
    s->data.audio.decoder->decoder->close(s);
    free(s->data.audio.decoder);
    s->data.audio.decoder = (bgav_audio_decoder_context_t*)0;
    }
  }

const char * bgav_get_audio_description(bgav_t * b, int s)
  {
  return b->tt->current_track->audio_streams[s].description;
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
  bgav_stream_t * s = &(b->tt->current_track->audio_streams[stream]);
  num_samples = s->data.audio.format.samples_per_frame;
  return bgav_audio_decode(s, frame, num_samples);
  }

void bgav_audio_dump(bgav_stream_t * s)
  {
  fprintf(stderr, "Bits per sample: %d\n", s->data.audio.bits_per_sample);
  fprintf(stderr, "Block align:     %d\n", s->data.audio.block_align);
  //  fprintf(stderr, "Bitrate:         %d\n", s->data.audio.bitrate);
  fprintf(stderr, "Format:\n");
  gavl_audio_format_dump(&(s->data.audio.format));
  }

