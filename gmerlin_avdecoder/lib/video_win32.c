/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

/* Ported from xine */

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include <avdec_private.h>
#include <codecs.h>
#include <nanosoft.h>
#define LOG_DOMAIN "video_win32"

#include "libw32dll/wine/msacm.h"
#include "libw32dll/wine/driver.h"
#include "libw32dll/wine/avifmt.h"
#include "libw32dll/wine/vfw.h"
#include "libw32dll/wine/mmreg.h"
#include "libw32dll/wine/ldt_keeper.h"
#include "libw32dll/wine/win32.h"
#include "libw32dll/wine/wineacm.h"
#include "libw32dll/wine/loader.h"

#define NOAVIFILE_HEADERS
#include "libw32dll/DirectShow/guids.h"
#include "libw32dll/DirectShow/DS_AudioDecoder.h"
#include "libw32dll/DirectShow/DS_VideoDecoder.h"
#include "libw32dll/dmo/DMO_AudioDecoder.h"
#include "libw32dll/dmo/DMO_VideoDecoder.h"

#include "libw32dll/libwin32.h"

#include <win32codec.h>

#define CODEC_STD 0
#define CODEC_DS  1
#define CODEC_DMO 2

/* Global locking for win32 codecs */

pthread_mutex_t win32_mutex = PTHREAD_MUTEX_INITIALIZER;
void bgav_windll_lock()
  {
  pthread_mutex_lock(&win32_mutex);
  }

void bgav_windll_unlock()
  {
  pthread_mutex_unlock(&win32_mutex);
  }

typedef struct
  {
  char * name;
  char * format_name;
  uint32_t * fourccs;
  char * dll_name;
  int type;
  GUID guid;
  int ex_functions;
  } codec_info_t;

static codec_info_t codec_infos[] =
  {
#if 0
    {
      .name =        "Win32 Indeo 3.2 decoder",
      .format_name = "Indeo 3.2",
      .fourccs =     (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '3', '2'), 0x00 },
      .dll_name =    "ir32_32.dll",
      .type =        CODEC_STD,
    },
#endif
    {
      .name =        "Vivo H.263 decoder",
      .format_name = "Vivo H.263",
      .fourccs =     (uint32_t[]){ BGAV_MK_FOURCC('v', 'i', 'v', 'o'), 0x00 },
      .dll_name =    "ivvideo.dll",
      .type =        CODEC_STD,
    },
    {
      .name =        "Win32 Indeo 4.1 decoder",
      .format_name = "Indeo 4.1",
      .fourccs =     (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '4', '1'), 0x00 },
      .dll_name =    "ir41_32.dll",
      .type =        CODEC_STD,
    },
    {
      .name =        "DirectShow Indeo 5.0 decoder",
      .format_name = "Indeo 5.0",
      .fourccs =     (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '5', '0'), 0x00 },
      .dll_name =    "ir50_32.dll",
      .type =        CODEC_DS,
      .guid =        { 0x30355649, 0, 16,{ 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71 } }
    },
#if 0
    {
      .name =        "DirectShow VP6 decoder",
      .format_name = "VP6",
      .fourccs =     (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '5', '0'), 0x00 },
      .dll_name =    "vp6dec.ax",
      .type =        CODEC_DS,
      .guid =        { 0x30355649, 0, 16,{ 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71 } }
    },
#endif
    {
      .name =        "Win32 Indeo 5.0 decoder",
      .format_name = "Indeo 5.0",
      .fourccs =     (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '5', '0'), 0x00 },
      .dll_name =    "ir50_32.dll",
      .type =        CODEC_STD,
    },
    {
      .name =        "Win32 VP4 decoder",
      .format_name = "VP4",
      .fourccs =     (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '4', '0'), 0x00 },
      .dll_name =    "vp4vfw.dll",
      .type =        CODEC_STD,
      .ex_functions = 1,
    },
    {
      .name =        "Win32 VP5 decoder",
      .format_name = "VP5",
      .fourccs =     (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '5', '0'), 0x00 },
      .dll_name =    "vp5vfw.dll",
      .type =        CODEC_STD,
      .ex_functions = 1,
    },
    {
      .name =        "Win32 VP6 decoder",
      .format_name = "VP6",
      .fourccs =     (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', '0'),
                                 BGAV_MK_FOURCC('V', 'P', '6', '1'),
                                 BGAV_MK_FOURCC('V', 'P', '6', '2'), 
                                 0x00 },
      .dll_name =    "vp6vfw.dll",
      .type =        CODEC_STD,
      .ex_functions = 1,
    },
