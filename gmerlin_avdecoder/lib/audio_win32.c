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
  DS_AudioDecoder    * ds_dec;
  DMO_AudioDecoder   * dmo_dec;

  
  } win32_priv_t;

static void pack_wf(WAVEFORMATEX * dst, bgav_WAVEFORMATEX * src)
  {
  dst->nChannels       = src->nChannels;
  dst->nSamplesPerSec  = src->nSamplesPerSec;
  dst->nAvgBytesPerSec = src->nAvgBytesPerSec;
  dst->nBlockAlign     = src->nBlockAlign;
  dst->wFormatTag      = src->wFormatTag;
  dst->cbSize          = src->cbSize;
  dst->wBitsPerSample  = src->wBitsPerSample;
  }


static int init_w32(bgav_stream_t * s)
  {
  int codec_index;
  codec_info_t * info;
  win32_priv_t * priv = NULL;

  WAVEFORMATEX in_format;
  bgav_WAVEFORMATEX _in_format;

  WAVEFORMATEX out_format;
  
  codec_index = find_codec_index(s);
  if(codec_index == -1)
    goto fail;

  info = &(codec_infos[codec_index]);

  priv = calloc(1, sizeof(*priv));

  s->data.audio.decoder->priv = priv;

  /* Create input- and output formats */

  bgav_WAVEFORMATEX_set_format(&_in_format, s);
  pack_wf(&in_format, &_in_format);

  /* We want 16 bit pcm as output */

  out_format.nChannels       = (in_format.nChannels >= 2)?2:1;
  out_format.nSamplesPerSec  = in_format.nSamplesPerSec;
  out_format.nAvgBytesPerSec = 2*out_format.nSamplesPerSec*out_format.nChannels;
  out_format.wFormatTag      = 0x0001;
  out_format.nBlockAlign     = 2*in_format.nChannels;
  out_format.wBitsPerSample  = 16;
  out_format.cbSize          = 0;

  /* Setup LDT */

  priv->ldt_fs = Setup_LDT_Keeper();
  
  switch(info->type)
    {
    case CODEC_STD:
      break;
    case CODEC_DS:
      priv->ds_dec=DS_AudioDecoder_Open(info->dll_name,
                                        &(info->guid),
                                        &in_format);
      if(!priv->ds_dec)
        goto fail;
      break;
    case CODEC_DMO:
      break;
    }
  
  return 1;
  fail:

  if(priv)
    {
    
    free(priv);
    s->data.audio.decoder->priv = NULL;
    }

  return 0;
  }

static int decode_w32(bgav_stream_t * s, gavl_audio_frame_t * frame, int num_samples)
  {
  return 0;
  
  }

static void resync_w32(bgav_stream_t * s)
  {
  //  return 0;

  }

static void close_w32(bgav_stream_t * s)
  {
  //  return 0;
  
  }

void bgav_init_audio_decoders_win32()
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

      codecs[i].init   = init_w32;
      codecs[i].decode = decode_w32;
      codecs[i].close  = close_w32;
      codecs[i].resync = resync_w32;
      bgav_audio_decoder_register(&codecs[i]);
      }
    else
      {
      fprintf(stderr, "Didn't find %s, %s will not be enabled\n",
              dll_filename, codec_infos[i].name);
      }
    }
  }
