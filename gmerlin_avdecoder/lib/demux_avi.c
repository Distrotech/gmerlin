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

/* AVI Flags */

#define AVI_HASINDEX       0x00000010  // Index at end of file?
#define AVI_MUSTUSEINDEX   0x00000020
#define AVI_ISINTERLEAVED  0x00000100
#define AVI_TRUSTCKTYPE    0x00000800  // Use CKType to find key frames?
#define AVI_WASCAPTUREFILE 0x00010000
#define AVI_COPYRIGHTED    0x00020000
#define AVIF_WASCAPTUREFILE     0x00010000
#define AVI_KEYFRAME       0x10
#define AVI_INDEX_OF_CHUNKS 0x01
#define AVI_INDEX_OF_INDEXES 0x00

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

static void dump_chunk_header(chunk_header_t * chunk)
  {
  bgav_dump_fourcc(chunk->ckID);
  fprintf(stderr, "\nSize %d\n", chunk->ckSize);
  }

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

int read_avih(bgav_input_context_t* input,
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
    fprintf(stderr, "AVIH: Skipping %lld bytes\n",
            ch->ckSize - (input->position - start_pos));
    
    bgav_input_skip(input, PADD(ch->ckSize) - (input->position - start_pos));
    }
  return result;
  }

void dump_avih(avih_t * h)
  {
  fprintf(stderr, "dwMicroSecPerFrame: %d\n",    h->dwMicroSecPerFrame);
  fprintf(stderr, "dwMaxBytesPerSec: %d\n",      h->dwMaxBytesPerSec);
  fprintf(stderr, "dwReserved1: %d\n",           h->dwReserved1);
  fprintf(stderr, "dwFlags: %08x\n",             h->dwFlags);
  fprintf(stderr, "dwTotalFrames: %d\n",         h->dwTotalFrames);
  fprintf(stderr, "dwInitialFrames: %d\n",       h->dwInitialFrames);
  fprintf(stderr, "dwStreams: %d\n",             h->dwStreams);
  fprintf(stderr, "dwSuggestedBufferSize: %d\n", h->dwSuggestedBufferSize);
  fprintf(stderr, "dwWidth: %d\n",               h->dwWidth);
  fprintf(stderr, "dwHeight: %d\n",              h->dwHeight);
  fprintf(stderr, "dwScale: %d\n",               h->dwScale);
  fprintf(stderr, "dwRate: %d\n",                h->dwRate);
  fprintf(stderr, "dwLength: %d\n",              h->dwLength);
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
  fprintf(stderr, "\nReading Index done %d\n", ret->num_entries);
  return 1;
  }



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
    fprintf(stderr, "STRH: Skipping %lld bytes\n",
            ch->ckSize - (input->position - start_pos));
    
    bgav_input_skip(input, PADD(ch->ckSize) - (input->position - start_pos));
    }
  return result;
  }

static void dump_strh(strh_t * ret)
  {
  fprintf(stderr, "strh\nfccType: ");
  bgav_dump_fourcc(ret->fccType);
  fprintf(stderr, "\nfccHandler: ");
  bgav_dump_fourcc(ret->fccHandler);
  fprintf(stderr, "\ndwFlags: %d (%08x)\n",
          ret->dwFlags, ret->dwFlags);
  fprintf(stderr, "dwReserved1: %d (%08x)\n",
          ret->dwReserved1, ret->dwReserved1);
  fprintf(stderr, "dwInitialFrames: %d (%08x)\n",
          ret->dwInitialFrames, ret->dwInitialFrames);
  fprintf(stderr, "dwScale: %d (%08x)\n", ret->dwScale, ret->dwScale);
  fprintf(stderr, "dwRate: %d (%08x)\n", ret->dwRate, ret->dwRate);
  fprintf(stderr, "dwStart: %d (%08x)\n", ret->dwStart, ret->dwStart);
  fprintf(stderr, "dwLength: %d (%08x)\n", ret->dwLength, ret->dwLength);
  fprintf(stderr, "dwSuggestedBufferSize: %d (%08x)\n",
          ret->dwSuggestedBufferSize,
          ret->dwSuggestedBufferSize);
  fprintf(stderr, "dwQuality: %d (%08x)\n", ret->dwQuality, ret->dwQuality);
  fprintf(stderr, "dwSampleSize: %d (%08x)\n", ret->dwSampleSize, ret->dwSampleSize);
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
  } avi_priv;

