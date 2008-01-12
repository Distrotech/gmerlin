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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <nanosoft.h>

#include <dvframe.h>
#define LOG_DOMAIN "avi"

/* Define the variables below to get a detailed file dump
   on each open call */

// #define DUMP_HEADERS
// #define DUMP_INDICES
// #define DUMP_AUDIO_TYPE

/* AVI Flags */

#define AVI_HASINDEX        0x00000010  // Index at end of file?
#define AVI_MUSTUSEINDEX    0x00000020
#define AVI_ISINTERLEAVED   0x00000100
#define AVI_TRUSTCKTYPE     0x00000800  // Use CKType to find key frames?
#define AVI_WASCAPTUREFILE  0x00010000
#define AVI_COPYRIGHTED     0x00020000
#define AVIF_WASCAPTUREFILE 0x00010000
#define AVI_KEYFRAME        0x10

/* ODML Extensions */

#define AVI_INDEX_OF_CHUNKS 0x01
#define AVI_INDEX_OF_INDEXES 0x00

#define AVI_INDEX_2FIELD 0x01

#define ID_RIFF     BGAV_MK_FOURCC('R','I','F','F')
#define ID_RIFF_ON2 BGAV_MK_FOURCC('O','N','2',' ')

#define ID_AVI      BGAV_MK_FOURCC('A','V','I',' ')
#define ID_AVI_ON2  BGAV_MK_FOURCC('O','N','2','f')

#define ID_LIST     BGAV_MK_FOURCC('L','I','S','T')
#define ID_AVIH     BGAV_MK_FOURCC('a','v','i','h')
#define ID_AVIH_ON2 BGAV_MK_FOURCC('O','N','2','h')

#define ID_HDRL BGAV_MK_FOURCC('h','d','r','l')
#define ID_STRL BGAV_MK_FOURCC('s','t','r','l')
#define ID_STRH BGAV_MK_FOURCC('s','t','r','h')
#define ID_STRF BGAV_MK_FOURCC('s','t','r','f')
#define ID_STRD BGAV_MK_FOURCC('s','t','r','d')
#define ID_JUNK BGAV_MK_FOURCC('J','U','N','K')

#define ID_IDX1 BGAV_MK_FOURCC('i','d','x','1')

#define ID_VIDS BGAV_MK_FOURCC('v','i','d','s')
#define ID_AUDS BGAV_MK_FOURCC('a','u','d','s')
#define ID_IAVS BGAV_MK_FOURCC('i','a','v','s')

#define ID_MOVI BGAV_MK_FOURCC('m','o','v','i')

#define ID_INFO BGAV_MK_FOURCC('I','N','F','O')

/* OpenDML Extensions */

#define ID_ODML BGAV_MK_FOURCC('o','d','m','l')
#define ID_DMLH BGAV_MK_FOURCC('d','m','l','h')
#define ID_INDX BGAV_MK_FOURCC('i','n','d','x')

#define PADD(s) (((s)&1)?((s)+1):(s))

/* Hardwired stream IDs for DV avi. This prevents us from
   decoding AVIs with muiltiple DV streams if they exist */

#define DV_AUDIO_ID 0
#define DV_VIDEO_ID 1

/* Chunk */

typedef struct
  {
  uint32_t ckID;
  uint32_t ckSize;
  } chunk_header_t;

/* RIFF */

typedef struct
  {
  uint32_t ckID;
  uint32_t ckSize;
  uint32_t fccType;
  } riff_header_t;

/* idx1 */

typedef struct
  {
  uint32_t num_entries;
  struct
    {
    uint32_t ckid;
    uint32_t dwFlags;
    uint32_t dwChunkOffset;
    uint32_t dwChunkLength;
    } * entries;
  } idx1_t;

/* strh */

typedef struct
  {
  uint32_t fccType;
  uint32_t fccHandler;
  uint32_t dwFlags;
  uint32_t dwReserved1;
  uint32_t dwInitialFrames;
  uint32_t dwScale;
  uint32_t dwRate;
  uint32_t dwStart;
  uint32_t dwLength;
  uint32_t dwSuggestedBufferSize;
  uint32_t dwQuality;
  uint32_t dwSampleSize;
  } strh_t;


typedef struct
  {
  uint32_t dwMicroSecPerFrame;
  uint32_t dwMaxBytesPerSec;
  uint32_t dwReserved1;
  uint32_t dwFlags;
  uint32_t dwTotalFrames;
  uint32_t dwInitialFrames;
  uint32_t dwStreams;
  uint32_t dwSuggestedBufferSize;
  uint32_t dwWidth;
  uint32_t dwHeight;
  uint32_t dwScale;
  uint32_t dwRate;
  uint32_t dwStart;
  uint32_t dwLength;
  } avih_t;

/* dmlh */

typedef struct
  {
  uint32_t dwTotalFrames;
  } dmlh_t;

/* odml */

typedef struct
  {
  int has_dmlh;
  dmlh_t dmlh;
  } odml_t;

/* indx */

typedef struct indx_s
  {
  uint16_t wLongsPerEntry;
  uint8_t  bIndexSubType;
  uint8_t  bIndexType;
  uint32_t nEntriesInUse;
  uint32_t dwChunkID;
  //  uint32_t dwReserved[3];
  
  union
    {
    struct
      {
      uint64_t qwBaseOffset;
      uint32_t dwReserved3;
      struct
        {
        uint32_t dwOffset;
        uint32_t dwSize;
        } * entries;
      } chunk;
    struct
      {
      uint64_t qwBaseOffset;
      uint32_t dwReserved3;
      struct
        {
        uint32_t dwOffset;
        uint32_t dwSize;
        uint32_t dwOffsetField2;
        } * entries;
      } field_chunk;
    struct
      {
      uint32_t dwReserved[3];
      struct
        {
        uint64_t qwOffset;
        uint32_t dwSize;
        uint32_t dwDuration;
        struct indx_s * subindex;
        } * entries;
      } index;
    } i;
  } indx_t;


/* Master structures */

typedef struct
  {
  avih_t avih;
  idx1_t idx1;
  int has_idx1;
  
  uint32_t movi_size;

  odml_t odml;
  int has_odml;

  bgav_RIFFINFO_t * info;

  /* DV Stuff */
  bgav_dv_dec_t * dv_dec;
  int dv_frame_size;
  uint8_t * dv_frame_buffer;
  int has_iavs;
  
  } avi_priv_t;

typedef struct
  {
  strh_t strh;
  indx_t indx;
  int has_indx;

  int vbr;
  int64_t total_bytes;
  int64_t total_blocks;
  } audio_priv_t;

typedef struct
  {
  strh_t strh;
  indx_t indx;
  int has_indx;

  int64_t total_frames;

  } video_priv_t;

/* Chunk */

static int read_chunk_header(bgav_input_context_t * input,
                             chunk_header_t * chunk)
  {
  return bgav_input_read_fourcc(input, &(chunk->ckID)) &&
    bgav_input_read_32_le(input, &(chunk->ckSize));
  }

#ifdef DUMP_HEADERS
static void dump_chunk_header(chunk_header_t * chunk)
  {
  bgav_dprintf("chunk header:\n");
  bgav_dprintf("  ckID: ");
  bgav_dump_fourcc(chunk->ckID);
  bgav_dprintf("\n  ckSize %d\n", chunk->ckSize);
  }
#endif

#ifdef DUMP_AUDIO_TYPE
static int do_msg = 1;
#endif

