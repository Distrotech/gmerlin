/*****************************************************************
 
  audio_win32.c
 
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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>
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
      name:        "Voxware Metasound DirectShow decoder",
      format_name: "Voxware Metasound",
      fourccs:     (int[]){ BGAV_WAVID_2_FOURCC(0x75), 0x00 },
      dll_name:    "voxmsdec.ax",
      type:        CODEC_DS,
      guid:        { 0x73f7a062, 0x8829, 0x11d1,
                     { 0xb5, 0x50, 0x00, 0x60, 0x97, 0x24, 0x2d, 0x8d } }
    },
    {
      name:        "ACELP.net DirectShow decoder",
      format_name: "ACELP.net",
      fourccs:     (int[]){ BGAV_WAVID_2_FOURCC(0x0130), 0x00 },
      dll_name:    "acelpdec.ax",
      type:        CODEC_DS,
      guid:        { 0x4009f700, 0xaeba, 0x11d1,
                     { 0x83, 0x44, 0x00, 0xc0, 0x4f, 0xb9, 0x2e, 0xb7 } }
    },
    {
      name:        "msgsm ACM decoder",
      format_name: "msgsm",
      fourccs:     (int[]){ BGAV_WAVID_2_FOURCC(0x0031), 0x00 },
      dll_name:    "msgsm32.acm",
      type:        CODEC_STD,
    },
#if 1
    {
      name:        "Vivo G.723/Siren Audio Codec",
      format_name: "Vivo G.723/Siren",
      fourccs:     (int[]){ BGAV_WAVID_2_FOURCC(0x0111),
                            BGAV_WAVID_2_FOURCC(0x0112),
                            0x00 },
      dll_name:    "vivog723.acm",
      type:        CODEC_STD,
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

  int last_frame_size;

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
  fprintf(stderr, "WAVEFORMATEX:\n");
  fprintf(stderr, "  wFormatTag      %04x\n", wf->wFormatTag);
  fprintf(stderr, "  nChannels:      %d\n",   wf->nChannels);
  fprintf(stderr, "  nSamplesPerSec  %d\n",   (int)wf->nSamplesPerSec);
  fprintf(stderr, "  nAvgBytesPerSec %d\n",   (int)wf->nAvgBytesPerSec);
  fprintf(stderr, "  nBlockAlign     %d\n",   wf->nBlockAlign);
  fprintf(stderr, "  wBitsPerSample  %d\n",   wf->wBitsPerSample);
  fprintf(stderr, "  cbSize          %d\n",   wf->cbSize);
  }
#endif
/* Get one packet worth of data */

static int get_data(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  bgav_packet_t * p;
  
  priv = (win32_priv_t*)(s->data.audio.decoder->priv);

  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;

  if(priv->buffer_alloc < priv->buffer_size + p->data_size)
    {
    priv->buffer_alloc = priv->buffer_size + p->data_size + 32;
    priv->buffer = realloc(priv->buffer, priv->buffer_alloc);
    }

  memcpy(priv->buffer + priv->buffer_size, p->data, p->data_size);
  priv->buffer_size += p->data_size;
  bgav_demuxer_done_packet_read(s->demuxer, p);
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
  priv = (win32_priv_t*)(s->data.audio.decoder->priv);

  while(priv->buffer_size < priv->src_size)
    if(!get_data(s))
      return 0;
  //  fprintf(stderr, "decode_frame_DS: ");
  while(!priv->frame->valid_samples)
    {
    //    fprintf(stderr, "DS_AudioDecoder_Convert: %p, %d, %d...", priv->buffer, priv->src_size,
    //            priv->dst_size);

    result = DS_AudioDecoder_Convert(priv->ds_dec, priv->buffer, priv->src_size,
                                     priv->frame->samples.s_16, priv->dst_size,
                                     &size_read, &size_written);

    if(result)
      return 0;
    
    //    fprintf(stderr, "size_read: %d, size_written: %d buffer_size: %d src_size: %d dst_size: %d\n",
    //            size_read, size_written, priv->src_size, priv->src_size, priv->dst_size);

    if(size_written)
      {
      priv->frame->valid_samples =
        size_written / (s->data.audio.format.num_channels*priv->bytes_per_sample);
      priv->last_frame_size = priv->frame->valid_samples;
      }
    if(size_read)
      buffer_done(priv, size_read);
    
    }
  return priv->last_frame_size;
  }

