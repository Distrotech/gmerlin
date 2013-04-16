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



#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include <avdec_private.h>
#include <codecs.h>

#include "libw32dll/qtx/qtxsdk/components.h"
#include "libw32dll/wine/windef.h"
#include "libw32dll/wine/ldt_keeper.h"

#include <semaphore.h>
#include <win32codec.h>

#define LOG_DOMAIN "qtwin32"

/*
 * This version of the macro avoids compiler warnings about
 * multiple-character constants. It also does NOT assume
 * that an unsigned long is 32 bits wide.
 */
#ifdef FOUR_CHAR_CODE
#  undef FOUR_CHAR_CODE
#endif
#define FOUR_CHAR_CODE BE_FOURCC


HMODULE   WINAPI LoadLibraryA(LPCSTR);
FARPROC   WINAPI GetProcAddress(HMODULE,LPCSTR);
int       WINAPI FreeLibrary(HMODULE);

typedef struct OpaqueSoundConverter*    SoundConverter;
typedef unsigned long                   UnsignedFixed;
typedef uint8_t                          Byte;

typedef struct SoundComponentData {
    long                            flags;
    OSType                          format;
    short                           numChannels;
    short                           sampleSize;
    UnsignedFixed                   sampleRate;
    long                            sampleCount;
    Byte *                          buffer;
    long                            reserved;
}SoundComponentData;

typedef int (__cdecl* LPFUNC1)(long flag);
typedef int (__cdecl* LPFUNC2)(const SoundComponentData *,
                               const SoundComponentData *,
                               SoundConverter *);
typedef int (__cdecl* LPFUNC3)(SoundConverter sc);
typedef int (__cdecl* LPFUNC4)(void);
typedef int (__cdecl* LPFUNC5)(SoundConverter sc, OSType selector,void * infoPtr);
typedef int (__cdecl* LPFUNC6)(SoundConverter sc,
                               unsigned long inputBytesTarget,
                               unsigned long *inputFrames,
                               unsigned long *inputBytes,
                               unsigned long *outputBytes );
typedef int (__cdecl* LPFUNC7)(SoundConverter sc,
                               const void    *inputPtr,
                               unsigned long inputFrames,
                               void          *outputPtr,
                               unsigned long *outputFrames,
                               unsigned long *outputBytes );
typedef int (__cdecl* LPFUNC8)(SoundConverter sc,
                               void      *outputPtr,
                               unsigned long *outputFrames,
                               unsigned long *outputBytes);
typedef int (__cdecl* LPFUNC9)(SoundConverter sc) ;

extern char            *win32_def_path;

typedef struct 
  {
  HINSTANCE           qtml_dll;
  HINSTANCE           qts_dll;
  LPFUNC1             InitializeQTML;
  LPFUNC2             SoundConverterOpen;
  LPFUNC3             SoundConverterClose;
  LPFUNC4             TerminateQTML;
  LPFUNC5             SoundConverterSetInfo;
  LPFUNC6             SoundConverterGetBufferSizes;
  LPFUNC7             SoundConverterConvertBuffer;
  LPFUNC8             SoundConverterEndConversion;
  LPFUNC9             SoundConverterBeginConversion;

  SoundConverter      myConverter;
  SoundComponentData  InputFormatInfo, OutputFormatInfo;

  int                 InFrameSize;
  int                 OutFrameSize;
  unsigned long       FramesToGet;

  ldt_fs_t *ldt_fs;

  char    * in_buffer;
  int in_buffer_size;
  int in_buffer_alloc;
  
  int16_t * out_buffer;
  gavl_audio_frame_t * frame;

  } qta_priv_t;

