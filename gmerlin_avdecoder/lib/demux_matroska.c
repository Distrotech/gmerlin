/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
#include <matroska.h>
#include <nanosoft.h>

#define LOG_DOMAIN "demux_matroska"

typedef struct
  {
  bgav_mkv_ebml_header_t ebml_header;
  bgav_mkv_meta_seek_info_t meta_seek_info;
  
  bgav_mkv_segment_info_t segment_info;
  bgav_mkv_cues_t cues;
  int have_cues;
  
  bgav_mkv_track_t * tracks;
  int num_tracks;

  int64_t segment_start;

  bgav_mkv_cluster_t cluster;

  int64_t pts_offset;

  bgav_mkv_block_group_t bg;
  
  uint64_t * lace_sizes;
  int lace_sizes_alloc;
  
  } mkv_t;
 
static int probe_matroska(bgav_input_context_t * input)
  {
  bgav_mkv_ebml_header_t h;
  bgav_input_context_t * input_mem;
  int ret = 0;
  
  /* We want a complete EBML header in the first 64 bits
   * with DocType either "matroska" or "webm"
   */
  uint8_t header[64];
  
  if(bgav_input_get_data(input, header, 64) < 64)
    return 0;

  if((header[0] != 0x1a) ||
     (header[1] != 0x45) ||
     (header[2] != 0xdf) ||
     (header[3] != 0xa3)) // No EBML signature
    return 0; 

  input_mem = bgav_input_open_memory(header, 64, input->opt);

  if(!bgav_mkv_ebml_header_read(input_mem, &h))
    return 0;

  if(!h.DocType)
    return 0;

  if(!strcmp(h.DocType, "matroska") ||
     !strcmp(h.DocType, "webm"))
    {
    ret = 1;
    }
  
  bgav_mkv_ebml_header_free(&h);
  bgav_input_close(input_mem);
  return ret;
  }

#define CODEC_FLAG_INCOMPLETE (1<<0)

typedef struct
  {
  const char * id;
  uint32_t fourcc;
  void (*init_func)(bgav_stream_t * s);
  int flags;
  int frame_granularity;
  } codec_info_t;

static void append_ogg_extradata(bgav_stream_t * s,
                                 uint8_t * data, int len)
  {
  uint8_t * ptr;
  s->ext_data = realloc(s->ext_data, s->ext_size + len + 4);
  ptr = s->ext_data + s->ext_size;
  BGAV_32BE_2_PTR(len, ptr); ptr+=4;
  memcpy(ptr, data, len);
  s->ext_size += len + 4;
  }

static void setup_ogg_extradata(bgav_stream_t * s)
  {
  uint8_t * ptr;
  int len1;
  int len2;
  int len3;
  
  bgav_mkv_track_t * p = s->priv;
  
  ptr = p->CodecPrivate;
  if(*ptr != 0x02)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Ogg extradata must start with 0x02n");
    return;
    }
  ptr++;

  /* 1st packet */
  len1 = 0;
  while(*ptr == 255)
    {
    len1 += 255;
    ptr++;
    }
  len1 += *ptr;
  ptr++;

  /* 2nd packet */
  len2 = 0;
  while(*ptr == 255)
    {
    len2 += 255;
    ptr++;
    }
  len2 += *ptr;
  ptr++;

  /* 3rd packet */
  len3 = p->CodecPrivateLen - (ptr - p->CodecPrivate) - len1 - len2;

  /* Append header packets */
  append_ogg_extradata(s, ptr, len1);
  ptr += len1;
  
  append_ogg_extradata(s, ptr, len2);
  ptr += len2;

  append_ogg_extradata(s, ptr, len3);
  
  }


static void init_vfw(bgav_stream_t * s)
  {
  uint8_t * data;
  uint8_t * end;
  
  bgav_BITMAPINFOHEADER_t bh;
  bgav_mkv_track_t * p = s->priv;
  
  data = p->CodecPrivate;
  end = data + p->CodecPrivateLen;
  bgav_BITMAPINFOHEADER_read(&bh, &data);
  s->fourcc = bgav_BITMAPINFOHEADER_get_fourcc(&bh);

  if(data < end)
    {
    s->ext_size = end - data;
    s->ext_data = malloc(s->ext_size);
    memcpy(s->ext_data, data, s->ext_size);
    }
  }