static void add_index_packet(bgav_superindex_t * si, bgav_stream_t * stream,
                             int64_t offset, int size, int keyframe)
  {
  audio_priv_t * avi_as;
  video_priv_t * avi_vs;
  int samplerate;
  int64_t time;

  
  if(stream->type == BGAV_STREAM_AUDIO)
    {
    avi_as = (audio_priv_t*)(stream->priv);
    samplerate = stream->data.audio.format.samplerate;

    /*
     * Some notes about Audio in AVI files.
     * -----------------------------------
     *
     * nBlockAlign isn't really important for AVI files, since we only
     * read complete cunks at once and assume, that chunks are already block aligned
     *
     * There multiple ways to store audio data in an AVI file:
     * 1. Set nAvgBytesPerSec and dwRate/dwScale to the number of bytes per second
     *    The timestamp will simply be determined from the byte position and the 
     *    byterate. This is used e.g. for PCM and mp3 audio. 
     */
    
    if ((avi_as->strh.dwSampleSize == 0) && (avi_as->strh.dwScale > 1))
      {
      /* variable bitrate */
      time = (samplerate * (gavl_time_t)avi_as->total_blocks *
              (gavl_time_t)avi_as->strh.dwScale) / avi_as->strh.dwRate;
      
      }
    else
      {
      /* constant bitrate */

      //        lprintf("get_audio_pts: CBR: nBlockAlign=%d, dwSampleSize=%d\n",
      //                at->wavex->nBlockAlign, at->dwSampleSize);
      if(stream->data.audio.block_align)
        {
      time =
          ((gavl_time_t)avi_as->total_bytes * (gavl_time_t)avi_as->strh.dwScale *
           samplerate) /
          (stream->data.audio.block_align * avi_as->strh.dwRate);
        }
      else
        {
      time =
          (samplerate * (gavl_time_t)avi_as->total_bytes *
           (gavl_time_t)avi_as->strh.dwScale) /
          (avi_as->strh.dwSampleSize * avi_as->strh.dwRate);
        }
      }

    bgav_superindex_add_packet(si,
                               stream,
                               offset,
                               size,
                               stream->stream_id,
                               time,
                               1, 0);
      
    /* Increase block count */
            
    if(stream->data.audio.block_align)
      {
      avi_as->total_blocks +=
        (size + stream->data.audio.block_align - 1)
        / stream->data.audio.block_align;
      }
    else
      {
      avi_as->total_blocks++;
      }
    avi_as->total_bytes += size;
    }
  else if(stream->type == BGAV_STREAM_VIDEO)
    {
    avi_vs = (video_priv_t*)(stream->priv);

    /* Some AVIs can have zero packet size, which means, that
       the previous frame will be shown */
    
    if(size)
      {
      time = stream->data.video.format.frame_duration *
        avi_vs->total_frames;
      
      bgav_superindex_add_packet(si,
                                 stream,
                                 offset,
                                 size,
                                 stream->stream_id,
                                 time,
                                 keyframe, 1);
      }
    else /* If we have zero size, the framerate will be nonconstant */
      {
      stream->data.video.format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
      }
    avi_vs->total_frames++;
    }
  
  }


/* RIFF */

static int read_riff_header(bgav_input_context_t * input,
                            riff_header_t * chunk)
  {
  return bgav_input_read_fourcc(input, &(chunk->ckID)) &&
    bgav_input_read_32_le(input, &(chunk->ckSize)) &&
    bgav_input_read_fourcc(input, &(chunk->fccType));
  }


#ifdef DUMP_HEADERS
void dump_avih(avih_t * h)
  {
  bgav_dprintf("avih:\n");
  bgav_dprintf("  dwMicroSecPerFrame: %d\n",    h->dwMicroSecPerFrame);
  bgav_dprintf("  dwMaxBytesPerSec: %d\n",      h->dwMaxBytesPerSec);
  bgav_dprintf("  dwReserved1: %d\n",           h->dwReserved1);
  bgav_dprintf("  dwFlags: %08x\n",             h->dwFlags);
  bgav_dprintf("  dwTotalFrames: %d\n",         h->dwTotalFrames);
  bgav_dprintf("  dwInitialFrames: %d\n",       h->dwInitialFrames);
  bgav_dprintf("  dwStreams: %d\n",             h->dwStreams);
  bgav_dprintf("  dwSuggestedBufferSize: %d\n", h->dwSuggestedBufferSize);
  bgav_dprintf("  dwWidth: %d\n",               h->dwWidth);
  bgav_dprintf("  dwHeight: %d\n",              h->dwHeight);
  bgav_dprintf("  dwScale: %d\n",               h->dwScale);
  bgav_dprintf("  dwRate: %d\n",                h->dwRate);
  bgav_dprintf("  dwLength: %d\n",              h->dwLength);
  }
#endif

static int read_avih(bgav_input_context_t* input,
              avih_t * ret, chunk_header_t * ch)
  {
  int64_t start_pos;
  int result;
  
  start_pos = input->position;
  
  result = bgav_input_read_32_le(input, &(ret->dwMicroSecPerFrame)) &&
    bgav_input_read_32_le(input, &(ret->dwMaxBytesPerSec)) &&
    bgav_input_read_32_le(input, &(ret->dwReserved1)) &&
    bgav_input_read_32_le(input, &(ret->dwFlags)) &&
    bgav_input_read_32_le(input, &(ret->dwTotalFrames)) &&
    bgav_input_read_32_le(input, &(ret->dwInitialFrames)) &&
    bgav_input_read_32_le(input, &(ret->dwStreams)) &&
    bgav_input_read_32_le(input, &(ret->dwSuggestedBufferSize)) &&
    bgav_input_read_32_le(input, &(ret->dwWidth)) &&
    bgav_input_read_32_le(input, &(ret->dwHeight)) &&
    bgav_input_read_32_le(input, &(ret->dwScale)) &&
    bgav_input_read_32_le(input, &(ret->dwRate)) &&
    bgav_input_read_32_le(input, &(ret->dwLength));

  if(input->position - start_pos < ch->ckSize)
    {
    
    bgav_input_skip(input, PADD(ch->ckSize) - (input->position - start_pos));
    }
#ifdef DUMP_HEADERS
  dump_avih(ret);
#endif
  return result;
  }

/* indx */

static void free_idx1(idx1_t * idx1)
  {
  if(idx1->entries)
    free(idx1->entries);
  }

#ifdef DUMP_INDICES
static void dump_idx1(idx1_t * idx1)
  {
  int i;
  bgav_dprintf("idx1, %d entries\n", idx1->num_entries);
  for(i = 0; i < idx1->num_entries; i++)
    {
    bgav_dprintf("ID: ");
    bgav_dump_fourcc(idx1->entries[i].ckid);
    bgav_dprintf(" Flags: %08x", idx1->entries[i].dwFlags);
    bgav_dprintf(" Offset: %d", idx1->entries[i].dwChunkOffset);
    bgav_dprintf(" Size: %d\n", idx1->entries[i].dwChunkLength);
    }
  }
#endif

static int probe_idx1(bgav_input_context_t * input)
  {
  uint32_t fourcc;

  if(!bgav_input_get_fourcc(input, &fourcc))
    return 0;

  if(fourcc == ID_IDX1)
    return 1;
  return 0;
  }

