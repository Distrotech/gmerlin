/*****************************************************************
 
  demux_avi.c
 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <nanosoft.h>

/* Define the variable below to get a detailed file dump
   on each open call */

// #define ENABLE_DUMP

/* AVI Flags */

#define AVI_HASINDEX       0x00000010  // Index at end of file?
#define AVI_MUSTUSEINDEX   0x00000020
#define AVI_ISINTERLEAVED  0x00000100
#define AVI_TRUSTCKTYPE    0x00000800  // Use CKType to find key frames?
#define AVI_WASCAPTUREFILE 0x00010000
#define AVI_COPYRIGHTED    0x00020000
#define AVIF_WASCAPTUREFILE     0x00010000
#define AVI_KEYFRAME       0x10

/* ODML Extensions */

#define AVI_INDEX_OF_CHUNKS 0x01
#define AVI_INDEX_OF_INDEXES 0x00

#define AVI_INDEX_2FIELD 0x01

#define ID_RIFF BGAV_MK_FOURCC('R','I','F','F')
#define ID_AVI  BGAV_MK_FOURCC('A','V','I',' ')
#define ID_LIST BGAV_MK_FOURCC('L','I','S','T')
#define ID_AVIH BGAV_MK_FOURCC('a','v','i','h')
#define ID_HDRL BGAV_MK_FOURCC('h','d','r','l')
#define ID_STRL BGAV_MK_FOURCC('s','t','r','l')
#define ID_STRH BGAV_MK_FOURCC('s','t','r','h')
#define ID_STRF BGAV_MK_FOURCC('s','t','r','f')
#define ID_STRD BGAV_MK_FOURCC('s','t','r','d')
#define ID_JUNK BGAV_MK_FOURCC('J','U','N','K')

#define ID_VIDS BGAV_MK_FOURCC('v','i','d','s')
#define ID_AUDS BGAV_MK_FOURCC('a','u','d','s')
#define ID_MOVI BGAV_MK_FOURCC('m','o','v','i')

/* OpenDML Extensions */

#define ID_ODML BGAV_MK_FOURCC('o','d','m','l')
#define ID_DMLH BGAV_MK_FOURCC('d','m','l','h')
#define ID_INDX BGAV_MK_FOURCC('i','n','d','x')

#define PADD(s) (((s)&1)?((s)+1):(s))

typedef struct
  {
  uint32_t ckID;
  uint32_t ckSize;
  } chunk_header_t;

static int read_chunk_header(bgav_input_context_t * input,
                             chunk_header_t * chunk)
  {
  return bgav_input_read_fourcc(input, &(chunk->ckID)) &&
    bgav_input_read_32_le(input, &(chunk->ckSize));
  }

#ifdef ENABLE_DUMP
static void dump_chunk_header(chunk_header_t * chunk)
  {
  fprintf(stderr, "chunk header:\n");
  fprintf(stderr, "  ckID: ");
  bgav_dump_fourcc(chunk->ckID);
  fprintf(stderr, "\n  ckSize %d\n", chunk->ckSize);
  }
#endif

typedef struct
  {
  uint32_t ckID;
  uint32_t ckSize;
  uint32_t fccType;
  } riff_header_t;

static int read_riff_header(bgav_input_context_t * input,
                            riff_header_t * chunk)
  {
  return bgav_input_read_fourcc(input, &(chunk->ckID)) &&
    bgav_input_read_32_le(input, &(chunk->ckSize)) &&
    bgav_input_read_fourcc(input, &(chunk->fccType));
  }

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


#ifdef ENABLE_DUMP
void dump_avih(avih_t * h)
  {
  fprintf(stderr, "avih:\n");
  fprintf(stderr, "  dwMicroSecPerFrame: %d\n",    h->dwMicroSecPerFrame);
  fprintf(stderr, "  dwMaxBytesPerSec: %d\n",      h->dwMaxBytesPerSec);
  fprintf(stderr, "  dwReserved1: %d\n",           h->dwReserved1);
  fprintf(stderr, "  dwFlags: %08x\n",             h->dwFlags);
  fprintf(stderr, "  dwTotalFrames: %d\n",         h->dwTotalFrames);
  fprintf(stderr, "  dwInitialFrames: %d\n",       h->dwInitialFrames);
  fprintf(stderr, "  dwStreams: %d\n",             h->dwStreams);
  fprintf(stderr, "  dwSuggestedBufferSize: %d\n", h->dwSuggestedBufferSize);
  fprintf(stderr, "  dwWidth: %d\n",               h->dwWidth);
  fprintf(stderr, "  dwHeight: %d\n",              h->dwHeight);
  fprintf(stderr, "  dwScale: %d\n",               h->dwScale);
  fprintf(stderr, "  dwRate: %d\n",                h->dwRate);
  fprintf(stderr, "  dwLength: %d\n",              h->dwLength);
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
    //    fprintf(stderr, "AVIH: Skipping %lld bytes\n",
    //            ch->ckSize - (input->position - start_pos));
    
    bgav_input_skip(input, PADD(ch->ckSize) - (input->position - start_pos));
    }
#ifdef ENABLE_DUMP
  dump_avih(ret);
#endif
  return result;
  }

