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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include <avdec_private.h>
#include <codecs.h>
#include <nanosoft.h>

#include "libw32dll/wine/msacm.h"
#include "libw32dll/wine/driver.h"
#include "libw32dll/wine/avifmt.h"
// #include "libw32dll/wine/vfw.h"
#include "libw32dll/wine/mmreg.h"
#include "libw32dll/wine/ldt_keeper.h"
#include "libw32dll/wine/win32.h"
#include "libw32dll/wine/wineacm.h"
#include "libw32dll/wine/loader.h"


#define NOAVIFILE_HEADERS
#include "libw32dll/DirectShow/guids.h"
#include "libw32dll/DirectShow/DS_AudioDecoder.h"
// #include "libw32dll/DirectShow/DS_VideoDecoder.h"
#include "libw32dll/dmo/DMO_AudioDecoder.h"
// #include "libw32dll/dmo/DMO_VideoDecoder.h"

#include "libw32dll/libwin32.h"

#define LOG_DOMAIN "audio_win32"

#define SAMPLES_PER_FRAME 1024

/* Ported from xine */

#define CODEC_STD 0
#define CODEC_DS  1
#define CODEC_DMO 2

typedef struct
  {
  char * name;
  char * format_name;
  uint32_t * fourccs;
  char * dll_name;
  int type;
  GUID guid;
  } codec_info_t;

codec_info_t codec_infos[] =
  {
    {
      .name =        "Voxware Metasound DirectShow decoder",
      .format_name = "Voxware Metasound",
      .fourccs =     (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x75), 0x00 },
      .dll_name =    "voxmsdec.ax",
      .type =        CODEC_DS,
      .guid =        { 0x73f7a062, 0x8829, 0x11d1,
                     { 0xb5, 0x50, 0x00, 0x60, 0x97, 0x24, 0x2d, 0x8d } }
    },
    {
      .name =        "ACELP.net DirectShow decoder",
      .format_name = "ACELP.net",
      .fourccs =     (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0130), 0x00 },
      .dll_name =    "acelpdec.ax",
      .type =        CODEC_DS,
      .guid =        { 0x4009f700, 0xaeba, 0x11d1,
                     { 0x83, 0x44, 0x00, 0xc0, 0x4f, 0xb9, 0x2e, 0xb7 } }
    },
#if 0 /* Handled by libgsm */
    {
      .name =        "msgsm ACM decoder",
      .format_name = "msgsm",
      .fourccs =     (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0031), 0x00 },
      .dll_name =    "msgsm32.acm",
      .type =        CODEC_STD,
    },
#endif
#if 1
    {
      .name =        "Vivo G.723/Siren Audio Codec",
      .format_name = "Vivo G.723/Siren",
      .fourccs =     (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0111),
                            BGAV_WAVID_2_FOURCC(0x0112),
                            0x00 },
      .dll_name =    "vivog723.acm",
      .type =        CODEC_STD,
    },
#endif
#if 0 /* Wrong WAV ID? */ 
    {
      .name =        "Lernout & Hauspie (CELP and SBC)",
      .format_name = "Lernout & Hauspie",
      .fourccs =     (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0072), 0x00 },
      .dll_name =    "lhacm.acm",
      .type =        CODEC_STD,
    },
#endif
#if 0 /* Crashes */
    {
      .name =        "DSP Group TrueSpeech(TM)",
      .format_name = "DSP Group TrueSpeech(TM)",
      .fourccs =     (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0022), 0x00 },
      .dll_name =    "tssoft32.acm",
      .type =        CODEC_STD,
    },
#endif
#if 0 /* Crashes */ 
    {
      .name =        "Sony ATRAC3",
      .format_name = "Sony ATRAC3",
      .fourccs =     (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x0270), 0x00 },
      .dll_name =    "atrac3.acm",
      .type =        CODEC_STD,
    },
#endif
  };

#define MAX_CODECS (sizeof(codec_infos)/sizeof(codec_infos[0]))

static bgav_audio_decoder_t codecs[MAX_CODECS];
extern char*   win32_def_path;

