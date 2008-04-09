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

#include <string.h>
#include <stdlib.h>

#include <avdec_private.h>
#include <mxf.h>

#define LOG_DOMAIN "mxf"

#define FRAME_WRAPPED      0
#define CLIP_WRAPPED_CBR   1
#define CLIP_WRAPPED_PARSE 2 /* Unsupported for now */

/* TODO: Find a better way */
static int probe_mxf(bgav_input_context_t * input)
  {
  char * pos;
  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(!pos)
      return 0;
    if(!strcasecmp(pos, ".mxf"))
      return 1;
    }
  return 0;
  }

typedef struct
  {
  int64_t pts_counter;
  
  /* Constant frame size for clip-wrapped streams, which are TRUE CBR */
  int frame_size;
  
  int (*next_packet)(bgav_demuxer_context_t * ctx, bgav_stream_t * s);
    
  } stream_priv_t;

typedef struct
  {
  mxf_file_t mxf;
  } mxf_t;

static const uint32_t pcm_fourccs[] =
  {
    BGAV_MK_FOURCC('s', 'o', 'w', 't'),
    BGAV_MK_FOURCC('t', 'w', 'o', 's'),
    BGAV_MK_FOURCC('a', 'l', 'a', 'w'),
  };

static int is_pcm(uint32_t fourcc)
  {
  int i;
  for(i = 0; i < sizeof(pcm_fourccs)/sizeof(pcm_fourccs[0]); i++)
    {
    if(fourcc == pcm_fourccs[i])
      return 1;
    }
  return 0;
  }

static void init_audio_stream(bgav_demuxer_context_t * ctx, bgav_stream_t * s,
                              mxf_track_t * st, mxf_descriptor_t * sd,
                              uint32_t fourcc)
  {
  stream_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->priv = priv;
  s->fourcc = fourcc;
  if(sd->sample_rate_num % sd->sample_rate_den)
    bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN, "Rounding fractional audio samplerate");
  s->data.audio.format.samplerate = sd->sample_rate_num / sd->sample_rate_den;
  s->data.audio.format.num_channels = sd->channels;
  s->data.audio.bits_per_sample = sd->bits_per_sample;

  if(is_pcm(fourcc))
    s->data.audio.block_align = sd->channels * ((sd->bits_per_sample+7)/8);
  }

static void init_video_stream(bgav_demuxer_context_t * ctx, bgav_stream_t * s,
                              mxf_track_t * st, mxf_descriptor_t * sd,
                              uint32_t fourcc)
  {
  stream_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->priv = priv;
  s->fourcc = fourcc;
  
  if(s->fourcc == BGAV_MK_FOURCC('m','p','g','v'))
    s->data.video.frametime_mode = BGAV_FRAMETIME_CODEC;
  
  s->data.video.format.timescale      = sd->sample_rate_num;
  s->data.video.format.frame_duration = sd->sample_rate_den;
  s->data.video.format.image_width    = sd->width;
  s->data.video.format.image_height   = sd->height;
  s->data.video.format.frame_width    = sd->width;
  s->data.video.format.frame_height   = sd->height;

  if(sd->ext_size)
    {
    s->ext_size = sd->ext_size;
    s->ext_data = sd->ext_data;
    }
  
  /* Todo: Aspect ratio */
  s->data.video.format.pixel_width    = 1;
  s->data.video.format.pixel_height   = 1;
  }

#if 0
static mxf_track_t * get_source_track(mxf_file_t * file, mxf_package_t * p,
                                      mxf_source_clip_t * c)
  {
  int i;
  mxf_track_t * ret;
  for(i = 0; i < p->num_track_refs; i++)
    {
    if(!p->tracks[i])
      continue;
    ret = (mxf_track_t*)p->tracks[i];
    
    if((((mxf_preface_t*)(file->preface))->operational_pattern == MXF_OP_ATOM) ||
       (ret->track_id == c->linked_track_id))
      {
      return ret;
      break;
      }
    } 
  return (mxf_track_t*)0;
  }
#endif

