/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <avdec_private.h>
#include AVFORMAT_HEADER

#define LOG_DOMAIN "demux_ffmpeg"

#define PROBE_SIZE 2048 /* Same as in MPlayer */

static void cleanup_stream_ffmpeg(bgav_stream_t * s)
  {
  if(s->type == BGAV_STREAM_VIDEO)
    {
    if(s->priv)
      free(s->priv);
    }
  }

typedef struct
  {
  AVInputFormat *avif;
  AVFormatContext *avfc;
#if LIBAVFORMAT_VERSION_INT < ((52<<16)+(0<<8)+0)
  ByteIOContext pb;
#else
  ByteIOContext * pb;
#endif
  } ffmpeg_priv_t;

/* Callbacks for URLProtocol */

static int lavf_open(URLContext *h, const char *filename, int flags)
  {
  return 0;
  }

static int lavf_read(URLContext *h, unsigned char *buf, int size)
  {
  bgav_input_context_t * input;
  int result;
  input = (bgav_input_context_t *)h->priv_data;

  result = bgav_input_read_data(input, buf, size);
  if(!result)
    return -1;
  return result;
  }

static int lavf_write(URLContext *h, unsigned char *buf, int size)
  {
  return -1;
  }

static offset_t lavf_seek(URLContext *h, offset_t pos, int whence)
  {
  bgav_input_context_t * input;
  input = (bgav_input_context_t *)h->priv_data;

  if(!input->input->seek_byte)
    return -1;
  if(pos > input->total_bytes)
    return -1;
#if LIBAVFORMAT_BUILD >= ((51<<16)+(8<<8)+0)
  if(whence == AVSEEK_SIZE)
    return input->total_bytes;
#endif
  bgav_input_seek(input, pos, whence);
  return input->position;
  }

static int lavf_close(URLContext *h)
  {
  return 0;
  }

static URLProtocol bgav_protocol = {
    "bgav",
    lavf_open,
    lavf_read,
    lavf_write,
    lavf_seek,
    lavf_close,
};

/* Demuxer functions */

static AVInputFormat * get_format(bgav_input_context_t * input)
  {
  uint8_t data[PROBE_SIZE];
  AVProbeData avpd;

  if(!input->filename)
    return 0;
  
  if(bgav_input_get_data(input, data, PROBE_SIZE) < PROBE_SIZE)
    return 0;
  
  avpd.filename= input->filename;
  avpd.buf= data;
  avpd.buf_size= PROBE_SIZE;
  return av_probe_input_format(&avpd, 1);
  }

static int probe_ffmpeg(bgav_input_context_t * input)
  {
  AVInputFormat * format;
  /* This sucks */
  av_register_all();
  format= get_format(input);
  
  
  if(format)
    {
    bgav_log(input->opt, BGAV_LOG_INFO, LOG_DOMAIN,
             "Detected %s format", format->long_name);
    return 1;
    }
  return 0;
  }

/* Maps from Codec-IDs to fourccs. These must match
   [audio|video]_ffmpeg.c */

typedef struct
  {
  enum CodecID id;
  uint32_t fourcc;
  int bits; /* For audio codecs */
  uint32_t codec_tag;
  } audio_codec_map_t;