typedef struct
  {
  strh_t strh;
  uint32_t index_position;
  int vbr;
  int64_t total_bytes;
  int64_t total_blocks;
  } audio_priv;

typedef struct
  {
  strh_t strh;
  uint32_t index_position;
  int64_t total_frames;
  } video_priv;

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
  audio_priv * avi_as;

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
        bgav_WAVEFORMATEX_dump(&wf);
        if(wf.cbSize)
          {
          bg_as->ext_size = wf.cbSize;
          bg_as->ext_data = malloc(wf.cbSize);
          memcpy(bg_as->ext_data, pos, wf.cbSize);
          }
        bg_as->fourcc = BGAV_WAVID_2_FOURCC(wf.wFormatTag);
        if(wf.wBitsPerSample)
          bg_as->data.audio.bits_per_sample = wf.wBitsPerSample;
        else if(strh->dwSampleSize)
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
        fprintf(stderr, "Found data %d bytes\n", ch->ckSize);
        
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_JUNK:
        //        fprintf(stderr, "Found junk %d bytes\n", ch->ckSize);
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_LIST:
        keep_going = 0;
        break;
      default:
        fprintf(stderr, "Unknown subchunk ");
        bgav_dump_fourcc(ch->ckID);
        fprintf(stderr, "\n");
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
  audio_priv * avi_vs;
  
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
        bgav_BITMAPINFOHEADER_dump(&bh);

        if(ch->ckSize > 40)
          {
          fprintf(stderr, "Adding extradata %d bytes\n",
                  ch->ckSize - 40);
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
        fprintf(stderr, "Found data %d bytes\n", ch->ckSize);
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_JUNK:
        //        fprintf(stderr, "Found junk %d bytes\n", ch->ckSize);
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      case ID_LIST:
        keep_going = 0;
        break;
      default:
        fprintf(stderr, "Unknown subchunk ");
        bgav_dump_fourcc(ch->ckID);
        fprintf(stderr, "\n");
        bgav_input_skip(ctx->input, PADD(ch->ckSize));
        break;
      }
    }

  /* Get frame rate */

  if(strh->dwScale && strh->dwRate)
    {
    bg_vs->data.video.format.framerate_num = strh->dwRate;
    bg_vs->data.video.format.framerate_den = strh->dwScale;
    }
  else if(avih->dwMicroSecPerFrame)
    {
    bg_vs->data.video.format.framerate_num = 1000000;
    bg_vs->data.video.format.framerate_den = avih->dwMicroSecPerFrame;
    }
  else
    {
    bg_vs->data.video.format.framerate_num = 25;
    bg_vs->data.video.format.framerate_den = 1;
    }
  bg_vs->stream_id = (ctx->tt->current_track->num_audio_streams + ctx->tt->current_track->num_video_streams) - 1;
  return 1;
  }

/* Get a stream ID (internally used) from the stream ID in the chunk header */

static int get_stream_id(uint32_t fourcc)
  {
  int ret;
  ret = 10 * (((fourcc & 0xff000000) >> 24) - '0');
  ret += ((fourcc & 0x00ff0000) >> 16) - '0';

  return ret;  
  }


/*
 *  Calculate the timestamp field of each chunk in the index
 */