#if 1
    {
      .name =        "DMO WMV9 decoder",
      .format_name = "WMV9",
      .fourccs =  (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '3'), 0x00 },
      .dll_name = "wmv9dmod.dll",
      .type =     CODEC_DMO,
      .guid =     { 0x724bb6a4, 0xe526, 0x450f,
                  {0xaf, 0xfa, 0xab, 0x9b, 0x45, 0x12, 0x91, 0x11} },
    },
#else
    {
      .name =        "DMO WMV9 decoder",
      .format_name = "WMV9",
      .fourccs =  (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '3'), 0x00 },
      .dll_name = "wmvdmod.dll",
      .type =     CODEC_DMO,
      .guid =     { 0x82d353df, 0x90bd, 0x4382,
                  {0x8b, 0xc2, 0x3f, 0x61, 0x92, 0xb7, 0x6e, 0x34} }
    },
#endif
  };

#if 0
static uint32_t swap_endian(uint32_t val)
  {
  return ((val & 0x000000FF) << 24) |
    ((val & 0x0000FF00) << 8) |
    ((val & 0x00FF0000) >> 8) |
    ((val & 0xFF000000) >> 24);
  }

static void dump_bi(BITMAPINFOHEADER * bh)
  {
  uint32_t fourcc_be;
  bgav_dprintf( "BITMAPINFOHEADER:\n");
  bgav_dprintf( "  biSize: %ld\n", bh->biSize); /* sizeof(BITMAPINFOHEADER) */
  bgav_dprintf( "  biWidth: %ld\n", bh->biWidth);
  bgav_dprintf( "  biHeight: %ld\n", bh->biHeight);
  bgav_dprintf( "  biPlanes: %d\n", bh->biPlanes);
  bgav_dprintf( "  biBitCount: %d\n", bh->biBitCount);
  fourcc_be = swap_endian(bh->biCompression);
  bgav_dprintf( "  biCompression: ");
  bgav_dump_fourcc(fourcc_be);
  bgav_dprintf( "\n");
  bgav_dprintf( "  biSizeImage: %ld\n", bh->biSizeImage);
  bgav_dprintf( "  biXPelsPerMeter: %ld\n", bh->biXPelsPerMeter);
  bgav_dprintf( "  biYPelsPerMeter: %ld\n", bh->biXPelsPerMeter);
  bgav_dprintf( "  biClrUsed: %ld\n", bh->biClrUsed);
  bgav_dprintf( "  biClrImportant: %ld\n", bh->biClrImportant);

  }
#endif

extern char*   win32_def_path;

#define MAX_CODECS (sizeof(codec_infos)/sizeof(codec_infos[0]))

static bgav_video_decoder_t codecs[MAX_CODECS];

static codec_info_t * find_decoder(bgav_video_decoder_t * dec)
  {
  int i;
  for(i = 0; i < MAX_CODECS; i++)
    {
    if(codecs[i].name && !strcmp(codecs[i].name, dec->name))
      return  &codec_infos[i];
    }
  return NULL;
  }

typedef struct
  {
  gavl_video_frame_t * frame;
  BITMAPINFOHEADER bih_in;
  BITMAPINFOHEADER bih_out;
  
  /* Standard win32 */

  HIC hic;
  int ex_functions;

  /* Direct Show */
  
  DS_VideoDecoder  *ds_dec;

  /* dmo */
  
  DMO_VideoDecoder *dmo_dec;

  int bytes_per_pixel;

  int flip_y; /* Flip the image vertically */
  } win32_priv_t;