static int read_idx1(bgav_input_context_t * input, idx1_t * ret)
  {
  int i;
  chunk_header_t ch;
  if(!read_chunk_header(input, &ch))
    return 0;
  ret->num_entries = ch.ckSize / 16;
  
  ret->entries = calloc(ret->num_entries, sizeof(*ret->entries));
  for(i = 0; i < ret->num_entries; i++)
    {
    if(!bgav_input_read_fourcc(input, &(ret->entries[i].ckid)) ||
       !bgav_input_read_32_le(input, &(ret->entries[i].dwFlags)) ||
       !bgav_input_read_32_le(input, &(ret->entries[i].dwChunkOffset)) ||
       !bgav_input_read_32_le(input, &(ret->entries[i].dwChunkLength)))
      return 0;
    }
  return 1;
  }

/* strh */


#ifdef DUMP_HEADERS
static void dump_strh(strh_t * ret)
  {
  bgav_dprintf("strh\n  fccType: ");
  bgav_dump_fourcc(ret->fccType);
  bgav_dprintf("\n  fccHandler: ");
  bgav_dump_fourcc(ret->fccHandler);
  bgav_dprintf("\n  dwFlags: %d (%08x)\n",
          ret->dwFlags, ret->dwFlags);
  bgav_dprintf("  dwReserved1: %d (%08x)\n",
          ret->dwReserved1, ret->dwReserved1);
  bgav_dprintf("  dwInitialFrames: %d (%08x)\n",
          ret->dwInitialFrames, ret->dwInitialFrames);
  bgav_dprintf("  dwScale: %d (%08x)\n", ret->dwScale, ret->dwScale);
  bgav_dprintf("  dwRate: %d (%08x)\n", ret->dwRate, ret->dwRate);
  bgav_dprintf("  dwStart: %d (%08x)\n", ret->dwStart, ret->dwStart);
  bgav_dprintf("  dwLength: %d (%08x)\n", ret->dwLength, ret->dwLength);
  bgav_dprintf("  dwSuggestedBufferSize: %d (%08x)\n",
          ret->dwSuggestedBufferSize,
          ret->dwSuggestedBufferSize);
  bgav_dprintf("  dwQuality: %d (%08x)\n", ret->dwQuality, ret->dwQuality);
  bgav_dprintf("  dwSampleSize: %d (%08x)\n", ret->dwSampleSize, ret->dwSampleSize);
  }
#endif

static int read_strh(bgav_input_context_t * input, strh_t * ret,
                     chunk_header_t * ch)
  {
  int64_t start_pos;
  int result;
  
  start_pos = input->position;
  
  result = 
    bgav_input_read_fourcc(input, &(ret->fccType)) &&
    bgav_input_read_fourcc(input, &(ret->fccHandler)) &&
    bgav_input_read_32_le(input, &(ret->dwFlags)) &&
    bgav_input_read_32_le(input, &(ret->dwReserved1)) &&
    bgav_input_read_32_le(input, &(ret->dwInitialFrames)) &&
    bgav_input_read_32_le(input, &(ret->dwScale)) &&
    bgav_input_read_32_le(input, &(ret->dwRate)) &&
    bgav_input_read_32_le(input, &(ret->dwStart)) &&
    bgav_input_read_32_le(input, &(ret->dwLength)) &&
    bgav_input_read_32_le(input, &(ret->dwSuggestedBufferSize)) &&
    bgav_input_read_32_le(input, &(ret->dwQuality)) &&
    bgav_input_read_32_le(input, &(ret->dwSampleSize));

  if(input->position - start_pos < ch->ckSize)
    {
    bgav_input_skip(input, PADD(ch->ckSize) - (input->position - start_pos));
    }
#ifdef DUMP_HEADERS
  dump_strh(ret);
#endif
  return result;
  }

/* odml extensions */

/* dmlh */


#ifdef DUMP_HEADERS
static void dump_dmlh(dmlh_t * dmlh)
  {
  bgav_dprintf("dmlh:\n");
  bgav_dprintf("  dwTotalFrames: %d\n", dmlh->dwTotalFrames);
  }
#endif

static int read_dmlh(bgav_input_context_t * input, dmlh_t * ret,
                     chunk_header_t * ch)
  {
  int64_t start_pos;

  start_pos = input->position;
  if(!bgav_input_read_32_le(input, &(ret->dwTotalFrames)))
    return 0;

  if(input->position - start_pos < ch->ckSize)
    {
    bgav_input_skip(input, PADD(ch->ckSize) - (input->position - start_pos));
    }
  return 1;
  }
/* odml */

#ifdef DUMP_HEADERS
static void dump_odml(odml_t * odml)
  {
  bgav_dprintf("odml:\n");
  if(odml->has_dmlh)
    dump_dmlh(&(odml->dmlh));
  }
#endif

static int read_odml(bgav_input_context_t * input, odml_t * ret,
                     chunk_header_t * ch)
  {
  chunk_header_t ch1;
  int keep_going;
  int64_t start_pos = input->position;

  keep_going = 1;
  
  while(keep_going)
    {
    if(input->position - start_pos >= ch->ckSize - 4)
      break;
    
    if(!read_chunk_header(input, &ch1))
      return 0;
    switch(ch1.ckID)
      {
      case ID_DMLH:
        if(!read_dmlh(input, &(ret->dmlh), &ch1))
          return 0;
        //        dump_dmlh(&(ret->dmlh));
        ret->has_dmlh = 1;
        break;
      default:
        keep_going = 0;
        break;
      }
    }
  
  if(input->position - start_pos < ch->ckSize - 4)
    {
    bgav_input_skip(input, ch->ckSize - 4 - input->position - start_pos);
    }
#ifdef DUMP_HEADERS
  dump_odml(ret);
#endif

  return 1;
  }

/* indx */

static int read_indx(bgav_input_context_t * input, indx_t * ret,
                     chunk_header_t * ch)
  {
  int64_t pos;
  int i;

  chunk_header_t ch1;

  //  dump_chunk_header(ch);
  pos = input->position;
  if(!bgav_input_read_16_le(input, &(ret->wLongsPerEntry)) ||
     !bgav_input_read_8(input, &(ret->bIndexSubType)) ||
     !bgav_input_read_8(input, &(ret->bIndexType)) ||
     !bgav_input_read_32_le(input, &(ret->nEntriesInUse)) ||
     !bgav_input_read_fourcc(input, &(ret->dwChunkID)))
    return 0;
  
  switch(ret->bIndexType)
    {
    case AVI_INDEX_OF_INDEXES:
      if(!bgav_input_read_32_le(input, &(ret->i.index.dwReserved[0])) ||
         !bgav_input_read_32_le(input, &(ret->i.index.dwReserved[1])) ||
         !bgav_input_read_32_le(input, &(ret->i.index.dwReserved[2])))
        return 0;
      ret->i.index.entries =
        calloc(ret->nEntriesInUse, sizeof(*(ret->i.index.entries)));
      for(i = 0; i < ret->nEntriesInUse; i++)
        {
        if(!bgav_input_read_64_le(input, &(ret->i.index.entries[i].qwOffset)) ||
           !bgav_input_read_32_le(input, &(ret->i.index.entries[i].dwSize)) ||
           !bgav_input_read_32_le(input, &(ret->i.index.entries[i].dwDuration)))
          return 0;
        }
      break;
    case AVI_INDEX_OF_CHUNKS:
      
      if(ret->bIndexSubType == AVI_INDEX_2FIELD)
        {
        if(!bgav_input_read_64_le(input, &(ret->i.field_chunk.qwBaseOffset)) ||
           !bgav_input_read_32_le(input, &(ret->i.field_chunk.dwReserved3)))
          return 0;
        ret->i.field_chunk.entries =
          malloc(ret->nEntriesInUse * sizeof(*(ret->i.field_chunk.entries)));

        for(i = 0; i < ret->nEntriesInUse; i++)
          {
          if(!bgav_input_read_32_le(input, &(ret->i.field_chunk.entries[i].dwOffset)) ||
             !bgav_input_read_32_le(input, &(ret->i.field_chunk.entries[i].dwSize)) ||
             !bgav_input_read_32_le(input, &(ret->i.field_chunk.entries[i].dwOffsetField2)))
            return 0;
          }
        }
      else
        {
        if(!bgav_input_read_64_le(input, &(ret->i.chunk.qwBaseOffset)) ||
           !bgav_input_read_32_le(input, &(ret->i.chunk.dwReserved3)))
          return 0;
        ret->i.chunk.entries =
          malloc(ret->nEntriesInUse * sizeof(*(ret->i.chunk.entries)));
        
        for(i = 0; i < ret->nEntriesInUse; i++)
          {
          if(!bgav_input_read_32_le(input, &(ret->i.chunk.entries[i].dwOffset)) ||
             !bgav_input_read_32_le(input, &(ret->i.chunk.entries[i].dwSize)))
            return 0;
          }
        }
      break;
    }
  
  if(input->position - pos < ch->ckSize)
    {
    
    bgav_input_skip(input, PADD(ch->ckSize) - (input->position - pos));
    }
  /* Read subindices */

  if((ret->bIndexType == AVI_INDEX_OF_INDEXES) && (input->input->seek_byte))
    {
    pos = input->position;

    for(i = 0; i < ret->nEntriesInUse; i++)
      {
      bgav_input_seek(input, ret->i.index.entries[i].qwOffset, SEEK_SET);
      ret->i.index.entries[i].subindex = calloc(1, sizeof(*ret->i.index.entries[i].subindex));
      if(!read_chunk_header(input, &ch1) ||
         !read_indx(input, ret->i.index.entries[i].subindex,
                    &ch1))
      return 0;
      }
    bgav_input_seek(input, pos, SEEK_SET);
    }
  return 1;
  }