static mxf_descriptor_t * get_source_descriptor(mxf_file_t * file, mxf_package_t * p, mxf_track_t * st)
  {
  int i;
  mxf_descriptor_t * desc;
  mxf_descriptor_t * sub_desc;
  if(!p->descriptor)
    {
    if((((mxf_preface_t *)(file->preface))->operational_pattern == MXF_OP_ATOM) &&
       (file->num_descriptors == 1))
      {
      for(i = 0; i < file->header.num_metadata; i++)
        {
        if(file->header.metadata[i]->type == MXF_TYPE_DESCRIPTOR)
          return (mxf_descriptor_t *)(file->header.metadata[i]);
        }
      }
    return (mxf_descriptor_t *)0;
    }
  if(p->descriptor->type == MXF_TYPE_DESCRIPTOR)
    return (mxf_descriptor_t *)p->descriptor;
  else if(p->descriptor->type == MXF_TYPE_MULTIPLE_DESCRIPTOR)
    {
    desc = (mxf_descriptor_t*)(p->descriptor);
    for(i = 0; i < desc->num_subdescriptor_refs; i++)
      {
      sub_desc = (mxf_descriptor_t *)(desc->subdescriptors[i]);
      if(sub_desc && (sub_desc->linked_track_id == st->track_id))
        return sub_desc;
      } 
    }
  return (mxf_descriptor_t*)0;
  }

#if 0
static void
handle_material_track_simple(bgav_demuxer_context_t * ctx, mxf_track_t * t)
  {
  mxf_sequence_t * ms;
  mxf_descriptor_t * sd;
  mxf_package_t * sp;
  //  mxf_sequence_t * ss;
  mxf_source_clip_t * sc;
  mxf_track_t * st;
  mxf_t * priv;
  uint32_t fourcc;
  bgav_stream_t * s = (bgav_stream_t*)0;
  
  priv = (mxf_t*)ctx->priv;
  
  ms = (mxf_sequence_t*)(t->sequence);
  
  if(!ms)
    return;
  
  if(ms->is_timecode)
    {
    /* TODO */
    return;
    }
  else if(ms->stream_type == BGAV_STREAM_UNKNOWN)
    {
    return;
    }
  else
    {
    /* Essence is somewhere else */
    if(!ms->structural_components)
      return;
    
    if(ms->structural_components[0]->type != MXF_TYPE_SOURCE_CLIP)
      return;

    sc = (mxf_source_clip_t*)(ms->structural_components[0]);
    
    sp = (mxf_package_t*)(sc->source_package);

    if(!sp)
      {
      fprintf(stderr, "Got no source package\n");
      return;
      }
    st = get_source_track(&priv->mxf, sp, sc);

    if(!st)
      {
      fprintf(stderr, "Got no source track %d\n", sc->source_track_id);
      return;
      }
    sd = get_source_descriptor(&priv->mxf, sp, sc);
    if(!sd)
      {
      fprintf(stderr, "Got no source descriptor\n");
      return;
      }
    
    if(ms->stream_type == BGAV_STREAM_AUDIO)
      {
      //      fprintf(stderr, "Got audio stream %p\n", sd);
      //      bgav_mxf_descriptor_dump(0, sd);

      fourcc = bgav_mxf_get_audio_fourcc(sd);
      if(!fourcc)
        return;
      s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
      init_audio_stream(ctx, s, st, sd, fourcc);
      }
    else if(ms->stream_type == BGAV_STREAM_VIDEO)
      {
      fourcc = bgav_mxf_get_video_fourcc(sd);
      if(!fourcc)
        return;
      s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
      init_video_stream(ctx, s, st, sd, fourcc);
      }

    /* Should not happen */
    if(!s)
      return;

    s->stream_id =
      st->track_number[0] << 24 |
      st->track_number[1] << 16 |
      st->track_number[2] <<  8 |
      st->track_number[3];
        
    bgav_stream_dump(s);
    if(ms->stream_type == BGAV_STREAM_VIDEO)
      bgav_video_dump(s);
    else if(ms->stream_type == BGAV_STREAM_AUDIO)
      bgav_audio_dump(s);
    }
  
  return;
  }
#endif