audio_codec_map_t audio_codecs[] =
  {
    /* various PCM "codecs" */
    { CODEC_ID_PCM_S16LE, BGAV_WAVID_2_FOURCC(0x0001), 16 },
    { CODEC_ID_PCM_S16BE, BGAV_MK_FOURCC('t','w','o','s'), 16 },
    //    { CODEC_ID_PCM_U16LE, },
    //    { CODEC_ID_PCM_U16BE, },
    { CODEC_ID_PCM_S8, BGAV_MK_FOURCC('t','w','o','s'), 8},
    { CODEC_ID_PCM_U8, BGAV_WAVID_2_FOURCC(0x0001), 8},
    { CODEC_ID_PCM_MULAW, BGAV_MK_FOURCC('u', 'l', 'a', 'w')},
    { CODEC_ID_PCM_ALAW, BGAV_MK_FOURCC('a', 'l', 'a', 'w')},
    { CODEC_ID_PCM_S32LE, BGAV_WAVID_2_FOURCC(0x0001), 32 },
    { CODEC_ID_PCM_S32BE, BGAV_MK_FOURCC('t','w','o','s'), 32},
    //    { CODEC_ID_PCM_U32LE, },
    //    { CODEC_ID_PCM_U32BE, },
    { CODEC_ID_PCM_S24LE, BGAV_WAVID_2_FOURCC(0x0001), 24 },
    { CODEC_ID_PCM_S24BE, BGAV_MK_FOURCC('t','w','o','s'), 24},
    //    { CODEC_ID_PCM_U24LE, },
    //    { CODEC_ID_PCM_U24BE, },
    { CODEC_ID_PCM_S24DAUD, BGAV_MK_FOURCC('d','a','u','d') },
    // { CODEC_ID_PCM_ZORK, },

    /* various ADPCM codecs */
    { CODEC_ID_ADPCM_IMA_QT, BGAV_MK_FOURCC('i', 'm', 'a', '4')},
    { CODEC_ID_ADPCM_IMA_WAV, BGAV_WAVID_2_FOURCC(0x11) },
    { CODEC_ID_ADPCM_IMA_DK3, BGAV_WAVID_2_FOURCC(0x62) },
    { CODEC_ID_ADPCM_IMA_DK4, BGAV_WAVID_2_FOURCC(0x61) },
    { CODEC_ID_ADPCM_IMA_WS, BGAV_MK_FOURCC('w','s','p','c') },
    { CODEC_ID_ADPCM_IMA_SMJPEG, BGAV_MK_FOURCC('S','M','J','A') },
    { CODEC_ID_ADPCM_MS, BGAV_WAVID_2_FOURCC(0x02) },
    { CODEC_ID_ADPCM_4XM, BGAV_MK_FOURCC('4', 'X', 'M', 'A') },
    { CODEC_ID_ADPCM_XA, BGAV_MK_FOURCC('A','D','X','A') },
    //    { CODEC_ID_ADPCM_ADX, },
    { CODEC_ID_ADPCM_EA, BGAV_MK_FOURCC('w','v','e','a') },
    { CODEC_ID_ADPCM_G726, BGAV_WAVID_2_FOURCC(0x0045) },
    { CODEC_ID_ADPCM_CT, BGAV_WAVID_2_FOURCC(0x200)},
    { CODEC_ID_ADPCM_SWF, BGAV_MK_FOURCC('F', 'L', 'A', '1') },
    { CODEC_ID_ADPCM_YAMAHA, BGAV_MK_FOURCC('S', 'M', 'A', 'F') },
    { CODEC_ID_ADPCM_SBPRO_4, BGAV_MK_FOURCC('S', 'B', 'P', '4') },
    { CODEC_ID_ADPCM_SBPRO_3, BGAV_MK_FOURCC('S', 'B', 'P', '3') },
    { CODEC_ID_ADPCM_SBPRO_2, BGAV_MK_FOURCC('S', 'B', 'P', '2') },
#if LIBAVCODEC_BUILD >= ((51<<16)+(40<<8)+4)
    { CODEC_ID_ADPCM_THP, BGAV_MK_FOURCC('T', 'H', 'P', 'A') },
#endif
    //    { CODEC_ID_ADPCM_IMA_AMV, },
    //    { CODEC_ID_ADPCM_EA_R1, },
    //    { CODEC_ID_ADPCM_EA_R3, },
    //    { CODEC_ID_ADPCM_EA_R2, },
    //    { CODEC_ID_ADPCM_IMA_EA_SEAD, },
    //    { CODEC_ID_ADPCM_IMA_EA_EACS, },
    //    { CODEC_ID_ADPCM_EA_XAS, },

        /* AMR */
    //    { CODEC_ID_AMR_NB, },
    //    { CODEC_ID_AMR_WB, },

    /* RealAudio codecs*/
    { CODEC_ID_RA_144, BGAV_MK_FOURCC('1', '4', '_', '4') },
    { CODEC_ID_RA_288, BGAV_MK_FOURCC('2', '8', '_', '8') },

    /* various DPCM codecs */
    { CODEC_ID_ROQ_DPCM, BGAV_MK_FOURCC('R','O','Q','A') },
    { CODEC_ID_INTERPLAY_DPCM, BGAV_MK_FOURCC('I','P','D','C') },
    
    //    { CODEC_ID_XAN_DPCM, },
    { CODEC_ID_SOL_DPCM, BGAV_MK_FOURCC('S','O','L','1'), 0, 1 },
    { CODEC_ID_SOL_DPCM, BGAV_MK_FOURCC('S','O','L','2'), 0, 2 },
    { CODEC_ID_SOL_DPCM, BGAV_MK_FOURCC('S','O','L','3'), 0, 3 },
    
    { CODEC_ID_MP2, BGAV_MK_FOURCC('.','m','p','2') },
    { CODEC_ID_MP3, BGAV_MK_FOURCC('.','m','p','3') }, /* preferred ID for decoding MPEG audio layer 1, }, 2 or 3 */
    { CODEC_ID_AAC, BGAV_MK_FOURCC('a','a','c',' ') },
    { CODEC_ID_AC3, BGAV_MK_FOURCC('.', 'a', 'c', '3') },
    { CODEC_ID_DTS, BGAV_MK_FOURCC('d', 't', 's', ' ') },
    //    { CODEC_ID_VORBIS, },
    //    { CODEC_ID_DVAUDIO, },
    { CODEC_ID_WMAV1, BGAV_WAVID_2_FOURCC(0x160) },
    { CODEC_ID_WMAV2, BGAV_WAVID_2_FOURCC(0x161) },
    { CODEC_ID_MACE3, BGAV_MK_FOURCC('M', 'A', 'C', '3') },
    { CODEC_ID_MACE6, BGAV_MK_FOURCC('M', 'A', 'C', '6') },
    { CODEC_ID_VMDAUDIO, BGAV_MK_FOURCC('V', 'M', 'D', 'A')},
    { CODEC_ID_SONIC, BGAV_WAVID_2_FOURCC(0x2048) },
    //    { CODEC_ID_SONIC_LS, },
    //    { CODEC_ID_FLAC, },
    { CODEC_ID_MP3ADU, BGAV_MK_FOURCC('r', 'm', 'p', '3') },
    { CODEC_ID_MP3ON4, BGAV_MK_FOURCC('m', '4', 'a', 29) },
    { CODEC_ID_SHORTEN, BGAV_MK_FOURCC('.','s','h','n')},
    { CODEC_ID_ALAC, BGAV_MK_FOURCC('a', 'l', 'a', 'c') },
    { CODEC_ID_WESTWOOD_SND1, BGAV_MK_FOURCC('w','s','p','1') },
    { CODEC_ID_GSM, BGAV_MK_FOURCC('a', 'g', 's', 'm') }, /* as in Berlin toast format */
    { CODEC_ID_QDM2, BGAV_MK_FOURCC('Q', 'D', 'M', '2') },
    { CODEC_ID_COOK, BGAV_MK_FOURCC('c', 'o', 'o', 'k') },
    { CODEC_ID_TRUESPEECH, BGAV_WAVID_2_FOURCC(0x0022) },
    { CODEC_ID_TTA, BGAV_MK_FOURCC('T', 'T', 'A', '1')  },
    { CODEC_ID_SMACKAUDIO, BGAV_MK_FOURCC('S','M','K','A') },
    //    { CODEC_ID_QCELP, },
#if LIBAVCODEC_BUILD >= ((51<<16)+(16<<8)+0)
    { CODEC_ID_WAVPACK, BGAV_MK_FOURCC('w', 'v', 'p', 'k') },
#endif

#if LIBAVCODEC_BUILD >= ((51<<16)+(18<<8)+0)
    { CODEC_ID_DSICINAUDIO, BGAV_MK_FOURCC('d', 'c', 'i', 'n') },
#endif

#if LIBAVCODEC_BUILD >= ((51<<16)+(23<<8)+0)
    { CODEC_ID_IMC, BGAV_WAVID_2_FOURCC(0x0401) },
#endif
    //    { CODEC_ID_MUSEPACK7, },
    //    { CODEC_ID_MLP, },
#if LIBAVCODEC_BUILD >= ((51<<16)+(34<<8)+0)
    { CODEC_ID_GSM_MS, BGAV_WAVID_2_FOURCC(0x31) }, /* as found in WAV */
#endif

#if LIBAVCODEC_BUILD >= ((51<<16)+(40<<8)+4)
    { CODEC_ID_ATRAC3, BGAV_MK_FOURCC('a', 't', 'r', 'c') },
#endif
    //    { CODEC_ID_VOXWARE, },
#if LIBAVCODEC_BUILD >= ((51<<16)+(44<<8)+0)
    { CODEC_ID_APE, BGAV_MK_FOURCC('.', 'a', 'p', 'e')},
#endif
#if LIBAVCODEC_BUILD >= ((51<<16)+(46<<8)+0)
    { CODEC_ID_NELLYMOSER, BGAV_MK_FOURCC('N', 'E', 'L', 'L')},
#endif
    //    { CODEC_ID_MUSEPACK8, },
    
    { /* End */ }
  };

