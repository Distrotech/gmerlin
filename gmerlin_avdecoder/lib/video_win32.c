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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <avdec_private.h>
#include <nanosoft.h>

#include <sys/types.h>
#include <sys/stat.h>


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


typedef struct
  {
  char * name;
  uint32_t * fourccs;
  char * dll_name;
  int type;
  GUID guid;
  int ex_functions;
  } codec_info_t;

static codec_info_t codec_infos[] =
  {
    {
      name:     "Win32 Indeo 3.2 decoder",
      fourccs:  (int[]){ BGAV_MK_FOURCC('I', 'V', '3', '2'), 0x00 },
      dll_name: "ir32_32.dll",
      type:     CODEC_STD,
    },
    {
      name:     "Win32 Indeo 4.1 decoder",
      fourccs:  (int[]){ BGAV_MK_FOURCC('I', 'V', '4', '1'), 0x00 },
      dll_name: "ir41_32.dll",
      type:     CODEC_STD,
    },
    {
      name:     "DirectShow Indeo 5.0 decoder",
      fourccs:  (int[]){ BGAV_MK_FOURCC('I', 'V', '5', '0'), 0x00 },
      dll_name: "ir50_32.dll",
      type:     CODEC_DS,
      guid:     { 0x30355649, 0, 16,{ 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71 } }
    },
    {
      name:     "Win32 Indeo 5.0 decoder",
      fourccs:  (int[]){ BGAV_MK_FOURCC('I', 'V', '5', '0'), 0x00 },
      dll_name: "ir50_32.dll",
      type:     CODEC_STD,
    },
    {
      name:         "Win32 VP4 decoder",
      fourccs:      (int[]){ BGAV_MK_FOURCC('V', 'P', '4', '0'), 0x00 },
      dll_name:     "vp4vfw.dll",
      type:         CODEC_STD,
      ex_functions: 1,
    },
    {
      name:     "DMO WMV9 decoder",
      fourccs:  (int[]){ BGAV_MK_FOURCC('W', 'M', 'V', '3'), 0x00 },
      dll_name: "wmv9dmod.dll",
      type:     CODEC_DMO,
      guid:     { 0x724bb6a4, 0xe526, 0x450f,
                  {0xaf, 0xfa, 0xab, 0x9b, 0x45, 0x12, 0x91, 0x11} },
    },
  };

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
  
  } win32_priv_t;

static void pack_bih(BITMAPINFOHEADER * dst, bgav_BITMAPINFOHEADER * src)
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

static void unpack_bih(bgav_BITMAPINFOHEADER * dst, BITMAPINFOHEADER * src)
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
  HRESULT result;
  codec_info_t * info;
  win32_priv_t * priv;
  bgav_BITMAPINFOHEADER bih_in;
  bgav_BITMAPINFOHEADER bih_out;
    
  priv = calloc(1, sizeof(*priv));
  priv->ldt_fs = Setup_LDT_Keeper();

  info = find_decoder(s->data.video.decoder->decoder);

  priv->ex_functions = info->ex_functions;
    
  bgav_BITMAPINFOHEADER_set_format(&bih_in, s);
  
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
  
  if(result)
    {
    fprintf(stderr, "Cannot get format\n");
    bgav_BITMAPINFOHEADER_dump(&bih_in);
    }

  fprintf(stderr, "Formats:");
  bgav_BITMAPINFOHEADER_dump(&bih_in);
  bgav_BITMAPINFOHEADER_dump(&bih_out);
    
  s->data.video.decoder->priv = priv;

  switch(bih_out.biCompression)
    {
    case 0:
      switch(bih_out.biBitCount)
        {
        case 24:
          s->data.video.format.colorspace = GAVL_RGB_24;
          break;
        default:
          fprintf(stderr, "Warning: Unsupported depth %d\n",
                  bih_out.biBitCount);
          return 0;
        }
      break;
    default:
      fprintf(stderr, "Warning: Unsupported Colorspace\n");
      bgav_dump_fourcc(bih_out.biCompression);
      return 0;
      
    }

  /* Check if we can output YUV data */

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
    bih_out.biSizeImage = bih_out.biWidth * bih_out.biHeight * 3;
    bih_out.biBitCount  = 24;
    pack_bih(&priv->bih_out, &bih_out);
    }
  else
    {
    fprintf(stderr, "Decoder supports YUY2 output\n");
    //    unpack_bih(&bih_out, &priv->bih_out);
    //    bgav_BITMAPINFOHEADER_dump(&bih_out);
    s->data.video.format.colorspace = GAVL_YUY2;
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
  return 1;
  }

static int decode_std(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  bgav_packet_t * p;
  uint32_t flags;
  HRESULT result;
  win32_priv_t * priv;

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
    gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    frame->time = p->timestamp;
    }
  bgav_demuxer_done_packet_read(s->demuxer, p);
  Restore_LDT_Keeper(priv->ldt_fs);
  return 1;
  }