static void
handle_source_track_simple(bgav_demuxer_context_t * ctx, mxf_package_t * sp, mxf_track_t * t)
  {
  mxf_descriptor_t * sd;
  mxf_sequence_t * ss;
  mxf_source_clip_t * sc;
  mxf_t * priv;
  uint32_t fourcc;
  bgav_stream_t * s = (bgav_stream_t*)0;
  
  priv = (mxf_t*)ctx->priv;
  
  ss = (mxf_sequence_t*)(t->sequence);
  
  if(!ss)
    return;
  
  if(ss->is_timecode)
    {
    /* TODO */
    return;
    }
  else if(ss->stream_type == BGAV_STREAM_UNKNOWN)
    {
    return;
    }
  else
    {
    if(!ss->structural_components)
      return;
    
    if(ss->structural_components[0]->type != MXF_TYPE_SOURCE_CLIP)
      return;
    
    sc = (mxf_source_clip_t*)(ss->structural_components[0]);
    
    sd = get_source_descriptor(&priv->mxf, sp, t);
    if(!sd)
      {
      fprintf(stderr, "Got no source descriptor\n");
      return;
      }
    
    if(ss->stream_type == BGAV_STREAM_AUDIO)
      {
      //      fprintf(stderr, "Got audio stream %p\n", sd);
      //      bgav_mxf_descriptor_dump(0, sd);

      fourcc = bgav_mxf_get_audio_fourcc(sd);
      if(!fourcc)
        return;
      s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
      init_audio_stream(ctx, s, t, sd, fourcc);
      }
    else if(ss->stream_type == BGAV_STREAM_VIDEO)
      {
      fourcc = bgav_mxf_get_video_fourcc(sd);
      if(!fourcc)
        return;
      s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
      init_video_stream(ctx, s, t, sd, fourcc);
      }

    /* Should not happen */
    if(!s)
      return;

    s->stream_id =
      t->track_number[0] << 24 |
      t->track_number[1] << 16 |
      t->track_number[2] <<  8 |
      t->track_number[3];
#if 0        
    bgav_stream_dump(s);
    if(ms->stream_type == BGAV_STREAM_VIDEO)
      bgav_video_dump(s);
    else if(ms->stream_type == BGAV_STREAM_AUDIO)
      bgav_audio_dump(s);
#endif
    }
  
  return;
  }



/* Simple initialization: One material and one source package */
static int init_simple(bgav_demuxer_context_t * ctx)
  {
  mxf_t * priv;
  int i, j, num_tracks = 0;
  mxf_package_t * sp = (mxf_package_t *)0;
  
  priv = (mxf_t*)ctx->priv;
  /* We simply open the Source packages */
  
  for(i = 0; i < priv->mxf.header.num_metadata; i++)
    {
    if(priv->mxf.header.metadata[i]->type == MXF_TYPE_SOURCE_PACKAGE)
      num_tracks++;
    }

  ctx->tt = bgav_track_table_create(num_tracks);
  
  for(i = 0; i < priv->mxf.header.num_metadata; i++)
    {
    if(priv->mxf.header.metadata[i]->type == MXF_TYPE_SOURCE_PACKAGE)
      {
      sp = (mxf_package_t*)(priv->mxf.header.metadata[i]);

      /* Loop over tracks */
      for(j = 0; j < sp->num_track_refs; j++)
        {
        handle_source_track_simple(ctx, sp, (mxf_track_t*)sp->tracks[j]);
        }
      }
    }
  
  return 1;
  }

static int open_mxf(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  mxf_t * priv;

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  if(!bgav_mxf_file_read(ctx->input, &priv->mxf))
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Parsing MXF file failed, please report");
    return 0;
    }
  bgav_mxf_file_dump(&priv->mxf);

  if((priv->mxf.num_source_packages == 1) &&
     (priv->mxf.max_sequence_components == 1))
    {
    if(!init_simple(ctx))
      return 0;
    }
  else
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Unsupported MXF type, please report");
    return 0;
    }

  ctx->data_start = priv->mxf.data_start;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
  
  return 1;
  }