static void init_theora(bgav_stream_t * s)
  {
  setup_ogg_extradata(s);
  s->fourcc = BGAV_MK_FOURCC('T', 'H', 'R', 'A');
  }

static const codec_info_t video_codecs[] =
  {
    { "V_MS/VFW/FOURCC", 0x00,                            init_vfw, 0 },
    { "V_MPEG4/ISO/SP",  BGAV_MK_FOURCC('m','p','4','v'), NULL,     0 },
    { "V_MPEG4/ISO/ASP", BGAV_MK_FOURCC('m','p','4','v'), NULL,     0 },
    { "V_MPEG4/ISO/AP",  BGAV_MK_FOURCC('m','p','4','v'), NULL,     0 },
    { "V_MPEG4/MS/V1",   BGAV_MK_FOURCC('M','P','G','4'), NULL,     0 },
    { "V_MPEG4/MS/V2",   BGAV_MK_FOURCC('M','P','4','2'), NULL,     0 },
    { "V_MPEG4/MS/V3",   BGAV_MK_FOURCC('M','P','4','3'), NULL,     0 },
    { "V_REAL/RV10",     BGAV_MK_FOURCC('R','V','1','0'), NULL,     0 },
    { "V_REAL/RV20",     BGAV_MK_FOURCC('R','V','2','0'), NULL,     0 },
    { "V_REAL/RV30",     BGAV_MK_FOURCC('R','V','3','0'), NULL,     0 },
    { "V_REAL/RV40",     BGAV_MK_FOURCC('R','V','4','0'), NULL,     0 },
    { "V_VP8",           BGAV_MK_FOURCC('V','P','8','0'), NULL,     0 },
    { "V_THEORA",        0x00,                            init_theora, 0 },
    { "V_MPEG4/ISO/AVC", BGAV_MK_FOURCC('a','v','c','1'), NULL,     0 },
    { /* End */ }
  };

static void init_acm(bgav_stream_t * s)
  {
  bgav_WAVEFORMAT_t wf;
  bgav_mkv_track_t * p = s->priv;
  
  bgav_WAVEFORMAT_read(&wf, p->CodecPrivate, p->CodecPrivateLen);
  bgav_WAVEFORMAT_get_format(&wf, s);
  bgav_WAVEFORMAT_free(&wf);
  }

static void init_vorbis(bgav_stream_t * s)
  {
  setup_ogg_extradata(s);
  s->fourcc = BGAV_MK_FOURCC('V','B','I','S');
  s->flags |= STREAM_LACING;
  }

/* AAC Initialization inspired by ffmpeg (matroskadec.c) */
static int aac_profile(const char *codec_id)

  {
  static const char * const aac_profiles[] = { "MAIN", "LC", "SSR" };
  int profile;
  for (profile=0; profile<sizeof(aac_profiles)/sizeof(aac_profiles[0]); profile++)
    {
    if(strstr(codec_id, aac_profiles[profile]))
      break;
    }
  return profile + 1;
  }

static int aac_sri(double r)
  {
  int sri;
  int samplerate= (int)(r+0.5);
  const int sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
  };
  
  for (sri=0; sri < sizeof(sample_rates)/sizeof(sample_rates[0]); sri++)
    {
    if(sample_rates[sri] == samplerate)
      break;
    }
  return sri;
  }

static void init_aac(bgav_stream_t * s)
  {
  bgav_mkv_track_t * p = s->priv;
  if(p->CodecPrivate)
    {
    s->ext_data = malloc(p->CodecPrivateLen);
    memcpy(s->ext_data, p->CodecPrivate, p->CodecPrivateLen);
    s->ext_size = p->CodecPrivateLen;
    }
  else
    {
    int profile, sri;
    s->ext_data = malloc(5);
    profile = aac_profile(p->CodecID);
    sri = aac_sri(p->audio.SamplingFrequency);
    s->ext_data[0] = (profile << 3) | ((sri&0x0E) >> 1);
    s->ext_data[1] = ((sri&0x01) << 7) | (p->audio.Channels<<3);
    if(strstr(p->CodecID, "SBR"))
      {
      sri = aac_sri(p->audio.OutputSamplingFrequency);
      s->ext_data[2] = 0x56;
      s->ext_data[3] = 0xE5;
      s->ext_data[4] = 0x80 | (sri<<3);
      s->ext_size = 5;
      }
    else
      s->ext_size = 2;
    }
  s->fourcc = BGAV_MK_FOURCC('m','p','4','a');
  }