static int init_qtaudio(bgav_stream_t * s)
  {
  unsigned long InputBufferSize=0;  /* size of the input buffer */
  unsigned long OutputBufferSize=0; /* size of the output buffer */
  unsigned long WantedBufferSize=0; /* the size you want your buffers to be */
  int result;
  qta_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  bgav_windll_lock();
  priv->ldt_fs = Setup_LDT_Keeper();

  //preload quicktime.qts to avoid the problems caused by the hardcoded path inside the dll
  priv->qts_dll = LoadLibraryA("QuickTime.qts");
  if(!priv->qts_dll)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Cannot open QuickTime.qts");
    goto fail;
    }
  
  priv->qtml_dll = LoadLibraryA("qtmlClient.dll");
  if(!priv->qtml_dll)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Cannot open qtmlClient.dll");
    goto fail;
    }

  priv->InitializeQTML =
    (LPFUNC1)GetProcAddress (priv->qtml_dll, "InitializeQTML");
  if ( priv->InitializeQTML == NULL )
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, 
	     "Getting proc address InitializeQTML failed");
    goto fail;
    }
  priv->SoundConverterOpen =
    (LPFUNC2)GetProcAddress (priv->qtml_dll, "SoundConverterOpen");
  if ( priv->SoundConverterOpen == NULL )
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, 
	     "Getting proc address SoundConverterOpen failed");
    goto fail;
    }
  
  priv->SoundConverterClose =
    (LPFUNC3)GetProcAddress (priv->qtml_dll, "SoundConverterClose");
  if ( priv->SoundConverterClose == NULL )
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, 
	     "Getting proc address SoundConverterClose failed");
    goto fail;
    }
  priv->TerminateQTML =
    (LPFUNC4)GetProcAddress (priv->qtml_dll, "TerminateQTML");
  if ( priv->TerminateQTML == NULL )
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, 
	     "Getting proc address TerminateQTML failed");
    goto fail;
    }
  priv->SoundConverterSetInfo =
    (LPFUNC5)GetProcAddress (priv->qtml_dll, "SoundConverterSetInfo");
  if ( priv->SoundConverterSetInfo == NULL )
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, 
	     "Getting proc address SoundConverterSetInfo failed");
    goto fail;
    }
  priv->SoundConverterGetBufferSizes =
    (LPFUNC6)GetProcAddress (priv->qtml_dll, "SoundConverterGetBufferSizes");
  if ( priv->SoundConverterGetBufferSizes == NULL )
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, 
	     "Getting proc address SoundConverterGetBufferSizes failed");
    goto fail;
    }
  priv->SoundConverterConvertBuffer =
    (LPFUNC7)GetProcAddress (priv->qtml_dll, "SoundConverterConvertBuffer");
  if ( priv->SoundConverterConvertBuffer == NULL )
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, 
	     "Getting proc address SoundConverterConvertBuffer1 failed");
    goto fail;
    }
  priv->SoundConverterEndConversion =
    (LPFUNC8)GetProcAddress (priv->qtml_dll, "SoundConverterEndConversion");
  if ( priv->SoundConverterEndConversion == NULL )
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, 
	     "Getting proc address SoundConverterEndConversion failed");
    goto fail;
    }
  priv->SoundConverterBeginConversion =
    (LPFUNC9)GetProcAddress (priv->qtml_dll, "SoundConverterBeginConversion");
  if ( priv->SoundConverterBeginConversion == NULL )
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, 
	     "Getting proc address SoundConverterBeginConversion failed");
    goto fail;
    }

  if(priv->InitializeQTML(6+16))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "InitializeQTML failed");
    goto fail;
    }

  priv->OutputFormatInfo.flags       = priv->InputFormatInfo.flags       = 0;
  priv->OutputFormatInfo.sampleCount = priv->InputFormatInfo.sampleCount = 0;
  priv->OutputFormatInfo.buffer      = priv->InputFormatInfo.buffer      = NULL;
  priv->OutputFormatInfo.reserved    = priv->InputFormatInfo.reserved    = 0;
  priv->OutputFormatInfo.numChannels = priv->InputFormatInfo.numChannels =
    s->data.audio.format.num_channels;
  priv->InputFormatInfo.sampleSize   =
    s->data.audio.bits_per_sample;
  priv->OutputFormatInfo.sampleSize  = 16;
  priv->OutputFormatInfo.sampleRate  = priv->InputFormatInfo.sampleRate  =
    s->data.audio.format.samplerate;
  
  priv->OutputFormatInfo.format = BGAV_MK_FOURCC('N','O','N','E');
  priv->InputFormatInfo.format = s->fourcc;

  switch(s->fourcc)
    {
    case BGAV_MK_FOURCC('Q','D','M','C'):
      s->description = gavl_strdup("QDMC");
      break;
    case BGAV_MK_FOURCC('Q','D','M','2'):
      s->description = gavl_strdup("QDM2");
      break;
    case BGAV_MK_FOURCC('Q','c','l','p'):
      s->description = gavl_strdup("Qclp");
      break;
    case BGAV_MK_FOURCC('M','A','C','6'):
      s->description = gavl_strdup("MACE 6");
      break;
    }

  if(priv->SoundConverterOpen (&priv->InputFormatInfo, 
                               &priv->OutputFormatInfo, 
                               &priv->myConverter))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "SoundConverterOpen failed");
    goto fail;
    }

  if(s->ext_size > 0)
    {
    if(priv->SoundConverterSetInfo(priv->myConverter,
                                   BGAV_MK_FOURCC('w','a','v','e'),
                                   s->ext_data))
      {
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "SoundConverterSetInfo failed");
      goto fail;
      }
    }
  
  WantedBufferSize =
    priv->OutputFormatInfo.numChannels*priv->OutputFormatInfo.sampleRate*2;
  
  result =
    priv->SoundConverterGetBufferSizes(priv->myConverter,
                                       WantedBufferSize, &priv->FramesToGet,
                                       &InputBufferSize, &OutputBufferSize);
  priv->InFrameSize   = (InputBufferSize+priv->FramesToGet-1)/priv->FramesToGet;
  priv->OutFrameSize  = OutputBufferSize/priv->FramesToGet;
  
  if(priv->SoundConverterBeginConversion (priv->myConverter))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "SoundConverterBeginConversion failed");
    goto fail;
    }
  
  s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
  s->data.audio.format.samples_per_frame = 1024;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.decoder->priv = priv;
  priv->frame = gavl_audio_frame_create(NULL);
  priv->out_buffer = malloc(1024*1024);
  priv->frame->samples.s_16 = priv->out_buffer;

  gavl_set_channel_setup(&s->data.audio.format);
  //  Restore_LDT_Keeper(priv->ldt_fs);
  bgav_windll_unlock();

  return 1;
  fail:
  bgav_windll_unlock();
  return 0;
  }

