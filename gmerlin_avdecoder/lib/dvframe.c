/*****************************************************************

  dvframe.c

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

/*
 *  Handler for DV frames
 *
 *  it will be used to parse raw DV streams as well as
 *  DV frames from AVI files.
 *
 *  It detects most interesting parameters from the DV
 *  data and can extract audio.
 */

#include <avdec_private.h>
#include <dvframe.h>
#include <libdv/dv.h>

struct bgav_dv_dec_s
  {
  uint8_t * buffer;
  dv_decoder_t *dv;

  gavl_audio_format_t audio_format;
  gavl_video_format_t video_format;
    
  int frame_size; /* Compressed frame in bytes */
  int is_pal;

  int64_t frame_counter;
  };

bgav_dv_dec_t * bgav_dv_dec_create()
  {
  bgav_dv_dec_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->dv = dv_decoder_new(0, 0, 0);
  return ret;
  }

void bgav_dv_dec_destroy(bgav_dv_dec_t * d)
  {
  dv_decoder_free(d->dv);
  free(d);
  }

/* Sets the header for parsing. data must be 6 bytes long */

void bgav_dv_dec_set_header(bgav_dv_dec_t * d, uint8_t * data)
  {
  if((data[3] & 0x80) == 0)
    {      /* DSF flag */
    d->frame_size = 120000;
    d->is_pal = 0;
    }
  else
    {
    d->frame_size = 144000;
    d->is_pal = 1;
    }
  }

int bgav_dv_dec_get_frame_size(bgav_dv_dec_t * d)
  {
  return d->frame_size;
  }

/* Sets the frame for parsing. data must be frame_size bytes long */

void bgav_dv_dec_set_frame(bgav_dv_dec_t * d, uint8_t * data)
  {
  d->buffer = data;

  dv_parse_header(d->dv, d->buffer);
  }

/* Set up audio and video streams */

void bgav_dv_dec_init_audio(bgav_dv_dec_t * d, bgav_stream_t * s)
  {
  s->data.audio.format.samplerate =  dv_get_frequency(d->dv);
  s->data.audio.format.num_channels =  dv_get_num_channels(d->dv);
  s->data.audio.format.sample_format =  GAVL_SAMPLE_S16;
  s->data.audio.format.interleave_mode =  GAVL_INTERLEAVE_NONE;
  s->data.audio.format.samples_per_frame = DV_AUDIO_MAX_SAMPLES;
  
  gavl_set_channel_setup(&(s->data.audio.format));
  s->fourcc = BGAV_MK_FOURCC('g', 'a', 'v', 'l');

  gavl_audio_format_copy(&(d->audio_format), &(s->data.audio.format));
  }

void bgav_dv_dec_init_video(bgav_dv_dec_t * d, bgav_stream_t * s)
  {
  s->fourcc = BGAV_MK_FOURCC('d', 'v', 'c', ' ');

  if(d->is_pal)
    {
    s->data.video.format.frame_duration = 1;
    s->data.video.format.timescale = 25;
    if(dv_format_wide(d->dv))
      {
      s->data.video.format.pixel_width = 118;
      s->data.video.format.pixel_height = 81;
      }
    else
      {
      s->data.video.format.pixel_width = 59;
      s->data.video.format.pixel_height = 54;
      }
    }
  else
    {
    s->data.video.format.frame_duration = 1001;
    s->data.video.format.timescale = 30000;
    if(dv_format_wide(d->dv))
      {
      s->data.video.format.pixel_width = 40;
      s->data.video.format.pixel_height = 33;
      }
    else
      {
      s->data.video.format.pixel_width = 10;
      s->data.video.format.pixel_height = 11;
      }
    }
  
  gavl_video_format_copy(&(d->video_format), &(s->data.video.format));
  }

void bgav_dv_dec_set_frame_counter(bgav_dv_dec_t * d, int64_t count)
  {
  d->frame_counter = count;
  }

/* Extract audio and video packets suitable for the decoders */

int bgav_dv_dec_get_audio_packet(bgav_dv_dec_t * d, bgav_packet_t * p)
  {
  if(!p->audio_frame)
    p->audio_frame = gavl_audio_frame_create(&(d->audio_format));
  p->audio_frame->valid_samples = dv_get_num_samples(d->dv);
  p->keyframe = 1;
    
  if(!dv_decode_full_audio(d->dv,
                           d->buffer, p->audio_frame->channels.s_16))
    p->audio_frame->valid_samples;

  //  fprintf(stderr, "Valid samples: %d\n", p->audio_frame->valid_samples);
  return 1;
  }

void bgav_dv_dec_get_video_packet(bgav_dv_dec_t * d, bgav_packet_t * p)
  {
  p->keyframe = 1;
  p->timestamp_scaled = d->video_format.frame_duration * d->frame_counter;
  d->frame_counter++;
  }
