/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <string.h>
#include <stdlib.h>

#include <avdec_private.h>
#include <gavl/gavf.h>
#include <gavl/compression.h>

typedef struct
  {
  
  gavf_io_t * io;
  gavf_t * dec;
  } gavf_demuxer_t;

static int probe_gavf(bgav_input_context_t * input)
  {
  uint8_t probe_data[8];

  if(bgav_input_get_data(input, probe_data, 8) < 8)
    return 0;

  if(!strncmp((char*)probe_data, "GAVFFIDX", 8) ||
     !strncmp((char*)probe_data, "GAVFPHDR", 8))
    return 1;
  return 0;
  }

static int read_func(void * priv, uint8_t * data, int len)
  {
  return bgav_input_read_data(priv, data, len);
  }

static int64_t seek_func(void * priv, int64_t pos, int whence)
  {
  bgav_input_context_t * ctx = priv;
  bgav_input_seek(ctx, pos, whence);
  return ctx->position;
  }

static const struct
  {
  gavl_codec_id_t id;
  uint32_t fourcc;
  }
fourccs[] =
  {
    { GAVL_CODEC_ID_NONE,      BGAV_MK_FOURCC('g','a','v','f') },
    /* Audio */
    { GAVL_CODEC_ID_ALAW,      BGAV_MK_FOURCC('a','l','a','w') }, 
    { GAVL_CODEC_ID_ULAW,      BGAV_MK_FOURCC('u','l','a','w') }, 
    { GAVL_CODEC_ID_MP2,       BGAV_MK_FOURCC('.','m','p','2') }, 
    { GAVL_CODEC_ID_MP3,       BGAV_MK_FOURCC('.','m','p','3') }, 
    { GAVL_CODEC_ID_AC3,       BGAV_MK_FOURCC('.','a','c','3') }, 
    { GAVL_CODEC_ID_AAC,       BGAV_MK_FOURCC('m','p','4','a') }, 
    { GAVL_CODEC_ID_VORBIS,    BGAV_MK_FOURCC('V','B','I','S') }, 
    { GAVL_CODEC_ID_FLAC,      BGAV_MK_FOURCC('F','L','A','C') }, 
    
    /* Video */
    { GAVL_CODEC_ID_JPEG,      BGAV_MK_FOURCC('j','p','e','g') }, 
    { GAVL_CODEC_ID_PNG,       BGAV_MK_FOURCC('p','n','g',' ') }, 
    { GAVL_CODEC_ID_TIFF,      BGAV_MK_FOURCC('t','i','f','f') }, 
    { GAVL_CODEC_ID_TGA,       BGAV_MK_FOURCC('t','g','a',' ') }, 
    { GAVL_CODEC_ID_MPEG1,     BGAV_MK_FOURCC('m','p','v','1') }, 
    { GAVL_CODEC_ID_MPEG2,     BGAV_MK_FOURCC('m','p','v','2') },
    { GAVL_CODEC_ID_MPEG4_ASP, BGAV_MK_FOURCC('m','p','4','v') },
    { GAVL_CODEC_ID_H264,      BGAV_MK_FOURCC('H','2','6','4') },
    { GAVL_CODEC_ID_THEORA,    BGAV_MK_FOURCC('T','H','R','A') },
    { GAVL_CODEC_ID_DIRAC,     BGAV_MK_FOURCC('d','r','a','c') },
    { GAVL_CODEC_ID_DV,        BGAV_MK_FOURCC('D','V',' ',' '),},
    { /* End */                                                },
  };

static uint32_t get_fourcc(gavl_codec_id_t id)
  {
  int i = 0;
  while(fourccs[i].fourcc)
    {
    if(fourccs[i].id == id)
      return fourccs[i].fourcc;
    i++;
    }
  return 0;
  }

static int init_track(bgav_track_t * track,
                      const gavf_program_header_t * ph, const bgav_options_t * opt)
  {
  int i;
  bgav_stream_t * s;

  gavl_metadata_copy(&track->metadata, &ph->m);
  
  for(i = 0; i < ph->num_streams; i++)
    {
    s = NULL;
    switch(ph->streams[i].type)
      {
      case GAVF_STREAM_AUDIO:
        s = bgav_track_add_audio_stream(track, opt);
        gavl_audio_format_copy(&s->data.audio.format, &ph->streams[i].format.audio);
        s->fourcc = get_fourcc(ph->streams[i].ci.id);

        bgav_stream_set_extradata(s, ph->streams[i].ci.global_header,
                                  ph->streams[i].ci.global_header_len);
        s->container_bitrate = ph->streams[i].ci.bitrate;
        break;
      case GAVF_STREAM_VIDEO:
        s = bgav_track_add_video_stream(track, opt);
        gavl_video_format_copy(&s->data.video.format, &ph->streams[i].format.video);
        
        s->fourcc = get_fourcc(ph->streams[i].ci.id);

        if(!(ph->streams[i].ci.id & GAVL_COMPRESSION_HAS_P_FRAMES))
          s->flags |= STREAM_INTRA_ONLY;
        if(ph->streams[i].ci.id & GAVL_COMPRESSION_HAS_B_FRAMES)
          s->flags |= STREAM_B_FRAMES;

        bgav_stream_set_extradata(s, ph->streams[i].ci.global_header,
                                  ph->streams[i].ci.global_header_len);
        s->container_bitrate = ph->streams[i].ci.bitrate;
        break;
      case GAVF_STREAM_TEXT:
        s = bgav_track_add_subtitle_stream(track, opt, 1, "UTF-8");
        s->timescale = ph->streams[i].format.text.timescale;
        break;
      }
    s->stream_id = ph->streams[i].id;
    gavl_metadata_copy(&s->m, &ph->streams[i].m);
    }
  
  return 1;
  }
  
