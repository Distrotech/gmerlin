/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <avdec_private.h>
#include <bswap.h>

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <config.h>
#include <codecs.h>

#include AVCODEC_HEADER

#define LOG_DOMAIN "audio_ffmpeg"

#ifdef HAVE_LIBAVCORE_AVCORE_H
#include <libavcore/avcore.h>
#endif

#if LIBAVCODEC_BUILD >= ((53<<16)+(25<<8)+0)
#define DECODE_AUDIO4
#elif LIBAVCODEC_BUILD >= ((52<<16)+(23<<8)+0)
#define DECODE_AUDIO3
#else
#define DECODE_AUDIO2
#endif

// #define DUMP_DECODE
// #define DUMP_PACKET
// #define DUMP_EXTRADATA

/* Different decoding functions */

// #define DECODE_FUNC avcodec_decode_audio2

#if LIBAVCORE_VERSION_INT >= ((0<<16)|(10<<8)|0)
#define SampleFormat    AVSampleFormat
#define SAMPLE_FMT_U8   AV_SAMPLE_FMT_U8
#define SAMPLE_FMT_S16  AV_SAMPLE_FMT_S16
#define SAMPLE_FMT_S32  AV_SAMPLE_FMT_S32
#define SAMPLE_FMT_FLT  AV_SAMPLE_FMT_FLT
#define SAMPLE_FMT_DBL  AV_SAMPLE_FMT_DBL
#define SAMPLE_FMT_NONE AV_SAMPLE_FMT_NONE
#endif

/* Sample formats */

static const struct
  {
  enum SampleFormat  ffmpeg_fmt;
  gavl_sample_format_t gavl_fmt;
  }
sampleformats[] =
  {
    { SAMPLE_FMT_U8,  GAVL_SAMPLE_U8 },
    { SAMPLE_FMT_S16, GAVL_SAMPLE_S16 },    ///< signed 16 bits
    { SAMPLE_FMT_S32, GAVL_SAMPLE_S32 },    ///< signed 32 bits
    { SAMPLE_FMT_FLT, GAVL_SAMPLE_FLOAT },  ///< float
    { SAMPLE_FMT_DBL, GAVL_SAMPLE_DOUBLE }, ///< double
  };

static gavl_sample_format_t
sample_format_ffmpeg_2_gavl(enum SampleFormat p)
  {
  int i;
  for(i = 0; i < sizeof(sampleformats)/sizeof(sampleformats[0]); i++)
    {
    if(sampleformats[i].ffmpeg_fmt == p)
      return sampleformats[i].gavl_fmt;
    }
  return GAVL_SAMPLE_NONE;
  }


/* Map of ffmpeg codecs to fourccs (from ffmpeg's avienc.c) */

typedef struct
  {
  const char * decoder_name;
  const char * format_name;
  enum CodecID ffmpeg_id;
  uint32_t * fourccs;
  int codec_tag;
  int preroll;
  } codec_info_t;

typedef struct
  {
  AVCodecContext * ctx;
  codec_info_t * info;
  
  gavl_audio_frame_t * frame;
#ifndef DECODE_AUDIO4
  int frame_alloc;
#endif
  bgav_bytebuffer_t buf;
  
  /* ffmpeg changes the extradata sometimes,
     so we save them locally here */
  uint8_t * ext_data;

#if (defined DECODE_AUDIO3) || (defined DECODE_AUDIO4)
  AVPacket pkt;
#endif
  int sample_size;
  } ffmpeg_audio_priv;

static codec_info_t * lookup_codec(bgav_stream_t * s);

static int init_format(bgav_stream_t * s)
  {
  ffmpeg_audio_priv * priv;
  priv= s->data.audio.decoder->priv;

  /* These might be set from the codec or the container */

  s->data.audio.format.num_channels = priv->ctx->channels;
  s->data.audio.format.samplerate   = priv->ctx->sample_rate;
  
  /* These come from the codec */
  
  s->data.audio.format.sample_format =
    sample_format_ffmpeg_2_gavl(priv->ctx->sample_fmt);
  
  /* If we got no sample format, initialization went wrong */
  if(s->data.audio.format.sample_format == GAVL_SAMPLE_NONE)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Could not get sample format (maybe codec init failed)");
      return 0;
    }
  priv->sample_size =
    gavl_bytes_per_sample(s->data.audio.format.sample_format);
  
  gavl_set_channel_setup(&s->data.audio.format);