typedef struct
  {
  enum CodecID id;
  uint32_t fourcc;
  } video_codec_map_t;

video_codec_map_t video_codecs[] =
  {

    { CODEC_ID_MPEG1VIDEO, BGAV_MK_FOURCC('m','p','g','v') },
    { CODEC_ID_MPEG2VIDEO, BGAV_MK_FOURCC('m','p','g','v') }, /* preferred ID for MPEG-1/2 video decoding */
    //    { CODEC_ID_MPEG2VIDEO_XVMC, },
    { CODEC_ID_H261, BGAV_MK_FOURCC('h', '2', '6', '1') },
    { CODEC_ID_H263, BGAV_MK_FOURCC('h', '2', '6', '3') },
    //    { CODEC_ID_RV10, },
    //    { CODEC_ID_RV20, },
    { CODEC_ID_MJPEG, BGAV_MK_FOURCC('j', 'p', 'e', 'g') },
    { CODEC_ID_MJPEGB, BGAV_MK_FOURCC('m', 'j', 'p', 'b')},
    { CODEC_ID_LJPEG, BGAV_MK_FOURCC('L', 'J', 'P', 'G') },
    //    { CODEC_ID_SP5X, },
    { CODEC_ID_JPEGLS, BGAV_MK_FOURCC('M', 'J', 'L', 'S') },
    { CODEC_ID_MPEG4, BGAV_MK_FOURCC('m', 'p', '4', 'v') },
    //    { CODEC_ID_RAWVIDEO, },
    { CODEC_ID_MSMPEG4V1, BGAV_MK_FOURCC('M', 'P', 'G', '4') },
    { CODEC_ID_MSMPEG4V2, BGAV_MK_FOURCC('D', 'I', 'V', '2') },
    { CODEC_ID_MSMPEG4V3, BGAV_MK_FOURCC('D', 'I', 'V', '3')},
    { CODEC_ID_WMV1, MKTAG('W', 'M', 'V', '1') },
    { CODEC_ID_WMV2, MKTAG('W', 'M', 'V', '2') },
    { CODEC_ID_H263P, MKTAG('H', '2', '6', '3') },
    { CODEC_ID_H263I, MKTAG('I', '2', '6', '3') },
    { CODEC_ID_FLV1, BGAV_MK_FOURCC('F', 'L', 'V', '1') },
    { CODEC_ID_SVQ1, BGAV_MK_FOURCC('S', 'V', 'Q', '1') },
    { CODEC_ID_SVQ3, BGAV_MK_FOURCC('S', 'V', 'Q', '3') },
    { CODEC_ID_DVVIDEO, BGAV_MK_FOURCC('d', 'v', 's', 'd') },
    { CODEC_ID_HUFFYUV, BGAV_MK_FOURCC('H', 'F', 'Y', 'U') },
    { CODEC_ID_CYUV, BGAV_MK_FOURCC('C', 'Y', 'U', 'V') },
    { CODEC_ID_H264, BGAV_MK_FOURCC('H', '2', '6', '4') },
    { CODEC_ID_INDEO3, BGAV_MK_FOURCC('i', 'v', '3', '2') },
    { CODEC_ID_VP3, BGAV_MK_FOURCC('V', 'P', '3', '1') },
    { CODEC_ID_THEORA, BGAV_MK_FOURCC('T', 'H', 'R', 'A') },
    { CODEC_ID_ASV1, BGAV_MK_FOURCC('A', 'S', 'V', '1') },
    { CODEC_ID_ASV2, BGAV_MK_FOURCC('A', 'S', 'V', '2') },
    { CODEC_ID_FFV1, BGAV_MK_FOURCC('F', 'F', 'V', '1') },
    { CODEC_ID_4XM, BGAV_MK_FOURCC('4', 'X', 'M', 'V') },
    { CODEC_ID_VCR1, BGAV_MK_FOURCC('V', 'C', 'R', '1') },
    { CODEC_ID_CLJR, BGAV_MK_FOURCC('C', 'L', 'J', 'R') },
    { CODEC_ID_MDEC, BGAV_MK_FOURCC('M', 'D', 'E', 'C') },
    { CODEC_ID_ROQ, BGAV_MK_FOURCC('R', 'O', 'Q', 'V') },
    { CODEC_ID_INTERPLAY_VIDEO, BGAV_MK_FOURCC('I', 'P', 'V', 'D') },
    //    { CODEC_ID_XAN_WC3, },
    //    { CODEC_ID_XAN_WC4, },
    { CODEC_ID_RPZA, BGAV_MK_FOURCC('r', 'p', 'z', 'a') },
    { CODEC_ID_CINEPAK, BGAV_MK_FOURCC('c', 'v', 'i', 'd') },
    { CODEC_ID_WS_VQA, BGAV_MK_FOURCC('W', 'V', 'Q', 'A') },
    { CODEC_ID_MSRLE, BGAV_MK_FOURCC('W', 'R', 'L', 'E') },
    { CODEC_ID_MSVIDEO1, BGAV_MK_FOURCC('M', 'S', 'V', 'C') },
    { CODEC_ID_IDCIN, BGAV_MK_FOURCC('I', 'D', 'C', 'I') },
    { CODEC_ID_8BPS, BGAV_MK_FOURCC('8', 'B', 'P', 'S') },
    { CODEC_ID_SMC, BGAV_MK_FOURCC('s', 'm', 'c', ' ')},
    { CODEC_ID_FLIC, BGAV_MK_FOURCC('F', 'L', 'I', 'C') },
    { CODEC_ID_TRUEMOTION1, BGAV_MK_FOURCC('D', 'U', 'C', 'K') },
    { CODEC_ID_VMDVIDEO, BGAV_MK_FOURCC('V', 'M', 'D', 'V') },
    { CODEC_ID_MSZH, BGAV_MK_FOURCC('M', 'S', 'Z', 'H') },
    { CODEC_ID_ZLIB, BGAV_MK_FOURCC('Z', 'L', 'I', 'B') },
    { CODEC_ID_QTRLE, BGAV_MK_FOURCC('r', 'l', 'e', ' ') },
    { CODEC_ID_SNOW, BGAV_MK_FOURCC('S', 'N', 'O', 'W') },
    { CODEC_ID_TSCC, BGAV_MK_FOURCC('T', 'S', 'C', 'C') },
    { CODEC_ID_ULTI, BGAV_MK_FOURCC('U', 'L', 'T', 'I') },
    { CODEC_ID_QDRAW, BGAV_MK_FOURCC('q', 'd', 'r', 'w') },
    { CODEC_ID_VIXL, BGAV_MK_FOURCC('V', 'I', 'X', 'L') },
    { CODEC_ID_QPEG, BGAV_MK_FOURCC('Q', '1', '.', '1') },
    //    { CODEC_ID_XVID, },
    { CODEC_ID_PNG, BGAV_MK_FOURCC('p', 'n', 'g', ' ') },
    //    { CODEC_ID_PPM, },
    //    { CODEC_ID_PBM, },
    //    { CODEC_ID_PGM, },
    //    { CODEC_ID_PGMYUV, },
    //    { CODEC_ID_PAM, },
    { CODEC_ID_FFVHUFF, BGAV_MK_FOURCC('F', 'F', 'V', 'H') },
    { CODEC_ID_RV30,    BGAV_MK_FOURCC('R', 'V', '3', '0') },
    { CODEC_ID_RV40,    BGAV_MK_FOURCC('R', 'V', '4', '0') },
    { CODEC_ID_VC1,     BGAV_MK_FOURCC('V', 'C', '-', '1') },
    { CODEC_ID_WMV3, BGAV_MK_FOURCC('W', 'M', 'V', '3') },
    { CODEC_ID_LOCO, BGAV_MK_FOURCC('L', 'O', 'C', 'O') },
    { CODEC_ID_WNV1, BGAV_MK_FOURCC('W', 'N', 'V', '1') },
    { CODEC_ID_AASC, BGAV_MK_FOURCC('A', 'A', 'S', 'C') },
    { CODEC_ID_INDEO2, BGAV_MK_FOURCC('R', 'T', '2', '1') },
    { CODEC_ID_FRAPS, BGAV_MK_FOURCC('F', 'P', 'S', '1') },
    { CODEC_ID_TRUEMOTION2, BGAV_MK_FOURCC('T', 'M', '2', '0') },
    //    { CODEC_ID_BMP, },
    { CODEC_ID_CSCD, BGAV_MK_FOURCC('C', 'S', 'C', 'D') },
    { CODEC_ID_MMVIDEO, BGAV_MK_FOURCC('M', 'M', 'V', 'D')},
    { CODEC_ID_ZMBV, BGAV_MK_FOURCC('Z', 'M', 'B', 'V') },
    { CODEC_ID_AVS, BGAV_MK_FOURCC('A', 'V', 'S', ' ') },
    { CODEC_ID_SMACKVIDEO, BGAV_MK_FOURCC('S', 'M', 'K', '2') },
    { CODEC_ID_NUV, BGAV_MK_FOURCC('R', 'J', 'P', 'G') },
    { CODEC_ID_KMVC, BGAV_MK_FOURCC('K', 'M', 'V', 'C') },
    { CODEC_ID_FLASHSV, BGAV_MK_FOURCC('F', 'L', 'V', 'S') },
#if LIBAVCODEC_BUILD >= ((51<<16)+(11<<8)+0)
    { CODEC_ID_CAVS, BGAV_MK_FOURCC('C', 'A', 'V', 'S') },
#endif
    //    { CODEC_ID_JPEG2000, },
#if LIBAVCODEC_BUILD >= ((51<<16)+(13<<8)+0)
    { CODEC_ID_VMNC, BGAV_MK_FOURCC('V', 'M', 'n', 'c') },
#endif
#if LIBAVCODEC_BUILD >= ((51<<16)+(14<<8)+0)    
    { CODEC_ID_VP5, BGAV_MK_FOURCC('V', 'P', '5', '0') },
    { CODEC_ID_VP6, BGAV_MK_FOURCC('V', 'P', '6', '0') },
#endif
    //    { CODEC_ID_VP6F, },
#if LIBAVCODEC_BUILD >= ((51<<16)+(17<<8)+0)
    { CODEC_ID_TARGA, BGAV_MK_FOURCC('t', 'g', 'a', ' ') },
#endif
#if LIBAVCODEC_BUILD >= ((51<<16)+(18<<8)+0)
    { CODEC_ID_DSICINVIDEO, BGAV_MK_FOURCC('d', 'c', 'i', 'n') },
#endif
#if LIBAVCODEC_BUILD >= ((51<<16)+(19<<8)+0)
    { CODEC_ID_TIERTEXSEQVIDEO, BGAV_MK_FOURCC('T', 'I', 'T', 'X') },
#endif
#if LIBAVCODEC_BUILD >= ((51<<16)+(20<<8)+0)
    { CODEC_ID_TIFF, BGAV_MK_FOURCC('t', 'i', 'f', 'f') },
#endif
#if LIBAVCODEC_BUILD >= ((51<<16)+(21<<8)+0)
    { CODEC_ID_GIF, BGAV_MK_FOURCC('g', 'i', 'f', ' ') },
#endif
    //    { CODEC_ID_FFH264, },
#if LIBAVCODEC_BUILD >= ((51<<16)+(39<<8)+0)
    { CODEC_ID_DXA, BGAV_MK_FOURCC('D', 'X', 'A', ' ') },
#endif
    //    { CODEC_ID_DNXHD, },
#if LIBAVCODEC_BUILD >= ((51<<16)+(40<<8)+3)
    { CODEC_ID_THP, BGAV_MK_FOURCC('T', 'H', 'P', 'V') },
#endif
    //    { CODEC_ID_SGI, },
#if LIBAVCODEC_BUILD >= ((51<<16)+(40<<8)+3)
    { CODEC_ID_C93, BGAV_MK_FOURCC('C','9','3','V') },
#endif
#if LIBAVCODEC_BUILD >= ((51<<16)+(40<<8)+3)
    { CODEC_ID_BETHSOFTVID, BGAV_MK_FOURCC('B','S','D','V')},
#endif
    //    { CODEC_ID_PTX, },
    //    { CODEC_ID_TXD, },
#if LIBAVCODEC_BUILD >= ((51<<16)+(45<<8)+0)
    { CODEC_ID_VP6A, BGAV_MK_FOURCC('V','P','6','A') },
#endif
    //    { CODEC_ID_AMV, },

#if LIBAVCODEC_BUILD >= ((51<<16)+(47<<8)+0)
    { CODEC_ID_VB, BGAV_MK_FOURCC('V','B','V','1') },
#endif
    
    { /* End */ }
  };