#ifdef DUMP_INDICES
static void dump_indx(indx_t * indx)
  {
  int i;
  bgav_dprintf("indx:\n");
  bgav_dprintf("wLongsPerEntry: %d\n", indx->wLongsPerEntry);
  bgav_dprintf("bIndexSubType:  0x%02x\n", indx->bIndexSubType);
  bgav_dprintf("bIndexType:     0x%02x\n", indx->bIndexType);
  bgav_dprintf("nEntriesInUse:  %d\n", indx->nEntriesInUse);
  bgav_dprintf("dwChunkID:      ");
  bgav_dump_fourcc(indx->dwChunkID);
  bgav_dprintf("\n");
  
  switch(indx->bIndexType)
    {
    case AVI_INDEX_OF_INDEXES:
      for(i = 0; i < 3; i++)
        bgav_dprintf("dwReserved[%d]: %d\n", i,
                indx->i.index.dwReserved[i]);

      for(i = 0; i < indx->nEntriesInUse; i++)
        {
        bgav_dprintf("%d qwOffset: %" PRId64 " dwSize: %d dwDuration: %d\n", i,
                indx->i.index.entries[i].qwOffset,
                indx->i.index.entries[i].dwSize,
                indx->i.index.entries[i].dwDuration);
        if(indx->i.index.entries[i].subindex)
          {
          bgav_dprintf("Subindex follows:\n");
          dump_indx(indx->i.index.entries[i].subindex);
          }
        }
      break;
    case AVI_INDEX_OF_CHUNKS:
      
      if(indx->bIndexSubType == AVI_INDEX_2FIELD)
        {
        bgav_dprintf("qwBaseOffset:  %" PRId64 "\n",
                indx->i.field_chunk.qwBaseOffset);
        bgav_dprintf("dwReserved3:   %d\n",
                indx->i.field_chunk.dwReserved3);
        
        for(i = 0; i < indx->nEntriesInUse; i++)
          {
          bgav_dprintf("%d dwOffset: %d dwSize: %d dwOffsetField2: %d Keyframe: %d\n", i,
                  indx->i.field_chunk.entries[i].dwOffset,
                  indx->i.field_chunk.entries[i].dwSize & 0x7FFFFFFF,
                  indx->i.field_chunk.entries[i].dwOffsetField2,
                  !(indx->i.field_chunk.entries[i].dwSize & 0x80000000));
          }
        }
      else
        {
        bgav_dprintf("qwBaseOffset:  %" PRId64 "\n",
                indx->i.chunk.qwBaseOffset);
        bgav_dprintf("dwReserved3:   %d\n",
                indx->i.chunk.dwReserved3);
        
        for(i = 0; i < indx->nEntriesInUse; i++)
          {
          bgav_dprintf("%d dwOffset: %d dwSize: 0x%08x Keyframe: %d\n", i,
                  indx->i.chunk.entries[i].dwOffset,
                  indx->i.chunk.entries[i].dwSize & 0x7FFFFFFF,
                  // indx->i.chunk.entries[i].dwSize,
                  !(indx->i.chunk.entries[i].dwSize & 0x80000000));
          }
        }
      
      break;
    }
  }
#endif

static void free_indx(indx_t * indx)
  {
  int i;
  switch(indx->bIndexType)
    {
    case AVI_INDEX_OF_INDEXES:
      for(i = 0; i < indx->nEntriesInUse; i++)
        {
        if(indx->i.index.entries[i].subindex)
          {
          free_indx(indx->i.index.entries[i].subindex);
          free(indx->i.index.entries[i].subindex);
          }
        }
      free(indx->i.index.entries);
      break;
    case AVI_INDEX_OF_CHUNKS:
      if(indx->bIndexSubType == AVI_INDEX_2FIELD)
        free(indx->i.field_chunk.entries);
      else
        free(indx->i.chunk.entries);
      break;
    default:
      break;
    }
  }

