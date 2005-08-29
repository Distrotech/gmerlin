/*****************************************************************
 
  audio_flac.c
 
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
#include <stdio.h>
#include <FLAC/stream_decoder.h>

#include <codecs.h>

#define MAX_FRAME_SAMPLES 65535

typedef struct
  {
  FLAC__StreamDecoder * dec;

  bgav_packet_t * p;
  uint8_t * data_ptr;
  uint8_t * header_ptr;
  
  int last_frame_samples;

  gavl_audio_frame_t * frame;

  void (*copy_samples)(gavl_audio_frame_t * f,
                       const FLAC__int32 *const buffer[],
                       int num_channels,
                       int shift_bits);
  int shift_bits;
  } flac_priv_t;

/* FLAC Callbacks */

static FLAC__StreamDecoderReadStatus
read_callback(const FLAC__StreamDecoder *decoder,
              FLAC__byte buffer[],
              unsigned *bytes, void *client_data)
  {
  int bytes_read = 0;
  int bytes_to_copy;
  flac_priv_t * priv;
  bgav_stream_t * s;
  s = (bgav_stream_t *)client_data;

  priv = (flac_priv_t*)(s->data.audio.decoder->priv);
  
  //  fprintf(stderr, "Read callback\n"); 
  while(bytes_read < *bytes)
    {
    /* Read header */
    if(priv->header_ptr - s->ext_data < s->ext_size)
      {
      bytes_to_copy =
        (s->ext_size - (priv->header_ptr - s->ext_data));
      if(bytes_to_copy > *bytes - bytes_read)
        bytes_to_copy = *bytes - bytes_read;
      memcpy(&(buffer[bytes_read]), priv->header_ptr, bytes_to_copy);
      bytes_read += bytes_to_copy;
      priv->header_ptr += bytes_to_copy;

      //      fprintf(stderr, "Added header %d\n", bytes_to_copy);
      }
    
    if(!priv->p)
      {
      priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);
      if(!priv->p)
        break;
      priv->data_ptr = priv->p->data;
      }

    /* Bytes, which are left from the packet */
    bytes_to_copy = (priv->p->data_size - (priv->data_ptr - priv->p->data));
    if(bytes_to_copy > *bytes - bytes_read)
      bytes_to_copy = *bytes - bytes_read;

    memcpy(&(buffer[bytes_read]), priv->data_ptr, bytes_to_copy);

    bytes_read += bytes_to_copy;
    priv->data_ptr += bytes_to_copy;

    if(priv->data_ptr - priv->p->data == priv->p->data_size)
      {
      bgav_demuxer_done_packet_read(s->demuxer, priv->p);
      priv->p = (bgav_packet_t*)0;
      }
    
    }
  //  fprintf(stderr, "Read callback %d %d\n", bytes_read, *bytes); 
  if(!bytes_read)
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
  else
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
  }

static void copy_samples_8(gavl_audio_frame_t * f,
                           const FLAC__int32 *const buffer[],
                           int num_channels,
                           int shift_bits)
  {
  int i, j;
  
  for(i = 0; i < num_channels; i++)
    {
    for(j = 0; j < f->valid_samples; j++)
      {
      f->channels.s_8[i][j] = buffer[i][j] << shift_bits;
      }
    }

  }

static void copy_samples_16(gavl_audio_frame_t * f,
                            const FLAC__int32 *const buffer[],
                            int num_channels,
                            int shift_bits)
  {
  int i, j;
  
  for(i = 0; i < num_channels; i++)
    {
    for(j = 0; j < f->valid_samples; j++)
      {
      f->channels.s_16[i][j] = buffer[i][j] << shift_bits;
      }
    }
  
  }

static void copy_samples_32(gavl_audio_frame_t * f,
                            const FLAC__int32 *const buffer[],
                            int num_channels,
                            int shift_bits)
  {
  int i, j;
  
  for(i = 0; i < num_channels; i++)
    {
    for(j = 0; j < f->valid_samples; j++)
      {
      f->channels.s_32[i][j] = buffer[i][j] << shift_bits;
      }
    }
  }

static FLAC__StreamDecoderWriteStatus
write_callback(const FLAC__StreamDecoder *decoder,
               const FLAC__Frame *frame,
               const FLAC__int32 *const buffer[],
               void *client_data)
  {
  flac_priv_t * priv;
  bgav_stream_t * s;
  s = (bgav_stream_t *)client_data;
  //  fprintf(stderr, "Write callback %d\n", frame->header.blocksize);
  priv = (flac_priv_t*)(s->data.audio.decoder->priv);

  priv->frame->valid_samples = frame->header.blocksize;
  priv->copy_samples(priv->frame, buffer, s->data.audio.format.num_channels,
                     priv->shift_bits);
  
  priv->last_frame_samples = priv->frame->valid_samples;
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  }

