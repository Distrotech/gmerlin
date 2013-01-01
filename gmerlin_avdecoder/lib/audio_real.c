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


#include <limits.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <avdec_private.h>
#include <codecs.h>
#include <utils.h>

// #define DUMP_EXTRADATA

static int init_real(bgav_stream_t * s);
static gavl_source_status_t decode_frame_real(bgav_stream_t * s);
static void close_real(bgav_stream_t * s);
static void resync_real(bgav_stream_t * s);
// static void skip_real(bgav_stream_t * s,int);

#define LOG_DOMAIN "audio_real"

typedef struct
  {
  const char * dll_name;
  const char * format_name;
  bgav_audio_decoder_t decoder;
  } codec_info_t;

static codec_info_t real_codecs[] =
  {
    {
      .dll_name = "sipr.so",
      .format_name = "Real Audio sipr",
      decoder:
      {
        .name =   "Real audio sipr DLL decoder",
        .fourccs =  (uint32_t[]){ BGAV_MK_FOURCC('s', 'i', 'p', 'r'), 0x00  },
        .init =   init_real,
        .decode_frame = decode_frame_real,
        .close =  close_real,
        .resync = resync_real,
      },
    },
#if 0 // Handled by ffmpeg
    {
      .dll_name = "cook.so",
      .format_name = "Real Audio cook",
      decoder:
      {
        .name =   "Real audio cook DLL decoder",
        .fourccs =  (uint32_t[]){ BGAV_MK_FOURCC('c', 'o', 'o', 'k'), 0x00  },
        .init =   init_real,
        .decode_frame = decode_frame_real,
        .close =  close_real,
        .resync = resync_real,
      },
    },
#endif
    {
      .dll_name = "atrc.so.6.0",
      .format_name = "Real Audio atrc",
      decoder:
      {
        .name =   "Real audio atrc DLL decoder",
        .fourccs =  (uint32_t[]){ BGAV_MK_FOURCC('a', 't', 'r', 'c'),
                                0x00  },
        .init =   init_real,
        .decode_frame = decode_frame_real,
        .close =  close_real,
        .resync = resync_real,
      },
      
    },
  };

int bgav_init_audio_decoders_real(bgav_options_t * opt)
  {
  int ret = 1;
  struct stat stat_buf;
  char test_filename[PATH_MAX];
  int i;

  for(i = 0; i < sizeof(real_codecs) / sizeof(real_codecs[0]); i++)
    {
    sprintf(test_filename, "%s/%s", bgav_dll_path_real, real_codecs[i].dll_name);
    if(!stat(test_filename, &stat_buf))
      bgav_audio_decoder_register(&real_codecs[i].decoder);
    else
      {
      
      
      ret = 0;
      }
    }
  return ret;
  }

typedef struct /*__attribute__((__packed__))*/ {
    int samplerate;
    short bits;
    short channels;
    short quality;
    /* 2bytes padding here, by gcc */
    int bits_per_frame;
    int packetsize;
    int extradata_len;
    void* extradata;
} ra_init_t;

#if 0
static void dump_init_data(ra_init_t * i)
  {
  bgav_dprintf("*** INIT DATA ******\n");
  bgav_dprintf("samplerate:     %d\n", i->samplerate);
  bgav_dprintf("bits:           %d\n", i->bits);
  bgav_dprintf("channels:       %d\n", i->channels);
  bgav_dprintf("quality:        %d\n", i->quality);
  bgav_dprintf("bits_per_frame: %d\n", i->bits_per_frame);
  bgav_dprintf("packetsize:     %d\n", i->packetsize);
  bgav_dprintf("extradata_len:  %d\n", i->extradata_len);
  bgav_hexdump(i->extradata, i->extradata_len, 16);
  bgav_dprintf("*** END INIT DATA **\n");
  }
#endif