static const codec_info_t audio_codecs[] =
  {
    { "A_MS/ACM",        0x00,                            init_acm,    0       },
    { "A_VORBIS",        0x00,                            init_vorbis, 0, 128  },
    { "A_MPEG/L3",       BGAV_MK_FOURCC('.','m','p','3'), NULL,        0, 576  },
    { "A_MPEG/L2",       BGAV_MK_FOURCC('.','m','p','2'), NULL,        0, 1152 },
    { "A_MPEG/L1",       BGAV_MK_FOURCC('.','m','p','1'), NULL,        0, 576  },
    { "A_AAC/",          BGAV_MK_FOURCC('m','p','4','a'), init_aac,    CODEC_FLAG_INCOMPLETE, 1024 },
    { "A_AC3",           BGAV_MK_FOURCC('.','a','c','3'), NULL,        0, 1536 },
    { /* End */ }
  };

static void init_stream_common(mkv_t * m,
                               bgav_stream_t * s,
                               bgav_mkv_track_t * track,
                               const codec_info_t * codecs)
  {
  int i = 0;
  const codec_info_t * info = NULL;
  
  s->priv = track;

  while(codecs[i].id)
    {
    if(((codecs[i].flags & CODEC_FLAG_INCOMPLETE) &&
        !strncmp(codecs[i].id, track->CodecID, strlen(codecs[i].id))) ||
       !strcmp(codecs[i].id, track->CodecID))
      {
      info = &codecs[i];
      break;
      }
    i++;
    }

  if(info)
    {
    s->fourcc = info->fourcc;
    if(info->init_func)
      info->init_func(s);
    else if(track->CodecPrivateLen)
      {
      s->ext_data = malloc(track->CodecPrivateLen);
      memcpy(s->ext_data, track->CodecPrivate, track->CodecPrivateLen);
      s->ext_size = track->CodecPrivateLen;
      }
    }
  s->stream_id = track->TrackNumber;
  s->timescale = 1000000000 / m->segment_info.TimecodeScale;
  }

static int init_audio(bgav_demuxer_context_t * ctx,
                      bgav_mkv_track_t * track)
  {
  bgav_stream_t * s;
  bgav_mkv_track_audio_t * a;
  gavl_audio_format_t * fmt;
  mkv_t * m = ctx->priv;
  
  s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  init_stream_common(m, s, track, audio_codecs);

  fmt = &s->data.audio.format;
  a = &track->audio;

  if(a->SamplingFrequency > 0.0)
    {
    fmt->samplerate = (int)a->SamplingFrequency;

    if(a->OutputSamplingFrequency > a->SamplingFrequency)
      {
      fmt->samplerate = (int)a->OutputSamplingFrequency;
      s->flags |= STREAM_SBR;
      }
    }
  if(a->Channels > 0)
    fmt->num_channels = a->Channels;
  if(a->BitDepth > 0)
    s->data.audio.bits_per_sample = a->BitDepth;
  return 1;
  }

static int init_video(bgav_demuxer_context_t * ctx,
                      bgav_mkv_track_t * track)
  {
  bgav_stream_t * s;
  bgav_mkv_track_video_t * v;
  gavl_video_format_t * fmt;
  mkv_t * m = ctx->priv;
  
  s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
  init_stream_common(m, s, track, video_codecs);

  fmt = &s->data.video.format;
  v = &track->video;
  
  if(v->PixelWidth)
    fmt->image_width = v->PixelWidth;
  if(v->PixelHeight)
    fmt->image_height = v->PixelHeight;

  fmt->pixel_width = 1;
  fmt->pixel_height = 1;
  
  if(v->DisplayWidth && v->DisplayHeight &&
     ((v->DisplayWidth != v->PixelWidth) ||
      (v->DisplayHeight != v->PixelHeight)))
    {
    if(v->PixelHeight == v->DisplayHeight)
      {
      fmt->pixel_width  = v->DisplayWidth;
      fmt->pixel_height = v->PixelWidth;
      }

    }
  
  fmt->frame_width = fmt->image_width;
  fmt->frame_height = fmt->image_height;
  fmt->timescale = s->timescale;
  fmt->framerate_mode = GAVL_FRAMERATE_VARIABLE;
  
  return 1;
  }

#define MAX_HEADER_LEN 16

