/*****************************************************************
 
  audio_qtwin32.c
 
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



#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>


#include "libw32dll/qtx/qtxsdk/components.h"
#include "libw32dll/wine/windef.h"
#include "libw32dll/wine/ldt_keeper.h"
/*
 * This version of the macro avoids compiler warnings about
 * multiple-character constants. It also does NOT assume
 * that an unsigned long is 32 bits wide.
 */
#ifdef FOUR_CHAR_CODE
#  undef FOUR_CHAR_CODE
#endif
#define FOUR_CHAR_CODE BE_FOURCC

extern void bgav_windll_lock();
extern void bgav_windll_unlock();

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
  long                FramesToGet;

  ldt_fs_t *ldt_fs;

  char    * in_buffer;
  int in_buffer_size;
  int in_buffer_alloc;
  
  int16_t * out_buffer;
  gavl_audio_frame_t * frame;
  int last_block_size;
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
  
  priv->qtml_dll = LoadLibraryA("qtmlClient.dll");
  if(!priv->qtml_dll)
    {
    //    fprintf(stderr, "Cannot open qtmlClient.dll\n");
    goto fail;
    }

  priv->InitializeQTML =
    (LPFUNC1)GetProcAddress (priv->qtml_dll, "InitializeQTML");
  if ( priv->InitializeQTML == NULL )
    {
    fprintf(stderr, 
	     "qt_audio: failed geting proc address InitializeQTML\n");
    goto fail;
    }
  priv->SoundConverterOpen =
    (LPFUNC2)GetProcAddress (priv->qtml_dll, "SoundConverterOpen");
  if ( priv->SoundConverterOpen == NULL )
    {
    fprintf(stderr, 
	     "qt_audio: failed getting proc address SoundConverterOpen\n");
    goto fail;
    }
  
  priv->SoundConverterClose =
    (LPFUNC3)GetProcAddress (priv->qtml_dll, "SoundConverterClose");
  if ( priv->SoundConverterClose == NULL )
    {
    fprintf(stderr, 
	     "qt_audio: failed getting proc address SoundConverterClose\n");
    goto fail;
    }
  priv->TerminateQTML =
    (LPFUNC4)GetProcAddress (priv->qtml_dll, "TerminateQTML");
  if ( priv->TerminateQTML == NULL )
    {
    fprintf(stderr, 
	     "qt_audio: failed getting proc address TerminateQTML\n");
    goto fail;
    }
  priv->SoundConverterSetInfo =
    (LPFUNC5)GetProcAddress (priv->qtml_dll, "SoundConverterSetInfo");
  if ( priv->SoundConverterSetInfo == NULL )
    {
    fprintf(stderr, 
	     "qt_audio: failed getting proc address SoundConverterSetInfo\n");
    goto fail;
    }
  priv->SoundConverterGetBufferSizes =
    (LPFUNC6)GetProcAddress (priv->qtml_dll, "SoundConverterGetBufferSizes");
  if ( priv->SoundConverterGetBufferSizes == NULL )
    {
    fprintf(stderr, 
	     "qt_audio: failed getting proc address SoundConverterGetBufferSizes\n");
    goto fail;
    }
  priv->SoundConverterConvertBuffer =
    (LPFUNC7)GetProcAddress (priv->qtml_dll, "SoundConverterConvertBuffer");
  if ( priv->SoundConverterConvertBuffer == NULL )
    {
    fprintf(stderr, 
	     "qt_audio: failed getting proc address SoundConverterConvertBuffer1\n");
    goto fail;
    }
  priv->SoundConverterEndConversion =
    (LPFUNC8)GetProcAddress (priv->qtml_dll, "SoundConverterEndConversion");
  if ( priv->SoundConverterEndConversion == NULL )
    {
    fprintf(stderr, 
	     "qt_audio: failed getting proc address SoundConverterEndConversion\n");
    goto fail;
    }
  priv->SoundConverterBeginConversion =
    (LPFUNC9)GetProcAddress (priv->qtml_dll, "SoundConverterBeginConversion");
  if ( priv->SoundConverterBeginConversion == NULL )
    {
    fprintf(stderr, 
	     "qt_audio: failed getting proc address SoundConverterBeginConversion\n");
    goto fail;
    }

  if(priv->InitializeQTML(6+16))
    {
    fprintf(stderr, "InitializeQTML failed\n");
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
      s->description = bgav_strndup("QDMC", NULL);
      break;
    case BGAV_MK_FOURCC('Q','D','M','2'):
      s->description = bgav_strndup("QDM2", NULL);
      break;
    case BGAV_MK_FOURCC('Q','c','l','p'):
      s->description = bgav_strndup("Qclp", NULL);
      break;
    case BGAV_MK_FOURCC('M','A','C','6'):
      s->description = bgav_strndup("MACE 6", NULL);
      break;
    }

  if(priv->SoundConverterOpen (&priv->InputFormatInfo, 
                               &priv->OutputFormatInfo, 
                               &priv->myConverter))
    {
    fprintf(stderr, "SoundConverterOpen failed\n");
    goto fail;
    }

  if(s->ext_size > 0)
    {
    if(priv->SoundConverterSetInfo (priv->myConverter,
                                    BGAV_MK_FOURCC('w','a','v','e'),
                                    s->ext_data))
      {
      fprintf(stderr, "SoundConverterSetInfo failed\n");
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

  //  fprintf(stderr, "audio: SoundConverterGetBufferSizes:%i\n", result);
  //  fprintf(stderr, "audio: WantedBufferSize = %li\n", WantedBufferSize);
  //  fprintf(stderr, "audio: InputBufferSize  = %li\n", InputBufferSize);
  //  fprintf(stderr, "audio: OutputBufferSize = %li\n", OutputBufferSize);
  //  fprintf(stderr, "audio: priv->FramesToGet = %li\n", priv->FramesToGet);
  //  fprintf(stderr, "audio: FrameSize: %i -> %i\n", priv->InFrameSize, priv->OutFrameSize);

  if(priv->SoundConverterBeginConversion (priv->myConverter))
    {
    fprintf(stderr, "SoundConverterBeginConversion failed\n");
    goto fail;
    }
  
  s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
  s->data.audio.format.samples_per_frame = 1024;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.decoder->priv = priv;
  priv->frame = gavl_audio_frame_create(NULL);
  priv->out_buffer = malloc(1024*1024);
  priv->frame->samples.s_16 = priv->out_buffer;

  gavl_set_channel_setup(&(s->data.audio.format));
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
  qta_priv_t * priv = (qta_priv_t*)s->data.audio.decoder->priv;
  //  fprintf(stderr, "read data...");
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  //  fprintf(stderr, "read data 1");
  if(!p)
    {
    //    fprintf(stderr, " -> EOF\n");
    return 0;
    }
  //  fprintf(stderr, "Read %d bytes\n", p->data_size);

  if(p->data_size + priv->in_buffer_size > priv->in_buffer_alloc)
    {
    priv->in_buffer_alloc = p->data_size + priv->in_buffer_size;
    priv->in_buffer = realloc(priv->in_buffer, priv->in_buffer_alloc);
    }
  memcpy(priv->in_buffer + priv->in_buffer_size, p->data, p->data_size);
  priv->in_buffer_size += p->data_size;
  bgav_demuxer_done_packet_read(s->demuxer, p);
  //  fprintf(stderr, "read data done\n");
  return 1;
  }

static int decode(bgav_stream_t * s)
  {
  int num_frames;
  long out_frames, out_bytes;
  qta_priv_t * priv = (qta_priv_t*)s->data.audio.decoder->priv;
  //  fprintf(stderr, "decode qtwin32...");
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
    fprintf(stderr, "SoundConverterConvertBuffer failed\n");
    bgav_windll_unlock();
    return 0;
    }
  priv->frame->valid_samples = out_bytes / (2 * s->data.audio.format.num_channels);
  priv->last_block_size = priv->frame->valid_samples;

  //  fprintf(stderr, "Decoded %d frames, %d samples, used %d bytes\n", num_frames,
  //          priv->frame->valid_samples, num_frames * priv->InFrameSize);

  priv->in_buffer_size -= priv->InFrameSize * num_frames;
  if(priv->in_buffer_size > 0)
    memmove(priv->in_buffer, priv->in_buffer + num_frames * priv->InFrameSize, priv->in_buffer_size);
  //  Restore_LDT_Keeper(priv->ldt_fs);
  //  fprintf(stderr, "decode qtwin32 done\n");
  bgav_windll_unlock();
  return 1;
  }