typedef struct
  {
  unsigned long (*raCloseCodec)(void*);
  unsigned long (*raDecode)(void*, char*,unsigned long,char*,unsigned int*,long);
  unsigned long (*raFlush)(unsigned long,unsigned long,unsigned long);
  unsigned long (*raFreeDecoder)(void*);
  void*         (*raGetFlavorProperty)(void*,unsigned long,unsigned long,int*);
  //unsigned long (*raGetNumberOfFlavors2)(void);
  unsigned long (*raInitDecoder)(void*, void*);
  unsigned long (*raOpenCodec)(void*);
  unsigned long (*raOpenCodec2)(void*, void*);
  unsigned long (*raSetFlavor)(void*,unsigned long);
  void  (*raSetDLLAccessPath)(char*);
  void  (*raSetPwd)(char*,char*);

  void * module;
  void * real_handle;

  uint8_t * read_buffer;
  uint8_t * read_buffer_ptr;
  int read_buffer_alloc;
  int read_buffer_size;
  
  gavl_audio_frame_t * frame;
  } real_priv_t;


static int init_real(bgav_stream_t * s)
  {
  ra_init_t init_data;
  char codec_filename[PATH_MAX];
  real_priv_t * priv;
  int i;
  char * path;
  int len;
  void * prop;
  const codec_info_t * info = NULL;
    
  priv = calloc(1, sizeof(*priv));
  s->decoder_priv = priv;
  
  for(i = 0; i < sizeof(real_codecs) / sizeof(real_codecs[0]); i++)
    {
    if(&real_codecs[i].decoder == s->data.audio.decoder)
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

  priv->raCloseCodec = dlsym(priv->module, "RACloseCodec");
  priv->raDecode = dlsym(priv->module, "RADecode");
  priv->raFlush = dlsym(priv->module, "RAFlush");
  priv->raFreeDecoder = dlsym(priv->module, "RAFreeDecoder");
  priv->raGetFlavorProperty = dlsym(priv->module, "RAGetFlavorProperty");
  priv->raOpenCodec = dlsym(priv->module, "RAOpenCodec");
  priv->raOpenCodec2 = dlsym(priv->module, "RAOpenCodec2");
  priv->raInitDecoder = dlsym(priv->module, "RAInitDecoder");
  priv->raSetFlavor = dlsym(priv->module, "RASetFlavor");
  priv->raSetDLLAccessPath = dlsym(priv->module, "SetDLLAccessPath");
  priv->raSetPwd = dlsym(priv->module, "RASetPwd"); // optional, used by SIPR

  if(!(priv->raCloseCodec &&
       priv->raDecode &&
       priv->raFreeDecoder &&
       priv->raGetFlavorProperty &&
       (priv->raOpenCodec||priv->raOpenCodec2) &&
       priv->raSetFlavor &&
       priv->raInitDecoder))
    return 0;

  path= bgav_sprintf("DT_Codecs=%s", bgav_dll_path_real);
  if(path[strlen(path)-1]!='/')
    path = bgav_strncat(path, "/", NULL);

  /* Append one zero byte */

  path = realloc(path, strlen(path)+2);
  path[strlen(path)+1] = '\0';
  
  /* Set the codec path */

  if(priv->raSetDLLAccessPath)
    {
    // used by 'SIPR'
    priv->raSetDLLAccessPath(path);
    }

  if(priv->raOpenCodec2)
    {
    if(priv->raOpenCodec2(&priv->real_handle,&path[10]))
      {
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "raOpenCodec2 failed");
      return 0;
      }
    }
  else if(priv->raOpenCodec(priv->real_handle))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "raOpenCodec2 failed");
    return 0;
    }

  free(path);
  
  init_data.samplerate = s->data.audio.format.samplerate;
  init_data.bits       = s->data.audio.bits_per_sample;
  init_data.channels   = s->data.audio.format.num_channels;
  init_data.quality    = 100;
    /* 2bytes padding here, by gcc */
  init_data.bits_per_frame = s->data.audio.block_align;
  init_data.packetsize  =    s->data.audio.block_align;

  init_data.extradata_len =  s->ext_size;
  init_data.extradata =      s->ext_data;

#ifdef DUMP_EXTRADATA  
  bgav_dprintf("Extradata: %d bytes\n", s->ext_size);
  bgav_hexdump(s->ext_data, s->ext_size, 16);