static int find_codec_index(bgav_stream_t * s)
  {
  int i, j;
  for(i = 0; i < MAX_CODECS; i++)
    {
    j = 0;
    while(codec_infos[i].fourccs[j])
      {
      if(codec_infos[i].fourccs[j] == s->fourcc)
        return i;
      j++;
      }
    }
  return -1;
  }

typedef struct
  {
  gavl_audio_frame_t * frame;
  ldt_fs_t           * ldt_fs;
  DS_AudioDecoder    * ds_dec;  /* DirectShow */
  DMO_AudioDecoder   * dmo_dec; /* DMO */
  HACMSTREAM         acmstream;
  
  int bytes_per_sample;
  
  int src_size;
  int dst_size;

  uint8_t * buffer;
  int buffer_size;
  int buffer_alloc;
  
  /* Decode function */

  int (*decode_frame)(bgav_stream_t*);
  
  } win32_priv_t;

static void pack_wf(WAVEFORMATEX * dst, bgav_WAVEFORMAT_t * src)
  {
  memset(dst, 0, sizeof(*dst));
  dst->nChannels       = src->f.WAVEFORMAT.nChannels;
  dst->nSamplesPerSec  = src->f.WAVEFORMAT.nSamplesPerSec;
  dst->nAvgBytesPerSec = src->f.WAVEFORMAT.nAvgBytesPerSec;
  dst->nBlockAlign     = src->f.WAVEFORMAT.nBlockAlign;
  dst->wFormatTag      = src->f.WAVEFORMAT.wFormatTag;
  dst->wBitsPerSample  = src->f.PCMWAVEFORMAT.wBitsPerSample;
  dst->cbSize          = src->f.WAVEFORMATEX.cbSize;
  }
#if 0
static void dump_wf(WAVEFORMATEX * wf)
  {
  bgav_dprintf("WAVEFORMATEX:\n");
  bgav_dprintf("  wFormatTag      %04x\n", wf->wFormatTag);
  bgav_dprintf("  nChannels:      %d\n",   wf->nChannels);
  bgav_dprintf("  nSamplesPerSec  %d\n",   (int)wf->nSamplesPerSec);
  bgav_dprintf("  nAvgBytesPerSec %d\n",   (int)wf->nAvgBytesPerSec);
  bgav_dprintf("  nBlockAlign     %d\n",   wf->nBlockAlign);
  bgav_dprintf("  wBitsPerSample  %d\n",   wf->wBitsPerSample);
  bgav_dprintf("  cbSize          %d\n",   wf->cbSize);
  }
#endif
/* Get one packet worth of data */

static int get_data(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  bgav_packet_t * p;
  
  priv = s->data.audio.decoder->priv;

  p = bgav_stream_get_packet_read(s);
  if(!p)
    return 0;

  if(priv->buffer_alloc < priv->buffer_size + p->data_size)
    {
    priv->buffer_alloc = priv->buffer_size + p->data_size + 32;
    priv->buffer = realloc(priv->buffer, priv->buffer_alloc);
    }

  memcpy(priv->buffer + priv->buffer_size, p->data, p->data_size);
  priv->buffer_size += p->data_size;
  bgav_stream_done_packet_read(s, p);
  return 1;
  }

/* Tell how many bytes we needed */

static void buffer_done(win32_priv_t * priv, int bytes)
  {
  if(priv->buffer_size > bytes)
    memmove(priv->buffer, priv->buffer + bytes, priv->buffer_size - bytes);
  priv->buffer_size -= bytes;
  }

static int decode_frame_DS(bgav_stream_t * s)
  {
  int result;
  unsigned int size_read;
  unsigned int size_written;
  
  win32_priv_t * priv;
  priv = s->data.audio.decoder->priv;

  while(priv->buffer_size < priv->src_size)
    if(!get_data(s))
      return 0;
  while(!priv->frame->valid_samples)
    {

    result = DS_AudioDecoder_Convert(priv->ds_dec, priv->buffer, priv->src_size,
                                     priv->frame->samples.s_16, priv->dst_size,
                                     &size_read, &size_written);

    if(result)
      return 0;
    

    if(size_written)
      {
      priv->frame->valid_samples =
        size_written / (s->data.audio.format.num_channels*priv->bytes_per_sample);
      }
    if(size_read)
      buffer_done(priv, size_read);
    
    }
  return 1;
  }