static void pack_bih(BITMAPINFOHEADER * dst, bgav_BITMAPINFOHEADER_t * src)
  {
  dst->biSize          = src->biSize;  /* sizeof(BITMAPINFOHEADER) */
  dst->biWidth         = src->biWidth;
  dst->biHeight        = src->biHeight;
  dst->biPlanes        = src->biPlanes; /* unused */
  dst->biBitCount      = src->biBitCount;
  dst->biCompression   = src->biCompression; /* fourcc of image */
  dst->biSizeImage     = src->biSizeImage;   /* size of image. For uncompressed images */
                           /* ( biCompression 0 or 3 ) can be zero.  */
  dst->biXPelsPerMeter = src->biXPelsPerMeter; /* unused */
  dst->biYPelsPerMeter = src->biYPelsPerMeter; /* unused */
  dst->biClrUsed       = src->biClrUsed;       /* valid only for palettized images. */
  /* Number of colors in palette. */
  dst->biClrImportant  = src->biClrImportant;
  }

static void unpack_bih(bgav_BITMAPINFOHEADER_t * dst, BITMAPINFOHEADER * src)
  {
  dst->biSize          = src->biSize;  /* sizeof(BITMAPINFOHEADER) */
  dst->biWidth         = src->biWidth;
  dst->biHeight        = src->biHeight;
  dst->biPlanes        = src->biPlanes; /* unused */
  dst->biBitCount      = src->biBitCount;
  dst->biCompression   = src->biCompression; /* fourcc of image */
  dst->biSizeImage     = src->biSizeImage;   /* size of image. For uncompressed images */
                           /* ( biCompression 0 or 3 ) can be zero.  */
  dst->biXPelsPerMeter = src->biXPelsPerMeter; /* unused */
  dst->biYPelsPerMeter = src->biYPelsPerMeter; /* unused */
  dst->biClrUsed       = src->biClrUsed;       /* valid only for palettized images. */
  /* Number of colors in palette. */
  dst->biClrImportant  = src->biClrImportant;
  }

static int init_std(bgav_win32_thread_t * thread)
  {
  int old_bit_count;
  HRESULT result;
  codec_info_t * info;
  win32_priv_t * priv;
  bgav_BITMAPINFOHEADER_t bih_in;
  bgav_BITMAPINFOHEADER_t bih_out;
  bgav_stream_t * s = thread->s;
  
  priv = calloc(1, sizeof(*priv));
  thread->priv = priv;
  

  info = find_decoder(s->data.video.decoder->decoder);

  priv->ex_functions = info->ex_functions;

  if(s->data.video.format.image_height < 0)
    {
    s->data.video.format.image_height = -s->data.video.format.image_height;
    priv->flip_y = 1;
    }
  
  bgav_BITMAPINFOHEADER_set_format(&bih_in, s);
  //  bih_in.biCompression = 0x6f766976;
  //  bih_in.biCompression = s->fourcc;
  //  bih_in.biCompression = BGAV_MK_FOURCC('V','P','6','2');
  priv->hic = ICOpen ((int)(info->dll_name), bih_in.biCompression,
                      ICMODE_FASTDECOMPRESS);
  
  if(!priv->hic)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot open %s", info->dll_name);
    return 0;
    }

  bih_in.biSizeImage = 0;
  pack_bih(&priv->bih_in, &bih_in);

          
  result = ICDecompressGetFormat(priv->hic, &priv->bih_in, &priv->bih_out);

  unpack_bih(&bih_out, &priv->bih_out);
  
  if(result < 0)
    {
    return 0;
    }
  
  switch(bih_out.biCompression)
    {
    case 0:
      break;
    default:
      return 0;
      
    }

  /* Check if we can output YUV data */

  old_bit_count = bih_out.biBitCount;
  
