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

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>

typedef struct
  {
  faacDecHandle dec;
  float * sample_buffer;
  int sample_buffer_size;

  uint8_t * data;
  uint8_t * data_ptr;
  int data_size;
  int data_alloc;
  
  gavl_audio_frame_t * frame;
  int last_block_size;
  } faad_priv_t;

static int get_data(bgav_stream_t * s)
  {
  faad_priv_t * priv;
  bgav_packet_t * p;
  int buffer_offset;
  
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);

  fprintf(stderr, "Get data %d\n", priv->data_size);
  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;
  
  if(priv->data_alloc < p->data_size + priv->data_size)
    {
    buffer_offset = priv->data_ptr - priv->data;
    
    priv->data_alloc = p->data_size + priv->data_size + 32;
    priv->data = realloc(priv->data, priv->data_alloc);
    priv->data_ptr = priv->data + buffer_offset;
    }

  if(priv->data_size)
    memmove(priv->data, priv->data_ptr, priv->data_size);
  priv->data_ptr = priv->data;
  
  memcpy(priv->data + priv->data_size, p->data, p->data_size);
  priv->data_size += p->data_size;
  bgav_demuxer_done_packet_read(s->demuxer, p);

  //  fprintf(stderr, "done %d\n", priv->data_size);
  return 1;
  }

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
  s->data.audio.decoder->priv = priv;
  
  /* Init the library using a DecoderSpecificInfo */

  //  fprintf(stderr, "Extradata size: %d\n", s->ext_size);

  if(!s->ext_size)
    {
    if(!get_data(s))
      return 0;

    result = faacDecInit(priv->dec, priv->data_ptr,
                         priv->data_size,
                         &samplerate, &channels);
    //    fprintf(stderr, "faacDecInit %d bytes used\n", result);

    priv->data_size -= result;
    priv->data_ptr += result;
    }
  else
    result = faacDecInit2(priv->dec, s->ext_data,
                          s->ext_size,
                          &samplerate, &channels);
  //  fprintf(stderr, "Result: %d %d %d\n", result, samplerate, channels);

  /* Some mp4 files have a wrong samplerate in the sample description,
     so we correct it here */

  s->data.audio.format.samplerate = samplerate;
  s->data.audio.format.num_channels = channels;
  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  //  s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.format.samples_per_frame = 1024;

  gavl_set_channel_setup(&(s->data.audio.format));
  
  cfg = faacDecGetCurrentConfiguration(priv->dec);
  cfg->outputFormat = FAAD_FMT_FLOAT;
  // cfg->outputFormat = FAAD_FMT_16BIT;
  faacDecSetConfiguration(priv->dec, cfg);

  s->description = bgav_sprintf("%s", "AAC Audio stream");
  
  return 1;
  }


static int frame_number = 0;

static int decode_frame(bgav_stream_t * s)
  {
  faacDecFrameInfo frame_info;
  faad_priv_t * priv;
  
  priv = (faad_priv_t *)(s->data.audio.decoder->priv);

  //  fprintf(stderr, "Decode frame %d\n", priv->data_size);
  
  memset(&frame_info, 0, sizeof(&frame_info));
  
  if(priv->data_size < FAAD_MIN_STREAMSIZE*GAVL_MAX_CHANNELS)
    if(!get_data(s))
      return 0;

  //  fprintf(stderr, "Decode %d\n", priv->data_size);

  /*
   * Dirty trick: Frames from some mp4 files are randomly padded with
   * one byte, padding bits are (hopefully always) set to zero.
   * Doing this cleanly would require heavy changes in the mp4 demuxer
   * code.
   */
#if 1
  if(*priv->data_ptr == 0x00)
    {
    priv->data_ptr++;
    priv->data_size--;
    }
#endif
  while(1)
    {
    fprintf(stderr, "faacDecDecode %d, buf: %d: ", frame_number, priv->data_size);
    //    bgav_hexdump(priv->data_ptr, 16, 16);
    priv->frame->samples.f = faacDecDecode(priv->dec,
                                           &frame_info,
                                           priv->data_ptr,
                                           priv->data_size);
    priv->data_ptr  += frame_info.bytesconsumed;
    priv->data_size -= frame_info.bytesconsumed;

    //    fprintf(stderr, "Bytes used: %ld\n", frame_info.bytesconsumed);
    
    if(!priv->frame->samples.f)
      {
      if(frame_info.error == 14) /* Too little data */
        {
        //        fprintf(stderr, "Need more data %d\n", priv->data_size);
        if(!get_data(s))
          return 0;
        }
      else
        {
        fprintf(stderr, "faad2: faacDecDecode failed %s\n",
                faacDecGetErrorMessage(frame_info.error));
        //    priv->data_size = 0;
        //    priv->frame->valid_samples = 0;
        return 0; /* Recatching the stream is doomed to failure, so we end here */
        }
      }
    else
      break;
    }
  frame_number++;
  priv->frame->valid_samples = frame_info.samples  / s->data.audio.format.num_channels;
  priv->last_block_size = priv->frame->valid_samples;
  
  fprintf(stderr, "Decoded %d samples, used %d bytes\n", priv->last_block_size,
          frame_info.bytesconsumed);
  
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
      {
      //      fprintf(stderr, "decode frame...");
      if(!decode_frame(s))
        {
        if(f)
          f->valid_samples = samples_decoded;
        return samples_decoded;
        }
      //      fprintf(stderr, "done\n");
      }
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
  priv->data_size = 0;
  
  }

static bgav_audio_decoder_t decoder =
  {
    name:   "FAAD AAC audio decoder",
    fourccs: (int[]){ BGAV_MK_FOURCC('m','p','4','a'),
                      BGAV_MK_FOURCC('a','a','c',' '),
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