static int open_matroska(bgav_demuxer_context_t * ctx)
  {
  bgav_mkv_element_t e;
  int done;
  int64_t pos;
  mkv_t * p;
  int i;
  bgav_input_context_t * input_mem;

  uint8_t buf[MAX_HEADER_LEN];
  int buf_len;
  int head_len;
  
  input_mem = bgav_input_open_memory(NULL, 0, ctx->opt);
    
  p = calloc(1, sizeof(*p));
  ctx->priv = p;
  p->pts_offset = BGAV_TIMESTAMP_UNDEFINED;
  
  if(!bgav_mkv_ebml_header_read(ctx->input, &p->ebml_header))
    return 0;

  //  bgav_mkv_ebml_header_dump(&p->ebml_header);
  
  /* Get the first segment */
  
  while(1)
    {
    if(!bgav_mkv_element_read(ctx->input, &e))
      return 0;

    if(e.id == MKV_ID_Segment)
      {
      p->segment_start = ctx->input->position;
      break;
      }
    else
      bgav_input_skip(ctx->input, e.size);
    }
  done = 0;
  while(!done)
    {
    pos = ctx->input->position;
    
    buf_len =
      bgav_input_get_data(ctx->input, buf, MAX_HEADER_LEN);

    if(buf_len <= 0)
      return 0;
    
    bgav_input_reopen_memory(input_mem, buf, buf_len);
    
    if(!bgav_mkv_element_read(input_mem, &e))
      return 0;
    head_len = input_mem->position;

    //    fprintf(stderr, "Got element (%d header bytes)\n",
    //            head_len);
    //    bgav_mkv_element_dump(&e);
    
    switch(e.id)
      {
      case MKV_ID_SeekHead:
        bgav_input_skip(ctx->input, head_len);
        e.end += pos;
        if(!bgav_mkv_meta_seek_info_read(ctx->input,
                                         &p->meta_seek_info,
                                         &e))
          return 0;
        // bgav_mkv_meta_seek_info_dump(&p->meta_seek_info);
        break;
      case MKV_ID_Info:
        bgav_input_skip(ctx->input, head_len);
        e.end += pos;
        if(!bgav_mkv_segment_info_read(ctx->input, &p->segment_info, &e))
          return 0;
        bgav_mkv_segment_info_dump(&p->segment_info);
        break;
      case MKV_ID_Tracks:
        bgav_input_skip(ctx->input, head_len);
        e.end += pos;
        if(!bgav_mkv_tracks_read(ctx->input, &p->tracks, &p->num_tracks, &e))
          return 0;
        break;
      case MKV_ID_Cues:
        if(!bgav_mkv_cues_read(ctx->input, &p->cues, p->num_tracks))
          return 0;
        fprintf(stderr, "Got cues before clusters\n");
        p->have_cues = 1;
        break;
      case MKV_ID_Cluster:
        done = 1;
        ctx->data_start = pos;
        ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
        break;

        
      default:
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Skipping %"PRId64" bytes of element %x in segment\n",
                 e.size, e.id);
        bgav_input_skip(ctx->input, head_len + e.size);
      }
    }

  /* Create track table */
  ctx->tt = bgav_track_table_create(1);
  
  for(i = 0; i < p->num_tracks; i++)
    {
    switch(p->tracks[i].TrackType)
      {
      case MKV_TRACK_VIDEO:
        if(!init_video(ctx, &p->tracks[i]))
          return 0;
        break;
      case MKV_TRACK_AUDIO:
        if(!init_audio(ctx, &p->tracks[i]))
          return 0;
        break;
      case MKV_TRACK_COMPLEX:
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Complex tracks not supported yet\n");
        break;
      case MKV_TRACK_LOGO:
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Logo tracks not supported yet\n");
        break;
      case MKV_TRACK_SUBTITLE:
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Subtitle tracks not supported yet\n");
        break;
      case MKV_TRACK_BUTTONS:
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Button tracks not supported yet\n");
        break;
      case MKV_TRACK_CONTROL:
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Control tracks not supported yet\n");
        break;
      }
    }

  /* Look for file index (cues) */
  if(p->meta_seek_info.num_entries && ctx->input->input->seek_byte &&
     !p->have_cues)
    {
    for(i = 0; i < p->meta_seek_info.num_entries; i++)
      {
      if(p->meta_seek_info.entries[i].SeekID == MKV_ID_Cues)
        {
        fprintf(stderr, "Found index at %"PRId64"\n",
                p->meta_seek_info.entries[i].SeekPosition);

        pos = ctx->input->position;

        bgav_input_seek(ctx->input,
                        p->segment_start +
                        p->meta_seek_info.entries[i].SeekPosition, SEEK_SET);
        
        if(bgav_mkv_cues_read(ctx->input, &p->cues, p->num_tracks))
          p->have_cues = 1;
        bgav_input_seek(ctx->input, pos, SEEK_SET);
        }
      }
    }

  /* get duration */
  if(p->segment_info.Duration > 0.0)
    ctx->tt->cur->duration =
      gavl_seconds_to_time(p->segment_info.Duration * 
                           p->segment_info.TimecodeScale * 1.0e-9);
  return 1;
  }
  