typedef struct
  {
  uint32_t num_entries;
  struct
    {
    uint32_t ckid;
    uint32_t dwFlags;
    uint32_t dwChunkOffset;
    uint32_t dwChunkLength;
    /* These are not present in the AVI, but are calculated after reading */
    gavl_time_t time;
    } * entries;
  } idx1_t;

static void free_idx1(idx1_t * idx1)
  {
  if(idx1->entries)
    free(idx1->entries);
  }

#ifdef ENABLE_DUMP
static void dump_idx1(idx1_t * idx1)
  {
  int i;
  fprintf(stderr, "idx1, %d entries\n", idx1->num_entries);
  for(i = 0; i < idx1->num_entries; i++)
    {
    fprintf(stderr, "ID: ");
    bgav_dump_fourcc(idx1->entries[i].ckid);
    fprintf(stderr, " Flags: %08x", idx1->entries[i].dwFlags);
    fprintf(stderr, " O: %08x", idx1->entries[i].dwChunkOffset);
    fprintf(stderr, " L: %08x", idx1->entries[i].dwChunkLength);
    fprintf(stderr, " T: %f\n", gavl_time_to_seconds(idx1->entries[i].time));
    }
  }
#endif

static int read_idx1(bgav_input_context_t * input, idx1_t * ret)
  {
  int i;
  chunk_header_t ch;
  //  fprintf(stderr, "Reading Index %08llx\n", input->position);
  if(!read_chunk_header(input, &ch))
    return 0;
  //  bgav_dump_fourcc(ch.ckID);
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
  //  fprintf(stderr, "\nReading Index done %d\n", ret->num_entries);
  return 1;
  }

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


#ifdef ENABLE_DUMP
static void dump_strh(strh_t * ret)
  {
  fprintf(stderr, "strh\n  fccType: ");
  bgav_dump_fourcc(ret->fccType);
  fprintf(stderr, "\n  fccHandler: ");
  bgav_dump_fourcc(ret->fccHandler);
  fprintf(stderr, "\n  dwFlags: %d (%08x)\n",
          ret->dwFlags, ret->dwFlags);
  fprintf(stderr, "  dwReserved1: %d (%08x)\n",
          ret->dwReserved1, ret->dwReserved1);
  fprintf(stderr, "  dwInitialFrames: %d (%08x)\n",
          ret->dwInitialFrames, ret->dwInitialFrames);
  fprintf(stderr, "  dwScale: %d (%08x)\n", ret->dwScale, ret->dwScale);
  fprintf(stderr, "  dwRate: %d (%08x)\n", ret->dwRate, ret->dwRate);
  fprintf(stderr, "  dwStart: %d (%08x)\n", ret->dwStart, ret->dwStart);
  fprintf(stderr, "  dwLength: %d (%08x)\n", ret->dwLength, ret->dwLength);
  fprintf(stderr, "  dwSuggestedBufferSize: %d (%08x)\n",
          ret->dwSuggestedBufferSize,
          ret->dwSuggestedBufferSize);
  fprintf(stderr, "  dwQuality: %d (%08x)\n", ret->dwQuality, ret->dwQuality);
  fprintf(stderr, "  dwSampleSize: %d (%08x)\n", ret->dwSampleSize, ret->dwSampleSize);
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
    //    fprintf(stderr, "STRH: Skipping %lld bytes\n",
    //            ch->ckSize - (input->position - start_pos));
    
    bgav_input_skip(input, PADD(ch->ckSize) - (input->position - start_pos));
    }
#ifdef ENABLE_DUMP
  dump_strh(ret);
#endif
  return result;
  }

/* odml extensions */

/* dmlh */

typedef struct
  {
  uint32_t dwTotalFrames;
  } dmlh_t;


#ifdef ENABLE_DUMP
static void dump_dmlh(dmlh_t * dmlh)
  {
  fprintf(stderr, "dmlh:\n");
  fprintf(stderr, "  dwTotalFrames: %d\n", dmlh->dwTotalFrames);
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
    //    fprintf(stderr, "dmlh: Skipping %lld bytes\n",
    //            ch->ckSize - (input->position - start_pos));
    bgav_input_skip(input, PADD(ch->ckSize) - (input->position - start_pos));
    }
  return 1;
  }
/* odml */

typedef struct
  {
  int has_dmlh;
  dmlh_t dmlh;
  } odml_t;

#ifdef ENABLE_DUMP
static void dump_odml(odml_t * odml)
  {
  fprintf(stderr, "odml:\n");
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
        //        fprintf(stderr, "Unknown chunk inside odml list, skipping rest:\n");
        //        dump_chunk_header(&ch1);
        keep_going = 0;
        break;
      }
    }
  
  if(input->position - start_pos < ch->ckSize - 4)
    {
    //    fprintf(stderr, "odml: Skipping %lld bytes\n",
    //            ch->ckSize - 4 - input->position - start_pos);
    bgav_input_skip(input, ch->ckSize - 4 - input->position - start_pos);
    }
#ifdef ENABLE_DUMP
  dump_odml(ret);