#ifndef DECODE_AUDIO4
  s->data.audio.format.samples_per_frame = 2*AVCODEC_MAX_AUDIO_FRAME_SIZE;
  priv->frame = gavl_audio_frame_create(&s->data.audio.format);
  priv->frame_alloc =
    priv->sample_size *
    s->data.audio.format.num_channels *
    s->data.audio.format.samples_per_frame;
  s->data.audio.format.samples_per_frame = 1024;
#endif

  fprintf(stderr, "Got format\n");
  return 1;
  }


static int fill_buffer(bgav_stream_t * s)
  {
  ffmpeg_audio_priv * priv;
  bgav_packet_t * p;
  priv= s->data.audio.decoder->priv;

  /* Read data if necessary */
  while(!priv->buf.size ||
     (s->data.audio.block_align &&
      (priv->buf.size < s->data.audio.block_align)))
    {
    /* Get packet */
    p = bgav_stream_get_packet_read(s);
    if(!p)
      return 0;
#ifdef DUMP_PACKET
    bgav_dprintf("Got packet\n");
    bgav_packet_dump(p);
#endif
    bgav_bytebuffer_append(&priv->buf, p, FF_INPUT_BUFFER_PADDING_SIZE);
    bgav_stream_done_packet_read(s, p);
    }
  return 1;
  }

/*
 *  Old version
 */

#ifndef DECODE_AUDIO4
static int decode_frame_ffmpeg(bgav_stream_t * s)
  {
  int bytes_used;

  ffmpeg_audio_priv * priv;
  int frame_size;
  
  priv= s->data.audio.decoder->priv;

  if(!fill_buffer(s))
    return 0;

  frame_size = priv->frame_alloc;
  
#ifdef DUMP_DECODE
  bgav_dprintf("decode_audio Size: %d, Frame size: %d\n",
               priv->buf.size, frame_size);
  //  bgav_hexdump(priv->packet_buffer_ptr, 16, 16);
#endif
  
  if(priv->frame)
    {
#ifdef
    priv->pkt.data = priv->buf.buffer;
    priv->pkt.size = priv->buf.size;
    
    bytes_used = avcodec_decode_audio3(priv->ctx,
                                       priv->frame->samples.s_16,
                                       &frame_size,
                                       &priv->pkt);
#else
    bytes_used = avcodec_decode_audio2(priv->ctx,
                                       priv->frame->samples.s_16,
                                       &frame_size,
                                       priv->buf.buffer,
                                       priv->buf.size);
#endif
    }
  else
    {
    int16_t * tmp_buf = malloc(priv->frame_alloc);
#ifdef DECODE_AUDIO3
    priv->pkt.data = priv->buf.buffer;
    priv->pkt.size = priv->buf.size;
    
    bytes_used = avcodec_decode_audio3(priv->ctx,
                                       tmp_buf,
                                       &frame_size,
                                       &priv->pkt);
#else
    bytes_used = avcodec_decode_audio2(priv->ctx,
                                       tmp_buf,
                                       &frame_size,
                                       priv->buf.buffer,
                                       priv->buf.size);
#endif

    if(!init_format(s))
      return 0;
    memcpy(priv->frame->samples.s_16, tmp_buf, frame_size);
    free(tmp_buf);
    }
  
#ifdef DUMP_DECODE
  bgav_dprintf("Used %d bytes (frame size: %d)\n", bytes_used, frame_size);
#endif
  
  if(bytes_used < 0)
    {
    frame_size = -1;
    }

  /* Check for error */
    
  if(frame_size < 0)
    {
    //      if(f)
    //        f->valid_samples = samples_decoded;
    //      return samples_decoded;
    }
  /* Advance packet buffer */


  if(bytes_used > 0)
    bgav_bytebuffer_remove(&priv->buf, bytes_used);
  else
    {
    //    priv->bytes_in_packet_buffer = 0;
    }
  
  /* No Samples decoded, get next packet */

  if(frame_size < 0)
    return 1;

  /* Sometimes, frame_size is terribly wrong */

  if(frame_size > AVCODEC_MAX_AUDIO_FRAME_SIZE*2)
    {
    frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE*2;
    }
  
  frame_size /= (priv->sample_size * s->data.audio.format.num_channels);
  priv->frame->valid_samples = frame_size;

#ifdef DUMP_DECODE
  bgav_dprintf("Got %d samples\n", priv->frame->valid_samples);
#endif
  
  gavl_audio_frame_copy_ptrs(&s->data.audio.format,
                             s->data.audio.frame, priv->frame);
  
  return 1;
  }