static void indx_build_superindex(bgav_demuxer_context_t * ctx)
  {
  int num_entries, num_streams;
  int64_t offset, test_offset;
  int i, j;
  int num_audio_streams;
  audio_priv_t * avi_as;
  video_priv_t * avi_vs;
  uint32_t size = 0, test_size;

  avi_priv_t * priv = (avi_priv_t *)(ctx->priv);
  
  struct
    {
    int index_position;
    int index_index;
    indx_t * indx;
    indx_t * indx_cur;
    bgav_stream_t * s;
    } * streams;
  int stream_index;
  
  /* Check if we can build this and reset timestamps */

  num_audio_streams = 0;

  if(!priv->has_iavs)
    {
    for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
      {
      avi_as = (audio_priv_t *)(ctx->tt->cur->audio_streams[i].priv);
      
      if(!avi_as->has_indx)
        return;
      
      avi_as->total_bytes = 0;
      avi_as->total_blocks = 0;
      }
    num_audio_streams = ctx->tt->cur->num_audio_streams;
    }
  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    {
    avi_vs = (video_priv_t *)(ctx->tt->cur->video_streams[i].priv);
    
    if(!avi_vs->has_indx)
      return;

    avi_vs->total_frames = 0;
    }
  
  /* Count the entries */
  
  num_entries = 0;

  if(priv->has_iavs)
    num_streams = 1;
  else
    num_streams = num_audio_streams + ctx->tt->cur->num_video_streams;
  
  streams = calloc(num_streams, sizeof(*streams));

  for(i = 0; i < num_audio_streams; i++)
    {
    streams[i].s = &(ctx->tt->cur->audio_streams[i]);
    
    avi_as = (audio_priv_t *)(streams[i].s->priv);

    streams[i].indx = &(avi_as->indx);
    
    if(avi_as->indx.bIndexType == AVI_INDEX_OF_INDEXES)
      {
      for(j = 0; j < avi_as->indx.nEntriesInUse; j++)
        num_entries += avi_as->indx.i.index.entries[j].subindex->nEntriesInUse;
      streams[i].indx_cur = streams[i].indx->i.index.entries[0].subindex;
      streams[i].index_index = 0;
      }
    else
      {
      num_entries += avi_as->indx.nEntriesInUse;
      streams[i].indx_cur = &(avi_as->indx);
      streams[i].index_index = -1;
      }
    }

  for(i = num_audio_streams;
      i < ctx->tt->cur->num_video_streams + num_audio_streams; i++)
    {
    streams[i].s = &(ctx->tt->cur->video_streams[i-num_audio_streams]);

    avi_vs = (video_priv_t *)(streams[i].s->priv);

    streams[i].indx = &(avi_vs->indx);
    
    if(avi_vs->indx.bIndexType == AVI_INDEX_OF_INDEXES)
      {
      for(j = 0; j < avi_vs->indx.nEntriesInUse; j++)
        num_entries += avi_vs->indx.i.index.entries[j].subindex->nEntriesInUse;
      streams[i].indx_cur = streams[i].indx->i.index.entries[0].subindex;
      streams[i].index_index = 0;
      }
    else
      {
      num_entries += avi_vs->indx.nEntriesInUse;
      streams[i].indx_cur = &(avi_vs->indx);
      streams[i].index_index = -1;
      }
    }
    
  ctx->si = bgav_superindex_create(num_entries);
  
  /* Add the packets */

  stream_index = -1;
  for(i = 0; i < num_entries; i++)
    {
    /* Find packet with the lowest chunk offset */
    offset = 0x7fffffffffffffffLL;
    for(j = 0; j < num_streams; j++)
      {
      if(streams[j].index_position < 0)
        continue;

      if(streams[j].indx_cur->bIndexSubType == AVI_INDEX_2FIELD)
        {
        test_offset = streams[j].indx_cur->i.field_chunk.entries[streams[j].index_position].dwOffset + streams[j].indx_cur->i.field_chunk.qwBaseOffset;
        test_size   = streams[j].indx_cur->i.field_chunk.entries[streams[j].index_position].dwSize;
        }
      else
        {
        test_offset = streams[j].indx_cur->i.chunk.entries[streams[j].index_position].dwOffset + streams[j].indx_cur->i.chunk.qwBaseOffset;
        test_size   = streams[j].indx_cur->i.chunk.entries[streams[j].index_position].dwSize;
        }
      if(test_offset < offset)
        {
        stream_index = j;
        offset = test_offset;
        size = test_size;
        }
      }

    if(stream_index >= 0)
      add_index_packet(ctx->si, streams[stream_index].s,
                       offset, size & 0x7FFFFFFF, !(size & 0x80000000));

    /* Increase indices */

    streams[stream_index].index_position++;
    if(streams[stream_index].index_position >= streams[stream_index].indx_cur->nEntriesInUse)
      {
      if(streams[stream_index].index_index >= 0)
        {
        streams[stream_index].index_index++;
        streams[stream_index].index_position = 0;
        if(streams[stream_index].index_index >= streams[stream_index].indx->nEntriesInUse)
          streams[stream_index].index_position = -1;
        else
          streams[stream_index].indx_cur = streams[stream_index].indx->i.index.entries[streams[stream_index].index_index].subindex;
        }
      else
        streams[stream_index].index_position = -1;
      }
    }

  free(streams);
  }



static int probe_avi(bgav_input_context_t * input)
  {
  uint8_t data[12];
  if(bgav_input_get_data(input, data, 12) < 12)
    return 0;

  if ((data[0] == 'R') &&
      (data[1] == 'I') &&
      (data[2] == 'F') &&
      (data[3] == 'F') &&
      (data[8] == 'A') &&
      (data[9] == 'V') &&
      (data[10] == 'I') &&
      (data[11] == ' '))
    return 1;
  else if((data[0] == 'O') &&
          (data[1] == 'N') &&
          (data[2] == '2') &&
          (data[3] == ' ') &&
          (data[8] == 'O') &&
          (data[9] == 'N') &&
          (data[10] == '2') &&
          (data[11] == 'f'))
    return 1;
  else
    return 0;
  
  }

static int init_audio_stream(bgav_demuxer_context_t * ctx,
                             strh_t * strh, chunk_header_t * ch)
  {
  uint8_t * buf, * pos;
  bgav_WAVEFORMAT_t wf;
  int keep_going;
  bgav_stream_t * bg_as;
  audio_priv_t * avi_as;
  keep_going = 1;

  bg_as = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  avi_as = calloc(1, sizeof(*avi_as));
  bg_as->priv = avi_as;

  memcpy(&(avi_as->strh), strh, sizeof(*strh));
  
  while(keep_going)
    {
    read_chunk_header(ctx->input, ch);
    switch(ch->ckID)
      {
      case ID_STRF:
        buf = malloc(ch->ckSize);
        if(bgav_input_read_data(ctx->input, buf, ch->ckSize) < ch->ckSize)
          return 0;
        pos = buf;
        bgav_WAVEFORMAT_read(&wf, buf, ch->ckSize);
        bgav_WAVEFORMAT_get_format(&wf, bg_as);
#ifdef DUMP_HEADERS
        bgav_WAVEFORMAT_dump(&wf);
#endif
        bgav_WAVEFORMAT_free(&wf);
        //        bg_as->fourcc = BGAV_WAVID_2_FOURCC(wf.wFormatTag);
        //        if(!bg_as->data.audio.bits_per_sample)
        //          bg_as->data.audio.bits_per_sample = strh->dwSampleSize * 8;          
        
        /* Seek support */
        if(!strh->dwSampleSize)
          {
          avi_as->vbr = 1;
          }
        free(buf);
        break;
      case ID_STRD:
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_JUNK:
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_LIST:
        keep_going = 0;
        break;
      case ID_INDX:
        if(!read_indx(ctx->input, &avi_as->indx, ch))
          return 0;
#ifdef DUMP_INDICES
        dump_indx(&avi_as->indx);
#endif
        avi_as->has_indx = 1;
        break;
      default:
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      }
    }
  bg_as->stream_id =
    (ctx->tt->cur->num_audio_streams + ctx->tt->cur->num_video_streams) - 1;
  return 1;
  }

#define BYTE_2_COLOR(c) (((uint16_t)c) | ((uint16_t)(c)<<8))