static int open_gavf(bgav_demuxer_context_t * ctx)
  {
  const gavl_chapter_list_t * cl;
  gavf_options_t * opt;
  uint32_t flags = 0;
  
  gavf_demuxer_t * priv = calloc(1, sizeof(*priv));
  
  ctx->priv = priv;

  priv->dec = gavf_create();

  opt = gavf_get_options(priv->dec);

  if(ctx->opt->dump_headers)
    flags |= GAVF_OPT_FLAG_DUMP_HEADERS;
  if(ctx->opt->dump_indices)
    flags |= GAVF_OPT_FLAG_DUMP_INDICES;
  gavf_options_set_flags(opt, flags);

  priv->io = gavf_io_create(read_func,
                      NULL,
                      (ctx->input->input->seek_byte ? seek_func : 0),
                      NULL,
                      ctx->input);

  if(!gavf_open_read(priv->dec, priv->io))
    return 0;
  
  ctx->tt = bgav_track_table_create(1);

  if(!init_track(ctx->tt->cur, gavf_get_program_header(priv->dec), ctx->opt))
    return 0;

  cl = gavf_get_chapter_list(priv->dec);
  ctx->tt->cur->chapter_list = gavl_chapter_list_copy(cl);
  
  return 1;
  }

static int select_track_gavf(bgav_demuxer_context_t * ctx, int track)
  {
  gavf_demuxer_t * priv;
  priv = ctx->priv;
  return 1;
  }

static int next_packet_gavf(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * bp;
  gavl_packet_t gp;
  
  const gavf_packet_header_t * ph;
  gavf_demuxer_t * priv;
  bgav_stream_t * s;

  int64_t position;
  
  priv = ctx->priv;

  position = ctx->input->position;
  
  ph = gavf_packet_read_header(priv->dec);
  if(!ph)
    return 0;

  s = bgav_track_find_stream(ctx, ph->stream_id);
  if(!s)
    {
    gavf_packet_skip(priv->dec);
    return 1;
    }

  gavl_packet_init(&gp);
    
  bp = bgav_stream_get_packet_write(s);
    
  gp.data       = bp->data;
  gp.data_alloc = bp->data_alloc;
    
  if(!gavf_packet_read_packet(priv->dec, &gp))
    return 0;

  bp->data       = gp.data;
  bp->data_alloc = gp.data_alloc;

  bp->data_size = gp.data_len;
  
  bp->pts = gp.pts;
  bp->duration = gp.duration;

  if(gp.flags & GAVL_PACKET_KEYFRAME)
    PACKET_SET_KEYFRAME(bp);

  if(s->type == BGAV_STREAM_VIDEO)
    {
    switch(gp.flags & GAVL_PACKET_TYPE_MASK)
      {
      case GAVL_PACKET_TYPE_I:
        PACKET_SET_CODING_TYPE(bp, BGAV_CODING_TYPE_I);
        break;
      case GAVL_PACKET_TYPE_P:
        PACKET_SET_CODING_TYPE(bp, BGAV_CODING_TYPE_P);
        break;
      case GAVL_PACKET_TYPE_B:
        PACKET_SET_CODING_TYPE(bp, BGAV_CODING_TYPE_B);
        break;
      }
    }
  bgav_stream_done_packet_write(s, bp);
  return 1;
  }

static void resync_gavf(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  gavf_demuxer_t * priv;
  priv = ctx->priv;
  }

static void close_gavf(bgav_demuxer_context_t * ctx)
  {
  gavf_demuxer_t * priv;
  priv = ctx->priv;
  gavf_close(priv->dec);
  gavf_io_destroy(priv->io);
  
  free(priv);
  }


const bgav_demuxer_t bgav_demuxer_gavf =
  {
    .probe        = probe_gavf,
    .open         = open_gavf,
    .select_track = select_track_gavf,
    .next_packet = next_packet_gavf,
    .resync      = resync_gavf,
    .close =       close_gavf
  };