static int decode_frame_std(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  ACMSTREAMHEADER ash;
  HRESULT hr = 0;
  priv = (win32_priv_t*)(s->data.audio.decoder->priv);

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
  ash.pbDst=priv->frame->samples.s_8;
  ash.cbDstLength=priv->dst_size;

    
  hr=acmStreamPrepareHeader(priv->acmstream,&ash,0);
  if(hr)
    {
    fprintf(stderr, "(ACM_Decoder) acmStreamPrepareHeader failed %d\n",(int)hr);
    return 0;
    }

  hr=acmStreamConvert(priv->acmstream,&ash,0);
  if(hr)
    {
    fprintf(stderr, "(ACM_Decoder) acmStreamConvert failed %d\n",(int)hr);
    return 0;
    }

  if(ash.cbSrcLengthUsed)
    buffer_done(priv, ash.cbSrcLengthUsed);
  priv->frame->valid_samples = ash.cbDstLengthUsed /
    (s->data.audio.format.num_channels*priv->bytes_per_sample);
  priv->last_frame_size = priv->frame->valid_samples;

  hr=acmStreamUnprepareHeader(priv->acmstream,&ash,0);
  if(hr)
    {
    fprintf(stderr, "(ACM_Decoder) acmStreamUnprepareHeader failed %d\n",(int)hr);
    }
  
  return priv->last_frame_size;
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
  uint8_t * in_fmt_buffer = (uint8_t*)0;

  WAVEFORMATEX out_format;

  fprintf(stderr, "OPEN AUDIO\n");
  
  codec_index = find_codec_index(s);
  if(codec_index == -1)
    goto fail;
  
  info = &(codec_infos[codec_index]);

  priv = calloc(1, sizeof(*priv));

  s->data.audio.decoder->priv = priv;

  /* Create input- and output formats */
  
  bgav_WAVEFORMAT_set_format(&_in_format, s);
  bgav_WAVEFORMAT_dump(&_in_format);

  in_fmt_buffer = malloc(sizeof(*in_format) + s->ext_size);

  in_format = (WAVEFORMATEX*)in_fmt_buffer;
  
  pack_wf(in_format, &_in_format);

  in_format->cbSize = s->ext_size;
  if(in_format->cbSize)
    {
    //    fprintf(stderr, "Adding extradata:");
    //    bgav_hexdump(s->ext_data, s->ext_size, 16);
    memcpy(in_fmt_buffer + sizeof(*in_format), s->ext_data, s->ext_size);
    }
#if 0
  fprintf(stderr, "*** BGAV WAVEFORMATEX\n");
  bgav_WAVEFORMATEX_dump(&_in_format);
  
  fprintf(stderr, "*** WAVEFORMATEX\n");
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
      fprintf(stderr, "acmStreamOpen:\n");
      fprintf(stderr, "in_format:\n");
      dump_wf(in_format);
      fprintf(stderr, "out_format:\n");
      dump_wf(&out_format);
#endif
      result=acmStreamOpen(&(priv->acmstream),(HACMDRIVER)NULL,
                           in_format,
                           &out_format,
                           NULL,0,0,0);
      if(result)
        {
        if(result == ACMERR_NOTPOSSIBLE)
          {
          fprintf(stderr, "(ACM_Decoder) Unappropriate audio format\n");
          }
        else
          fprintf(stderr, "(ACM_Decoder) acmStreamOpen error %d\n", result);
        priv->acmstream = 0;
        return 0;
        }


      acmStreamSize(priv->acmstream, s->data.audio.block_align,
                    &out_size, ACM_STREAMSIZEF_SOURCE);    
#if 0
      fprintf(stderr, "Opened ACM decoder, in_size: %d, out_size: %ld\n",
              s->data.audio.block_align, out_size);
#endif
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
                                        &(info->guid),
                                        in_format);
      if(!priv->ds_dec)
        {
        fprintf(stderr, "DS_AudioDecoder_Open failed\n");
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

  gavl_set_channel_setup(&(s->data.audio.format));
  
  priv->frame = gavl_audio_frame_create(&(s->data.audio.format));

  if(in_fmt_buffer)
    free(in_fmt_buffer);

  s->description = bgav_sprintf("%s", info->format_name);

  fprintf(stderr, "OPEN AUDIO DONE\n");
  
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

static int decode_w32(bgav_stream_t * s, gavl_audio_frame_t * f, int num_samples)
  {
  win32_priv_t * priv;
  int samples_copied;
  int samples_decoded = 0;
  Setup_FS_Segment();
  
  priv = (win32_priv_t *)(s->data.audio.decoder->priv);

  //  fprintf(stderr, "decode_w32 %d %d....\n", num_samples, priv->frame->valid_samples);
  
  while(samples_decoded < num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      if(!priv->decode_frame(s))
        {
        if(f)
          f->valid_samples = samples_decoded;
        return samples_decoded;
        }
      }
    
    samples_copied = gavl_audio_frame_copy(&(s->data.audio.format),
                                           f,
                                           priv->frame,
                                           samples_decoded, /* out_pos */
                                           priv->last_frame_size - priv->frame->valid_samples,  /* in_pos */
                                           num_samples - samples_decoded, /* out_size, */
                                           priv->frame->valid_samples /* in_size */);
    //    fprintf(stderr, "Copied %d samples\n", samples_copied);
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(f)
    f->valid_samples = samples_decoded;

  //  fprintf(stderr, "decode_w32 done\n");

  return samples_decoded;
  }

static void resync_w32(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  priv = (win32_priv_t*)(s->data.audio.decoder->priv);

  priv->buffer_size = 0;
  priv->frame->valid_samples = 0;
  }

static void close_w32(bgav_stream_t * s)
  {
  win32_priv_t * priv;
  priv = (win32_priv_t*)(s->data.audio.decoder->priv);

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

int bgav_init_audio_decoders_win32()
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
      codecs[i].decode = decode_w32;
      codecs[i].close  = close_w32;
      codecs[i].resync = resync_w32;
      bgav_audio_decoder_register(&codecs[i]);
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