void idx1_calculate_timestamps(bgav_demuxer_context_t * ctx)
  {
  uint32_t i, j;
  int stream_id;
  audio_priv * avi_as;
  video_priv * avi_vs;
  bgav_stream_t * stream;
  avi_priv * avi;
  
  avi = (avi_priv*)(ctx->priv);
  if(avi->has_idx1_timestamps)
    return;

  fprintf(stderr, "Getting index timestamps\n");
  
  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    avi_as = (audio_priv*)(ctx->tt->current_track->audio_streams[i].priv);
    avi_as->total_bytes = 0;
    avi_as->total_blocks = 0;
    }

  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    avi_vs = (video_priv*)(ctx->tt->current_track->video_streams[i].priv);
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
      avi_as = (audio_priv*)(stream->priv);
      
      /* Now, we must distinguish between VBR and CBR audio */

      if ((avi_as->strh.dwSampleSize == 0) && (avi_as->strh.dwScale > 1))
        {
        /* variable bitrate */
        avi->idx1.entries[i].time =
          (GAVL_TIME_SCALE * (gavl_time_t)avi_as->total_blocks *
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
            ((gavl_time_t)avi_as->total_bytes * (gavl_time_t)avi_as->strh.dwScale * GAVL_TIME_SCALE) /
            (stream->data.audio.block_align * avi_as->strh.dwRate);
          }
        else
          {
          avi->idx1.entries[i].time =
            (GAVL_TIME_SCALE * (gavl_time_t)avi_as->total_bytes * (gavl_time_t)avi_as->strh.dwScale) /
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
      avi_vs = (video_priv*)(stream->priv);
      avi->idx1.entries[i].time = gavl_frames_to_time(stream->data.video.format.framerate_num,
                                                      stream->data.video.format.framerate_den,
                                                      avi_vs->total_frames);
      avi_vs->total_frames++;
      }
    }
  avi->has_idx1_timestamps = 1;
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

  ctx->duration = p->avih.dwMicroSecPerFrame * p->avih.dwTotalFrames;
    
  /* Streams */

  read_chunk_header(ctx->input, &ch);
  
  for(i = 0; i < (p->avih).dwStreams; i++)
    {
    if(!bgav_input_read_fourcc(ctx->input, &fourcc) ||
       (fourcc != ID_STRL))
      goto fail;
    
    if(!read_chunk_header(ctx->input, &ch) ||
       (ch.ckID != ID_STRH))
      goto fail;
    
    read_strh(ctx->input, &strh, &ch);
    //    dump_strh(&strh);

    if(strh.fccType == ID_AUDS)
      init_audio_stream(ctx, &strh, &ch);
    else if(strh.fccType == ID_VIDS)
      init_video_stream(ctx, &strh, &ch);
    }

  while(1)
    {
    bgav_input_read_fourcc(ctx->input, &fourcc);
    if(fourcc != ID_MOVI)
      {
      fprintf(stderr, "Skipping chunk\n");
      bgav_dump_fourcc(fourcc);
      fprintf(stderr, "\n");
      bgav_input_skip(ctx->input, ch.ckSize-4);
      if(!read_chunk_header(ctx->input, &ch))
        goto fail;
      }
    else
      break;
    }

  //  dump_chunk_header(&ch);
  p->movi_start = ctx->input->position;
  p->movi_size =  ch.ckSize - 4;
  fprintf(stderr, "Reached MOVI atom %08llx\n", p->movi_start + p->movi_size);

  if((p->avih.dwFlags & AVI_HASINDEX) && (ctx->input->input->seek_byte))
    {
    /* The index must come after the MOVI chunk */
    bgav_input_seek(ctx->input, p->movi_start + p->movi_size, SEEK_SET);
    if(read_idx1(ctx->input, &(p->idx1)))
      {
      p->has_idx1 = 1;
      ctx->can_seek = 1;
      idx1_calculate_timestamps(ctx);
      //      dump_idx1(&(p->idx1));
      }
    bgav_input_seek(ctx->input, p->movi_start, SEEK_SET);
    }
  
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
  priv = (avi_priv*)(ctx->priv);

  if(priv->has_idx1)
    free_idx1(&(priv->idx1));
  
  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    if(ctx->tt->current_track->audio_streams[i].ext_data)
      free(ctx->tt->current_track->audio_streams[i].ext_data);
    free(ctx->tt->current_track->audio_streams[i].priv);
    }

  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    if(ctx->tt->current_track->video_streams[i].data.video.palette_size)
      free(ctx->tt->current_track->video_streams[i].data.video.palette);
    if(ctx->tt->current_track->video_streams[i].ext_data)
      free(ctx->tt->current_track->video_streams[i].ext_data);
    free(ctx->tt->current_track->video_streams[i].priv);
    }
  
  free(priv);
  }


