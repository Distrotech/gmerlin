/*****************************************************************
 
  video_win32.c
 
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

/* Ported from xine */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>
#include <nanosoft.h>

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
    {
      name:        "Win32 Indeo 3.2 decoder",
      format_name: "Indeo 3.2",
      fourccs:     (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '3', '2'), 0x00 },
      dll_name:    "ir32_32.dll",
      type:        CODEC_STD,
    },
    {
      name:        "Vivo H.263 decoder",
      format_name: "Vivo H.263",
      fourccs:     (uint32_t[]){ BGAV_MK_FOURCC('v', 'i', 'v', 'o'), 0x00 },
      dll_name:    "ivvideo.dll",
      type:        CODEC_STD,
    },
    {
      name:        "Win32 Indeo 4.1 decoder",
      format_name: "Indeo 4.1",
      fourccs:     (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '4', '1'), 0x00 },
      dll_name:    "ir41_32.dll",
      type:        CODEC_STD,
    },
    {
      name:        "DirectShow Indeo 5.0 decoder",
      format_name: "Indeo 5.0",
      fourccs:     (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '5', '0'), 0x00 },
      dll_name:    "ir50_32.dll",
      type:        CODEC_DS,
      guid:        { 0x30355649, 0, 16,{ 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71 } }
    },
    {
      name:        "Win32 Indeo 5.0 decoder",
      format_name: "Indeo 5.0",
      fourccs:     (uint32_t[]){ BGAV_MK_FOURCC('I', 'V', '5', '0'), 0x00 },
      dll_name:    "ir50_32.dll",
      type:        CODEC_STD,
    },
    {
      name:        "Win32 VP4 decoder",
      format_name: "VP4",
      fourccs:     (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '4', '0'), 0x00 },
      dll_name:    "vp4vfw.dll",
      type:        CODEC_STD,
      ex_functions: 1,
    },
    {
      name:        "Win32 VP5 decoder",
      format_name: "VP5",
      fourccs:     (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '5', '0'), 0x00 },
      dll_name:    "vp5vfw.dll",
      type:        CODEC_STD,
      ex_functions: 1,
    },
    {
      name:        "Win32 VP6 decoder",
      format_name: "VP6",
      fourccs:     (uint32_t[]){ BGAV_MK_FOURCC('V', 'P', '6', '0'),
                                 BGAV_MK_FOURCC('V', 'P', '6', '2'), 
                                 0x00 },
      dll_name:    "vp6vfw.dll",
      type:        CODEC_STD,
      ex_functions: 1,
    },
#if 0
    {
      name:        "DMO WMV9 decoder",
      format_name: "WMV9",
      fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '3'), 0x00 },
      dll_name: "wmv9dmod.dll",
      type:     CODEC_DMO,
      guid:     { 0x724bb6a4, 0xe526, 0x450f,
                  {0xaf, 0xfa, 0xab, 0x9b, 0x45, 0x12, 0x91, 0x11} },
    },
#else
    {
      name:        "DMO WMV9 decoder",
      format_name: "WMV9",
      fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('W', 'M', 'V', '3'), 0x00 },
      dll_name: "wmvdmod.dll",
      type:     CODEC_DMO,
      guid:     { 0x82d353df, 0x90bd, 0x4382,
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
  fprintf(stderr, "BITMAPINFOHEADER:\n");
  fprintf(stderr, "  biSize: %ld\n", bh->biSize); /* sizeof(BITMAPINFOHEADER) */
  fprintf(stderr, "  biWidth: %ld\n", bh->biWidth);
  fprintf(stderr, "  biHeight: %ld\n", bh->biHeight);
  fprintf(stderr, "  biPlanes: %d\n", bh->biPlanes);
  fprintf(stderr, "  biBitCount: %d\n", bh->biBitCount);
  fourcc_be = swap_endian(bh->biCompression);
  fprintf(stderr, "  biCompression: ");
  bgav_dump_fourcc(fourcc_be);
  fprintf(stderr, "\n");
  fprintf(stderr, "  biSizeImage: %ld\n", bh->biSizeImage);
  fprintf(stderr, "  biXPelsPerMeter: %ld\n", bh->biXPelsPerMeter);
  fprintf(stderr, "  biYPelsPerMeter: %ld\n", bh->biXPelsPerMeter);
  fprintf(stderr, "  biClrUsed: %ld\n", bh->biClrUsed);
  fprintf(stderr, "  biClrImportant: %ld\n", bh->biClrImportant);

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
  return (codec_info_t *)0;
  }