void
metadata_callback(const FLAC__StreamDecoder *decoder,
                  const FLAC__StreamMetadata *metadata,
                  void *client_data)
  {
  //  fprintf(stderr, "Metadata_callback\n"); 
  }

void
error_callback(const FLAC__StreamDecoder *decoder,
               FLAC__StreamDecoderErrorStatus status,
               void *client_data)
  {
  fprintf(stderr, "Error callback %s\n",
          FLAC__StreamDecoderErrorStatusString[status]);
  }

static int init_flac(bgav_stream_t * s)
  {
  flac_priv_t * priv;
  gavl_audio_format_t frame_format;
  if(s->ext_size < 42)
    {
    return 0;
    fprintf(stderr, "FLAC decoder needs 42 bytes ext_data\n");
    }
  
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;
  priv->header_ptr = s->ext_data;
  priv->dec = FLAC__stream_decoder_new();
  
  FLAC__stream_decoder_set_read_callback(priv->dec,
                                         read_callback);
  FLAC__stream_decoder_set_write_callback(priv->dec,
                                          write_callback);

  FLAC__stream_decoder_set_error_callback(priv->dec,
                                          error_callback);
  FLAC__stream_decoder_set_metadata_callback(priv->dec,
                                             metadata_callback);

  FLAC__stream_decoder_set_client_data(priv->dec, s);
  FLAC__stream_decoder_init(priv->dec);
  
  gavl_set_channel_setup(&(s->data.audio.format));
  s->data.audio.format.samples_per_frame = 1024;

  /* Set sample format stuff */

  if(s->data.audio.bits_per_sample <= 8)
    {
    priv->shift_bits = 8 - s->data.audio.bits_per_sample;
    s->data.audio.format.sample_format = GAVL_SAMPLE_S8;
    priv->copy_samples = copy_samples_8;
    }
  else if(s->data.audio.bits_per_sample <= 16)
    {
    priv->shift_bits = 16 - s->data.audio.bits_per_sample;
    s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
    priv->copy_samples = copy_samples_16;
    }
  else
    {
    priv->shift_bits = 32 - s->data.audio.bits_per_sample;
    s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
    priv->copy_samples = copy_samples_32;
    }
  
  gavl_audio_format_copy(&(frame_format),
                         &(s->data.audio.format));

  frame_format.samples_per_frame = MAX_FRAME_SAMPLES;
  priv->frame = gavl_audio_frame_create(&frame_format);

  s->description = bgav_sprintf("FLAC (%d bits per sample)", s->data.audio.bits_per_sample);
  
  return 1;
  }

static int decode_flac(bgav_stream_t * s,
                       gavl_audio_frame_t * frame,
                       int num_samples)
  {
  int samples_decoded = 0;
  int samples_copied;
  flac_priv_t * priv;
  priv = (flac_priv_t*)(s->data.audio.decoder->priv);
  
  while(samples_decoded <  num_samples)
    {
    /* Decode another frame */
    while(!priv->frame->valid_samples)
      {
      FLAC__stream_decoder_process_single(priv->dec);

      if(FLAC__stream_decoder_get_state(priv->dec) ==
         FLAC__STREAM_DECODER_END_OF_STREAM)
        {
        if(frame)
          frame->valid_samples = samples_decoded;
        return samples_decoded;
        }
        
      }
    samples_copied =
      gavl_audio_frame_copy(&(s->data.audio.format),
                            frame,       /* dst */
                            priv->frame, /* src */
                            samples_decoded, /* int dst_pos */
                            priv->last_frame_samples -
                            priv->frame->valid_samples, /* int src_pos */
                            num_samples - samples_decoded, /* int dst_size, */
                            priv->frame->valid_samples /* int src_size*/ );
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(frame)
    frame->valid_samples = samples_decoded;
  return samples_decoded;
  }

static void close_flac(bgav_stream_t * s)
  {
  flac_priv_t * priv;
  priv = (flac_priv_t*)(s->data.audio.decoder->priv);
  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);
  if(priv->dec)
    FLAC__stream_decoder_delete(priv->dec);
  free(priv);
  }

static void resync_flac(bgav_stream_t * s)
  {
  flac_priv_t * priv;
  priv = (flac_priv_t*)(s->data.audio.decoder->priv);
  
  FLAC__stream_decoder_flush(priv->dec);
  priv->frame->valid_samples = 0;
  priv->p = 0;
  }


static bgav_audio_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('F', 'L', 'A', 'C'),
                           0x00 },
    name: "FLAC audio decoder",
    init: init_flac,
    close: close_flac,
    resync: resync_flac,
    decode: decode_flac
  };

void bgav_init_audio_decoders_flac()
  {
  bgav_audio_decoder_register(&decoder);
  }