static int init_video_stream(bgav_demuxer_context_t * ctx,
                             strh_t * strh, chunk_header_t * ch)
  {
  avih_t * avih;
  uint8_t * buf, * pos;
  bgav_BITMAPINFOHEADER_t bh;
  int keep_going, i;
  bgav_stream_t * bg_vs;
  video_priv_t * avi_vs;
  keep_going = 1;
  
  avih = &(((avi_priv_t*)(ctx->priv))->avih);
    
  
  bg_vs = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
  bg_vs->vfr_timestamps = 1;

  avi_vs = calloc(1, sizeof(*avi_vs));

  memcpy(&(avi_vs->strh), strh, sizeof(*strh));
  
  bg_vs->priv = avi_vs;
  while(keep_going)
    {
    read_chunk_header(ctx->input, ch);
    switch(ch->ckID)
      {
      case ID_STRF:
        buf = malloc(ch->ckSize);
        if(bgav_input_read_data(ctx->input, buf, ch->ckSize) < ch->ckSize)
          return 0;
        pos = buf;
        bgav_BITMAPINFOHEADER_read(&bh, &pos);
        bgav_BITMAPINFOHEADER_get_format(&bh, bg_vs);
#ifdef DUMP_HEADERS
        bgav_BITMAPINFOHEADER_dump(&bh);
#endif

        /* We don't add extradata if the fourcc is MJPG */
        /* This lets us play blender AVIs. */
                
        if((ch->ckSize > 40) && (bg_vs->fourcc != BGAV_MK_FOURCC('M','J','P','G')))
          {
          bg_vs->ext_size = ch->ckSize - 40;
          bg_vs->ext_data = calloc(bg_vs->ext_size + 16, 1);
          memcpy(bg_vs->ext_data, pos, bg_vs->ext_size);
          }

        /* Add palette if depth <= 8 */

        
        if((bh.biBitCount <= 8) && (bh.biClrUsed))
          {
          bg_vs->data.video.palette_size = (ch->ckSize - 40) / 4;
          bg_vs->data.video.palette      =
            malloc(bg_vs->data.video.palette_size * sizeof(*(bg_vs->data.video.palette)));
          for(i = 0; i < bg_vs->data.video.palette_size; i++)
            {
            bg_vs->data.video.palette[i].b = BYTE_2_COLOR(pos[4*i]);
            bg_vs->data.video.palette[i].g = BYTE_2_COLOR(pos[4*i+1]);
            bg_vs->data.video.palette[i].r = BYTE_2_COLOR(pos[4*i+2]);
            bg_vs->data.video.palette[i].a = 0xffff;
            }
          }

        
        free(buf);
        break;
      case ID_STRD:
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_JUNK:
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_LIST:
        keep_going = 0;
        break;
      case ID_INDX:
        if(!read_indx(ctx->input, &avi_vs->indx, ch))
          return 0;

#ifdef DUMP_INDICES
        dump_indx(&avi_vs->indx);
#endif
        avi_vs->has_indx = 1;
        break;
      default:
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      }
    }

  /* Get frame rate */

  if(strh->dwScale && strh->dwRate)
    {
    bg_vs->data.video.format.timescale = strh->dwRate;
    bg_vs->data.video.format.frame_duration = strh->dwScale;
    }
  else if(avih->dwMicroSecPerFrame)
    {
    bg_vs->data.video.format.timescale = 1000000;
    bg_vs->data.video.format.frame_duration = avih->dwMicroSecPerFrame;
    }
  else
    {
    bg_vs->data.video.format.timescale = 25;
    bg_vs->data.video.format.frame_duration = 1;
    }

    
  bg_vs->stream_id = (ctx->tt->cur->num_audio_streams +
                      ctx->tt->cur->num_video_streams) - 1;
  return 1;
  }

static int process_packet_iavs(bgav_demuxer_context_t * ctx)
  {
  int do_init = 0;
  bgav_stream_t * as, *vs;
  bgav_packet_t * ap = (bgav_packet_t *)0;
  bgav_packet_t * vp = (bgav_packet_t *)0;
  uint8_t header[DV_HEADER_SIZE];
  avi_priv_t * priv;
  
  priv = (avi_priv_t*)(ctx->priv);
  
  if(!priv->dv_dec)
    {
    priv->dv_dec = bgav_dv_dec_create();
    do_init = 1;
    /* Get formats */
    if(bgav_input_get_data(ctx->input, header, DV_HEADER_SIZE) <
       DV_HEADER_SIZE)
      return 0;

    bgav_dv_dec_set_header(priv->dv_dec, header);
    priv->dv_frame_size = bgav_dv_dec_get_frame_size(priv->dv_dec);
    priv->dv_frame_buffer = malloc(priv->dv_frame_size);
    }

  if(bgav_input_read_data(ctx->input, priv->dv_frame_buffer,
                          priv->dv_frame_size) < priv->dv_frame_size)
    return 0;
  
  if(do_init)
    {
    vs = ctx->tt->cur->video_streams;
    as = ctx->tt->cur->audio_streams;
    }
  else
    {
    vs = bgav_track_find_stream(ctx->tt->cur, DV_VIDEO_ID);
    as = bgav_track_find_stream(ctx->tt->cur, DV_AUDIO_ID);
    }

  if(!do_init)
    bgav_dv_dec_set_header(priv->dv_dec, priv->dv_frame_buffer);
  bgav_dv_dec_set_frame(priv->dv_dec, priv->dv_frame_buffer);
  
  if(do_init)
    {
    bgav_dv_dec_init_audio(priv->dv_dec, as);
    bgav_dv_dec_init_video(priv->dv_dec, vs);
    }
  
  if(vs)
    {
    vp = bgav_stream_get_packet_write(vs);
    }
  if(as)
    {
    ap = bgav_stream_get_packet_write(as);
    }
  if(!bgav_dv_dec_get_audio_packet(priv->dv_dec, ap))
    return 0;
  bgav_dv_dec_get_video_packet(priv->dv_dec, vp);

  if(vs) bgav_dv_dec_set_frame_counter(priv->dv_dec, vs->in_position);

  if(ap) bgav_packet_done_write(ap);
  if(vp) bgav_packet_done_write(vp);
  
  
  return 1;
  }

static void seek_iavs(bgav_demuxer_context_t * ctx, gavl_time_t time,
                      int scale)
  {
  bgav_superindex_seek(ctx->si,
                       ctx->tt->cur->video_streams,
                       time, scale);
  ctx->tt->cur->audio_streams->time_scaled =
    ctx->tt->cur->video_streams->time_scaled;
  ctx->si->current_position = ctx->tt->cur->video_streams->index_position;
  bgav_input_seek(ctx->input, ctx->si->entries[ctx->si->current_position].offset,
                  SEEK_SET);
  ctx->tt->cur->video_streams->in_position =
    ctx->tt->cur->video_streams->index_position;
  }

static int next_packet_iavs_si(bgav_demuxer_context_t * ctx)
  {
  int result;
  
  if(ctx->si->current_position >= ctx->si->num_entries)
    {
    return 0;
    }
  
  if(ctx->input->position >=
     ctx->si->entries[ctx->si->num_entries - 1].offset +
     ctx->si->entries[ctx->si->num_entries - 1].size)
    {
    return 0;
    }

  if(ctx->si->entries[ctx->si->current_position].offset > ctx->input->position)
    {
    bgav_input_skip(ctx->input,
                    ctx->si->entries[ctx->si->current_position].offset - ctx->input->position);
    }

  result = process_packet_iavs(ctx);
  
  ctx->si->current_position++;
  return result;
  }