static void init_audio_stream(bgav_demuxer_context_t * ctx,
                              AVStream * st, int index)
  {
  bgav_stream_t * s;
  int i;
  audio_codec_map_t * map = (audio_codec_map_t *)0;
  AVCodecContext *codec= st->codec;
  
  /* Get fourcc */
  for(i = 0; i < sizeof(audio_codecs)/sizeof(audio_codecs[0]); i++)
    {
    if((audio_codecs[i].id == st->codec->codec_id) &&
       (!audio_codecs[i].codec_tag ||
        (audio_codecs[i].codec_tag == st->codec->codec_tag)))
      {
      map = &audio_codecs[i];
      break;
      }
    }
  if(!map)
    return;
  
  s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  s->fourcc = map->fourcc;

  st->discard = AVDISCARD_NONE;

  if(map->bits)
    s->data.audio.bits_per_sample = map->bits;
  else
#if LIBAVCODEC_VERSION_INT < ((52<<16)+(0<<8)+0)
    s->data.audio.bits_per_sample = codec->bits_per_sample;
#else
    s->data.audio.bits_per_sample = codec->bits_per_coded_sample;
#endif
  
  s->data.audio.block_align = codec->block_align;
  if(!s->data.audio.block_align &&
     map->bits)
    {
    s->data.audio.block_align = ((map->bits + 7) / 8) * codec->channels;
    }
  
  s->timescale = st->time_base.den;
  
  s->data.audio.format.num_channels = codec->channels;
  s->data.audio.format.samplerate = codec->sample_rate;
  s->ext_size = codec->extradata_size;
  s->ext_data = codec->extradata;
  s->container_bitrate = codec->bit_rate;
  s->stream_id = index;
  
  }