#else

/*
 *  New version
 */

static int decode_frame_ffmpeg(bgav_stream_t * s)
  {
  int bytes_used;

  ffmpeg_audio_priv * priv;
  AVFrame f;
  int got_frame;
  
  priv= s->data.audio.decoder->priv;

  if(!fill_buffer(s))
    return 0;
 
#ifdef DUMP_DECODE
  bgav_dprintf("decode_audio Size: %d\n",
               priv->buf.size);
  //  bgav_hexdump(priv->buf.buffer, 186, 16);
#endif

  priv->pkt.data = priv->buf.buffer;
  priv->pkt.size = priv->buf.size;

  bytes_used = avcodec_decode_audio4(priv->ctx, &f,
                                     &got_frame, &priv->pkt);
  
  if(got_frame && f.nb_samples)
    {
    /* Allocate or enlarge frame */

    if(s->data.audio.format.samples_per_frame < f.nb_samples)
      {
      if(!priv->frame)
        {
        if(!init_format(s))
          {
          fprintf(stderr, "Init format failed\n");
          return 0;
          }
        }
      else
        gavl_audio_frame_destroy(priv->frame);

      s->data.audio.format.samples_per_frame = f.nb_samples;
      priv->frame = gavl_audio_frame_create(&s->data.audio.format);
      }
    /* This will break with planar formats */
    memcpy(priv->frame->samples.s_16,
           f.extended_data[0],
           f.nb_samples * priv->sample_size *
           s->data.audio.format.num_channels);
    
    priv->frame->valid_samples = f.nb_samples;
    }
  
#ifdef DUMP_DECODE
  bgav_dprintf("Used %d bytes\n", bytes_used);
#endif
  
  if(bytes_used < 0)
    {
    /* Error */
    return 0;
    }

  /* Advance packet buffer */

  if(bytes_used > 0)
    bgav_bytebuffer_remove(&priv->buf, bytes_used);
  else
    {
    //    priv->bytes_in_packet_buffer = 0;
    }
  
  /* No Samples decoded, get next packet */

  if(!got_frame)
    return 1;
  
#ifdef DUMP_DECODE
  bgav_dprintf("Got %d samples\n", priv->frame->valid_samples);
#endif
  
  gavl_audio_frame_copy_ptrs(&s->data.audio.format,
                             s->data.audio.frame, priv->frame);
  
  return 1;
  }


#endif



