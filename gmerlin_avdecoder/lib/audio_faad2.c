/*****************************************************************
 
  audio_faad2.c
 
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
#include <string.h>
#include <faad.h>
#include <avdec_private.h>

typedef struct
  {
  faacDecHandle dec;
  float * sample_buffer;
  int sample_buffer_size;

  uint8_t * data_buffer;
  uint8_t * data_buffer_ptr;
  int data_buffer_size;
  int data_buffer_alloc;
  
  gavl_audio_frame_t * frame;
  int last_block_size;
  } faad_priv_t;

static int init_faad2(bgav_stream_t * s)
  {
  faad_priv_t * priv;
  unsigned long samplerate;
  unsigned char channels;
  char result;
  faacDecConfigurationPtr cfg;
  
  priv = calloc(1, sizeof(*priv));
  priv->dec = faacDecOpen();
  priv->frame = gavl_audio_frame_create(NULL);
  
  /* Init the library using a DecoderSpecificInfo */

  //  fprintf(stderr, "Extradata size: %d\n", s->ext_size);
  result = faacDecInit2(priv->dec, s->ext_data,
                        s->ext_size,
                        &samplerate, &channels);
  //  fprintf(stderr, "Result: %d %d %d\n", result, samplerate, channels);

  /* Some mp4 files have a wrong samplerate in the sample description,
     so we correct it here */

  s->data.audio.format.samplerate = samplerate;
  s->data.audio.format.num_channels = channels;
  //  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.format.samples_per_frame = 1024;

  gavl_set_channel_setup(&(s->data.audio.format));
  
  cfg = faacDecGetCurrentConfiguration(priv->dec);
  //  cfg->outputFormat = FAAD_FMT_FLOAT;
  cfg->outputFormat = FAAD_FMT_16BIT;
  faacDecSetConfiguration(priv->dec, cfg);
  
  s->data.audio.decoder->priv = priv;

  s->description = bgav_sprintf("%s", "AAC Audio stream");
  
  return 1;
  }

static int get_data(bgav_stream_t * s)
  {
  faad_priv_t * priv;
  bgav_packet_t * p;
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);

  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;
  
  if(priv->data_buffer_alloc < p->data_size)
    {
    priv->data_buffer_alloc = p->data_size + 32;
    priv->data_buffer = realloc(priv->data_buffer, priv->data_buffer_alloc);
    }
  memcpy(priv->data_buffer, p->data, p->data_size);
  priv->data_buffer_size = p->data_size;
  priv->data_buffer_ptr = priv->data_buffer;
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

static int decode_frame(bgav_stream_t * s)
  {
  faacDecFrameInfo frame_info;
  faad_priv_t * priv;
  
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);
  
  if(!priv->data_buffer_size)
    if(!get_data(s))
      return 0;

  //  fprintf(stderr, "Decode %d\n", priv->data_buffer_size);
  
  priv->frame->samples.f = faacDecDecode(priv->dec,
                                         &frame_info,
                                         priv->data_buffer_ptr,
                                         priv->data_buffer_size);

  if(!priv->frame->samples.f)
    return 0;
  // fprintf(stderr, "Decoded %d samples, used %d bytes\n", frame_info.samples,
  //         frame_info.bytesconsumed);

  priv->data_buffer_ptr += frame_info.bytesconsumed;
  priv->data_buffer_size -= frame_info.bytesconsumed;
  priv->last_block_size = frame_info.samples / s->data.audio.format.num_channels;
  priv->frame->valid_samples = frame_info.samples  / s->data.audio.format.num_channels;
  
  return 1;
  }

static int decode_faad2(bgav_stream_t * s, gavl_audio_frame_t * f,
                        int num_samples)
  {
  faad_priv_t * priv;
  int samples_copied;
  int samples_decoded = 0;

  priv = (faad_priv_t *)(s->data.audio.decoder->priv);

  while(samples_decoded < num_samples)
    {
    if(!priv->frame->valid_samples)
      if(!decode_frame(s))
        return samples_decoded;
    samples_copied = gavl_audio_frame_copy(&(s->data.audio.format),
                                           f,
                                           priv->frame,
                                           samples_decoded, /* out_pos */
                                           priv->last_block_size - priv->frame->valid_samples,  /* in_pos */
                                           num_samples - samples_decoded, /* out_size, */
                                           priv->frame->valid_samples /* in_size */);
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(f)
    f->valid_samples = samples_decoded;
  return samples_decoded;
  }

static void close_faad2(bgav_stream_t * s)
  {
  faad_priv_t * priv;
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);

  gavl_audio_frame_null(priv->frame);
  gavl_audio_frame_destroy(priv->frame);
  free(priv);
  }

static void resync_faad2(bgav_stream_t * s)
  {
  faad_priv_t * priv;
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);
  priv->frame->valid_samples = 0;
  priv->sample_buffer_size = 0;
  priv->data_buffer_size = 0;
  
  }

static bgav_audio_decoder_t decoder =
  {
    name:   "FAAD AAC audio decoder",
    fourccs: (int[]){ BGAV_MK_FOURCC('m','p','4','a'),
                      0x0 },
    
    init:   init_faad2,
    decode: decode_faad2,
    close:  close_faad2,
    resync:  resync_faad2
  };

void bgav_init_audio_decoders_faad2()
  {
  bgav_audio_decoder_register(&decoder);
  }