static void init_video_stream(bgav_demuxer_context_t * ctx,
                              AVStream * st, int index)
  {
  bgav_stream_t * s;
  video_codec_map_t * map = (video_codec_map_t *)0;
  int i;
  uint32_t tag;
  AVCodecContext *codec= st->codec;

  tag   =
    ((st->codec->codec_tag & 0x000000ff) << 24) |
    ((st->codec->codec_tag & 0x0000ff00) << 8) |
    ((st->codec->codec_tag & 0x00ff0000) >> 8) |
    ((st->codec->codec_tag & 0xff000000) >> 24);
  

  if(tag)
    {
    s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
    s->fourcc = tag;
    s->cleanup = cleanup_stream_ffmpeg;
    }
  else
    {
    for(i = 0; i < sizeof(video_codecs)/sizeof(video_codecs[0]); i++)
      {
      if(video_codecs[i].id == st->codec->codec_id)
        {
        map = &video_codecs[i];
        break;
        }
      }
    if(!map)
      return;
    
    s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
    s->fourcc = map->fourcc;
    }
  st->discard = AVDISCARD_NONE;
  
  
  s->data.video.format.image_width = codec->width;
  s->data.video.format.image_height = codec->height;
  s->data.video.format.frame_width = codec->width;
  s->data.video.format.frame_height = codec->height;

  if(codec->time_base.den && codec->time_base.num)
    {
    s->data.video.format.timescale      = codec->time_base.den;
    s->data.video.format.frame_duration = codec->time_base.num;
    }
  
  s->timescale = st->time_base.den;
  
  s->data.video.format.pixel_width = codec->sample_aspect_ratio.num;
  s->data.video.format.pixel_height = codec->sample_aspect_ratio.den;
  if(!s->data.video.format.pixel_width) s->data.video.format.pixel_width = 1;
  if(!s->data.video.format.pixel_height) s->data.video.format.pixel_height = 1;
#if LIBAVCODEC_VERSION_INT < ((52<<16)+(0<<8)+0)
  s->data.video.depth = codec->bits_per_sample;
#else
  s->data.video.depth = codec->bits_per_coded_sample;
#endif
  s->ext_size = codec->extradata_size;
  s->ext_data = codec->extradata;
  s->container_bitrate = codec->bit_rate;
  s->stream_id = index;

  if(codec->palctrl)
    {
    s->priv = calloc(AVPALETTE_COUNT, sizeof(bgav_palette_entry_t));
    s->data.video.palette = s->priv;
    s->data.video.palette_size = AVPALETTE_COUNT;
    }
  }