static int init_ffmpeg_audio(bgav_stream_t * s)
  {
  AVCodec * codec;
  ffmpeg_audio_priv * priv;
  priv = calloc(1, sizeof(*priv));
  priv->info = lookup_codec(s);
  codec = avcodec_find_decoder(priv->info->ffmpeg_id);

#if LIBAVCODEC_VERSION_INT < ((53<<16)|(8<<8)|0)
  priv->ctx = avcodec_alloc_context();
#else
  priv->ctx = avcodec_alloc_context3(NULL);
#endif
  
  //  priv->ctx->width = s->format.frame_width;
  //  priv->ctx->height = s->format.frame_height;

  if(s->ext_size)
    {
    priv->ext_data = calloc(1, s->ext_size +
                            FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(priv->ext_data, s->ext_data, s->ext_size);
    
    priv->ctx->extradata = priv->ext_data;
    priv->ctx->extradata_size = s->ext_size;
    }
  
#ifdef DUMP_EXTRADATA
  bgav_dprintf("Adding extradata %d bytes\n", priv->ctx->extradata_size);
  bgav_hexdump(priv->ctx->extradata, priv->ctx->extradata_size, 16);
#endif    
  priv->ctx->channels        = s->data.audio.format.num_channels;
  priv->ctx->sample_rate     = s->data.audio.format.samplerate;
  priv->ctx->block_align     = s->data.audio.block_align;
  priv->ctx->bit_rate        = s->codec_bitrate;
  priv->ctx->bits_per_coded_sample = s->data.audio.bits_per_sample;
  if(priv->info->codec_tag != -1)
    priv->ctx->codec_tag = priv->info->codec_tag;
  else
    priv->ctx->codec_tag = bswap_32(s->fourcc);

  priv->ctx->codec_type  = codec->type;
  priv->ctx->codec_id    = codec->id;
  
  /* Some codecs need extra stuff */
    
  /* Open codec */

  bgav_ffmpeg_lock();
  
#if LIBAVCODEC_VERSION_INT < ((53<<16)|(8<<8)|0)
  if(avcodec_open(priv->ctx, codec) != 0)
    {
    bgav_ffmpeg_unlock();
    return 0;
    }
#else
  if(avcodec_open2(priv->ctx, codec, NULL) != 0)
    {
    bgav_ffmpeg_unlock();
    return 0;
    }
#endif
  
  bgav_ffmpeg_unlock();
  
  s->data.audio.decoder->priv = priv;

  /* Set missing format values */
  
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.preroll = priv->info->preroll;

  //  /* Check if we know the format already */
  //  if(s->data.audio.format.num_channels &&
  //     s->data.audio.format.samplerate &&
  //     (priv->ctx->sample_fmt != SAMPLE_FMT_NONE))
  //    {
  //    if(!init_format(s))
  //      return 0;
  //    }
  //  else /* Let ffmpeg find out the format */
  //    {
#ifndef DECODE_AUDIO4
    priv->frame_alloc = AVCODEC_MAX_AUDIO_FRAME_SIZE;
#endif
    if(!decode_frame_ffmpeg(s))
      return 0;
  //    }
    
  s->description = bgav_sprintf("%s", priv->info->format_name);
  return 1;
  }

static void resync_ffmpeg(bgav_stream_t * s)
  {
  ffmpeg_audio_priv * priv;
  priv = s->data.audio.decoder->priv;
  avcodec_flush_buffers(priv->ctx);
  priv->frame->valid_samples = 0;
  bgav_bytebuffer_flush(&priv->buf);
  }

static void close_ffmpeg(bgav_stream_t * s)
  {
  ffmpeg_audio_priv * priv;
  priv= s->data.audio.decoder->priv;

  if(!priv)
    return;
  
  if(priv->ext_data)
    free(priv->ext_data);
  
  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);

  bgav_bytebuffer_free(&priv->buf);
  
  if(priv->ctx)
    {
    bgav_ffmpeg_lock();
    avcodec_close(priv->ctx);
    bgav_ffmpeg_unlock();
    free(priv->ctx);
    }
  free(priv);
  }