static int next_packet_frame_wrapped(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  bgav_packet_t * p;
  mxf_klv_t klv;
  int64_t position;
  stream_priv_t * sp;
  mxf_t * priv;

  priv = (mxf_t*)ctx->priv;
  position = ctx->input->position;
  
  if(!bgav_mxf_klv_read(ctx->input, &klv))
    return 0;
  s = bgav_mxf_find_stream(&priv->mxf, ctx, klv.key);
  if(!s)
    {
    bgav_input_skip(ctx->input, klv.length);
    return 1;
    }
  
  sp = (stream_priv_t*)(s->priv);

  p = bgav_stream_get_packet_write(s);
  p->position = position;
  /* check for 8 channels AES3 element */
  if(klv.key[12] == 0x06 && klv.key[13] == 0x01 && klv.key[14] == 0x10)
    {
    int i, j;
    int32_t sample;
    int64_t end_pos = ctx->input->position + klv.length;
    int num_samples;
    uint8_t * ptr;

    //    fprintf(stderr, "Got AES3 packet\n");

    /* Skip  SMPTE 331M header */
    bgav_input_skip(ctx->input, 4);

    num_samples = (end_pos - ctx->input->position) / 32; /* 8 channels*4 bytes/channel */
    
    bgav_packet_alloc(p, num_samples * s->data.audio.block_align);
    ptr = p->data;
    
    for(i = 0; i < num_samples; i++)
      {
      for(j = 0; j < s->data.audio.format.num_channels; j++)
        {
        if(!bgav_input_read_32_le(ctx->input, (uint32_t*)(&sample)))
          return 0;
        if(s->data.audio.bits_per_sample == 24)
          {
          sample = (sample >> 4) & 0xffffff;
          BGAV_24LE_2_PTR(sample, ptr);
          ptr += 3;
          p->data_size += 3;
          }
        else if(s->data.audio.bits_per_sample == 16)
          {
          sample = (sample >> 12) & 0xffff;
          BGAV_16LE_2_PTR(sample, ptr);
          ptr += 2;
          p->data_size += 2;
          }
        }
      bgav_input_skip(ctx->input, 32 - s->data.audio.format.num_channels * 4);
      }
    p->pts = sp->pts_counter;
    p->duration = num_samples;
    sp->pts_counter += num_samples;
    }
  else
    {
    bgav_packet_alloc(p, klv.length);
    if((p->data_size = bgav_input_read_data(ctx->input, p->data, klv.length)) < klv.length)
      return 0;
    
    if(s->type == BGAV_STREAM_VIDEO)
      {
      if(s->data.video.frametime_mode != BGAV_FRAMETIME_CODEC)
        {
        p->pts = sp->pts_counter;
        p->duration = s->data.video.format.frame_duration;
        sp->pts_counter += p->duration;
        }
      }
    else if(s->type == BGAV_STREAM_AUDIO)
      {
      p->pts = sp->pts_counter;
      if(s->data.audio.block_align)
        p->duration = p->data_size / s->data.audio.block_align;
      sp->pts_counter += p->duration;
      }
    }
  
  if(p)
    bgav_packet_done_write(p);
  
  return 1;
  }

static int next_packet_mxf(bgav_demuxer_context_t * ctx)
  {
  return next_packet_frame_wrapped(ctx);
  //  return 0;
  }

static void seek_mxf(bgav_demuxer_context_t * ctx, int64_t time,
                    int scale)
  {
  
  }

#if 0
static int select_track_mxf(bgav_demuxer_context_t * ctx, int track)
 {
 return 0;
  
 }
#endif

static void close_mxf(bgav_demuxer_context_t * ctx)
  {
  int i, j;
  mxf_t * priv;
  stream_priv_t * sp;
  priv = (mxf_t*)ctx->priv;
  bgav_mxf_file_free(&priv->mxf);

  for(i = 0; i < ctx->tt->num_tracks; i++)
    {
    for(j = 0; j < ctx->tt->tracks[i].num_audio_streams; j++)
      {
      sp = ctx->tt->tracks[i].audio_streams[j].priv;
      if(sp)
        free(sp);
      }
    for(j = 0; j < ctx->tt->tracks[i].num_video_streams; j++)
      {
      sp = ctx->tt->tracks[i].video_streams[j].priv;
      if(sp)
        free(sp);
      }
    /* Not supported yet but well.. */
    for(j = 0; j < ctx->tt->tracks[i].num_subtitle_streams; j++)
      {
      sp = ctx->tt->tracks[i].subtitle_streams[j].priv;
      if(sp)
        free(sp);
      }
    }

  free(priv);
  }

static void resync_mxf(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  
  }

const bgav_demuxer_t bgav_demuxer_mxf =
  {
    .probe        = probe_mxf,
    .open         = open_mxf,
    //    .select_track = select_track_mxf,
    .next_packet  =  next_packet_mxf,
    .resync       = resync_mxf,
    .seek         = seek_mxf,
    .close        = close_mxf
  };