#endif

  return 1;
  }

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
        int64_t qwOffset;
        int32_t dwSize;
        uint32_t dwDuration;
        struct indx_s * subindex;
        } * entries;
      } index;
    } i;
  } indx_t;

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
        malloc(ret->nEntriesInUse * sizeof(*(ret->i.index.entries)));
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
    //    fprintf(stderr, "indx: Skipping %lld bytes\n",
    //            ch->ckSize - (input->position - pos));
    
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
  //  fprintf(stderr, "Read indx\n");
  return 1;
  }

#ifdef ENABLE_DUMP
static void dump_indx(indx_t * indx)
  {
  int i;
  fprintf(stderr, "indx:\n");
  fprintf(stderr, "wLongsPerEntry: %d\n", indx->wLongsPerEntry);
  fprintf(stderr, "bIndexSubType:  0x%02x\n", indx->bIndexSubType);
  fprintf(stderr, "bIndexType:     0x%02x\n", indx->bIndexType);
  fprintf(stderr, "nEntriesInUse:  %d\n", indx->nEntriesInUse);
  fprintf(stderr, "dwChunkID:      ");
  bgav_dump_fourcc(indx->dwChunkID);
  fprintf(stderr, "\n");
  
  switch(indx->bIndexType)
    {
    case AVI_INDEX_OF_INDEXES:
      for(i = 0; i < 3; i++)
        fprintf(stderr, "dwReserved[%d]: %d\n", i,
                indx->i.index.dwReserved[i]);

      for(i = 0; i < indx->nEntriesInUse; i++)
        {
        fprintf(stderr, "%d qwOffset: %lld dwSize: %d dwDuration: %d\n", i,
                indx->i.index.entries[i].qwOffset,
                indx->i.index.entries[i].dwSize,
                indx->i.index.entries[i].dwDuration);
        if(indx->i.index.entries[i].subindex)
          {
          fprintf(stderr, "Subindex follows:\n");
          dump_indx(indx->i.index.entries[i].subindex);
          }
        }
      break;
    case AVI_INDEX_OF_CHUNKS:
      
      if(indx->bIndexSubType == AVI_INDEX_2FIELD)
        {
        fprintf(stderr, "qwBaseOffset:  %lld\n",
                indx->i.field_chunk.qwBaseOffset);
        fprintf(stderr, "dwReserved3:   %d\n",
                indx->i.field_chunk.dwReserved3);
        
        for(i = 0; i < indx->nEntriesInUse; i++)
          {
          fprintf(stderr, "%d dwOffset: %d dwSize: %d dwOffsetField2: %d Keyframe: %d\n", i,
                  indx->i.field_chunk.entries[i].dwOffset,
                  indx->i.field_chunk.entries[i].dwSize & 0x7FFFFFFF,
                  indx->i.field_chunk.entries[i].dwOffsetField2,
                  !(indx->i.field_chunk.entries[i].dwSize & 0x80000000));
          }
        }
      else
        {
        fprintf(stderr, "qwBaseOffset:  %lld\n",
                indx->i.chunk.qwBaseOffset);
        fprintf(stderr, "dwReserved3:   %d\n",
                indx->i.chunk.dwReserved3);
        
        for(i = 0; i < indx->nEntriesInUse; i++)
          {
          fprintf(stderr, "%d dwOffset: %d dwSize: 0x%08x Keyframe: %d\n", i,
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


typedef struct
  {
  avih_t avih;
  idx1_t idx1;
  int has_idx1;
  int has_idx1_timestamps; /* True if the timestamps are already calculated */
  int64_t movi_start;
  uint32_t movi_size;
  uint32_t index_position;
  int do_seek; /* Are wee seeking? */

  odml_t odml;
  int has_odml;
  
  } avi_priv;

typedef struct
  {
  strh_t strh;
  uint32_t index_position;
  int vbr;
  int64_t total_bytes;
  int64_t total_blocks;

  indx_t indx;
  int has_indx;
  } audio_priv_t;

typedef struct
  {
  strh_t strh;
  uint32_t index_position;
  int64_t total_frames;

  indx_t indx;
  int has_indx;
  } video_priv_t;

static int probe_avi(bgav_input_context_t * input)
  {
  uint8_t data[12];
  if(bgav_input_get_data(input, data, 12) < 12)
    return 0;

  return (data[0] == 'R') &&
    (data[1] == 'I') &&
    (data[2] == 'F') &&
    (data[3] == 'F') &&
    (data[8] == 'A') &&
    (data[9] == 'V') &&
    (data[10] == 'I') &&
    (data[11] == ' ');
  }

static int init_audio_stream(bgav_demuxer_context_t * ctx,
                             strh_t * strh, chunk_header_t * ch)
  {
  uint8_t * buf, * pos;
  bgav_WAVEFORMATEX wf;
  int keep_going;
  keep_going = 1;
  bgav_stream_t * bg_as;
  audio_priv_t * avi_as;

  //  fprintf(stderr, "Init audio stream\n");
  
  bg_as = bgav_track_add_audio_stream(ctx->tt->current_track);
  avi_as = calloc(1, sizeof(*avi_as));
  bg_as->priv = avi_as;

  memcpy(&(avi_as->strh), strh, sizeof(*strh));
  
  while(keep_going)
    {
    read_chunk_header(ctx->input, ch);
    switch(ch->ckID)
      {
      case ID_STRF:
        //        fprintf(stderr, "Found format %d bytes\n", ch.ckSize);
        buf = malloc(ch->ckSize);
        if(bgav_input_read_data(ctx->input, buf, ch->ckSize) < ch->ckSize)
          return 0;
        pos = buf;
        bgav_WAVEFORMATEX_read(&wf, &pos);
        bgav_WAVEFORMATEX_get_format(&wf, bg_as);
#ifdef ENABLE_DUMP
        bgav_WAVEFORMATEX_dump(&wf);
#endif
        if(wf.cbSize)
          {
          bg_as->ext_size = wf.cbSize;
          bg_as->ext_data = malloc(wf.cbSize);
          memcpy(bg_as->ext_data, pos, wf.cbSize);
          }
        //        bg_as->fourcc = BGAV_WAVID_2_FOURCC(wf.wFormatTag);
        if(!bg_as->data.audio.bits_per_sample)
          bg_as->data.audio.bits_per_sample = strh->dwSampleSize * 8;          
        
        /* Seek support */
        if(!strh->dwSampleSize)
          {
          //          fprintf(stderr, "Audio is VBR!\n");
          avi_as->vbr = 1;
          }
        free(buf);
        break;
      case ID_STRD:
        //        fprintf(stderr, "Found data %d bytes\n", ch->ckSize);
        
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_JUNK:
        //        fprintf(stderr, "Found junk %d bytes\n", ch->ckSize);
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_LIST:
        keep_going = 0;
        break;
      case ID_INDX:
        if(!read_indx(ctx->input, &avi_as->indx, ch))
          return 0;
        //        dump_indx(&avi_as->indx);
        avi_as->has_indx = 1;
        break;
      default:
        //        fprintf(stderr, "Unknown subchunk ");
        //        bgav_dump_fourcc(ch->ckID);
        //        fprintf(stderr, "\n");
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      }
    }
  bg_as->stream_id = (ctx->tt->current_track->num_audio_streams + ctx->tt->current_track->num_video_streams) - 1;
  return 1;
  }

#define BYTE_2_COLOR(c) (((uint16_t)c) | ((uint16_t)(c)<<8))

static int init_video_stream(bgav_demuxer_context_t * ctx,
                             strh_t * strh, chunk_header_t * ch)
  {
  avih_t * avih;
  uint8_t * buf, * pos;
  bgav_BITMAPINFOHEADER bh;
  int keep_going, i;
  keep_going = 1;
  
  avih = &(((avi_priv*)(ctx->priv))->avih);
    
  bgav_stream_t * bg_vs;
  audio_priv_t * avi_vs;
  
  bg_vs = bgav_track_add_video_stream(ctx->tt->current_track);
  avi_vs = calloc(1, sizeof(*avi_vs));

  memcpy(&(avi_vs->strh), strh, sizeof(*strh));
  
  bg_vs->priv = avi_vs;

  //  fprintf(stderr, "Init video stream\n");

  while(keep_going)
    {
    read_chunk_header(ctx->input, ch);
    switch(ch->ckID)
      {
      case ID_STRF:
        //        fprintf(stderr, "Found format %d bytes\n", ch->ckSize);

        buf = malloc(ch->ckSize);
        if(bgav_input_read_data(ctx->input, buf, ch->ckSize) < ch->ckSize)
          return 0;
        pos = buf;
        bgav_BITMAPINFOHEADER_read(&bh, &pos);
        bgav_BITMAPINFOHEADER_get_format(&bh, bg_vs);
#ifdef ENABLE_DUMP
        bgav_BITMAPINFOHEADER_dump(&bh);
#endif
        if(ch->ckSize > 40)
          {
          //          fprintf(stderr, "Adding extradata %d bytes\n",
          //                  ch->ckSize - 40);
          bg_vs->ext_size = ch->ckSize - 40;
          bg_vs->ext_data = malloc(bg_vs->ext_size);
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
        //        fprintf(stderr, "Found data %d bytes\n", ch->ckSize);
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_JUNK:
        //        fprintf(stderr, "Found junk %d bytes\n", ch->ckSize);
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_LIST:
        keep_going = 0;
        break;
      case ID_INDX:
        if(!read_indx(ctx->input, &avi_vs->indx, ch))
          return 0;
        //        dump_indx(&avi_vs->indx);
        avi_vs->has_indx = 1;
        break;
      default:
        //        fprintf(stderr, "Unknown subchunk ");
        //        bgav_dump_fourcc(ch->ckID);
        //        fprintf(stderr, "\n");
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
  bg_vs->stream_id = (ctx->tt->current_track->num_audio_streams + ctx->tt->current_track->num_video_streams) - 1;
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


/*
 *  Calculate the timestamp field of each chunk in the index
 */

static void idx1_calculate_timestamps(bgav_demuxer_context_t * ctx)
  {
  uint32_t i, j;
  int stream_id;
  audio_priv_t * avi_as;
  video_priv_t * avi_vs;
  bgav_stream_t * stream;
  avi_priv * avi;
  int samplerate;
  avi = (avi_priv*)(ctx->priv);
  if(avi->has_idx1_timestamps)
    return;

  //  fprintf(stderr, "Getting index timestamps\n");
  
  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    avi_as = (audio_priv_t*)(ctx->tt->current_track->audio_streams[i].priv);
    avi_as->total_bytes = 0;
    avi_as->total_blocks = 0;
    }

  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    avi_vs = (video_priv_t*)(ctx->tt->current_track->video_streams[i].priv);
    avi_vs->total_frames = 0;
    }
  
  for(i = 0; i < avi->idx1.num_entries; i++)
    {
    stream_id = get_stream_id(avi->idx1.entries[i].ckid);
    stream = (bgav_stream_t *)0;
    for(j = 0; j < ctx->tt->current_track->num_audio_streams; j++)
      {
      if(ctx->tt->current_track->audio_streams[j].stream_id == stream_id)
        {
        stream = &(ctx->tt->current_track->audio_streams[j]);
        break;
        }
      }
    if(!stream)
      {
      for(j = 0; j < ctx->tt->current_track->num_video_streams; j++)
        {
        if(ctx->tt->current_track->video_streams[j].stream_id == stream_id)
          {
          stream = &(ctx->tt->current_track->video_streams[j]);
          break;
          }
        }
      }
  
    if(!stream) /* Stream not used */
      continue;

    if(stream->type == BGAV_STREAM_AUDIO)
      {
      avi_as = (audio_priv_t*)(stream->priv);
      samplerate = stream->data.audio.format.samplerate;
            
      /* Now, we must distinguish between VBR and CBR audio */

      if ((avi_as->strh.dwSampleSize == 0) && (avi_as->strh.dwScale > 1))
        {
        /* variable bitrate */
        avi->idx1.entries[i].time =
          (samplerate * (gavl_time_t)avi_as->total_blocks *
           (gavl_time_t)avi_as->strh.dwScale) / avi_as->strh.dwRate;
        }
      else
        {
        /* constant bitrate */
        //        lprintf("get_audio_pts: CBR: nBlockAlign=%d, dwSampleSize=%d\n",
        //                at->wavex->nBlockAlign, at->dwSampleSize);
        if(stream->data.audio.block_align)
          {
          avi->idx1.entries[i].time =
            ((gavl_time_t)avi_as->total_bytes * (gavl_time_t)avi_as->strh.dwScale *
             samplerate) /
            (stream->data.audio.block_align * avi_as->strh.dwRate);
          }
        else
          {
          avi->idx1.entries[i].time =
            (samplerate * (gavl_time_t)avi_as->total_bytes *
             (gavl_time_t)avi_as->strh.dwScale) /
            (avi_as->strh.dwSampleSize * avi_as->strh.dwRate);
          }
        }
      
      /* Increase block count */
            
      if(stream->data.audio.block_align)
        {
        avi_as->total_blocks += (avi->idx1.entries[i].dwChunkLength + stream->data.audio.block_align - 1)
          / stream->data.audio.block_align;
        }
      else
        {
        avi_as->total_blocks += 1;
        }
      avi_as->total_bytes += avi->idx1.entries[i].dwChunkLength;
      }
    else if(stream->type == BGAV_STREAM_VIDEO)
      {
      avi_vs = (video_priv_t*)(stream->priv);
      avi->idx1.entries[i].time = stream->data.video.format.frame_duration *
        avi_vs->total_frames;
      avi_vs->total_frames++;
      }
    }
  avi->has_idx1_timestamps = 1;
  }

/* Fix offsets (yes, some files have the index offset relative to the file start,
   indstead of relative to the movi atom */

static void idx1_fix_offsets(bgav_demuxer_context_t * ctx)
  {
  int i, err;
  avi_priv * avi;
  avi = (avi_priv*)(ctx->priv);
  if(avi->idx1.entries[0].dwChunkOffset == 4)
    return;

  err = avi->idx1.entries[0].dwChunkOffset - 4;

  for(i = 0; i < avi->idx1.num_entries; i++)
    avi->idx1.entries[i].dwChunkOffset -= err;
  }


static int open_avi(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int i;
  avi_priv * p;
  chunk_header_t ch;
  riff_header_t rh;
  strh_t strh;
  uint32_t fourcc;
  int keep_going;
  
  /* Create track */
  ctx->tt = bgav_track_table_create(1);
  
  /* Read the main header */
    
  if(!read_riff_header(ctx->input, &rh) ||
     (rh.ckID != ID_RIFF) ||
     (rh.fccType != ID_AVI))
    goto fail;
  
  /* Now, a LIST chunk must come */

  if(!read_riff_header(ctx->input, &rh) ||
     (rh.ckID != ID_LIST) ||
     (rh.fccType != ID_HDRL))
    goto fail;

  /* Now, read the hdrl stuff */

  if(!read_chunk_header(ctx->input, &ch) ||
     (ch.ckID != ID_AVIH))
    goto fail;

  /* Main avi header */

  p = calloc(1, sizeof(*p));
  ctx->priv = p;
  read_avih(ctx->input, &(p->avih), &ch);
  //  dump_avih(&(p->avih));
      
  /* Streams */

  read_chunk_header(ctx->input, &ch);
  
  for(i = 0; i < (p->avih).dwStreams; i++)
    {
    if(!bgav_input_read_fourcc(ctx->input, &fourcc))
      {
      fprintf(stderr, "EOF during header parsing\n");
      goto fail;
      }
    if(fourcc != ID_STRL)
      {
      fprintf(stderr, "Wrong fourcc ");
      bgav_dump_fourcc(fourcc);
      fprintf(stderr, "\nExpected ");
      bgav_dump_fourcc(ID_STRL);
      fprintf(stderr, "\n");
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
#if 0
    else
      {
      fprintf(stderr, "Unknown stream type: ");
      bgav_dump_fourcc(strh.fccType);
      fprintf(stderr, "\n");
      }
#endif
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
        //        fprintf(stderr, "Found odml chunk\n");
        //        dump_chunk_header(&ch);

        if(!read_odml(ctx->input, &(p->odml), &ch))
          goto fail;
        p->has_odml = 1;

        //        dump_odml(&(p->odml));
        
        //        bgav_input_skip(ctx->input, ch.ckSize-4);
        break;
      default:
#if 0
        fprintf(stderr, "Skipping unknown chunk\n");
        bgav_dump_fourcc(fourcc);
        fprintf(stderr, "\n");
        dump_chunk_header(&ch);
#endif
        bgav_input_skip(ctx->input, ch.ckSize-4);
        break;
      }

    if(!keep_going)
      break;
    
    if(!read_chunk_header(ctx->input, &ch))
      goto fail;
    }
#ifdef ENABLE_DUMP
  fprintf(stderr, "movi:\n");
  dump_chunk_header(&ch);
#endif
  p->movi_start = ctx->input->position;

  if(ch.ckSize)
    p->movi_size =  ch.ckSize - 4;
  else
    {
    fprintf(stderr, "Warning: MOVI Atom has zero size, seeking will be impossible\n");
    if(ctx->input->total_bytes)
      p->movi_size = ctx->input->total_bytes - p->movi_start;
    }
  //  fprintf(stderr, "Reached MOVI atom start: %lld size: %d\n", p->movi_start, p->movi_size);
  //  bgav_dump_fourcc(fourcc);
  if((p->avih.dwFlags & AVI_HASINDEX) && (ctx->input->input->seek_byte))
    {
    /* The index must come after the MOVI chunk */
    bgav_input_seek(ctx->input, p->movi_start + p->movi_size, SEEK_SET);
    if(read_idx1(ctx->input, &(p->idx1)))
      {
      p->has_idx1 = 1;
      ctx->can_seek = 1;
      idx1_calculate_timestamps(ctx);
      idx1_fix_offsets(ctx);
      //      dump_idx1(&(p->idx1));
      //      fprintf(stderr, "Found standard index\n");
      
#ifdef ENABLE_DUMP
      dump_idx1(&(p->idx1));
#endif
      }
    bgav_input_seek(ctx->input, p->movi_start, SEEK_SET);
    }

  /* Now, get the duration */

  if(p->has_odml && p->odml.has_dmlh)
    {
    ctx->tt->current_track->duration =
      gavl_frames_to_time(ctx->tt->current_track->video_streams[0].data.video.format.timescale,
                          ctx->tt->current_track->video_streams[0].data.video.format.frame_duration,
                          p->odml.dmlh.dwTotalFrames);
    }
  else
    {
    ctx->tt->current_track->duration =
      gavl_frames_to_time(ctx->tt->current_track->video_streams[0].data.video.format.timescale,
                          ctx->tt->current_track->video_streams[0].data.video.format.frame_duration,
                          p->avih.dwTotalFrames);
    
    }

  if(!ctx->tt->current_track->duration)
    ctx->tt->current_track->duration = GAVL_TIME_UNDEFINED;
  
  //  for(i = 1; i < 10; i++)
  //    {
  //    next_packet_avi(ctx);
  //    }

  ctx->stream_description = bgav_sprintf("Microsoft AVI");
  
  return 1;
  fail:
  return 0;
  }

static void close_avi(bgav_demuxer_context_t * ctx)
  {
  int i;
  avi_priv * priv;
  audio_priv_t * avi_as;
  video_priv_t * avi_vs;
  
  priv = (avi_priv*)(ctx->priv);

  if(priv->has_idx1)
    free_idx1(&(priv->idx1));
  
  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    if(ctx->tt->current_track->audio_streams[i].ext_data)
      free(ctx->tt->current_track->audio_streams[i].ext_data);
    avi_as = (audio_priv_t*)(ctx->tt->current_track->audio_streams[i].priv);
    if(avi_as->has_indx)
      free_indx(&(avi_as->indx));
    
    free(avi_as);
    }

  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    if(ctx->tt->current_track->video_streams[i].data.video.palette_size)
      free(ctx->tt->current_track->video_streams[i].data.video.palette);
    if(ctx->tt->current_track->video_streams[i].ext_data)
      free(ctx->tt->current_track->video_streams[i].ext_data);

    avi_vs = (video_priv_t*)(ctx->tt->current_track->video_streams[i].priv);
    if(avi_vs->has_indx)
      free_indx(&(avi_vs->indx));

    free(avi_vs);
    }
  
  free(priv);
  }


static int next_packet_avi(bgav_demuxer_context_t * ctx)
  {
  chunk_header_t ch;
  bgav_packet_t * p;
  bgav_stream_t * s = (bgav_stream_t*)0;
  uint32_t fourcc;
  int stream_id;
  avi_priv * priv;
  priv = (avi_priv*)(ctx->priv);
      
  if(ctx->input->position + 8 >= priv->movi_start + priv->movi_size)
    return 0;

  //  fprintf(stderr, "Position: %lld\n", ctx->input->position);

  /* Sometimes, there are unknown entries in the index, we skip them here */
  if(priv->has_idx1)
    {
    if(priv->index_position >= priv->idx1.num_entries)
      {
      //      fprintf(stderr, "AVI EOF\n");
      return 0;
      }

    while(!s)
      {
      stream_id = get_stream_id(priv->idx1.entries[priv->index_position].ckid);
      s = bgav_track_find_stream(ctx->tt->current_track, stream_id);
      
      if(!s)
        priv->index_position++;

//      fprintf(stderr, "%d %d\n", priv->index_position, priv->idx1.num_entries);
      
      }
    ch.ckID   = priv->idx1.entries[priv->index_position].ckid;
    ch.ckSize = priv->idx1.entries[priv->index_position].dwChunkLength;
    //    fprintf(stderr, "Stream ID: %d, stream: %p length: %d\n", stream_id, s,
    //            ch.ckSize);

    /* Skip to the Chunk header */
    
    bgav_input_skip(ctx->input,
                    priv->movi_start +
                    priv->idx1.entries[priv->index_position].dwChunkOffset + 4 -
                    ctx->input->position);
    //    fprintf(stderr, "Position: %x\n", priv->idx1.entries[priv->index_position].dwChunkOffset);

    }
  else /* No index present */
    {
    while(!s)
      {
      if(!read_chunk_header(ctx->input, &ch))
        return 0;
      
      if(ch.ckID == BGAV_MK_FOURCC('L','I','S','T'))
        {
        //    fprintf(stderr, "Got list chunk\n");
        bgav_input_read_fourcc(ctx->input, &fourcc);
        //    bgav_dump_fourcc(fourcc);
        //    fprintf(stderr, "\n");
        }
      else
        {
        stream_id = get_stream_id(ch.ckID);
        s = bgav_track_find_stream(ctx->tt->current_track, stream_id);
        }
      }
    }
  /*
    fprintf(stderr, "Position: %lld, index position: %d\n",
    ctx->input->position - priv->movi_start,
    priv->idx1.entries[priv->index_position].dwChunkOffset+4);
    fprintf(stderr, "Size: %d, index size: %d\n",
    ch.ckSize,
    priv->idx1.entries[priv->index_position].dwChunkLength);
  */

#ifdef ENABLE_DUMP
  dump_chunk_header(&ch);
#endif
  /* Skip ignored streams */

  if(priv->do_seek)
    {
    if(((s->type == BGAV_STREAM_AUDIO) &&
        (((audio_priv_t*)(s->priv))->index_position > priv->index_position)) ||
       ((s->type == BGAV_STREAM_VIDEO) &&
        (((video_priv_t*)(s->priv))->index_position > priv->index_position)))
      {
      bgav_input_skip(ctx->input, PADD(ch.ckSize));
      priv->index_position++;
      return 1;
      }
    }

  p = bgav_packet_buffer_get_packet_write(s->packet_buffer);
  
  bgav_packet_alloc(p, PADD(ch.ckSize));
  
  if(bgav_input_read_data(ctx->input, p->data, ch.ckSize) < ch.ckSize)
    return 0;
  p->data_size = ch.ckSize;

  if(priv->has_idx1_timestamps)
    {
    p->timestamp_scaled = priv->idx1.entries[priv->index_position].time;
    if(s->type == BGAV_STREAM_AUDIO)
      {
      p->timestamp = (p->timestamp_scaled * GAVL_TIME_SCALE) /
        s->data.audio.format.samplerate;
      }
    else if(s->type == BGAV_STREAM_VIDEO)
      {
      p->timestamp = (p->timestamp_scaled * GAVL_TIME_SCALE) /
        s->data.video.format.timescale;
      }
    p->keyframe  = !!(priv->idx1.entries[priv->index_position].dwFlags & AVI_KEYFRAME);
    }
  bgav_packet_done_write(p);
    
  //  fprintf(stderr, " Stream ID %d\n", stream_id);
  if(ch.ckSize & 1)
    bgav_input_skip(ctx->input, 1);
  priv->index_position++;
  return 1;
  }

static void seek_avi(bgav_demuxer_context_t * ctx, gavl_time_t time)
  {
  int i, j;
  uint32_t chunk_min;
  uint32_t chunk_max;
  audio_priv_t * avi_as;
  video_priv_t * avi_vs;
  avi_priv * avi = (avi_priv*)(ctx->priv);
  bgav_stream_t * stream;
  int stream_id;
  int keep_going;
  int64_t time_scaled;
  
  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    avi_as = (audio_priv_t*)(ctx->tt->current_track->audio_streams[i].priv);
    avi_as->index_position = -1;
    }
    
  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    avi_vs = (video_priv_t*)(ctx->tt->current_track->video_streams[i].priv);
    avi_vs->index_position = -1;
    }

  /* Set the index_position members of the streams */

  //  fprintf(stderr, "Seek AVI\n");
  
  for(i = avi->idx1.num_entries - 1; i >= 0; i--)
    {
    keep_going = 0;
    stream = (bgav_stream_t*)0;
    
    stream_id = get_stream_id(avi->idx1.entries[i].ckid);
    for(j = 0; j < ctx->tt->current_track->num_audio_streams; j++)
      {
      if(stream_id == ctx->tt->current_track->audio_streams[j].stream_id)
        {
        stream = &(ctx->tt->current_track->audio_streams[j]);
        avi_as = (audio_priv_t*)(stream->priv);

        time_scaled = (time * stream->data.audio.format.samplerate) / 
          GAVL_TIME_SCALE;
        
        if(avi_as->index_position == -1)
          {
          if(avi->idx1.entries[i].time > time_scaled)
            {
            keep_going = 1;
            break;
            }
          
          /* Set timestamp and stuff */

#if 0 /* We assume that all audio packets are keyframes */
          if(avi->idx1.entries[i].dwFlags & AVI_KEYFRAME)
            {
#endif
          /* We assume all audio packets to be key frames */
          avi_as->index_position = i;
          stream->time_scaled = avi->idx1.entries[i].time;
          stream->time = (stream->time_scaled * GAVL_TIME_SCALE) / 
            stream->data.audio.format.samplerate;
#if 0
            }
          
          else
            {
            fprintf(stderr, "%d: Audio packet is no keyframe\n", i);
            keep_going = 1;
            }
#endif
          }
        break;
        }
      }
    if(!stream)
      {
      for(j = 0; j < ctx->tt->current_track->num_video_streams; j++)
        {
        if(stream_id == ctx->tt->current_track->video_streams[j].stream_id)
          {
          stream = &(ctx->tt->current_track->video_streams[j]);
          avi_vs = (video_priv_t*)(stream->priv);
          
          if(avi_vs->index_position == -1)
            {
            time_scaled = (time * stream->data.video.format.timescale) / 
              GAVL_TIME_SCALE;
            
            if(avi->idx1.entries[i].time > time_scaled)
              {
              keep_going = 1;
              break;
              }
            if(avi->idx1.entries[i].dwFlags & AVI_KEYFRAME)
              {
              avi_vs->index_position = i;
              stream->time_scaled = avi->idx1.entries[i].time;
              stream->time = (avi->idx1.entries[i].time * GAVL_TIME_SCALE) / 
                stream->data.video.format.timescale;
              }
            else
              keep_going = 1;
            }
          break;
          }
        }
      }
    
    /* Check if we are done */
    
    for(j = 0; j < ctx->tt->current_track->num_audio_streams; j++)
      {
      avi_as = (audio_priv_t*)(ctx->tt->current_track->audio_streams[j].priv);
      if(avi_as->index_position == -1)
        keep_going = 1;
      }

    for(j = 0; j < ctx->tt->current_track->num_video_streams; j++)
      {
      avi_vs = (video_priv_t*)(ctx->tt->current_track->video_streams[j].priv);
      if(avi_vs->index_position == -1)
        keep_going = 1;
      }
    if(!keep_going)
      break;
    }

  /* Get the minimum and maximum chunk indices */
  chunk_min = ~0;;
  chunk_max = 0;

  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    avi_as = (audio_priv_t*)(ctx->tt->current_track->audio_streams[i].priv);
    if(avi_as->index_position < chunk_min)
      chunk_min = avi_as->index_position;
    if(avi_as->index_position > chunk_max)
      chunk_max = avi_as->index_position;
    //    fprintf(stderr, "AS: %d\n", avi_as->index_position);
    }

  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    avi_vs = (video_priv_t*)(ctx->tt->current_track->video_streams[i].priv);
    if(avi_vs->index_position < chunk_min)
      chunk_min = avi_vs->index_position;
    if(avi_vs->index_position > chunk_max)
      chunk_max = avi_vs->index_position;
    //    fprintf(stderr, "VS: %d\n", avi_vs->index_position);
    }

  /* Ok we have everything, let's seek */
  //  fprintf(stderr, "Seek: %lld
  bgav_input_seek(ctx->input, avi->idx1.entries[chunk_min].dwChunkOffset - 4 + avi->movi_start, SEEK_SET);
  avi->index_position = chunk_min;
  avi->do_seek = 1;

  //  fprintf(stderr, "%d %d, %d\n", avi->index_position, chunk_min, chunk_max);
  
  while(avi->index_position <= chunk_max)
    {
    //    fprintf(stderr, "%d %d, %d\n", avi->index_position, chunk_min, chunk_max);
    next_packet_avi(ctx);
    }
  avi->do_seek = 0;
  }


bgav_demuxer_t bgav_demuxer_avi =
  {
    probe:       probe_avi,
    open:        open_avi,
    next_packet: next_packet_avi,
    seek:        seek_avi,
    close:       close_avi

  };