static int init_iavs_stream(bgav_demuxer_context_t * ctx,
                            strh_t * strh, chunk_header_t * ch)
  {
  avih_t * avih;
  int keep_going;
  video_priv_t * video_priv;
  bgav_stream_t * bg_vs;
  bgav_stream_t * bg_as;
  avi_priv_t * priv = (avi_priv_t*)(ctx->priv);

  priv->has_iavs = 1;
  
  keep_going = 1;
  
  avih = &(((avi_priv_t*)(ctx->priv))->avih);
  
  bg_vs = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
  bg_vs->stream_id = DV_VIDEO_ID;
  
  video_priv = calloc(1, sizeof(*video_priv));
  bg_vs->priv = video_priv;

  memcpy(&video_priv->strh, strh, sizeof(*strh));
  
  bg_as = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  bg_as->stream_id = DV_AUDIO_ID;
  
  /* Tell the core that we do our own demuxing */
  ctx->flags |= BGAV_DEMUXER_SI_PRIVATE_FUNCS;
  
  /* Prevent the core from thinking these streams are unsupported */
  bg_vs->fourcc = BGAV_MK_FOURCC('d', 'v', 'c', ' ');
  bg_as->fourcc = BGAV_MK_FOURCC('g', 'a', 'v', 'l');

  while(keep_going)
    {
    if(!read_chunk_header(ctx->input, ch))
      return 0;
    switch(ch->ckID)
      {
      case ID_STRF:
        /* Format chunk can be skipped */
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_STRD:
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_JUNK:
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_LIST:
        keep_going = 0;
        break;
      case ID_INDX:
        if(!read_indx(ctx->input, &video_priv->indx, ch))
          return 0;

#ifdef DUMP_INDICES
        dump_indx(&video_priv->indx);
#endif
        video_priv->has_indx = 1;
        break;
      default:
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      }
    }

  /* Get frame rate (These values will be overwritten later,
     they are only for getting the correct duration */
  if(strh->dwScale && strh->dwRate)
    {
    bg_vs->data.video.format.timescale = strh->dwRate;
    bg_vs->data.video.format.frame_duration = strh->dwScale;
    }
  else if(avih->dwMicroSecPerFrame)
    {
    bg_vs->data.video.format.timescale = 1000000;
    bg_vs->data.video.format.frame_duration = avih->dwMicroSecPerFrame;
    }
  else
    {
    bg_vs->data.video.format.timescale = 25;
    bg_vs->data.video.format.frame_duration = 1;
    }
  bg_vs->timescale = bg_vs->data.video.format.timescale;
  bg_as->timescale = bg_vs->timescale;

  ctx->flags |= BGAV_DEMUXER_SI_PRIVATE_FUNCS;
  
  return 1;
  }


/* Get a stream ID (internally used) from the stream ID in the chunk header */

static int get_stream_id(uint32_t fourcc)
  {
  int ret;
  unsigned char c1, c2;

  c1 = ((fourcc & 0xff000000) >> 24);
  c2 = ((fourcc & 0x00ff0000) >> 16);

  if((c1 > '9') || (c1 < '0') ||
     (c2 > '9') || (c2 < '0'))
    return -1;
  
  ret = 10 * (c1 - '0') + (c2 - '0');
  return ret;  
  }

static void idx1_build_superindex(bgav_demuxer_context_t * ctx)
  {
  uint32_t i;
  int stream_id;
  audio_priv_t * avi_as;
  video_priv_t * avi_vs;
  bgav_stream_t * stream;
  avi_priv_t * avi;
  int64_t base_offset;
  
  avi = (avi_priv_t*)(ctx->priv);

  /* Reset timestamps */
  
  for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
    {
    avi_as = (audio_priv_t*)(ctx->tt->cur->audio_streams[i].priv);
    avi_as->total_bytes = 0;
    avi_as->total_blocks = 0;
    }

  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    {
    avi_vs = (video_priv_t*)(ctx->tt->cur->video_streams[i].priv);
    avi_vs->total_frames = 0;
    }

  ctx->si = bgav_superindex_create(avi->idx1.num_entries);

  if(avi->idx1.entries[0].dwChunkOffset == 4)
    base_offset = 4 + ctx->data_start;
  else
    /* For invalid files, which have the index relative to file start */
    base_offset = 4 + ctx->data_start -
      (avi->idx1.entries[0].dwChunkOffset - 4);
  
  for(i = 0; i < avi->idx1.num_entries; i++)
    {
    stream_id = get_stream_id(avi->idx1.entries[i].ckid);

    stream = bgav_track_find_stream_all(ctx->tt->cur, stream_id);
    if(!stream) /* Stream not used */
      continue;

    add_index_packet(ctx->si, stream,
                     avi->idx1.entries[i].dwChunkOffset + base_offset,
                     avi->idx1.entries[i].dwChunkLength,
                     !!(avi->idx1.entries[i].dwFlags & AVI_KEYFRAME));
    }
  }


static int open_avi(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int i;
  avi_priv_t * p;
  chunk_header_t ch;
  riff_header_t rh;
  strh_t strh;
  uint32_t fourcc;
  int keep_going;
    
  /* Create track */
  ctx->tt = bgav_track_table_create(1);
  
  /* Read the main header */
    
  if(!read_riff_header(ctx->input, &rh) ||
     ((rh.ckID != ID_RIFF) && (rh.ckID != ID_RIFF_ON2)) ||
     ((rh.fccType != ID_AVI) && (rh.fccType != ID_AVI_ON2)))
    goto fail;
  
  /* Skip all LIST chunks until a hdrl comes */

  while(1)
    {
    if(!read_riff_header(ctx->input, &rh) ||
       (rh.ckID != ID_LIST))
      goto fail;

    if(rh.fccType == ID_HDRL)
      break;
    else
      bgav_input_skip(ctx->input, rh.ckSize - 4);
    }
  
  /* Now, read the hdrl stuff */

  if(!read_chunk_header(ctx->input, &ch) ||
     ((ch.ckID != ID_AVIH) && (ch.ckID != ID_AVIH_ON2)))
    goto fail;

  /* Main avi header */

  p = calloc(1, sizeof(*p));
  ctx->priv = p;
  read_avih(ctx->input, &(p->avih), &ch);
  //  dump_avih(&(p->avih));
      
  /* Streams */

  read_chunk_header(ctx->input, &ch);
  
  for(i = 0; i < p->avih.dwStreams; i++)
    {
    if(!bgav_input_read_fourcc(ctx->input, &fourcc))
      {
      goto fail;
      }
    if(fourcc != ID_STRL)
      {
      goto fail;
      }
    if(!read_chunk_header(ctx->input, &ch) ||
       (ch.ckID != ID_STRH))
      goto fail;
    
    read_strh(ctx->input, &strh, &ch);
    //    dump_strh(&strh);

    if(strh.fccType == ID_AUDS)
      init_audio_stream(ctx, &strh, &ch);
    else if(strh.fccType == ID_VIDS)
      init_video_stream(ctx, &strh, &ch);
    else if(strh.fccType == ID_IAVS)
      init_iavs_stream(ctx, &strh, &ch);
    else
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Unknown stream .type = %c%c%c%c",
               (strh.fccType >> 24) & 0xff,
               (strh.fccType >> 16) & 0xff,
               (strh.fccType >> 8) & 0xff,
               strh.fccType & 0xff);
      return 0;
      }
    }

  keep_going = 1;
  while(keep_going)
    {
    /* AVIs have Junk */

    if(ch.ckID == ID_JUNK)
      {
      bgav_input_skip(ctx->input, ch.ckSize);
      if(!read_chunk_header(ctx->input, &ch))
        goto fail;
      }
    
    bgav_input_read_fourcc(ctx->input, &fourcc);
    
    switch(fourcc)
      {
      case ID_MOVI:
        keep_going = 0;
        break;
      case ID_ODML:

        if(!read_odml(ctx->input, &(p->odml), &ch))
          goto fail;
        p->has_odml = 1;

        //        dump_odml(&(p->odml));
        
        //        bgav_input_skip(ctx->input, ch.ckSize-4);
        break;
      case ID_INFO:
        p->info = bgav_RIFFINFO_read_without_header(ctx->input, ch.ckSize-4);
        break;
      default:
        bgav_input_skip(ctx->input, ch.ckSize-4);
        break;
      }

    if(!keep_going)
      break;
    
    if(!read_chunk_header(ctx->input, &ch))
      goto fail;
    }