#if 1
  bih_out.biCompression = BGAV_MK_FOURCC('2', 'Y', 'U', 'Y');
  bih_out.biSizeImage = bih_out.biWidth * bih_out.biHeight * 2;
  bih_out.biBitCount  = 16;
  pack_bih(&priv->bih_out, &bih_out);

  result = (!priv->ex_functions) 
    ?ICDecompressQuery(priv->hic,   &priv->bih_in, &priv->bih_out)
    :ICDecompressQueryEx(priv->hic, &priv->bih_in, &priv->bih_out);
#else
  result = 1;
#endif
  if(result)
    {
    bih_out.biCompression = 0;
    bih_out.biBitCount  = old_bit_count;
    
    switch(bih_out.biBitCount)
      {
      case 24:
        s->data.video.format.pixelformat = GAVL_RGB_24;
        priv->bytes_per_pixel = 3;
        break;
      case 16:
        s->data.video.format.pixelformat = GAVL_RGB_15;
        priv->bytes_per_pixel = 2;
        break;
      default:
        return 0;
      }
    bih_out.biSizeImage = bih_out.biWidth * bih_out.biHeight * priv->bytes_per_pixel;
    
    pack_bih(&priv->bih_out, &bih_out);
    }
  else
    {
    s->data.video.format.pixelformat = GAVL_YUY2;
    priv->bytes_per_pixel = 2;
    }

  /* Initialize decompression */
  
  result = (!priv->ex_functions) 
    ?ICDecompressBegin(priv->hic, &priv->bih_in, &priv->bih_out)
    :ICDecompressBeginEx(priv->hic, &priv->bih_in, &priv->bih_out);

    
  if(result)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "ICDecompressBegin failed");
    return 0;
    }
  priv->frame = gavl_video_frame_create(&s->data.video.format);

  s->description = bgav_strdup(info->format_name);


  if(gavl_pixelformat_is_rgb(s->data.video.format.pixelformat))
    priv->flip_y ^= 1;
  
  return 1;
  }

static int decode_std(bgav_win32_thread_t * thread)
  {
  uint32_t flags;
  HRESULT result;
  win32_priv_t * priv;
  bgav_stream_t * s = thread->s;
  gavl_video_frame_t * frame = thread->video_frame;
  
  priv = thread->priv;
  
  flags = 0;

  if(!thread->keyframe)
    flags |= ICDECOMPRESS_NOTKEYFRAME;

  if(!frame)
    flags |= ICDECOMPRESS_HURRYUP|ICDECOMPRESS_PREROL;

  priv->bih_in.biSizeImage = thread->data_len;

  priv->bih_out.biSizeImage =
    s->data.video.format.image_height * priv->frame->strides[0];
  priv->bih_out.biWidth     =
    priv->frame->strides[0] / priv->bytes_per_pixel;
  result = (!priv->ex_functions)
    ?ICDecompress(priv->hic, flags,
                  &priv->bih_in, thread->data, &priv->bih_out, 
                  priv->frame->planes[0])
    :ICDecompressEx(priv->hic, flags,
                  &priv->bih_in, thread->data, &priv->bih_out, 
                    priv->frame->planes[0]);
    
  if(result)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "ICDecompress failed");
    }
  if(frame)
    {
    if(priv->flip_y)
      {
      /* RGB pixelformats are upside down normally */
      gavl_video_frame_copy_flip_y(&s->data.video.format, frame, priv->frame);
      }
    else
      gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    }
  
  return 1;
  }


static void cleanup_std(bgav_win32_thread_t * thread)
  {
  win32_priv_t * priv;
  priv = thread->priv;
  if(priv->hic)
    {
    ICDecompressEnd(priv->hic);
    ICClose(priv->hic);
    }
  
  gavl_video_frame_destroy(priv->frame);
  free(priv);
  }

/* Direct show */