static int open_ffmpeg(bgav_demuxer_context_t * ctx)
  {
  int i;
  ffmpeg_priv_t * priv;
  AVFormatContext *avfc;
  AVFormatParameters ap;
  char * tmp_filename;
  memset(&ap, 0, sizeof(ap));
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* With the current implementation in ffmpeg, this can be
     called multiple times */
  register_protocol(&bgav_protocol);

  avfc = av_alloc_format_context();

  tmp_filename = bgav_sprintf("bgav:%s", ctx->input->filename);

  url_fopen(&priv->pb, tmp_filename, URL_RDONLY);
#if LIBAVFORMAT_VERSION_INT < ((52<<16)+(0<<8)+0)
  ((URLContext*)(priv->pb.opaque))->priv_data= ctx->input;
#else
  ((URLContext*)(priv->pb->opaque))->priv_data= ctx->input;
#endif
  priv->avif = get_format(ctx->input);
  
#if LIBAVFORMAT_VERSION_INT < ((52<<16)+(0<<8)+0)
  if(av_open_input_stream(&avfc, &priv->pb, tmp_filename, priv->avif, &ap)<0)
#else
  if(av_open_input_stream(&avfc, priv->pb, tmp_filename, priv->avif, &ap)<0)
#endif
    {
    bgav_log(ctx->opt,BGAV_LOG_ERROR,LOG_DOMAIN,
             "av_open_input_stream failed");
    free(tmp_filename);
    return 0;
    }
  free(tmp_filename);
  priv->avfc= avfc;
  /* Get the streams */
  if(av_find_stream_info(avfc) < 0)
    {
    bgav_log(ctx->opt,BGAV_LOG_ERROR,LOG_DOMAIN,
             "av_find_stream_info failed");
    return 0;
    }

  ctx->tt = bgav_track_table_create(1);
  
  for(i = 0; i < avfc->nb_streams; i++)
    {
    switch(avfc->streams[i]->codec->codec_type)
      {
      case CODEC_TYPE_AUDIO:
        init_audio_stream(ctx, avfc->streams[i], i);
        break;
      case CODEC_TYPE_VIDEO:
        init_video_stream(ctx, avfc->streams[i], i);
        break;
      case CODEC_TYPE_SUBTITLE:
        break;
      default:
        break;
      }
    }
  
  if((priv->avfc->duration != 0) && (priv->avfc->duration != AV_NOPTS_VALUE))
    {
    ctx->tt->cur->duration = (priv->avfc->duration * GAVL_TIME_SCALE) / AV_TIME_BASE;
    
    if(priv->avfc->iformat->read_seek)
      ctx->flags |= BGAV_DEMUXER_CAN_SEEK;
    }

  ctx->stream_description = bgav_sprintf(TRD("%s (via ffmpeg)"),
                                         priv->avfc->iformat->long_name);

  /* Metadata */
  if(avfc->title[0])
    ctx->tt->cur->metadata.title = bgav_strdup(avfc->title);
  if(avfc->author[0])
    ctx->tt->cur->metadata.author = bgav_strdup(avfc->author);
  if(avfc->copyright[0])
    ctx->tt->cur->metadata.copyright = bgav_strdup(avfc->copyright);
  if(avfc->album[0])
    ctx->tt->cur->metadata.album = bgav_strdup(avfc->album);
  if(avfc->genre[0])
    ctx->tt->cur->metadata.genre = bgav_strdup(avfc->genre);
  
  return 1;
  }