static int decode_qtaudio(bgav_stream_t * s,
                          gavl_audio_frame_t * frame,
                          int num_samples)
  {
  int samples_decoded = 0;
  int samples_copied;
  //  fprintf(stderr, "Decode 1 %d\n", num_samples);
  qta_priv_t * priv = (qta_priv_t*)s->data.audio.decoder->priv;
  while(samples_decoded < num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      if(!decode(s))
        break;
      }
    
    samples_copied = gavl_audio_frame_copy(&(s->data.audio.format),
                                           frame,
                                           priv->frame,
                                           samples_decoded, /* out_pos */
                                           priv->last_block_size - priv->frame->valid_samples,  /* in_pos */
                                           num_samples - samples_decoded, /* out_size, */
                                           priv->frame->valid_samples /* in_size */);
    //    fprintf(stderr, "Decode 2 %d\n", samples_copied);
    
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  
  if(frame)
    frame->valid_samples = samples_decoded;
  return samples_decoded;
  }

static void close_qtaudio(bgav_stream_t * s)
  {
  unsigned long ConvertedFrames=0;
  unsigned long ConvertedBytes=0;

  qta_priv_t * priv = (qta_priv_t*)s->data.audio.decoder->priv;

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
  qta_priv_t * priv = (qta_priv_t*)s->data.audio.decoder->priv;
  priv->in_buffer_size = 0;
  priv->frame->valid_samples = 0;
  }

static bgav_audio_decoder_t decoder =
  {
    name:   "Win32 Quicktime audio decoder",
    fourccs: (int[]){ BGAV_MK_FOURCC('Q','D','M','C'),
                      BGAV_MK_FOURCC('Q','D','M','2'),
                      BGAV_MK_FOURCC('Q','c','l','p'),
                      //                      BGAV_MK_FOURCC('M','A','C','6'),
                      0x0 },
    
    init:    init_qtaudio,
    decode:  decode_qtaudio,
    close:   close_qtaudio,
    resync:  resync_qtaudio
  };

/* We won't work unless these files are there */

static char * needed_filenames[] =
  {
    "QuickTimeEssentials.qtx",
    "QuickTime.qts",
    "QuickTimeInternetExtras.qtx",
    "qtmlClient.dll"
  };

int bgav_init_audio_decoders_qtwin32()
  {
  int i;
  char dll_filename[PATH_MAX];
  struct stat stat_buf;

  for(i = 0; i < sizeof(needed_filenames)/sizeof(needed_filenames[0]); i++)
    {
    sprintf(dll_filename, "%s/%s", win32_def_path, needed_filenames[i]);
    if(stat(dll_filename, &stat_buf))
      {
      fprintf(stderr, "Cannot find file %s, disabling %s\n",
              dll_filename, decoder.name);
      return 0;
      }
    }
  bgav_audio_decoder_register(&decoder);
  return 1;
  }