static codec_info_t codec_infos[] =
  {
    /*     CODEC_ID_PCM_S16LE= 0x10000, */
    /*     CODEC_ID_PCM_S16BE, */
    /*     CODEC_ID_PCM_U16LE, */
    /*     CODEC_ID_PCM_U16BE, */
    /*     CODEC_ID_PCM_S8, */
    /*     CODEC_ID_PCM_U8, */
    /*     CODEC_ID_PCM_MULAW, */
    /*     CODEC_ID_PCM_ALAW, */
    /*     CODEC_ID_PCM_S32LE, */
    /*     CODEC_ID_PCM_S32BE, */
    /*     CODEC_ID_PCM_U32LE, */
    /*     CODEC_ID_PCM_U32BE, */
    /*     CODEC_ID_PCM_S24LE, */
    /*     CODEC_ID_PCM_S24BE, */
    /*     CODEC_ID_PCM_U24LE, */
    /*     CODEC_ID_PCM_U24BE, */
    /*     CODEC_ID_PCM_S24DAUD, */
    { "FFmpeg D-Cinema decoder", "D-Cinema", CODEC_ID_PCM_S24DAUD,
      (uint32_t[]){ BGAV_MK_FOURCC('d','a','u','d'),
               0x00 },
      -1 },
    /*     CODEC_ID_PCM_ZORK, */

    /*     CODEC_ID_ADPCM_IMA_QT= 0x11000, */
    { "FFmpeg ima4 decoder", "ima4", CODEC_ID_ADPCM_IMA_QT,
      (uint32_t[]){ BGAV_MK_FOURCC('i', 'm', 'a', '4'), 0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_IMA_WAV, */

    { "FFmpeg WAV ADPCM decoder", "WAV IMA ADPCM", CODEC_ID_ADPCM_IMA_WAV,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x11),
                    BGAV_MK_FOURCC('m', 's', 0x00, 0x11), 0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_IMA_DK3, */
    { "FFmpeg IMA DK3 decoder", "IMA DK3", CODEC_ID_ADPCM_IMA_DK3,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x62), 0x00 },
      -1 },  /* rogue format number */
    /*     CODEC_ID_ADPCM_IMA_DK4, */
    { "FFmpeg IMA DK4 decoder", "IMA DK4", CODEC_ID_ADPCM_IMA_DK4,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x61), 0x00 },
      -1 },  /* rogue format number */
    /*     CODEC_ID_ADPCM_IMA_WS, */
    { "FFmpeg Westwood ADPCM decoder", "Westwood ADPCM", CODEC_ID_ADPCM_IMA_WS,
      (uint32_t[]){ BGAV_MK_FOURCC('w','s','p','c'), 0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_IMA_SMJPEG, */
    { "FFmpeg SMJPEG audio decoder", "SMJPEG audio", CODEC_ID_ADPCM_IMA_SMJPEG,
      (uint32_t[]){ BGAV_MK_FOURCC('S','M','J','A'), 0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_MS, */
    { "FFmpeg MS ADPCM decoder", "MS ADPCM", CODEC_ID_ADPCM_MS,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x02),
                    BGAV_MK_FOURCC('m', 's', 0x00, 0x02), 0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_4XM, */
    { "FFmpeg 4xm audio decoder", "4XM ADPCM", CODEC_ID_ADPCM_4XM,
      (uint32_t[]){ BGAV_MK_FOURCC('4', 'X', 'M', 'A'), 0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_XA, */
    { "FFmpeg Playstation ADPCM decoder", "Playstation ADPCM", CODEC_ID_ADPCM_XA,
      (uint32_t[]){ BGAV_MK_FOURCC('A','D','X','A'),
               0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_ADX, */
    
    /*     CODEC_ID_ADPCM_EA, */
    { "FFmpeg Electronicarts ADPCM decoder", "Electronicarts ADPCM",
      CODEC_ID_ADPCM_EA,
      (uint32_t[]){ BGAV_MK_FOURCC('w','v','e','a'),
                    0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_G726, */
    { "FFmpeg G726 decoder", "G726 ADPCM", CODEC_ID_ADPCM_G726,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0045),
                    0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_CT, */
    { "FFmpeg Creative ADPCM decoder", "Creative ADPCM", CODEC_ID_ADPCM_CT,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x200),
                    0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_SWF, */
#if 1 // Sounds disgusting (with ffplay as well). zelda.flv
    { "FFmpeg Flash ADPCM decoder", "Flash ADPCM", CODEC_ID_ADPCM_SWF,
      (uint32_t[]){ BGAV_MK_FOURCC('F', 'L', 'A', '1'), 0x00 },
      -1 },
#endif
    /*     CODEC_ID_ADPCM_YAMAHA, */
    { "FFmpeg SMAF audio decoder", "SMAF", CODEC_ID_ADPCM_YAMAHA,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'M', 'A', 'F'),
               0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_SBPRO_4, */
    { "FFmpeg Soundblaster Pro ADPCM 4 decoder", "Soundblaster Pro ADPCM 4",
      CODEC_ID_ADPCM_SBPRO_4,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'B', 'P', '4'), 0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_SBPRO_3, */
    { "FFmpeg Soundblaster Pro ADPCM 3 decoder", "Soundblaster Pro ADPCM 3",
      CODEC_ID_ADPCM_SBPRO_3,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'B', 'P', '3'), 0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_SBPRO_2, */
    { "FFmpeg Soundblaster Pro ADPCM 2 decoder", "Soundblaster Pro ADPCM 2",
      CODEC_ID_ADPCM_SBPRO_2,
      (uint32_t[]){ BGAV_MK_FOURCC('S', 'B', 'P', '2'), 0x00 },
      -1 },
    /*     CODEC_ID_ADPCM_THP, */
    { "FFmpeg THP Audio decoder", "THP Audio", CODEC_ID_ADPCM_THP,
      (uint32_t[]){ BGAV_MK_FOURCC('T', 'H', 'P', 'A'),
               0x00 } },
    /*     CODEC_ID_ADPCM_IMA_AMV, */
    /*     CODEC_ID_ADPCM_EA_R1, */
    /*     CODEC_ID_ADPCM_EA_R3, */
    /*     CODEC_ID_ADPCM_EA_R2, */
    /*     CODEC_ID_ADPCM_IMA_EA_SEAD, */
    /*     CODEC_ID_ADPCM_IMA_EA_EACS, */
    /*     CODEC_ID_ADPCM_EA_XAS, */
    /*     CODEC_ID_AMR_NB= 0x12000, */
    /*     CODEC_ID_AMR_WB, */
    /*     CODEC_ID_RA_144= 0x13000, */
    { "FFmpeg ra14.4 decoder", "Real audio 14.4", CODEC_ID_RA_144,
      (uint32_t[]){ BGAV_MK_FOURCC('1', '4', '_', '4'),
               BGAV_MK_FOURCC('l', 'p', 'c', 'J'),
                    0x00 },
      -1 },
    /*     CODEC_ID_RA_288, */
    { "FFmpeg ra28.8 decoder", "Real audio 28.8", CODEC_ID_RA_288,
      (uint32_t[]){ BGAV_MK_FOURCC('2', '8', '_', '8'), 0x00 },
      -1 },

    /*     CODEC_ID_ROQ_DPCM= 0x14000, */
    { "FFmpeg ID Roq Audio decoder", "ID Roq Audio", CODEC_ID_ROQ_DPCM,
      (uint32_t[]){ BGAV_MK_FOURCC('R','O','Q','A'),
               0x00 },
      -1 },
    /*     CODEC_ID_INTERPLAY_DPCM, */
    { "FFmpeg Interplay DPCM decoder", "Interplay DPCM", CODEC_ID_INTERPLAY_DPCM,
      (uint32_t[]){ BGAV_MK_FOURCC('I','P','D','C'),
               0x00 },
      1 },
    
    /*     CODEC_ID_XAN_DPCM, */
    /*     CODEC_ID_SOL_DPCM, */
    { "FFmpeg Old SOL decoder", "SOL (old)", CODEC_ID_SOL_DPCM,
      (uint32_t[]){ BGAV_MK_FOURCC('S','O','L','1'),
               0x00 },
      1 },

    { "FFmpeg SOL decoder (8 bit)", "SOL 8 bit", CODEC_ID_SOL_DPCM,
      (uint32_t[]){ BGAV_MK_FOURCC('S','O','L','2'),
               0x00 },
      2 },

    { "FFmpeg SOL decoder (16 bit)", "SOL 16 bit", CODEC_ID_SOL_DPCM,
      (uint32_t[]){ BGAV_MK_FOURCC('S','O','L','3'),
                    0x00 },
      3 },

    /*     CODEC_ID_MP2= 0x15000, */
#if 0
    { "FFmpeg mp2 decoder", "MPEG audio Layer 1/2/3", CODEC_ID_MP2,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x50), 0x00 },
      -1 },
#endif
    /*     CODEC_ID_MP3, /\* preferred ID for decoding MPEG audio layer 1, 2 or 3 *\/ */
#if 0    
    { "FFmpeg mp3 decoder", "MPEG audio Layer 1/2/3", CODEC_ID_MP3,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x55),
               BGAV_MK_FOURCC('.', 'm', 'p', '3'),
               BGAV_MK_FOURCC('m', 's', 0x00, 0x55),
               0x00 },
      -1 },
    
#endif
    /*     CODEC_ID_AAC, */
    /*     CODEC_ID_AC3, */
#if 0    
    { "FFmpeg ac3 decoder", "AC3", CODEC_ID_AC3,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x2000),
                    BGAV_MK_FOURCC('.', 'a', 'c', '3'),
                    0x00 },
      -1 },
#endif
    /*     CODEC_ID_DTS, */
    /*     CODEC_ID_VORBIS, */
    /*     CODEC_ID_DVAUDIO, */
    /*     CODEC_ID_WMAV1, */
    { "FFmpeg WMA1 decoder", "Window Media Audio 1", CODEC_ID_WMAV1,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x160), 0x00 },
      -1 },
    /*     CODEC_ID_WMAV2, */
    { "FFmpeg WMA2 decoder", "Window Media Audio 2", CODEC_ID_WMAV2,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x161), 0x00 },
      -1 },
    /*     CODEC_ID_MACE3, */
    { "FFmpeg MACE3 decoder", "Apple MACE 3", CODEC_ID_MACE3,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'A', 'C', '3'), 0x00 },
      -1 },
    /*     CODEC_ID_MACE6, */
    { "FFmpeg MACE6 decoder", "Apple MACE 6", CODEC_ID_MACE6,
      (uint32_t[]){ BGAV_MK_FOURCC('M', 'A', 'C', '6'), 0x00 },
      -1 },
    /*     CODEC_ID_VMDAUDIO, */
    { "FFmpeg Sierra VMD audio decoder", "Sierra VMD audio",
      CODEC_ID_VMDAUDIO,
      (uint32_t[]){ BGAV_MK_FOURCC('V', 'M', 'D', 'A'),
                    0x00 } },
    /*     CODEC_ID_SONIC, */
    { "FFmpeg Sonic decoder", "Sonic", CODEC_ID_SONIC,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x2048), 0x00 },
      -1 },
    
    /*     CODEC_ID_SONIC_LS, */
    /*     CODEC_ID_FLAC, */
    /*     CODEC_ID_MP3ADU, */