static int init_ds(bgav_win32_thread_t*t)
  {
  //  HRESULT result;
  uint8_t * init_data;
  codec_info_t * info;
  win32_priv_t * priv;
  bgav_BITMAPINFOHEADER_t bih_in;

  bgav_stream_t * s = t->s;
  
  //  bgav_BITMAPINFOHEADER_t bih_out;
  
  priv = calloc(1, sizeof(*priv));
  t->priv = priv;

  info = find_decoder(s->data.video.decoder->decoder);
    
  bgav_BITMAPINFOHEADER_set_format(&bih_in, s);
  //  bgav_BITMAPINFOHEADER_dump(&bih_in);
  pack_bih(&priv->bih_in, &bih_in);

  if(!s->ext_size)
    priv->ds_dec = DS_VideoDecoder_Open(info->dll_name, &info->guid,
                                          &priv->bih_in, 0 /* flipped */, 0);
  else
    {
    priv->bih_in.biSize = 40 + s->ext_size;
    init_data = malloc(40 + s->ext_size);
    memcpy(init_data, &priv->bih_in, 40);
    memcpy(&init_data[40], s->ext_data, s->ext_size);

    priv->ds_dec = DS_VideoDecoder_Open(info->dll_name, &info->guid,
                                          (BITMAPINFOHEADER*)init_data, 0 /* flipped */, 0);
    free(init_data);
    }
  
  if(!priv->ds_dec)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "DS_VideoDecoder_Open failed %d", s->ext_size);
    }

  //  capabilities = DS_VideoDecoder_GetCapabilities(priv->ds_dec);

  if(!DS_VideoDecoder_SetDestFmt(priv->ds_dec, 16, BGAV_MK_FOURCC('2', 'Y', 'U', 'Y')))
    {
    s->data.video.format.pixelformat = GAVL_YUY2;
    }
  else /* RGB */
    {
    DS_VideoDecoder_SetDestFmt(priv->ds_dec, 24, 0);
    s->data.video.format.pixelformat = GAVL_RGB_24;
    }
  DS_VideoDecoder_StartInternal(priv->ds_dec);
  priv->frame = gavl_video_frame_create(&s->data.video.format);
  s->description = bgav_strdup(info->format_name);

  return 1;
  }

static int decode_ds(bgav_win32_thread_t*t)
  {
  HRESULT result;
  win32_priv_t * priv;
  bgav_stream_t * s = t->s;
  gavl_video_frame_t * frame = t->video_frame;
  
  priv = t->priv;
  
  result = DS_VideoDecoder_DecodeInternal(priv->ds_dec, t->data, t->data_len,
                                           t->keyframe,
                                          (char*)(priv->frame->planes[0]));
  
  if(result)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Decode failed");
    }
  if(frame)
    {
    gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    }
  
  

  
  return 1;
  }


static void cleanup_ds(bgav_win32_thread_t*t)
  {
  win32_priv_t * priv;
  priv = t->priv;

  if(priv->ds_dec )
    DS_VideoDecoder_Destroy(priv->ds_dec);
  gavl_video_frame_destroy(priv->frame);
  free(priv);
  }

/* DMO */

static int init_dmo(bgav_win32_thread_t*t)
  {
  //  HRESULT result;
  uint8_t * init_data;
  codec_info_t * info;
  win32_priv_t * priv;
  bgav_BITMAPINFOHEADER_t bih_in;
  //  bgav_BITMAPINFOHEADER_t bih_out;

  bgav_stream_t * s = t->s;
  
  priv = calloc(1, sizeof(*priv));
  t->priv = priv;
  
  info = find_decoder(s->data.video.decoder->decoder);
    
  bgav_BITMAPINFOHEADER_set_format(&bih_in, s);
  //  bgav_BITMAPINFOHEADER_dump(&bih_in);
  pack_bih(&priv->bih_in, &bih_in);

  if(!s->ext_size)
    priv->dmo_dec = DMO_VideoDecoder_Open(info->dll_name, &info->guid,
                                          &priv->bih_in, 0 /* flipped */, 0);
  else
    {
    priv->bih_in.biSize = 40 + s->ext_size;
    init_data = malloc(40 + s->ext_size);
    memcpy(init_data, &priv->bih_in, 40);
    memcpy(&init_data[40], s->ext_data, s->ext_size);

    priv->dmo_dec = DMO_VideoDecoder_Open(info->dll_name, &info->guid,
                                          (BITMAPINFOHEADER*)init_data, 0 /* flipped */, 0);
    free(init_data);
    }
  
  if(!priv->dmo_dec)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "DMO_VideoDecoder_Open failed %d", s->ext_size);
    }

  //  capabilities = DMO_VideoDecoder_GetCapabilities(priv->dmo_dec);

  if(!DMO_VideoDecoder_SetDestFmt(priv->dmo_dec, 16, BGAV_MK_FOURCC('2', 'Y', 'U', 'Y')))
    {
    s->data.video.format.pixelformat = GAVL_YUY2;
    }
  else /* RGB */
    {
    DMO_VideoDecoder_SetDestFmt(priv->dmo_dec, 24, 0);
    s->data.video.format.pixelformat = GAVL_RGB_24;
    }
  DMO_VideoDecoder_StartInternal(priv->dmo_dec);
  priv->frame = gavl_video_frame_create(&s->data.video.format);
  s->description = bgav_strdup(info->format_name);
  return 1;
  }