#ifdef DUMP_HEADERS
  bgav_dprintf("movi:\n");
  dump_chunk_header(&ch);
#endif
  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;

  if(ch.ckSize)
    p->movi_size =  ch.ckSize - 4;
  else
    {
    if(ctx->input->total_bytes)
      p->movi_size = ctx->input->total_bytes - ctx->data_start;
    }

  if(ctx->input->input->seek_byte)
    {
    bgav_input_seek(ctx->input, ctx->data_start + p->movi_size, SEEK_SET);

    if(probe_idx1(ctx->input) && read_idx1(ctx->input, &(p->idx1)))
      {
      p->has_idx1 = 1;
#ifdef DUMP_INDICES
      dump_idx1(&(p->idx1));
#endif
      }
    bgav_input_seek(ctx->input, ctx->data_start, SEEK_SET);
    }

  /* Now, get the duration */

  /*
   *  Audio only AVIs won't have a duration, but aren't they illegal anyway?
   */

  if(ctx->tt->cur->num_video_streams)
    {
    if(p->has_odml && p->odml.has_dmlh)
      {
      ctx->tt->cur->duration =
        gavl_frames_to_time(ctx->tt->cur->video_streams[0].data.video.format.timescale,
                            ctx->tt->cur->video_streams[0].data.video.format.frame_duration,
                            p->odml.dmlh.dwTotalFrames);
      }
    else
      {
      ctx->tt->cur->duration =
        gavl_frames_to_time(ctx->tt->cur->video_streams[0].data.video.format.timescale,
                            ctx->tt->cur->video_streams[0].data.video.format.frame_duration,
                          p->avih.dwTotalFrames);
      
      }
    }
  
  /* Check, which index to build */

  if(ctx->input->input->seek_byte)
    {
    indx_build_superindex(ctx);
  
    if(!ctx->si && p->has_idx1)
      {
      idx1_build_superindex(ctx);
      }
    if(ctx->si)
      ctx->flags |= BGAV_DEMUXER_CAN_SEEK;
    }
  
  if(!ctx->tt->cur->duration)
    ctx->tt->cur->duration = GAVL_TIME_UNDEFINED;


  
  /* Build metadata */

  if(p->info)
    bgav_RIFFINFO_get_metadata(p->info, &(ctx->tt->cur->metadata));
  
  ctx->stream_description = bgav_sprintf("AVI");
  
  return 1;
  fail:
  return 0;
  }

#if 0
/* Only called, when no superindex is present */
static void select_track_avi(bgav_demuxer_context_t * ctx, int track)
  {
  avi_priv_t * priv;
  priv = (avi_priv_t*)(ctx->priv);
  }
#endif

static void close_avi(bgav_demuxer_context_t * ctx)
  {
  int i;
  avi_priv_t * priv;
  audio_priv_t * avi_as;
  video_priv_t * avi_vs;
  
  priv = (avi_priv_t*)(ctx->priv);
  
  if(priv)
    {
    if(priv->has_idx1)
      free_idx1(&(priv->idx1));
        
    if(priv->info)
      bgav_RIFFINFO_destroy(priv->info);
    if(priv->dv_dec)
      bgav_dv_dec_destroy(priv->dv_dec);
    if(priv->dv_frame_buffer)
      free(priv->dv_frame_buffer);
    free(priv);
    }
  for(i = 0; i < ctx->tt->cur->num_audio_streams; i++)
    {
    if(ctx->tt->cur->audio_streams[i].ext_data)
      free(ctx->tt->cur->audio_streams[i].ext_data);
    avi_as = (audio_priv_t*)(ctx->tt->cur->audio_streams[i].priv);
    if(avi_as)
      {
      if(avi_as->has_indx)
        free_indx(&(avi_as->indx));
      free(avi_as);
      }
    }
  
  for(i = 0; i < ctx->tt->cur->num_video_streams; i++)
    {
    if(ctx->tt->cur->video_streams[i].data.video.palette_size)
      free(ctx->tt->cur->video_streams[i].data.video.palette);
    if(ctx->tt->cur->video_streams[i].ext_data)
      free(ctx->tt->cur->video_streams[i].ext_data);

    avi_vs = (video_priv_t*)(ctx->tt->cur->video_streams[i].priv);

    if(avi_vs)
      {
      if(avi_vs->has_indx)
        free_indx(&(avi_vs->indx));
      free(avi_vs);
      }
    }
  }

static int next_packet_avi(bgav_demuxer_context_t * ctx)
  {
  chunk_header_t ch;
  bgav_packet_t * p;
  bgav_stream_t * s = (bgav_stream_t*)0;
  uint32_t fourcc;
  int stream_id;
  avi_priv_t * priv;
  video_priv_t* avi_vs;
  int result = 1;
  
  priv = (avi_priv_t*)(ctx->priv);

  if(priv->has_iavs && ctx->si)
    return next_packet_iavs_si(ctx);
  
  if(ctx->input->position + 8 >= ctx->data_start + priv->movi_size)
    {
    return 0;
    }
  while(!s)
    {
    if(!read_chunk_header(ctx->input, &ch))
      {
      return 0;
      }

#ifdef DUMP_HEADERS
    dump_chunk_header(&ch);
#endif

    if(ch.ckID == BGAV_MK_FOURCC('L','I','S','T'))
      {
      bgav_input_read_fourcc(ctx->input, &fourcc);
      }
    else if(ch.ckID == BGAV_MK_FOURCC('J','U','N','K'))
      {
      /* It's true, JUNK can appear EVERYWHERE!!! */
      bgav_input_skip(ctx->input, PADD(ch.ckSize));
      }
    else
      {
      stream_id = get_stream_id(ch.ckID);
      s = bgav_track_find_stream(ctx->tt->cur, stream_id);
      
      if(!s) /* Skip data for unused streams */
        bgav_input_skip(ctx->input, PADD(ch.ckSize));
      }
    }
  
  if(ch.ckSize)
    {
    if(priv->has_iavs)
      result = process_packet_iavs(ctx);
    else
      {
      p = bgav_stream_get_packet_write(s);
      
      bgav_packet_alloc(p, PADD(ch.ckSize));
      
      if(bgav_input_read_data(ctx->input, p->data, ch.ckSize) < ch.ckSize)
        {
        return 0;
        }
      p->data_size = ch.ckSize;
      
      if(s->type == BGAV_STREAM_VIDEO)
        {
        avi_vs = (video_priv_t*)s->priv;
        p->pts = s->data.video.format.frame_duration * s->in_position;
        avi_vs->total_frames++;
        }
      bgav_packet_done_write(p);
      }
    if(ch.ckSize & 1)
      bgav_input_skip(ctx->input, 1);
    }
  else if(s->type == BGAV_STREAM_VIDEO) // Increase timestamp for empty frames
    {
    avi_vs = (video_priv_t*)s->priv;
    avi_vs->total_frames++;
    }

  
  return result;
  }

static void seek_avi(bgav_demuxer_context_t * ctx, gavl_time_t time, int scale)
  {
  avi_priv_t * priv;
  priv = (avi_priv_t*)ctx->priv;
  if(ctx->si && priv->has_iavs)
    seek_iavs(ctx, time, scale);
  }

const const bgav_demuxer_t bgav_demuxer_avi =
  {
    .probe =       probe_avi,
    .open =        open_avi,
    .next_packet = next_packet_avi,
    .seek =        seek_avi,
    .close =       close_avi

  };