#if 1 // Sounds disgusting
    { "FFmpeg mp3 ADU decoder", "MP3 ADU", CODEC_ID_MP3ADU,
      (uint32_t[]){ BGAV_MK_FOURCC('r', 'm', 'p', '3'),
                    0x00 },
      -1 },
#endif
    /*     CODEC_ID_MP3ON4, */
    { "FFmpeg mp3on4 decoder", "MP3on4", CODEC_ID_MP3ON4,
      (uint32_t[]){ BGAV_MK_FOURCC('m', '4', 'a', 29),
                    0x00 },
      -1, 1152*32 },
    /*     CODEC_ID_SHORTEN, */
    { "FFmpeg Shorten decoder", "Shorten", CODEC_ID_SHORTEN,
      (uint32_t[]){ BGAV_MK_FOURCC('.','s','h','n'),
               0x00 },
      -1 },
    /*     CODEC_ID_ALAC, */
    { "FFmpeg alac decoder", "alac", CODEC_ID_ALAC,
      (uint32_t[]){ BGAV_MK_FOURCC('a', 'l', 'a', 'c'),
                    0x00 },
      -1 },
    /*     CODEC_ID_WESTWOOD_SND1, */
    { "FFmpeg Westwood SND1 decoder", "Westwood SND1", CODEC_ID_WESTWOOD_SND1,
      (uint32_t[]){ BGAV_MK_FOURCC('w','s','p','1'), 0x00 },
      -1 },
    /*     CODEC_ID_GSM, /\* as in Berlin toast format *\/ */
    /*     CODEC_ID_QDM2, */