static int next_packet_avi(bgav_demuxer_context_t * ctx)
  {
  chunk_header_t ch;
  bgav_packet_t * p;
  bgav_stream_t * s;
  uint32_t fourcc;
  int stream_id;
  avi_priv * priv;
  priv = (avi_priv*)(ctx->priv);
      
  if(ctx->input->position + 8 >= priv->movi_start + priv->movi_size)
    return 0;
  
  if(!read_chunk_header(ctx->input, &ch))
    return 0;
  /*
    fprintf(stderr, "Position: %lld, index position: %d\n",
    ctx->input->position - priv->movi_start,
    priv->idx1.entries[priv->index_position].dwChunkOffset+4);
    fprintf(stderr, "Size: %d, index size: %d\n",
    ch.ckSize,
    priv->idx1.entries[priv->index_position].dwChunkLength);
  */
  //  dump_chunk_header(&ch);
  if(ch.ckID == BGAV_MK_FOURCC('L','I','S','T'))
    {
    //    fprintf(stderr, "Got list chunk\n");
    bgav_input_read_fourcc(ctx->input, &fourcc);
    //    bgav_dump_fourcc(fourcc);
    //    fprintf(stderr, "\n");
    if(!read_chunk_header(ctx->input, &ch))
      return 0;
    }

  stream_id = get_stream_id(ch.ckID);
  s = bgav_track_find_stream(ctx->tt->current_track, stream_id);
  /* Skip ignored streams */

  if(!s)
    {
    fprintf(stderr, "No such stream %d\n", stream_id);
    dump_chunk_header(&ch);
    /* For some really bad streams, this can help */
    if(ch.ckSize < 0)
      {
      if(priv->has_idx1)
        ch.ckSize = priv->idx1.entries[priv->index_position].dwChunkLength;
      else
        {
        fprintf(stderr, "Broken file\n");
        return 0;
        }
      }
    bgav_input_skip(ctx->input, PADD(ch.ckSize));
    priv->index_position++;
    return 1;
    }
  else if(priv->do_seek)
    {
    if(((s->type == BGAV_STREAM_AUDIO) &&
        (((audio_priv*)(s->priv))->index_position > priv->index_position)) ||
       ((s->type == BGAV_STREAM_VIDEO) &&
        (((video_priv*)(s->priv))->index_position > priv->index_position)))
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

  if(priv->has_idx1)
    {
    p->timestamp = priv->idx1.entries[priv->index_position].time;
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
  audio_priv * avi_as;
  video_priv * avi_vs;
  avi_priv * avi = (avi_priv*)(ctx->priv);
  bgav_stream_t * stream;
  int stream_id;
  int keep_going;
  
  for(i = 0; i < ctx->tt->current_track->num_audio_streams; i++)
    {
    avi_as = (audio_priv*)(ctx->tt->current_track->audio_streams[i].priv);
    avi_as->index_position = -1;
    }
    
  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    avi_vs = (video_priv*)(ctx->tt->current_track->video_streams[i].priv);
    avi_vs->index_position = -1;
    }

  /* Set the index_position members of the streams */
    
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
        avi_as = (audio_priv*)(stream->priv);

        if(avi_as->index_position == -1)
          {
          if(avi->idx1.entries[i].time > time)
            {
            keep_going = 1;
            break;
            }
          
          /* Set timestamp and stuff */
          
          if(avi->idx1.entries[i].dwFlags & AVI_KEYFRAME)
            {
            avi_as->index_position = i;
            stream->time = avi->idx1.entries[i].time;
            }
          else
            keep_going = 1;
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
          avi_vs = (video_priv*)(stream->priv);

          if(avi_vs->index_position == -1)
            {
            if(avi->idx1.entries[i].time > time)
              {
              keep_going = 1;
              break;
              }
            if(avi->idx1.entries[i].dwFlags & AVI_KEYFRAME)
              {
              avi_vs->index_position = i;
              stream->time = avi->idx1.entries[i].time;
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
      avi_as = (audio_priv*)(ctx->tt->current_track->audio_streams[j].priv);
      if(avi_as->index_position == -1)
        keep_going = 1;
      }

    for(j = 0; j < ctx->tt->current_track->num_video_streams; j++)
      {
      avi_vs = (video_priv*)(ctx->tt->current_track->video_streams[j].priv);
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
    avi_as = (audio_priv*)(ctx->tt->current_track->audio_streams[i].priv);
    if(avi_as->index_position < chunk_min)
      chunk_min = avi_as->index_position;
    if(avi_as->index_position > chunk_max)
      chunk_max = avi_as->index_position;
    //    fprintf(stderr, "AS: %d\n", avi_as->index_position);
    }

  for(i = 0; i < ctx->tt->current_track->num_video_streams; i++)
    {
    avi_vs = (video_priv*)(ctx->tt->current_track->video_streams[i].priv);
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

  fprintf(stderr, "%d %d, %d\n", avi->index_position, chunk_min, chunk_max);
  
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