static int read_data(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  qta_priv_t * priv = s->data.audio.decoder->priv;
  p = bgav_stream_get_packet_read(s);
  if(!p)
    {
    return 0;
    }

  if(p->data_size + priv->in_buffer_size > priv->in_buffer_alloc)
    {
    priv->in_buffer_alloc = p->data_size + priv->in_buffer_size;
    priv->in_buffer = realloc(priv->in_buffer, priv->in_buffer_alloc);
    }
  memcpy(priv->in_buffer + priv->in_buffer_size, p->data, p->data_size);
  priv->in_buffer_size += p->data_size;
  bgav_stream_done_packet_read(s, p);
  return 1;
  }

static int decode_frame_qtaudio(bgav_stream_t * s)
  {
  int num_frames;
  unsigned long out_frames, out_bytes;
  qta_priv_t * priv = s->data.audio.decoder->priv;
  //  priv->ldt_fs = Setup_LDT_Keeper();
    
  while(priv->in_buffer_size < priv->InFrameSize)
    {
    if(!read_data(s))
      return 0;
    }
  Check_FS_Segment();
  bgav_windll_lock();
  
  num_frames = priv->in_buffer_size / priv->InFrameSize;

  if(priv->SoundConverterConvertBuffer(priv->myConverter,
                                       priv->in_buffer,
                                       num_frames, 
                                       priv->out_buffer,
                                       &out_frames, &out_bytes))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "SoundConverterConvertBuffer failed");
    bgav_windll_unlock();
    return 0;
    }
  priv->frame->valid_samples = out_bytes / (2 * s->data.audio.format.num_channels);
  
  priv->in_buffer_size -= priv->InFrameSize * num_frames;
  if(priv->in_buffer_size > 0)
    memmove(priv->in_buffer, priv->in_buffer + num_frames * priv->InFrameSize, priv->in_buffer_size);
  //  Restore_LDT_Keeper(priv->ldt_fs);
  bgav_windll_unlock();

  gavl_audio_frame_copy_ptrs(&s->data.audio.format,
                             s->data.audio.frame, priv->frame);
  
  return 1;
  }

static void close_qtaudio(bgav_stream_t * s)
  {
  unsigned long ConvertedFrames=0;
  unsigned long ConvertedBytes=0;

  qta_priv_t * priv = s->data.audio.decoder->priv;

  bgav_windll_lock();
  
  //  priv->ldt_fs = Setup_LDT_Keeper();

  priv->SoundConverterEndConversion (priv->myConverter,NULL,
                                     &ConvertedFrames,&ConvertedBytes);
  priv->SoundConverterClose (priv->myConverter);

  FreeLibrary(priv->qtml_dll);
  Restore_LDT_Keeper(priv->ldt_fs);
  bgav_windll_unlock();

  free(priv->in_buffer);
  free(priv->out_buffer);
  gavl_audio_frame_null(priv->frame);
  gavl_audio_frame_destroy(priv->frame);

  free(priv);
  }

static void resync_qtaudio(bgav_stream_t * s)
  {
  qta_priv_t * priv = s->data.audio.decoder->priv;
  priv->in_buffer_size = 0;
  priv->frame->valid_samples = 0;
  }

static bgav_audio_decoder_t decoder =
  {
    .name =   "Win32 Quicktime audio decoder",
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('Q','D','M','C'),
                      BGAV_MK_FOURCC('Q','D','M','2'),
                      BGAV_MK_FOURCC('Q','c','l','p'),
                      //                      BGAV_MK_FOURCC('M','A','C','6'),
                      0x0 },
    
    .init =    init_qtaudio,
    .decode_frame =  decode_frame_qtaudio,
    .close =   close_qtaudio,
    .resync =  resync_qtaudio
  };

/* We won't work unless these files are there */

static char * needed_filenames[] =
  {
    "QuickTimeEssentials.qtx",
    "QuickTime.qts",
    "QuickTimeInternetExtras.qtx",
    "qtmlClient.dll"
  };

int bgav_init_audio_decoders_qtwin32(bgav_options_t * opt)
  {
  int i;
  char dll_filename[PATH_MAX];
  struct stat stat_buf;

  for(i = 0; i < sizeof(needed_filenames)/sizeof(needed_filenames[0]); i++)
    {
    sprintf(dll_filename, "%s/%s", win32_def_path, needed_filenames[i]);
    if(stat(dll_filename, &stat_buf))
      {
      bgav_log(opt, BGAV_LOG_WARNING, LOG_DOMAIN, "DLL %s not found",
               dll_filename);
      return 0;
      }
    }
  bgav_audio_decoder_register(&decoder);
  return 1;
  }