#endif
  
  //  dump_init_data(&init_data);

  if(priv->raInitDecoder(priv->real_handle,&init_data))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "raInitDecoder failed");
    return 0;
    }
  if(priv->raSetPwd)
    {
    priv->raSetPwd(priv->real_handle,"Ardubancel Quazanga");
    }

  if(priv->raSetFlavor(priv->real_handle,s->subformat))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "raSetFlavor failed");
    return 0;
    }
  
  prop = priv->raGetFlavorProperty(priv->real_handle, s->subformat, 0, &len);
  
  if(prop)
    gavl_metadata_set_nocpy(&s->m, GAVL_META_FORMAT,
                            bgav_sprintf("%s (Flavor: %s)",
                                         info->format_name, (char*)prop));
  else
    gavl_metadata_set_nocpy(&s->m, GAVL_META_FORMAT,
                            bgav_sprintf("%s", info->format_name));
  
  //  prop = priv->raGetFlavorProperty(priv->real_handle, s->subformat, 1, &len);
  
  /* Allocate sample buffer and set audio format */
    
  
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.format.sample_format   = GAVL_SAMPLE_S16;
  gavl_set_channel_setup(&s->data.audio.format);

  s->data.audio.format.samples_per_frame = 10240;
  priv->frame = gavl_audio_frame_create(&s->data.audio.format);
  s->data.audio.format.samples_per_frame = 1024;
  
  return 1;
  }

#if 0
static unsigned char sipr_swaps[38][2]={
    {0,63},{1,22},{2,44},{3,90},{5,81},{7,31},{8,86},{9,58},{10,36},{12,68},
    {13,39},{14,73},{15,53},{16,69},{17,57},{19,88},{20,34},{21,71},{24,46},
    {25,94},{26,54},{28,75},{29,50},{32,70},{33,92},{35,74},{38,85},{40,56},
    {42,87},{43,65},{45,59},{48,79},{49,93},{51,89},{55,95},{61,76},{67,83},
    {77,80} };
#endif

static gavl_source_status_t fill_buffer(bgav_stream_t * s)
  {
  real_priv_t * priv;
  bgav_packet_t * p = NULL;
  gavl_source_status_t st;

  priv = s->decoder_priv;

  if((st = bgav_stream_get_packet_read(s, &p)) != GAVL_SOURCE_OK)
    return st;

  if(p->data_size > priv->read_buffer_alloc)
    {
    priv->read_buffer_alloc = p->data_size;
    priv->read_buffer = realloc(priv->read_buffer, priv->read_buffer_alloc);
    }

  memcpy(priv->read_buffer, p->data, p->data_size);
  bgav_stream_done_packet_read(s, p);

  priv->read_buffer_size = p->data_size;
  priv->read_buffer_ptr  = priv->read_buffer;
  
  return 1;
  }

static gavl_source_status_t decode_frame_real(bgav_stream_t * s)
  {
  unsigned int len;
  gavl_source_status_t st;
  real_priv_t * priv;
  priv = s->decoder_priv;
  if(!priv->read_buffer_size)
    {
    if((st = fill_buffer(s)) != GAVL_SOURCE_OK)
      return st;
    }
  /* Call the decoder */

  
  if(priv->raDecode(priv->real_handle, (char*)priv->read_buffer_ptr,
                    s->data.audio.block_align,
                    (char*)priv->frame->samples.s_8, &len, -1))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "raDecode failed");
    }
  priv->read_buffer_ptr += s->data.audio.block_align;
  priv->read_buffer_size -= s->data.audio.block_align;
  
  priv->frame->valid_samples = len / (2 * s->data.audio.format.num_channels);

  gavl_audio_frame_copy_ptrs(&s->data.audio.format, s->data.audio.frame, priv->frame);

  return GAVL_SOURCE_OK;
  }

static void close_real(bgav_stream_t * s)
  {
  real_priv_t * p = s->decoder_priv;

  if(p->frame)
    gavl_audio_frame_destroy(p->frame);
  
  if(p->read_buffer)
    free(p->read_buffer);
#if 1
  if(p->raFreeDecoder)
    p->raFreeDecoder(p->real_handle);
  if(p->raCloseCodec)
    p->raCloseCodec(p->real_handle);
#endif
  //  dlclose(p->module);
  free(p);
  }

static void resync_real(bgav_stream_t * s)
  {
  real_priv_t * p = s->decoder_priv;
  p->frame->valid_samples = 0;
  p->read_buffer_size = 0;
  }