#ifndef HAVE_W32DLL
    { "FFmpeg QDM2 decoder", "QDM2", CODEC_ID_QDM2,
      (uint32_t[]){ BGAV_MK_FOURCC('Q', 'D', 'M', '2'),
                    0x00 },
      -1 },
#endif
    /*     CODEC_ID_COOK, */
    { "FFmpeg Real cook decoder", "Real cook", CODEC_ID_COOK,
      (uint32_t[]){ BGAV_MK_FOURCC('c', 'o', 'o', 'k'),
                    0x00 },
      -1 },
    /*     CODEC_ID_TRUESPEECH, */
    { "FFmpeg Truespeech audio decoder", "Truespeech", CODEC_ID_TRUESPEECH,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0022),
               0x00 },
      -1 },
    /*     CODEC_ID_TTA, */
    { "FFmpeg True audio decoder", "True audio", CODEC_ID_TTA,
      (uint32_t[]){ BGAV_MK_FOURCC('T', 'T', 'A', '1'),
                    0x00 },
      -1 },
    /*     CODEC_ID_SMACKAUDIO, */
    { "FFmpeg Smacker Audio decoder", "Smacker Audio", CODEC_ID_SMACKAUDIO,
      (uint32_t[]){ BGAV_MK_FOURCC('S','M','K','A'),
               0x00 },
      -1 },
    /*     CODEC_ID_QCELP, */
    /*     CODEC_ID_WAVPACK, */
    { "FFmpeg Wavpack decoder", "Wavpack", CODEC_ID_WAVPACK,
      (uint32_t[]){ BGAV_MK_FOURCC('w', 'v', 'p', 'k'),
                    0x00 },
      -1 },
    /*     CODEC_ID_DSICINAUDIO, */
    { "FFmpeg Delphine CIN audio decoder", "Delphine CIN Audio",
      CODEC_ID_DSICINAUDIO,
      (uint32_t[]){ BGAV_MK_FOURCC('d', 'c', 'i', 'n'),
               0x00 },
      -1 },
    /*     CODEC_ID_IMC, */
    { "FFmpeg Intel Music decoder", "Intel Music coder", CODEC_ID_IMC,
      (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0401),
               0x00 },
      -1 },
    /*     CODEC_ID_MUSEPACK7, */
    /*     CODEC_ID_MLP, */
    /*     CODEC_ID_GSM_MS, /\* as found in WAV *\/ */
    /*     CODEC_ID_ATRAC3, */
    { "FFmpeg ATRAC3 decoder", "ATRAC3", CODEC_ID_ATRAC3,
      (uint32_t[]){ BGAV_MK_FOURCC('a', 't', 'r', 'c'),
                    BGAV_WAVID_2_FOURCC(0x0270),
                    0x00  } },
    /*     CODEC_ID_VOXWARE, */
    /*     CODEC_ID_APE, */
    { "FFmpeg Monkey's Audio decoder", "Monkey's Audio", CODEC_ID_APE,
      (uint32_t[]){ BGAV_MK_FOURCC('.', 'a', 'p', 'e'),
                    0x00  } },
    /*     CODEC_ID_NELLYMOSER, */
    { "FFmpeg Nellymoser decoder", "Nellymoser", CODEC_ID_NELLYMOSER,
      (uint32_t[]){ BGAV_MK_FOURCC('N', 'E', 'L', 'L'),
                    0x00 },
      -1 },
    