static int decode_frame_std(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  ACMSTREAMHEADER ash;
  HRESULT hr = 0;
  priv = s->data.audio.decoder->priv;

  /* Get new data */

  while(priv->buffer_size < priv->src_size)
    if(!get_data(s))
      return 0;

  /* Decode gthis stuff */
    
  memset(&ash, 0, sizeof(ash));
  ash.cbStruct=sizeof(ash);
  ash.fdwStatus=0;
  ash.dwUser=0; 
  ash.pbSrc=priv->buffer;
  ash.cbSrcLength=priv->src_size;
  ash.pbDst=priv->frame->samples.u_8;
  ash.cbDstLength=priv->dst_size;

    
  hr=acmStreamPrepareHeader(priv->acmstream,&ash,0);
  if(hr)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "acmStreamPrepareHeader failed %d",(int)hr);
    return 0;
    }

  hr=acmStreamConvert(priv->acmstream,&ash,0);
  if(hr)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "acmStreamConvert failed %d",(int)hr);
    return 0;
    }

  if(ash.cbSrcLengthUsed)
    buffer_done(priv, ash.cbSrcLengthUsed);
  priv->frame->valid_samples = ash.cbDstLengthUsed /
    (s->data.audio.format.num_channels*priv->bytes_per_sample);
  
  hr=acmStreamUnprepareHeader(priv->acmstream,&ash,0);
  if(hr)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "acmStreamUnprepareHeader failed %d",(int)hr);
    }
  
  return 1;
  }

static int init_w32(bgav_stream_t * s)
  {
  int result;
  unsigned long out_size;
  int codec_index;
  codec_info_t * info;
  win32_priv_t * priv = NULL;

  WAVEFORMATEX * in_format;
  bgav_WAVEFORMAT_t _in_format;
  uint8_t * in_fmt_buffer = NULL;

  WAVEFORMATEX out_format;

  codec_index = find_codec_index(s);
  if(codec_index == -1)
    goto fail;
  
  info = &codec_infos[codec_index];

  priv = calloc(1, sizeof(*priv));

  s->data.audio.decoder->priv = priv;

  /* Create input- and output formats */
  
  bgav_WAVEFORMAT_set_format(&_in_format, s);
  //  bgav_WAVEFORMAT_dump(&_in_format);

  in_fmt_buffer = malloc(sizeof(*in_format) + s->ext_size);

  in_format = (WAVEFORMATEX*)in_fmt_buffer;
  
  pack_wf(in_format, &_in_format);

  in_format->cbSize = s->ext_size;
  if(in_format->cbSize)
    {
    memcpy(in_fmt_buffer + sizeof(*in_format), s->ext_data, s->ext_size);
    }
#if 0
  bgav_WAVEFORMATEX_dump(&_in_format);
  dump_wf(in_format);
#endif
  /* We want 16 bit pcm as output */

  out_format.nChannels       = (in_format->nChannels >= 2)?2:1;
  out_format.nSamplesPerSec  = in_format->nSamplesPerSec;
  out_format.nAvgBytesPerSec = 2*out_format.nSamplesPerSec*out_format.nChannels;
  out_format.wFormatTag      = 0x0001;
  out_format.nBlockAlign     = 2*in_format->nChannels;
  out_format.wBitsPerSample  = 16;
  out_format.cbSize          = 0;

  /* Set gavl format */

  s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
      
  /* Setup LDT */

  priv->ldt_fs = Setup_LDT_Keeper();
  Setup_FS_Segment();
  
  switch(info->type)
    {
    case CODEC_STD:
      MSACM_RegisterDriver(info->dll_name, in_format->wFormatTag, 0);
#if 0
      dump_wf(in_format);
      dump_wf(&out_format);
#endif
      result=acmStreamOpen(&priv->acmstream,(HACMDRIVER)NULL,
                           in_format,
                           &out_format,
                           NULL,0,0,0);
      if(result)
        {
        if(result == ACMERR_NOTPOSSIBLE)
          {
          bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Unappropriate audio format");
          }
        else
          bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "acmStreamOpen error %d", result);
        priv->acmstream = 0;
        return 0;
        }


      acmStreamSize(priv->acmstream, s->data.audio.block_align,
                    &out_size, ACM_STREAMSIZEF_SOURCE);    
      s->data.audio.format.samples_per_frame =
        out_size / (s->data.audio.format.num_channels * 2);
      priv->bytes_per_sample = gavl_bytes_per_sample(s->data.audio.format.sample_format);
      priv->src_size = s->data.audio.block_align;
      priv->dst_size = out_size;
      priv->decode_frame = decode_frame_std;
      break;
    case CODEC_DS:
      s->data.audio.format.samples_per_frame = SAMPLES_PER_FRAME;
      
      priv->ds_dec=DS_AudioDecoder_Open(info->dll_name,
                                        &info->guid,
                                        in_format);
      if(!priv->ds_dec)
        {
        bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "DS_AudioDecoder_Open failed");
        goto fail;
        }
      priv->bytes_per_sample = gavl_bytes_per_sample(s->data.audio.format.sample_format);
      priv->dst_size = s->data.audio.format.num_channels *
        s->data.audio.format.samples_per_frame * priv->bytes_per_sample;
      
      priv->src_size = DS_AudioDecoder_GetSrcSize(priv->ds_dec,priv->dst_size);
    
    /* somehow DS_Filters seems to eat more than rec_audio_src_size if the output 
       buffer is big enough. Doubling rec_audio_src_size should make this 
       impossible */
      //      priv->src_size*=2; 

      priv->buffer_alloc = priv->src_size;
      priv->buffer = malloc(priv->buffer_alloc);

      priv->decode_frame = decode_frame_DS;

      s->data.audio.format.samples_per_frame *= 2;
      priv->dst_size *= 2;
      
      break;
    case CODEC_DMO:
      break;
    }

  gavl_set_channel_setup(&s->data.audio.format);
  
  priv->frame = gavl_audio_frame_create(&s->data.audio.format);

  if(in_fmt_buffer)
    free(in_fmt_buffer);

  s->description = bgav_sprintf("%s", info->format_name);

  return 1;
  fail:
  
  if(priv)
    {
    if(in_fmt_buffer)
      free(in_fmt_buffer);
    
    free(priv);
    s->data.audio.decoder->priv = NULL;
    }

  return 0;
  }