static int get_ebml_frame_size_uint(uint8_t * ptr, int size, int64_t * ret)
  {
  uint8_t mask = 0x80;
  int len = 1;
  int i;
  while(!(mask & (*ptr)) && mask)
    {
    mask >>= 1;
    len++;
    }

  if(!mask)
    return 0;
  
  *ret = *ptr & (0xff >> len);
  
  ptr++;

  for(i = 1; i < len; i++)
    {
    *ret <<= 8;
    *ret |= *ptr;
    ptr++;
    }
  return len;
  }

static int get_ebml_frame_size_int(uint8_t * ptr1, int size, int64_t * ret)
  {
  int64_t offset;
  int bytes = get_ebml_frame_size_uint(ptr1, size, ret);

  offset = (1 << (bytes * 7 - 1)) - 1;
  
  *ret -= offset;
  return bytes;
  }

static void append_packet_data(bgav_stream_t * s,
                               bgav_packet_t * p,
                               const uint8_t * data,
                               int len)
  {
  if(!(s->flags & STREAM_LACING))
    {
    bgav_packet_alloc(p, len);
    memcpy(p->data, data, len);
    p->data_size = len;
    }
  else
    {
    bgav_packet_append_segment(p, data, len);
    }
  }

static int process_block(bgav_demuxer_context_t * ctx,
                         bgav_mkv_block_t * b,
                         bgav_mkv_block_group_t * bg)
  {
  bgav_stream_t * s;
  bgav_packet_t * p;
  mkv_t * m = ctx->priv;
  
  s = bgav_track_find_stream(ctx, b->track);

  if(!s)
    return 1;

  if(s->type == BGAV_STREAM_AUDIO)
    fprintf(stderr, "Audio stream\n");
  
  switch(b->flags & MKV_LACING_MASK)
    {
    case MKV_LACING_NONE:
      p = bgav_stream_get_packet_write(s);
      p->data_size = 0;
      append_packet_data(s, p, b->data, b->data_size);
      
      p->pts = b->timecode + m->cluster.Timecode - m->pts_offset;
      
      bgav_packet_done_write(p);
      break;
    case MKV_LACING_EBML:
      {
      int i;
      uint8_t * ptr;
      int bytes;
      int64_t frame_size;
      int64_t frame_size_diff;
      
      //      fprintf(stderr, "MKV_LACING_EBML\n");
      //      bgav_hexdump(ptr, 32, 16);

      if(m->lace_sizes_alloc < b->num_laces)
        {
        m->lace_sizes_alloc = b->num_laces + 16;
        m->lace_sizes = realloc(m->lace_sizes,
                                m->lace_sizes_alloc *
                                sizeof(m->lace_sizes));
        }
      /* First lace */
      ptr = b->data;
      
      bytes = get_ebml_frame_size_uint(ptr,
                                       b->data_size - (ptr - b->data),
                                       &frame_size);
      if(!bytes)
        return 0;
      
      m->lace_sizes[0] = frame_size;
      ptr += bytes;
      
      /* Intermediate laces */
      for(i = 1; i < b->num_laces-1; i++)
        {
        bytes = get_ebml_frame_size_int(ptr,
                                        b->data_size - (ptr - b->data),
                                        &frame_size_diff);
        ptr += bytes;
        frame_size += frame_size_diff;
        m->lace_sizes[i] = frame_size;
        }
      /* Last lace */
      m->lace_sizes[b->num_laces-1] =
        b->data_size - (ptr - b->data);
      for(i = 0; i < b->num_laces-1; i++)
        m->lace_sizes[b->num_laces-1] -= m->lace_sizes[i];

      /* Send all laces as different packets */
      for(i = 0; i < b->num_laces; i++)
        {
        p = bgav_stream_get_packet_write(s);
        p->data_size = 0;
        append_packet_data(s, p, ptr, m->lace_sizes[i]);
        ptr += m->lace_sizes[i];
        bgav_packet_done_write(p);
        }
      
      }
      break;
    case MKV_LACING_XIPH:
      fprintf(stderr, "Xiph lacing not supported yet\n");
      bgav_mkv_block_dump(0, b);
      bgav_input_skip(ctx->input, b->data_size);
      return 0;
    case MKV_LACING_FIXED:
      {
      int i;
      uint8_t * ptr;
      int frame_size = b->data_size / b->num_laces;

      ptr = b->data;
      
      for(i = 0; i < b->num_laces; i++)
        {
        p = bgav_stream_get_packet_write(s);
        p->data_size = 0;
        append_packet_data(s, p, ptr, frame_size);
        ptr += frame_size;
        bgav_packet_done_write(p);
        }
      }
      break;
    default:
      fprintf(stderr, "Unknown lacing type\n");
      bgav_mkv_block_dump(0, b);
      bgav_input_skip(ctx->input, b->data_size);
      return 0;
      break;
    }
  return 1;
  }

