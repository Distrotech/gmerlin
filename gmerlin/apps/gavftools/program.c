/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <gavftools.h>

#define FLAG_DISCONTINUOUS (1<<0)

typedef struct stream_s
  {
  int type;
  
  gavl_audio_connector_t * aconn;
  gavl_video_connector_t * vconn;
  gavl_packet_connector_t * pconn;

  gavl_audio_source_t * asrc;
  gavl_video_source_t * vsrc;
  gavl_packet_source_t * psrc;
  
  gavl_time_t max_pts;
  uint32_t timescale;

  int flags;

  int out_index;
  
  int (*process)(struct stream_s * s);

  const gavl_metadata_t * m;
  } stream_t;

static void process_cb_audio(void * priv, gavl_audio_frame_t * frame)
  {
  
  stream_t * s = priv;

  s->max_pts = gavl_time_unscale(s->timescale,
                                 frame->timestamp + frame->valid_samples);
  
  }

static int process_audio(stream_t * s)
  {
  return gavl_audio_connector_process(s->aconn);
  }

static void process_cb_video(void * priv, gavl_video_frame_t * frame)
  {
  gavl_time_t pts;

  stream_t * s = priv;

  pts = gavl_time_unscale(s->timescale,
                          frame->timestamp + frame->duration);
  if(pts > s->max_pts)
    s->max_pts = pts;
  
  }

static int process_video(stream_t * s)
  {
  return gavl_video_connector_process(s->vconn);
  
  }


static void process_cb_packet(void * priv, gavl_packet_t * packet)
  {
  gavl_time_t pts;
  stream_t * s = priv;

  pts = gavl_time_unscale(s->timescale,
                          packet->pts + packet->duration);
  if(pts > s->max_pts)
    s->max_pts = pts;
  }

static int process_packet(stream_t * s)
  {
  return gavl_packet_connector_process(s->pconn);
  }


struct bg_program_s
  {
  int num_streams;
  
  stream_t ** streams;

  bg_plug_t * src_plug;
  bg_plug_t * dst_plug;
  };

bg_program_t * bg_program_create()
  {
  bg_program_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static stream_t * append_stream(bg_program_t * p)
  {
  stream_t * ret;

  p->streams = realloc(p->streams,
                       sizeof(*p->streams) * (p->num_streams+1));
  p->streams[p->num_streams] =
    calloc(1, sizeof(*(p->streams[p->num_streams])));
  ret = p->streams[p->num_streams];
  p->num_streams++;
  
  ret->max_pts = GAVL_TIME_UNDEFINED;
  return ret;
  }

void bg_program_add_audio_stream(bg_program_t * p,
                                 gavl_audio_source_t * asrc,
                                 gavl_packet_source_t * psrc,
                                 const gavl_metadata_t * m)
  {
  stream_t * s;
  s = append_stream(p);
  s->type = GAVF_STREAM_AUDIO;
  s->m = m;
  if(asrc)
    {
    s->asrc = asrc;
    s->aconn = gavl_audio_connector_create(asrc);
    gavl_audio_connector_set_process_func(s->aconn,
                                          process_cb_audio,
                                          s);
    s->process = process_audio;
    }
  else if(psrc)
    {
    s->psrc = psrc;
    s->pconn = gavl_packet_connector_create(psrc);
    gavl_packet_connector_set_process_func(s->pconn,
                                          process_cb_packet,
                                          s);
    s->process = process_packet;
    }
  }

void bg_program_add_video_stream(bg_program_t * p,
                                 gavl_video_source_t * vsrc,
                                 gavl_packet_source_t * psrc,
                                 const gavl_metadata_t * m)
  {
  stream_t * s;
  s = append_stream(p);
  s->type = GAVF_STREAM_VIDEO;
  s->m = m;
  
  if(vsrc)
    {
    s->vsrc = vsrc;
    s->vconn = gavl_video_connector_create(vsrc);
    gavl_video_connector_set_process_func(s->vconn,
                                          process_cb_video,
                                          s);
    s->process = process_video;
    }
  else if(psrc)
    {
    s->psrc = psrc;
    s->pconn = gavl_packet_connector_create(psrc);
    gavl_packet_connector_set_process_func(s->pconn,
                                          process_cb_packet,
                                          s);
    s->process = process_packet;
    }
  }


void bg_program_add_text_stream(bg_program_t * p,
                                gavl_packet_source_t * psrc,
                                uint32_t timescale,
                                const gavl_metadata_t * m)
  {
  stream_t * s;
  s = append_stream(p);
  s->type = GAVF_STREAM_VIDEO;
  s->m = m;

  s->psrc = psrc;
  s->pconn = gavl_packet_connector_create(psrc);
  gavl_packet_connector_set_process_func(s->pconn,
                                         process_cb_packet,
                                         s);
  
  s->process = process_packet;
  s->flags |= FLAG_DISCONTINUOUS;
  s->timescale = timescale;
  }

void bg_program_connect_dst_plug(bg_program_t * p, bg_plug_t * dst)
  {
  int i;
  gavf_t * g;
  stream_t * s;
  const gavl_audio_format_t * afmt;
  const gavl_video_format_t * vfmt;
  const gavl_compression_info_t * ci;
  
  p->dst_plug = dst;

  g = bg_plug_get_gavf(p->dst_plug);

  for(i = 0; i < p->num_streams; i++)
    {
    s = p->streams[i];

    switch(s->type)
      {
      case GAVF_STREAM_AUDIO:
        if(s->asrc)
          {
          afmt = gavl_audio_source_get_src_format(s->asrc);
          s->out_index = gavf_add_audio_stream(g, NULL, afmt, s->m);
          }
        else if(s->psrc)
          {
          afmt = gavl_packet_source_get_audio_format(s->psrc);
          ci = gavl_packet_source_get_ci(s->psrc);
          s->out_index = gavf_add_audio_stream(g, ci, afmt, s->m);
          }
        break;
      case GAVF_STREAM_VIDEO:
        if(s->vsrc)
          {
          vfmt = gavl_video_source_get_src_format(s->vsrc);
          s->out_index = gavf_add_video_stream(g, NULL, vfmt, s->m);
          }
        else if(s->psrc)
          {
          vfmt = gavl_packet_source_get_video_format(s->psrc);
          ci = gavl_packet_source_get_ci(s->psrc);
          s->out_index = gavf_add_video_stream(g, ci, vfmt, s->m);
          }
        break;
      case GAVF_STREAM_TEXT:
        if(s->pconn)
          {
          s->out_index = gavf_add_text_stream(g, s->timescale, s->m);
          }
        break;
      }
    }
  
  }

void bg_program_connect_src_plug(bg_program_t * p, bg_plug_t * src)
  {
  p->src_plug = src;

  
  
  }

void bg_program_destroy(bg_program_t * p)
  {
  
  }
