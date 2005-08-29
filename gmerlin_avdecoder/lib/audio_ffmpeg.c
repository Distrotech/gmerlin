/*****************************************************************
 
  audio_ffmpeg.c
 
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
#include <ffmpeg/avcodec.h>
#include <stdio.h>

#include <config.h>
#include <codecs.h>

/* Map of ffmpeg codecs to fourccs (from ffmpeg's avienc.c) */

typedef struct
  {
  const char * decoder_name;
  const char * format_name;
  enum CodecID ffmpeg_id;
  uint32_t * fourccs;
  } codec_info_t;

static codec_info_t codec_infos[] =
  {
#if 1
    { "FFmpeg ra28.8 decoder", "Real audio 28.8", CODEC_ID_RA_288,
      (uint32_t[]){ BGAV_MK_FOURCC('2', '8', '_', '8'), 0x00 } },
#endif
#if 1
    { "FFmpeg ra14.4 decoder", "Real audio 14.4", CODEC_ID_RA_144,
      (uint32_t[]){ BGAV_MK_FOURCC('1', '4', '_', '4'),
               BGAV_MK_FOURCC('l', 'p', 'c', 'J'),
               0x00 } },
#endif

    /* MPEG audio is handled by mad */
#if 0    
    { "FFmpeg mp2 decoder", "MPEG audio Layer 1/2/3", CODEC_ID_MP2,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x50), 0x00 } },

    { "FFmpeg mp3 decoder", "MPEG audio Layer 1/2/3", CODEC_ID_MP3,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x55),
               BGAV_MK_FOURCC('.', 'm', 'p', '3'),
               BGAV_MK_FOURCC('m', 's', 0x00, 0x55),
               0x00 } },
    { "FFmpeg ac3 decoder", "AC3", CODEC_ID_AC3,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x2000),
                    BGAV_MK_FOURCC('.', 'a', 'c', '3'),
                    0x00 } },
    
#endif
    { "FFmpeg alaw decoder", "alaw", CODEC_ID_PCM_ALAW,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x06),
                    BGAV_MK_FOURCC('a', 'l', 'a', 'w'),
                    BGAV_MK_FOURCC('A', 'L', 'A', 'W'),
                    0x00 }  },

    { "FFmpeg mulaw decoder", "ulaw", CODEC_ID_PCM_MULAW,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x07),
                    BGAV_MK_FOURCC('u', 'l', 'a', 'w'),
                    BGAV_MK_FOURCC('U', 'L', 'A', 'W'),
                    0x00 }  },
    
    { "FFmpeg MS ADPCM decoder", "MS ADPCM", CODEC_ID_ADPCM_MS,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x02),
                    BGAV_MK_FOURCC('m', 's', 0x00, 0x02), 0x00 } },

    { "FFmpeg WAV ADPCM decoder", "WAV ADPCM", CODEC_ID_ADPCM_IMA_WAV,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x11),
                    BGAV_MK_FOURCC('m', 's', 0x00, 0x11), 0x00 } },

    { "FFmpeg IMA DK4 decoder", "IMA DK4", CODEC_ID_ADPCM_IMA_DK4,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x61), 0x00 } },  /* rogue format number */
    { "FFmpeg IMA DK3 decoder", "IMA DK3", CODEC_ID_ADPCM_IMA_DK3,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x62), 0x00 } },  /* rogue format number */

    { "FFmpeg ima4 decoder", "ima4", CODEC_ID_ADPCM_IMA_QT,
      (uint32_t[]){ BGAV_MK_FOURCC('i', 'm', 'a', '4'), 0x00 }  },

    { "FFmpeg WMA1 decoder", "Window Media Audio 1", CODEC_ID_WMAV1,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x160), 0x00 } },
    { "FFmpeg WMA2 decoder", "Window Media Audio 2", CODEC_ID_WMAV2,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x161), 0x00 } },
    { "FFmpeg MACE3 decoder", "Apple MACE 3", CODEC_ID_MACE3,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'A', 'C', '3'), 0x00 } },
#if 1
    { "FFmpeg MACE6 decoder", "Apple MACE 6", CODEC_ID_MACE6,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'A', 'C', '6'), 0x00 } },
#endif
  };


#define NUM_CODECS sizeof(codec_infos)/sizeof(codec_infos[0])

static int real_num_codecs;

static struct
  {
  codec_info_t * info;
  bgav_audio_decoder_t decoder;
  } codecs[NUM_CODECS];

typedef struct
  {
  AVCodecContext * ctx;
  codec_info_t * info;

  gavl_audio_frame_t * frame;
  int last_frame_size;
  
  uint8_t * packet_buffer;
  uint8_t * packet_buffer_ptr;
  int packet_buffer_alloc;
  int bytes_in_packet_buffer;
  } ffmpeg_audio_priv;