#if LIBAVCODEC_BUILD >= ((52<<16)+(55<<8)+0)
    { "FFmpeg AMR NB decoder", "AMR Narrowband", CODEC_ID_AMR_NB,
      (uint32_t[]){ BGAV_MK_FOURCC('s', 'a', 'm', 'r'),
                    0x00 },
      -1 },
#endif

    
    /*     CODEC_ID_MUSEPACK8, */
  };


#define NUM_CODECS sizeof(codec_infos)/sizeof(codec_infos[0])

static int real_num_codecs;

static struct
  {
  codec_info_t * info;
  bgav_audio_decoder_t decoder;
  } codecs[NUM_CODECS];


static codec_info_t * lookup_codec(bgav_stream_t * s)
  {
  int i;
  
  for(i = 0; i < real_num_codecs; i++)
    {
    if(s->data.audio.decoder->decoder == &codecs[i].decoder)
      return codecs[i].info;
    }
  return NULL;
  }

void
bgav_init_audio_decoders_ffmpeg(bgav_options_t * opt)
  {
  int i;
  real_num_codecs = 0;
#if LIBAVCODEC_VERSION_MAJOR < 54
  avcodec_init();
#endif
  avcodec_register_all();

  for(i = 0; i < NUM_CODECS; i++)
    {
    if(avcodec_find_decoder(codec_infos[i].ffmpeg_id))
      {
      codecs[real_num_codecs].info = &codec_infos[i];
      codecs[real_num_codecs].decoder.name =
        codecs[real_num_codecs].info->decoder_name;
      codecs[real_num_codecs].decoder.fourccs =
        codecs[real_num_codecs].info->fourccs;
      codecs[real_num_codecs].decoder.init = init_ffmpeg_audio;
      codecs[real_num_codecs].decoder.decode_frame = decode_frame_ffmpeg;
      codecs[real_num_codecs].decoder.close = close_ffmpeg;
      codecs[real_num_codecs].decoder.resync = resync_ffmpeg;
      bgav_audio_decoder_register(&codecs[real_num_codecs].decoder);
      
      real_num_codecs++;
      }
    else
      bgav_log(opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Codec not found: %s",
               codec_infos[i].decoder_name);
    }
  }