#if 0
static int process_block_group(bgav_demuxer_context_t * ctx,
                               bgav_mkv_element_t * parent)
  {
  
  }
#endif

/* next packet: Processes a whole cluster at once */

static int next_packet_matroska(bgav_demuxer_context_t * ctx)
  {
  int num_blocks = 0;
  bgav_mkv_element_t e;
  mkv_t * priv = ctx->priv;
  //  fprintf(stderr, "next_packet_matroska\n");
  
  while(!num_blocks)
    {
    if(!bgav_mkv_element_read(ctx->input, &e))
      return 0;
    //  bgav_mkv_element_dump(&e);
    
    switch(e.id)
      {
      case MKV_ID_Cluster:
        //        fprintf(stderr, "Got Cluster\n");
        if(!bgav_mkv_cluster_read(ctx->input, &priv->cluster, &e))
          return 0;
        //        bgav_mkv_cluster_dump(&priv->cluster);

        if(priv->pts_offset == BGAV_TIMESTAMP_UNDEFINED)
          priv->pts_offset = priv->cluster.Timecode;
        break;
      case MKV_ID_BlockGroup:
        //        fprintf(stderr, "Got Block group\n");
        //        bgav_mkv_block_group_dump(&priv->bg);
        if(!bgav_mkv_block_group_read(ctx->input, &priv->bg, &e))
          return 0;
        if(!process_block(ctx, &priv->bg.block, &priv->bg))
          return 0;
        num_blocks++;
        break;
      case MKV_ID_Block:
      case MKV_ID_SimpleBlock:
        //        fprintf(stderr, "Got Block\n");
        //        bgav_mkv_block_dump(0, &priv->bg.block);

        if(!bgav_mkv_block_read(ctx->input, &priv->bg.block, &e))
          return 0;
        
        if(!process_block(ctx, &priv->bg.block, NULL))
          return 0;
        num_blocks++;
        break;
      default:
        /* Probably reached end of file */
        return 0;
      }
    }
  return 1;
  }


static void close_matroska(bgav_demuxer_context_t * ctx)
  {
  int i;
  mkv_t * priv = ctx->priv;

  bgav_mkv_ebml_header_free(&priv->ebml_header);  
  bgav_mkv_segment_info_free(&priv->segment_info);

  for(i = 0; i < priv->num_tracks; i++)
    bgav_mkv_track_free(&priv->tracks[i]);
  if(priv->tracks)
    free(priv->tracks);
  bgav_mkv_meta_seek_info_free(&priv->meta_seek_info);
  
  bgav_mkv_cues_free(&priv->cues);

  bgav_mkv_cluster_free(&priv->cluster);

  if(priv->lace_sizes)
    free(priv->lace_sizes);
  
  free(priv);
  }

const const bgav_demuxer_t bgav_demuxer_matroska =
  {
    .probe =       probe_matroska,
    .open =        open_matroska,
    // .select_track = select_track_matroska,
    .next_packet = next_packet_matroska,
    // .seek =        seek_matroska,
    // .resync  =     resync_matroska,
    .close =       close_matroska
  };
