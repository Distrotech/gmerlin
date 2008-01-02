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


#include <avdec_private.h>
#include <stdio.h>
#include <limits.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>

#include <codecs.h>

#define LOG_DOMAIN "video_realdll"

// static char codec_path[PATH_MAX];

char * bgav_dll_path_real = (char*)0;

static int init_real(bgav_stream_t * s);
static int decode_real(bgav_stream_t * s, gavl_video_frame_t * f);
static void close_real(bgav_stream_t * s);
static void resync_real(bgav_stream_t * s);

typedef struct
  {
  const char * dll_name;
  const char * format_name;
  bgav_video_decoder_t decoder;
  } codec_info_t;

static codec_info_t real_codecs[] =
  {
    {
      dll_name: "drvc.so",
      format_name: "Real Video 3.0/4.0",
      decoder:
      {
        name:   "Real Video 3.0/4.0 DLL decoder",
        fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('R', 'V', '3', '0'), 
                                BGAV_MK_FOURCC('R', 'V', '4', '0'),
                                0x00  },
        init:   init_real,
        decode: decode_real,
        close:  close_real,
        resync: resync_real,
      },
    },

  };

int bgav_init_video_decoders_real(bgav_options_t * opt)
  {
  int ret = 1;
  struct stat stat_buf;
  char test_filename[PATH_MAX];
  int i;

  /* REMOVE */
//  strcpy(codec_path, "/usr/lib/RealPlayer8/Codecs/");
  
  for(i = 0; i < sizeof(real_codecs) / sizeof(real_codecs[0]); i++)
    {
    sprintf(test_filename, "%s/%s", bgav_dll_path_real, real_codecs[i].dll_name);
    if(!stat(test_filename, &stat_buf))
      bgav_video_decoder_register(&real_codecs[i].decoder);
    else
      {
      bgav_log(opt, BGAV_LOG_WARNING, LOG_DOMAIN,
               "Cannot find file %s, disabling %s",
               test_filename, real_codecs[i].decoder.name);
      ret = 0;
      }
    }
  return ret;
  }

/*
 * Structures for data packets.  These used to be tables of unsigned ints, but
 * that does not work on 64 bit platforms (e.g. Alpha).  The entries that are
 * pointers get truncated.  Pointers on 64 bit platforms are 8 byte longs.
 * So we have to use structures so the compiler will assign the proper space
 * for the pointer.
 */
typedef struct cmsg_data_s {
        uint32_t data1;
        uint32_t data2;
        uint32_t* dimensions;
} cmsg_data_t;
                                                                                                                 
typedef struct transform_in_s {
        uint32_t len;
        uint32_t unknown1;
        uint32_t chunks;
        uint32_t* extra;
        uint32_t unknown2;
        uint32_t timestamp;
} transform_in_t;

typedef struct rv_init_t {
        short unk1;
        short w;
        short h;
        short unk3;
        int unk2;
        int subformat;
        int unk5;
        int format;
} rv_init_t;

typedef struct
  {
  unsigned long (*rvyuv_custom_message)(cmsg_data_t* ,void*);
  unsigned long (*rvyuv_free)(void*);
  unsigned long (*rvyuv_hive_message)(unsigned long,unsigned long);
  unsigned long (*rvyuv_init)(void*, void*); // initdata,context
  unsigned long (*rvyuv_transform)(char*, char*,transform_in_t*,unsigned int*,void*);
  void * module;
  void * real_context;

  gavl_video_frame_t * gavl_frame;
  
  } real_priv_t;

/* DLLs need this */

void *__builtin_vec_new(unsigned long size);
void *__builtin_new(unsigned long size);
void __builtin_delete(void *mem);
void __builtin_vec_delete(void *mem);
void __pure_virtual(void);

void *__builtin_vec_new(unsigned long size) {
        return malloc(size);
}
void *__builtin_new(unsigned long size) {
        return malloc(size);
}
void __builtin_vec_delete(void *mem) {
        free(mem);
}
void __pure_virtual(void) {
        printf("FATAL: __pure_virtual() called!\n");
//      exit(1);
}

void __builtin_delete(void *mem) {
        free(mem);
}


/* Initialize */
     