typedef struct
  {
  gavl_video_frame_t * frame;
  BITMAPINFOHEADER bih_in;
  BITMAPINFOHEADER bih_out;
  ldt_fs_t * ldt_fs;
  
  /* Standard win32 */

  HIC hic;
  int ex_functions;

  /* Direct Show */
  
  DS_VideoDecoder  *ds_dec;

  /* dmo */
  
  DMO_VideoDecoder *dmo_dec;

  int bytes_per_pixel;
  
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

static int init_std(bgav_stream_t * s)
  {
  int old_bit_count;
  HRESULT result;
  codec_info_t * info;
  win32_priv_t * priv;
  bgav_BITMAPINFOHEADER_t bih_in;
  bgav_BITMAPINFOHEADER_t bih_out;

  fprintf(stderr, "OPEN VIDEO\n");
  priv = calloc(1, sizeof(*priv));
  priv->ldt_fs = Setup_LDT_Keeper();

  info = find_decoder(s->data.video.decoder->decoder);

  priv->ex_functions = info->ex_functions;
    
  bgav_BITMAPINFOHEADER_set_format(&bih_in, s);
  //  bih_in.biCompression = 0x6f766976;
  //  bih_in.biCompression = s->fourcc;
  //  bih_in.biCompression = BGAV_MK_FOURCC('V','P','6','2');
  priv->hic = ICOpen ((int)(info->dll_name), bih_in.biCompression,
                      ICMODE_FASTDECOMPRESS);
  
  if(!priv->hic)
    {
    fprintf(stderr, "Cannot open %s\n", info->dll_name);
    return 0;
    }

  pack_bih(&priv->bih_in, &bih_in);
  
  result = ICDecompressGetFormat(priv->hic, &priv->bih_in, &priv->bih_out);

  unpack_bih(&bih_out, &priv->bih_out);
  
  if(result < 0)
    {
    fprintf(stderr, "Cannot get format\n");
    //    bgav_BITMAPINFOHEADER_dump(&bih_in);
    }

  fprintf(stderr, "Input Format:");
  bgav_BITMAPINFOHEADER_dump(&bih_in);
  fprintf(stderr, "Output Format:");
  bgav_BITMAPINFOHEADER_dump(&bih_out);
    
  s->data.video.decoder->priv = priv;

  switch(bih_out.biCompression)
    {
    case 0:
      break;
    default:
      fprintf(stderr, "Warning: Unsupported Colorspace\n");
      bgav_dump_fourcc(bih_out.biCompression);
      return 0;
      
    }

  /* Check if we can output YUV data */

  old_bit_count = bih_out.biBitCount;
  
  bih_out.biCompression = BGAV_MK_FOURCC('2', 'Y', 'U', 'Y');
  bih_out.biSizeImage = bih_out.biWidth * bih_out.biHeight * 2;
  bih_out.biBitCount  = 16;
  pack_bih(&priv->bih_out, &bih_out);
  
  result = (!priv->ex_functions) 
    ?ICDecompressQuery(priv->hic,   &priv->bih_in, &priv->bih_out)
    :ICDecompressQueryEx(priv->hic, &priv->bih_in, &priv->bih_out);

  if(result)
    {
    fprintf(stderr, "No YUV output possible, switching to RGB\n");
    bih_out.biCompression = 0;
    bih_out.biBitCount  = old_bit_count;
    
    switch(bih_out.biBitCount)
      {
      case 24:
        s->data.video.format.pixelformat = GAVL_RGB_24;
        fprintf(stderr, "Using RGB24 output\n");
        priv->bytes_per_pixel = 3;
        break;
      case 16:
        s->data.video.format.pixelformat = GAVL_RGB_15;
        fprintf(stderr, "Using RGB16 output\n");
        priv->bytes_per_pixel = 2;
        break;
      default:
        fprintf(stderr, "Warning: Unsupported depth %d\n",
                bih_out.biBitCount);
        return 0;
      }
    bih_out.biSizeImage = bih_out.biWidth * bih_out.biHeight * priv->bytes_per_pixel;
    
    pack_bih(&priv->bih_out, &bih_out);
    }
  else
    {
    fprintf(stderr, "Decoder supports YUY2 output\n");
    //    unpack_bih(&bih_out, &priv->bih_out);
    //    bgav_BITMAPINFOHEADER_dump(&bih_out);
    s->data.video.format.pixelformat = GAVL_YUY2;
    priv->bytes_per_pixel = 2;
    }

  /* Initialize decompression */

  result = (!priv->ex_functions) 
    ?ICDecompressBegin(priv->hic, &priv->bih_in, &priv->bih_out)
    :ICDecompressBeginEx(priv->hic, &priv->bih_in, &priv->bih_out);

  if(result)
    {
    fprintf(stderr, "ICDecompressBegin failed\n");
    return 0;
    }
  priv->frame = gavl_video_frame_create(&(s->data.video.format));
  Restore_LDT_Keeper(priv->ldt_fs);

  s->description = bgav_strndup(info->format_name, (char*)0);

  fprintf(stderr, "OPEN VIDEO DONE\n");

  return 1;
  }

static int decode_std(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  uint32_t flags;
  HRESULT result;
  win32_priv_t * priv;
  bgav_packet_t * p;
    
  priv = (win32_priv_t*)(s->data.video.decoder->priv);
  priv->ldt_fs = Setup_LDT_Keeper();

  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;
  
  flags = 0;

  if(!p->keyframe)
    flags |= ICDECOMPRESS_NOTKEYFRAME;

  if(!frame)
    flags |= ICDECOMPRESS_HURRYUP|ICDECOMPRESS_PREROL;

  //  fprintf(stderr, "ICDecompress %d\n", p->data_size);
  priv->bih_in.biSizeImage = p->data_size;

  priv->bih_out.biSizeImage = s->data.video.format.image_height * priv->frame->strides[0];
  priv->bih_out.biWidth     = priv->frame->strides[0] / priv->bytes_per_pixel;
#if 0
  fprintf(stderr, "ICDecompress\n");
  dump_bi(&priv->bih_in);
  dump_bi(&priv->bih_out);
#endif
  result = (!priv->ex_functions)
    ?ICDecompress(priv->hic, flags,
                  &priv->bih_in, p->data, &priv->bih_out, 
                  priv->frame->planes[0])
    :ICDecompressEx(priv->hic, flags,
                  &priv->bih_in, p->data, &priv->bih_out, 
                    priv->frame->planes[0]);
    
  if(result)
    {
    fprintf(stderr, "ICDecompress failed\n");
    }
  if(frame)
    {
    if(gavl_pixelformat_is_rgb(s->data.video.format.pixelformat))
      {
      /* RGB pixelformats are upside down normally */
      gavl_video_frame_copy_flip_y(&s->data.video.format, frame, priv->frame);
      }
    else
      gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    }
  bgav_demuxer_done_packet_read(s->demuxer, p);
  
  Restore_LDT_Keeper(priv->ldt_fs);
  return 1;
  }

static void resync_std(bgav_stream_t * s)
  {

  }

static void close_std(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  priv = (win32_priv_t*)(s->data.video.decoder->priv);
  priv->ldt_fs = Setup_LDT_Keeper();

  if(priv->hic)
    {
    ICDecompressEnd(priv->hic);
    ICClose(priv->hic);
    }
  
  Restore_LDT_Keeper(priv->ldt_fs);
  gavl_video_frame_destroy(priv->frame);
  free(priv);
  }

/* Direct show */

static int init_ds(bgav_stream_t * s)
  {
  //  HRESULT result;
  uint8_t * init_data;
  codec_info_t * info;
  win32_priv_t * priv;
  bgav_BITMAPINFOHEADER_t bih_in;
  //  bgav_BITMAPINFOHEADER_t bih_out;

  priv = calloc(1, sizeof(*priv));
  priv->ldt_fs = Setup_LDT_Keeper();

  info = find_decoder(s->data.video.decoder->decoder);
    
  bgav_BITMAPINFOHEADER_set_format(&bih_in, s);
  //  bgav_BITMAPINFOHEADER_dump(&bih_in);
  pack_bih(&priv->bih_in, &bih_in);

  if(!s->ext_size)
    priv->ds_dec = DS_VideoDecoder_Open(info->dll_name, &(info->guid),
                                          &priv->bih_in, 0 /* flipped */, 0);
  else
    {
    priv->bih_in.biSize = 40 + s->ext_size;
    init_data = malloc(40 + s->ext_size);
    memcpy(init_data, &priv->bih_in, 40);
    memcpy(&(init_data[40]), s->ext_data, s->ext_size);

    priv->ds_dec = DS_VideoDecoder_Open(info->dll_name, &(info->guid),
                                          (BITMAPINFOHEADER*)init_data, 0 /* flipped */, 0);
    free(init_data);
    }
  
  if(!priv->ds_dec)
    {
    fprintf(stderr, "DS_VideoDecoder_Open failed %d\n", s->ext_size);
    }

  //  capabilities = DS_VideoDecoder_GetCapabilities(priv->ds_dec);

  if(!DS_VideoDecoder_SetDestFmt(priv->ds_dec, 16, BGAV_MK_FOURCC('2', 'Y', 'U', 'Y')))
    {
    //    fprintf(stderr, "YUY2 output\n");
    s->data.video.format.pixelformat = GAVL_YUY2;
    }
  else /* RGB */
    {
    DS_VideoDecoder_SetDestFmt(priv->ds_dec, 24, 0);
    s->data.video.format.pixelformat = GAVL_RGB_24;
    }
  DS_VideoDecoder_StartInternal(priv->ds_dec);
  s->data.video.decoder->priv = priv;
  priv->frame = gavl_video_frame_create(&(s->data.video.format));
  Restore_LDT_Keeper(priv->ldt_fs);
  s->description = bgav_strndup(info->format_name, (char*)0);

  return 1;
  }

static int decode_ds(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  HRESULT result;
  win32_priv_t * priv;
  bgav_packet_t * p;

  priv = (win32_priv_t*)(s->data.video.decoder->priv);
  priv->ldt_fs = Setup_LDT_Keeper();

  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;
  
  //  fprintf(stderr, "Decode ds %d....", p->data_size);
  result = DS_VideoDecoder_DecodeInternal(priv->ds_dec, p->data, p->data_size,
                                           p->keyframe,
                                          (char*)(priv->frame->planes[0]));
  //  fprintf(stderr, "done\n");
  
  if(result)
    {
    fprintf(stderr, "Decode failed\n");
    }
  if(frame)
    {
    gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    }
  
  bgav_demuxer_done_packet_read(s->demuxer, p);
  

  Restore_LDT_Keeper(priv->ldt_fs);
  
  return 1;
  }

static void resync_ds(bgav_stream_t * s)
  {

  }

static void close_ds(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  priv = (win32_priv_t*)(s->data.video.decoder->priv);
  priv->ldt_fs = Setup_LDT_Keeper();

  if(priv->ds_dec )
    DS_VideoDecoder_Destroy(priv->ds_dec);
  
  Restore_LDT_Keeper(priv->ldt_fs);
  gavl_video_frame_destroy(priv->frame);
  free(priv);

  }

/* DMO */

static int init_dmo(bgav_stream_t * s)
  {
  //  HRESULT result;
  uint8_t * init_data;
  codec_info_t * info;
  win32_priv_t * priv;
  bgav_BITMAPINFOHEADER_t bih_in;
  //  bgav_BITMAPINFOHEADER_t bih_out;

  priv = calloc(1, sizeof(*priv));
  priv->ldt_fs = Setup_LDT_Keeper();

  info = find_decoder(s->data.video.decoder->decoder);
    
  bgav_BITMAPINFOHEADER_set_format(&bih_in, s);
  //  bgav_BITMAPINFOHEADER_dump(&bih_in);
  pack_bih(&priv->bih_in, &bih_in);

  if(!s->ext_size)
    priv->dmo_dec = DMO_VideoDecoder_Open(info->dll_name, &(info->guid),
                                          &priv->bih_in, 0 /* flipped */, 0);
  else
    {
    priv->bih_in.biSize = 40 + s->ext_size;
    init_data = malloc(40 + s->ext_size);
    memcpy(init_data, &priv->bih_in, 40);
    memcpy(&(init_data[40]), s->ext_data, s->ext_size);

    priv->dmo_dec = DMO_VideoDecoder_Open(info->dll_name, &(info->guid),
                                          (BITMAPINFOHEADER*)init_data, 0 /* flipped */, 0);
    free(init_data);
    }
  
  if(!priv->dmo_dec)
    {
    fprintf(stderr, "DMO_VideoDecoder_Open failed %d\n", s->ext_size);
    }

  //  capabilities = DMO_VideoDecoder_GetCapabilities(priv->dmo_dec);

  if(!DMO_VideoDecoder_SetDestFmt(priv->dmo_dec, 16, BGAV_MK_FOURCC('2', 'Y', 'U', 'Y')))
    {
    //    fprintf(stderr, "YUY2 output\n");
    s->data.video.format.pixelformat = GAVL_YUY2;
    }
  else /* RGB */
    {
    DMO_VideoDecoder_SetDestFmt(priv->dmo_dec, 24, 0);
    s->data.video.format.pixelformat = GAVL_RGB_24;
    }
  DMO_VideoDecoder_StartInternal(priv->dmo_dec);
  s->data.video.decoder->priv = priv;
  priv->frame = gavl_video_frame_create(&(s->data.video.format));
  Restore_LDT_Keeper(priv->ldt_fs);
  s->description = bgav_strndup(info->format_name, (char*)0);
  return 1;
  }

static int decode_dmo(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  HRESULT result;
  win32_priv_t * priv;
  bgav_packet_t * p;

  priv = (win32_priv_t*)(s->data.video.decoder->priv);
  priv->ldt_fs = Setup_LDT_Keeper();
  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;
  
  //  fprintf(stderr, "Decode dmo %d....", p->data_size);
  result = DMO_VideoDecoder_DecodeInternal(priv->dmo_dec, p->data, p->data_size,
                                           p->keyframe,
                                           (char*)(priv->frame->planes[0]));
  //  fprintf(stderr, "done\n");


  if(result)
    {
    fprintf(stderr, "Decode failed\n");
    }
  if(frame)
    {
    gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    }
  bgav_demuxer_done_packet_read(s->demuxer, p);
  Restore_LDT_Keeper(priv->ldt_fs);
  
  return 1;
  }

static void resync_dmo(bgav_stream_t * s)
  {

  }

static void close_dmo(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  priv = (win32_priv_t*)(s->data.video.decoder->priv);
  priv->ldt_fs = Setup_LDT_Keeper();

  if(priv->dmo_dec )
    DMO_VideoDecoder_Destroy(priv->dmo_dec);
    
  Restore_LDT_Keeper(priv->ldt_fs);
  gavl_video_frame_destroy(priv->frame);
  free(priv);
  }

int bgav_init_video_decoders_win32()
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
      
      switch(codec_infos[i].type)
        {
        case CODEC_STD:
          codecs[i].init   = init_std;
          codecs[i].decode = decode_std;
          codecs[i].close  = close_std;
          codecs[i].resync = resync_std;
          break;
        case CODEC_DS:
          codecs[i].init   = init_ds;
          codecs[i].decode = decode_ds;
          codecs[i].close  = close_ds;
          codecs[i].resync = resync_ds;
          break;
        case CODEC_DMO:
          codecs[i].init   = init_dmo;
          codecs[i].decode = decode_dmo;
          codecs[i].close  = close_dmo;
          codecs[i].resync = resync_dmo;
          break;
        }
      bgav_video_decoder_register(&codecs[i]);
      }
    else
      {
      fprintf(stderr, "Cannot find file %s, disabling %s\n",
              dll_filename, codec_infos[i].name);
      ret = 0;
      }
    }
  return ret;
  }