static int decode_frame_w32(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  int samples_copied;
  int samples_decoded = 0;
  Setup_FS_Segment();
  
  priv = s->data.audio.decoder->priv;

  if(!priv->decode_frame(s))
    return 0;

  gavl_audio_frame_copy_ptrs(&s->data.audio.format,
                             s->data.audio.frame, priv->frame);
  return 1;
  }

static void resync_w32(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  priv = s->data.audio.decoder->priv;

  priv->buffer_size = 0;
  priv->frame->valid_samples = 0;
  }

static void close_w32(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  priv = s->data.audio.decoder->priv;

  if(priv->buffer)
    free(priv->buffer);

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);

  if(priv->ds_dec)
    DS_AudioDecoder_Destroy(priv->ds_dec);

  if(priv->acmstream )
    acmStreamClose(priv->acmstream, 0);
  
  if(priv->ldt_fs)
    Restore_LDT_Keeper(priv->ldt_fs);
  
  
  free(priv);
  }

int bgav_init_audio_decoders_win32(bgav_options_t * opt)
  {
  int ret = 1;
  int i;
  char dll_filename[PATH_MAX];
  struct stat stat_buf;

  for(i = 0; i < MAX_CODECS; i++)
    {
    sprintf(dll_filename, "%s/%s", win32_def_path, codec_infos[i].dll_name);
    if(!stat(dll_filename, &stat_buf))
      {
      codecs[i].name = codec_infos[i].name;
      codecs[i].fourccs = codec_infos[i].fourccs;

      codecs[i].init   = init_w32;
      codecs[i].decode_frame = decode_frame_w32;
      codecs[i].close  = close_w32;
      codecs[i].resync = resync_w32;
      bgav_audio_decoder_register(&codecs[i]);
      }
    else
      {
      bgav_log(opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Codec DLL %s not found",
               dll_filename);
      ret = 0;
      }
    }
  return ret;
  }