static void clear_std(bgav_stream_t * s)
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
  bgav_BITMAPINFOHEADER bih_in;
  //  bgav_BITMAPINFOHEADER bih_out;

  priv = calloc(1, sizeof(*priv));
  priv->ldt_fs = Setup_LDT_Keeper();

  info = find_decoder(s->data.video.decoder->decoder);
    
  bgav_BITMAPINFOHEADER_set_format(&bih_in, s);
  bgav_BITMAPINFOHEADER_dump(&bih_in);
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
    fprintf(stderr, "YUY2 output\n");
    s->data.video.format.colorspace = GAVL_YUY2;
    }
  else /* RGB */
    {
    DS_VideoDecoder_SetDestFmt(priv->ds_dec, 24, 0);
    s->data.video.format.colorspace = GAVL_RGB_24;
    }
  DS_VideoDecoder_StartInternal(priv->ds_dec);
  s->data.video.decoder->priv = priv;
  priv->frame = gavl_video_frame_create(&(s->data.video.format));
  Restore_LDT_Keeper(priv->ldt_fs);
  return 1;
  }

static int decode_ds(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  bgav_packet_t * p;
  HRESULT result;
  win32_priv_t * priv;
  priv = (win32_priv_t*)(s->data.video.decoder->priv);
  priv->ldt_fs = Setup_LDT_Keeper();
  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;
  
  //  fprintf(stderr, "Decode ds %d....", p->data_size);
  result = DS_VideoDecoder_DecodeInternal(priv->ds_dec, p->data, p->data_size,
                                           p->keyframe,
                                           priv->frame->planes[0]);
  //  fprintf(stderr, "done\n");


  if(result)
    {
    fprintf(stderr, "Decode failed\n");
    }
  if(frame)
    {
    gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    frame->time = p->timestamp;
    }
  bgav_demuxer_done_packet_read(s->demuxer, p);
  Restore_LDT_Keeper(priv->ldt_fs);
  
  return 1;
  }

static void clear_ds(bgav_stream_t * s)
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
  bgav_BITMAPINFOHEADER bih_in;
  //  bgav_BITMAPINFOHEADER bih_out;

  priv = calloc(1, sizeof(*priv));
  priv->ldt_fs = Setup_LDT_Keeper();

  info = find_decoder(s->data.video.decoder->decoder);
    
  bgav_BITMAPINFOHEADER_set_format(&bih_in, s);
  bgav_BITMAPINFOHEADER_dump(&bih_in);
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
    fprintf(stderr, "YUY2 output\n");
    s->data.video.format.colorspace = GAVL_YUY2;
    }
  else /* RGB */
    {
    DMO_VideoDecoder_SetDestFmt(priv->dmo_dec, 24, 0);
    s->data.video.format.colorspace = GAVL_RGB_24;
    }
  DMO_VideoDecoder_StartInternal(priv->dmo_dec);
  s->data.video.decoder->priv = priv;
  priv->frame = gavl_video_frame_create(&(s->data.video.format));
  Restore_LDT_Keeper(priv->ldt_fs);
  return 1;
  }

static int decode_dmo(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  bgav_packet_t * p;
  HRESULT result;
  win32_priv_t * priv;
  priv = (win32_priv_t*)(s->data.video.decoder->priv);
  priv->ldt_fs = Setup_LDT_Keeper();
  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;
  
  fprintf(stderr, "Decode dmo %d....", p->data_size);
  result = DMO_VideoDecoder_DecodeInternal(priv->dmo_dec, p->data, p->data_size,
                                           p->keyframe,
                                           priv->frame->planes[0]);
  fprintf(stderr, "done\n");


  if(result)
    {
    fprintf(stderr, "Decode failed\n");
    }
  if(frame)
    {
    gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    frame->time = p->timestamp;
    }
  bgav_demuxer_done_packet_read(s->demuxer, p);
  Restore_LDT_Keeper(priv->ldt_fs);
  
  return 1;
  }

static void clear_dmo(bgav_stream_t * s)
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


void bgav_init_video_decoders_win32()
  {
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
      
      switch(codec_infos[i].type)
        {
        case CODEC_STD:
          codecs[i].init   = init_std;
          codecs[i].decode = decode_std;
          codecs[i].close = close_std;
          codecs[i].clear = clear_std;
          break;
        case CODEC_DS:
          codecs[i].init   = init_ds;
          codecs[i].decode = decode_ds;
          codecs[i].close = close_ds;
          codecs[i].clear = clear_ds;
          break;
        case CODEC_DMO:
          codecs[i].init   = init_dmo;
          codecs[i].decode = decode_dmo;
          codecs[i].close = close_dmo;
          codecs[i].clear = clear_dmo;
          break;
        }
      bgav_video_decoder_register(&codecs[i]);
      }

    }
  }
