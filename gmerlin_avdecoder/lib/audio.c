
/*****************************************************************
 
  audio.c
 
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
#include <stdlib.h>
#include <stdio.h>

#define LOG_DOMAIN "audio"

int bgav_num_audio_streams(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].num_audio_streams;
  }

const gavl_audio_format_t * bgav_get_audio_format(bgav_t *  bgav, int stream)
  {
  return &(bgav->tt->cur->audio_streams[stream].data.audio.format);
  }

int bgav_set_audio_stream(bgav_t * b, int stream, bgav_stream_action_t action)
  {
  if((stream >= b->tt->cur->num_audio_streams) || (stream < 0))
    return 0;
  b->tt->cur->audio_streams[stream].action = action;
  return 1;
  }

int bgav_audio_start(bgav_stream_t * stream)
  {
  bgav_audio_decoder_t * dec;
  bgav_audio_decoder_context_t * ctx;
  char tmp_string[128];
  
  dec = bgav_find_audio_decoder(stream);
  if(!dec)
    {
    if(!(stream->fourcc & 0xffff0000))
      bgav_log(stream->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "No audio decoder found for WAV ID 0x%04x", stream->fourcc);
    else
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
  stream->data.audio.decoder = ctx;
  stream->data.audio.decoder->decoder = dec;

  if(!stream->timescale && stream->data.audio.format.samplerate)
    stream->timescale = stream->data.audio.format.samplerate;

  if(!dec->init(stream))
    return 0;

  if(!stream->timescale)
    stream->timescale = stream->data.audio.format.samplerate;

  if(stream->has_first_timestamp && (stream->first_timestamp != BGAV_TIMESTAMP_UNDEFINED))
    {
    stream->out_position =
      gavl_time_rescale(stream->timescale, stream->data.audio.format.samplerate,
                        stream->first_timestamp);
    //    if(stream->out_position < 0)
    //      stream->out_position = 0;
    sprintf(tmp_string, "%" PRId64, stream->out_position);
    bgav_log(stream->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Got initial audio timestamp: %s",
             tmp_string);
    }
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
  s->has_first_timestamp = 0;
  }

const char * bgav_get_audio_description(bgav_t * b, int s)
  {
  return b->tt->cur->audio_streams[s].description;
  }

const char * bgav_get_audio_info(bgav_t * b, int s)
  {
  return b->tt->cur->audio_streams[s].info;
  }


const char * bgav_get_audio_language(bgav_t * b, int s)
  {
  return (b->tt->cur->audio_streams[s].language[0] != '\0') ?
    b->tt->cur->audio_streams[s].language : (char*)0;
  }

int bgav_read_audio(bgav_t * b, gavl_audio_frame_t * frame,
                    int stream, int num_samples)
  {
  int result;
  bgav_stream_t * s = &(b->tt->cur->audio_streams[stream]);

  if(b->eof)
    return 0;

  result = s->data.audio.decoder->decoder->decode(s, frame, num_samples);
  
  if(frame)
    frame->time_scaled = s->out_position;
  
  s->out_position += result;
  return result;
  }

void bgav_audio_dump(bgav_stream_t * s)
  {
  bgav_dprintf("  Bits per sample:   %d\n", s->data.audio.bits_per_sample);
  bgav_dprintf("  Block align:       %d\n", s->data.audio.block_align);
  bgav_dprintf("Format:\n");
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
  gavl_time_t diff_time;
  char tmp_string[128];
  
  stream_time = gavl_time_unscale(s->timescale,
                                  s->time_scaled);
  
  diff_time = *t - stream_time;

  num_samples = gavl_time_to_samples(s->data.audio.format.samplerate,
                                     diff_time);
  
  if(num_samples < 0)
    {
    sprintf(tmp_string, "%" PRId64, diff_time);
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "Cannot skip backwards: Stream time: %f skip time: %f difference: %s",
             gavl_time_to_seconds(stream_time), gavl_time_to_seconds(*t),
             tmp_string);
    }
  else
    if(num_samples > 0)
      {
      sprintf(tmp_string, "%" PRId64, num_samples);
      bgav_log(s->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
               "Skipping %s samples",
               tmp_string);
      
      samples_skipped =
        s->data.audio.decoder->decoder->decode(s, (gavl_audio_frame_t*)0,
                                               num_samples);
      }
  if(samples_skipped < num_samples)
    {
    return 0;
    }
  
  return 1;
  }


 