static int decode_dmo(bgav_win32_thread_t*t)
  {
  HRESULT result;
  win32_priv_t * priv;
  
  bgav_stream_t * s = t->s;
  gavl_video_frame_t * frame = t->video_frame;
  
  priv = t->priv;
    
  result = DMO_VideoDecoder_DecodeInternal(priv->dmo_dec, t->data,
                                           t->data_len,
                                           t->keyframe,
                                           (char*)(priv->frame->planes[0]));
  if(result)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Decode failed");
    }
  if(frame)
    {
    gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    }
  return 1;
  }

static void cleanup_dmo(bgav_win32_thread_t*t)
  {
  win32_priv_t * priv;
  priv = t->priv;

  if(priv->dmo_dec )
    DMO_VideoDecoder_Destroy(priv->dmo_dec);
    
  gavl_video_frame_destroy(priv->frame);
  free(priv);
  }

static int init_win32(bgav_stream_t * s)
  {
  bgav_win32_thread_t * t;
  codec_info_t * info;
  info = find_decoder(s->data.video.decoder->decoder);

  t = calloc(1, sizeof(*t));
  s->data.video.decoder->priv = t;
  
  switch(info->type)
    {
    case CODEC_STD:
      t->init = init_std;
      t->decode = decode_std;
      t->cleanup = cleanup_std;
      break;
    case CODEC_DMO:
      t->init = init_dmo;
      t->decode = decode_dmo;
      t->cleanup = cleanup_dmo;
      break;
    case CODEC_DS:
      t->init = init_ds;
      t->decode = decode_ds;
      t->cleanup = cleanup_ds;
      break;
    }
  return bgav_win32_codec_thread_init(t, s);
  }

static int decode_win32(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  bgav_win32_thread_t * t;
  
  int result;
  bgav_packet_t * p;

  t = s->data.video.decoder->priv;
  
  p = bgav_stream_get_packet_read(s);
  if(!p)
    return 0;
  
  result = bgav_win32_codec_thread_decode_video(t, f, p->data, p->data_size,
                                                !!PACKET_GET_KEYFRAME(p));

  if(f)
    {
    f->timestamp = p->pts;
    f->duration = p->duration;
    }
  
  bgav_stream_done_packet_read(s, p);
  
  return result;
  }

static void close_win32(bgav_stream_t * s)
  {
  
  }

int bgav_init_video_decoders_win32(bgav_options_t * opt)
  {
  int i;
  char dll_filename[PATH_MAX];
  struct stat stat_buf;
  int ret = 1;
  
  for(i = 0; i < MAX_CODECS; i++)
    {
    sprintf(dll_filename, "%s/%s", win32_def_path, codec_infos[i].dll_name);
    if(!stat(dll_filename, &stat_buf))
      {
      codecs[i].name = codec_infos[i].name;
      codecs[i].fourccs = codec_infos[i].fourccs;

      codecs[i].init   = init_win32;
      codecs[i].decode = decode_win32;
      codecs[i].close  = close_win32;
      bgav_video_decoder_register(&codecs[i]);
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