static void close_ffmpeg(bgav_demuxer_context_t * ctx)
  {
  ffmpeg_priv_t * priv;
  priv = (ffmpeg_priv_t*)ctx->priv;

  av_close_input_file(priv->avfc);
  if(priv)
    free(priv);
  }

static int next_packet_ffmpeg(bgav_demuxer_context_t * ctx)
  {
  int i;
  ffmpeg_priv_t * priv;
  AVPacket pkt;
  AVStream * avs;
  bgav_palette_entry_t * pal;
  bgav_packet_t * p;
  bgav_stream_t * s;
  int i_tmp;
  priv = (ffmpeg_priv_t*)ctx->priv;
  
  if(av_read_frame(priv->avfc, &pkt) < 0)
    return 0;
  
  s = bgav_track_find_stream(ctx, pkt.stream_index);
  if(!s)
    {
    av_free_packet(&pkt);
    return 1;
    }
  
  avs = priv->avfc->streams[pkt.stream_index];
  
  p = bgav_stream_get_packet_write(s);
  bgav_packet_alloc(p, pkt.size);
  memcpy(p->data, pkt.data, pkt.size);
  p->data_size = pkt.size;
  
  if(pkt.pts != AV_NOPTS_VALUE)
    {
    p->pts=pkt.pts * priv->avfc->streams[pkt.stream_index]->time_base.num;
    if(s->in_time == BGAV_TIMESTAMP_UNDEFINED)
      s->in_time = p->pts;
    
    if((s->type == BGAV_STREAM_VIDEO) && pkt.duration)
      p->duration = pkt.duration * avs->time_base.num;
    }
  /* Handle palette */
  if((s->type == BGAV_STREAM_VIDEO) &&
     avs->codec->palctrl &&
     avs->codec->palctrl->palette_changed)
    {
    pal = s->priv;
    
    for(i = 0; i < AVPALETTE_COUNT; i++)
      {
      i_tmp = (avs->codec->palctrl->palette[i] >> 24) & 0xff;
      pal[i].a = i_tmp | i_tmp << 8;
      i_tmp = (avs->codec->palctrl->palette[i] >> 16) & 0xff;
      pal[i].r = i_tmp | i_tmp << 8;
      i_tmp = (avs->codec->palctrl->palette[i] >> 8) & 0xff;
      pal[i].g = i_tmp | i_tmp << 8;
      i_tmp = (avs->codec->palctrl->palette[i]) & 0xff;
      pal[i].b = i_tmp | i_tmp << 8;
      }
    avs->codec->palctrl->palette_changed = 0;
    s->data.video.palette_changed = 1;
    }
  
  if(pkt.flags&PKT_FLAG_KEY)
    p->keyframe = 1;
  bgav_packet_done_write(p);
  
  av_free_packet(&pkt);
  
  return 1;
  }

static void seek_ffmpeg(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  ffmpeg_priv_t * priv;
  priv = (ffmpeg_priv_t*)ctx->priv;

  av_seek_frame(priv->avfc, -1,
                gavl_time_rescale(scale, AV_TIME_BASE, time), 0);
  
  while(!bgav_track_has_sync(ctx->tt->cur))
    {
    if(!next_packet_ffmpeg(ctx))
      break;
    }
  }

const bgav_demuxer_t bgav_demuxer_ffmpeg =
  {
    .probe =       probe_ffmpeg,
    .open =        open_ffmpeg,
    .next_packet = next_packet_ffmpeg,
    .seek =        seek_ffmpeg,
    .close =       close_ffmpeg

  };