static codec_info_t * lookup_codec(bgav_stream_t * s)
  {
  int i;
  
  for(i = 0; i < real_num_codecs; i++)
    {
    if(s->fourcc == BGAV_WAVID_2_FOURCC(0x01))
      {
      if((s->data.audio.bits_per_sample == 16) && (codecs[i].info->ffmpeg_id == CODEC_ID_PCM_S16LE))
        return codecs[i].info;
      else if((s->data.audio.bits_per_sample == 8) && (codecs[i].info->ffmpeg_id == CODEC_ID_PCM_U8))
        return codecs[i].info;
      }
    else if(s->fourcc == BGAV_MK_FOURCC('t', 'w', 'o', 's'))
      {
      if((s->data.audio.bits_per_sample == 16) && (codecs[i].info->ffmpeg_id == CODEC_ID_PCM_S16BE))
        return codecs[i].info;
      else if((s->data.audio.bits_per_sample == 8) && (codecs[i].info->ffmpeg_id == CODEC_ID_PCM_S8))
        return codecs[i].info;
      }
    else
      if(s->data.audio.decoder->decoder == &(codecs[i].decoder))
        return codecs[i].info;
    }
  return (codec_info_t*)0;
  }

static int init(bgav_stream_t * s)
  {
  AVCodec * codec;
  ffmpeg_audio_priv * priv;
  priv = calloc(1, sizeof(*priv));
  priv->info = lookup_codec(s);
  codec = avcodec_find_decoder(priv->info->ffmpeg_id);
  priv->ctx = avcodec_alloc_context();
  
  //  fprintf(stderr, "Initializing %s\n", s->data.audio.decoder->decoder->name);

  //  priv->ctx->width = s->format.frame_width;
  //  priv->ctx->height = s->format.frame_height;
  
  priv->ctx->extradata = s->ext_data;
  priv->ctx->extradata_size = s->ext_size;

  /* Dump extradata */

  //  fprintf(stderr, "Extradata:\n");
  //  bgav_hexdump(priv->ctx->extradata, priv->ctx->extradata_size,
  //               16);
  
    
  priv->ctx->channels =    s->data.audio.format.num_channels;
  priv->ctx->sample_rate = s->data.audio.format.samplerate;
  priv->ctx->block_align = s->data.audio.block_align;
  priv->ctx->bit_rate    = s->codec_bitrate;

  priv->ctx->codec_tag   =
    ((s->fourcc & 0x000000ff) << 24) ||
    ((s->fourcc & 0x0000ff00) << 8) ||
    ((s->fourcc & 0x00ff0000) >> 8) ||
    ((s->fourcc & 0xff000000) >> 24);
  priv->ctx->codec_id =
    codec->id;
    
  /* Some codecs need extra stuff */
    
  /* Open codec */
  
  if(avcodec_open(priv->ctx, codec) != 0)
    return 0;
  s->data.audio.decoder->priv = priv;

  /* Set missing format values */
  
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
  gavl_set_channel_setup(&(s->data.audio.format));

  s->data.audio.format.samples_per_frame = 2*AVCODEC_MAX_AUDIO_FRAME_SIZE;

  priv->frame = gavl_audio_frame_create(&(s->data.audio.format));
    
  s->data.audio.format.samples_per_frame = 1024;
    
  s->description = bgav_sprintf(priv->info->format_name);
  return 1;
  }