static int init_real(bgav_stream_t * s)
  {
  int i;
  unsigned int * extradata;
  uint32_t cmsg24[8];
  cmsg_data_t cmsg_data;
  
  char codec_filename[PATH_MAX];
  rv_init_t init_data;
  codec_info_t * info = (codec_info_t*)0;
  real_priv_t * priv;

  uint32_t version;
  
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;
  
  /* Check which codec we must open */

  for(i = 0; i < sizeof(real_codecs) / sizeof(real_codecs[0]); i++)
    {
    if(&(real_codecs[i].decoder) == s->data.video.decoder->decoder)
      {
      info = &real_codecs[i];
      break;
      }
    }
  if(!info)
    return 0;
  
  sprintf(codec_filename, "%s/%s", bgav_dll_path_real, info->dll_name);

  /* Try to dlopen it */

  if(!(priv->module = dlopen(codec_filename, RTLD_NOW)))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not open DLL %s %s", codec_filename, dlerror());
    return 0;
    }

  /* Get the symbols */

  priv->rvyuv_custom_message = dlsym(priv->module, "RV20toYUV420CustomMessage");
  priv->rvyuv_free = dlsym(priv->module, "RV20toYUV420Free");
  priv->rvyuv_hive_message = dlsym(priv->module, "RV20toYUV420HiveMessage");
  priv->rvyuv_init = dlsym(priv->module, "RV20toYUV420Init");
  priv->rvyuv_transform = dlsym(priv->module, "RV20toYUV420Transform");

  if(!priv->rvyuv_custom_message ||
     !priv->rvyuv_free ||
     !priv->rvyuv_hive_message ||
     !priv->rvyuv_init ||
     !priv->rvyuv_transform)
    {
    priv->rvyuv_custom_message = dlsym(priv->module, "RV40toYUV420CustomMessage");
    priv->rvyuv_free = dlsym(priv->module, "RV40toYUV420Free");
    priv->rvyuv_hive_message = dlsym(priv->module, "RV40toYUV420HiveMessage");
    priv->rvyuv_init = dlsym(priv->module, "RV40toYUV420Init");
    priv->rvyuv_transform = dlsym(priv->module, "RV40toYUV420Transform");
    }
  
  if(!priv->rvyuv_custom_message ||
     !priv->rvyuv_free ||
     !priv->rvyuv_hive_message ||
     !priv->rvyuv_init ||
     !priv->rvyuv_transform)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "DLL %s is not ok: %s", codec_filename, dlerror());
    return 0; 
    }
  
  extradata = (unsigned int*)(s->ext_data);

  init_data.unk1 = 11;
  init_data.w    = s->data.video.format.frame_width;
  init_data.h    = s->data.video.format.frame_height;
  init_data.unk3 = 0;
  init_data.unk2 = 0;
  init_data.subformat = extradata[0];
  init_data.unk5 = 1;
  init_data.format = extradata[1];

  if(priv->rvyuv_init(&init_data, &(priv->real_context)))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Init codec failed");
    return 0;
    }

  s->data.video.format.pixelformat = GAVL_YUV_420_P;

  priv->gavl_frame = gavl_video_frame_create_nopad(&s->data.video.format);
  
  version = ((s->fourcc & 0x000000FF) - '0') +
    (((s->fourcc & 0x0000FF00) >> 8)- '0') * 10;

  if((version <= 30) && (extradata[1]>=0x20200002))
    {
    cmsg24[0] = s->data.video.format.frame_width;
    cmsg24[1] = s->data.video.format.frame_height;
    cmsg24[2] = s->ext_data[8]*4;
    cmsg24[3] = s->ext_data[9]*4;
    cmsg24[4] = s->ext_data[10]*4;
    cmsg24[5] = s->ext_data[11]*4;
    cmsg24[6] = s->ext_data[12]*4;
    cmsg24[7] = s->ext_data[13]*4;
    
    cmsg_data.data1 = 0x24;
    cmsg_data.data2 = 1+((extradata[0]>>16)&7);
    cmsg_data.dimensions = &(cmsg24[0]);
    if(priv->rvyuv_custom_message(&cmsg_data,priv->real_context))
      {
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "rvyuv_custom_message failed");
      return 0;
      }
    }
  
  s->description = bgav_sprintf("%s", info->format_name);
  
  return 1;
  };

// copypaste from demux_real.c - it should match to get it working!
typedef struct dp_hdr_s {
    uint32_t chunks;    // number of chunks
    uint32_t timestamp; // timestamp from packet header
    uint32_t len;       // length of actual data
    uint32_t chunktab;  // offset to chunk offset array
} dp_hdr_t;

static int decode_real(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  real_priv_t * priv;
  bgav_packet_t * p;
  unsigned int transform_out[5];
  transform_in_t transform_in;
  uint32_t * extra;
  dp_hdr_t* dp_hdr;
  char * dp_data;

  priv = (real_priv_t *)(s->data.video.decoder->priv);
  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);

  
  if(!p)
    return 0;
    
  dp_hdr = (dp_hdr_t*)(p->data);
  extra = (uint32_t*)(((char*)(p->data))+dp_hdr->chunktab);
  dp_data=((char*)(p->data))+sizeof(dp_hdr_t);
  
  transform_in.len      = dp_hdr->len;
  transform_in.unknown1 = 0;
  transform_in.chunks   = dp_hdr->chunks;
  transform_in.extra    = extra;
  transform_in.unknown2 = 0;
  transform_in.timestamp = dp_hdr->timestamp;

  if(priv->rvyuv_transform(dp_data, (char*)(priv->gavl_frame->planes[0]), &transform_in,
                           transform_out, priv->real_context))
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Decoding failed");
    }
  if(f)
    gavl_video_frame_copy(&(s->data.video.format), f, priv->gavl_frame);
  bgav_demuxer_done_packet_read(s->demuxer, p);
  
  return 1;  
  };

static void close_real(bgav_stream_t * s)
  {
  real_priv_t * priv;
  priv = (real_priv_t *)(s->data.video.decoder->priv);
  
  if(priv->gavl_frame)
    gavl_video_frame_destroy(priv->gavl_frame);
  priv->rvyuv_free(priv->real_context);
  dlclose(priv->module);
  free(priv);
  };

static void resync_real(bgav_stream_t * s)
  {
  
  };
