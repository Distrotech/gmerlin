
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
  bgav_audio_decoder_t * dec;
  bgav_audio_decoder_context_t * ctx;
  
  dec = bgav_find_audio_decoder(stream);
  if(!dec)
    {
    fprintf(stderr, "No audio decoder found for fourcc ");
    bgav_dump_fourcc(stream->fourcc);
    fprintf(stderr, "\n");
    return 0;
    }
  ctx = calloc(1, sizeof(*ctx));
  stream->data.audio.decoder = ctx;
  stream->data.audio.decoder->decoder = dec;
  //  fprintf(stderr, "Opening codec %s\n", dec->name);

  if(!stream->timescale && stream->data.audio.format.samplerate)
    stream->timescale = stream->data.audio.format.samplerate;

  if(!dec->init(stream))
    return 0;

  if(!stream->timescale)
    stream->timescale = stream->data.audio.format.samplerate;
  return 1;
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
                    int stream, int num_samples)
  {
  int result;
  bgav_stream_t * s = &(b->tt->current_track->audio_streams[stream]);

  if(b->eof)
    return 0;

  //  fprintf(stderr, "Read audio %d\n", num_samples);
  result = bgav_audio_decode(s, frame, num_samples);
  s->position += result;
  return result;
  }

void bgav_audio_dump(bgav_stream_t * s)
  {
  fprintf(stderr, "  Bits per sample:   %d\n", s->data.audio.bits_per_sample);
  fprintf(stderr, "  Block align:       %d\n", s->data.audio.block_align);
  //  fprintf(stderr, "Bitrate:         %d\n", s->data.audio.bitrate);
  fprintf(stderr, "Format:\n");
  gavl_audio_format_dump(&(s->data.audio.format));
  }


void bgav_audio_resync(bgav_stream_t * s)
  {
  if(s->data.audio.decoder &&
     s->data.audio.decoder->decoder->resync)
    s->data.audio.decoder->decoder->resync(s);
  
  }

int bgav_audio_skipto(bgav_stream_t * s, gavl_time_t * t)
  {
  int64_t num_samples;
  int samples_skipped = 0;  
  gavl_time_t stream_time;

  stream_time = gavl_samples_to_time(s->timescale,
                                     s->time_scaled);

  num_samples = ((int64_t)(s->data.audio.format.samplerate)* (*t - stream_time)) /
    GAVL_TIME_SCALE;
  
  if(num_samples < 0)
    fprintf(stderr, "audio.c: Cannot skip backwards: Stream time: %f Skip time: %f\n",
            gavl_time_to_seconds(stream_time), gavl_time_to_seconds(*t));
  else if(num_samples > 0)
    samples_skipped = bgav_audio_decode(s, (gavl_audio_frame_t*)0, num_samples);

  if(samples_skipped < num_samples)
    {
    fprintf(stderr, "bgav_audio_skipto: EOF (%d %lld)\n", samples_skipped, num_samples);
    return 0;
    }
  //  fprintf(stderr, "bgav_audio_skip: Samples: %lld, stream time: %f, time: %f\n",
  //          num_samples, gavl_time_to_seconds(stream_time), 
  //          gavl_time_to_seconds(*t));
  
  return 1;
  }


 