static int decode_frame(bgav_stream_t * s)
  {
  int frame_size;
  int bytes_used;

  bgav_packet_t * p;
  ffmpeg_audio_priv * priv;
  priv= (ffmpeg_audio_priv*)(s->data.audio.decoder->priv);

  /* Read data if necessary */
  if(!priv->bytes_in_packet_buffer)
    {
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!p)
      {
      //        fprintf(stderr, "Got no packet\n");
      return 0;
      }
    if(p->data_size + FF_INPUT_BUFFER_PADDING_SIZE > priv->packet_buffer_alloc)
      {
      priv->packet_buffer_alloc = p->data_size + FF_INPUT_BUFFER_PADDING_SIZE + 32;
      priv->packet_buffer = realloc(priv->packet_buffer, priv->packet_buffer_alloc);
      }
    //      fprintf(stderr, "Got packet: %d bytes\n", p->data_size);      
    priv->bytes_in_packet_buffer = p->data_size;
    memcpy(priv->packet_buffer, p->data, p->data_size);
    priv->packet_buffer_ptr = priv->packet_buffer;
    
    memset(&(priv->packet_buffer[p->data_size]), 0, FF_INPUT_BUFFER_PADDING_SIZE);
    bgav_demuxer_done_packet_read(s->demuxer, p);
    }
  
  bytes_used = avcodec_decode_audio(priv->ctx,
                                    priv->frame->samples.s_16,
                                    &frame_size,
                                    priv->packet_buffer_ptr,
                                    priv->bytes_in_packet_buffer);
  //  fprintf(stderr, "Used: %d bytes, frame_size: %d\n", bytes_used, frame_size);
    
  /* Check for error */
    
  if(frame_size < 0)
    {
    fprintf(stderr, "Decoding failed %d\n", bytes_used);
    //      if(f)
    //        f->valid_samples = samples_decoded;
    //      return samples_decoded;
    }
  /* Advance packet buffer */
    
  priv->packet_buffer_ptr += bytes_used;
  priv->bytes_in_packet_buffer -= bytes_used;

  //  fprintf(stderr, "Frame size: %d\n", frame_size);
  
  /* No Samples decoded, get next packet */

  if(frame_size < 0)
    return 1;

  /* Sometimes, frame_size is terribly wrong */

  if(frame_size > AVCODEC_MAX_AUDIO_FRAME_SIZE*2)
    {
    fprintf(stderr, "Adjusting frame size %d -> ", frame_size);
    frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE*2;
    fprintf(stderr, "%d\n", frame_size);
    }
  
  frame_size /= (2 * s->data.audio.format.num_channels);
  priv->last_frame_size = frame_size;
  priv->frame->valid_samples = frame_size;

  return 1;
  }

static int decode(bgav_stream_t * s, gavl_audio_frame_t * f, int num_samples)
  {
  ffmpeg_audio_priv * priv;
  int samples_decoded = 0;
  int samples_copied;
  priv= (ffmpeg_audio_priv*)(s->data.audio.decoder->priv);

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
#if 0
    fprintf(stderr, "Copy out_pos: %d, in_pos: %d, out_size: %d, in_size: %d\n",
            samples_decoded, /* out_pos */
            priv->last_frame_size - priv->frame->valid_samples,  /* in_pos */
            num_samples - samples_decoded, /* out_size, */
            priv->frame->valid_samples);
#endif       
    samples_copied = gavl_audio_frame_copy(&(s->data.audio.format),
                                           f,
                                           priv->frame,
                                           samples_decoded, /* out_pos */
                                           priv->last_frame_size - priv->frame->valid_samples,  /* in_pos */
                                           num_samples - samples_decoded, /* out_size, */
                                           priv->frame->valid_samples /* in_size */);
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(f)
    f->valid_samples = samples_decoded;
  return samples_decoded;
  }

static void resync_ffmpeg(bgav_stream_t * s)
  {
  ffmpeg_audio_priv * priv;
  priv = (ffmpeg_audio_priv*)(s->data.audio.decoder->priv);
  avcodec_flush_buffers(priv->ctx);
  priv->frame->valid_samples = 0;
  priv->bytes_in_packet_buffer = 0;
  }

static void close_ffmpeg(bgav_stream_t * s)
  {
  ffmpeg_audio_priv * priv;
  priv= (ffmpeg_audio_priv*)(s->data.audio.decoder->priv);

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);
  
  if(priv->packet_buffer)
    free(priv->packet_buffer);

  if(priv->ctx)
    {
    avcodec_close(priv->ctx);
    free(priv->ctx);
    }
  free(priv);
  }

void
bgav_init_audio_decoders_ffmpeg()
  {
  int i;
  real_num_codecs = 0;
  avcodec_init();
  avcodec_register_all();

  for(i = 0; i < NUM_CODECS; i++)
    {
    if(avcodec_find_decoder(codec_infos[i].ffmpeg_id))
      {
      // fprintf(stderr, "Trying %s\n", codec_map_template[i].name);
      codecs[real_num_codecs].info = &(codec_infos[i]);
      codecs[real_num_codecs].decoder.name =
        codecs[real_num_codecs].info->decoder_name;
      codecs[real_num_codecs].decoder.fourccs =
        codecs[real_num_codecs].info->fourccs;
      codecs[real_num_codecs].decoder.init = init;
      codecs[real_num_codecs].decoder.decode = decode;
      codecs[real_num_codecs].decoder.close = close_ffmpeg;
      codecs[real_num_codecs].decoder.resync = resync_ffmpeg;
      // fprintf(stderr, "Registering Codec %s\n",
      //         codecs[real_num_codecs].decoder.name);
      bgav_audio_decoder_register(&codecs[real_num_codecs].decoder);
      
      real_num_codecs++;
      }
    }
  }

