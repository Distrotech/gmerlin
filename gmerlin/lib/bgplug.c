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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bgshm.h>

#include <gmerlin/bgplug.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "plug"

#define META_SHM_SIZE   "bgplug_shm_size"

#define SHM_THRESHOLD 1024 // Minimum max_packet_size to switch to shm

typedef struct
  {
  int id;  /* ID of the segment */
  int len; /* Real length       */
  } shm_info_t;

typedef struct
  {
  gavf_stream_header_t * h;

  bg_shm_pool_t * sp;
  bg_shm_t * shm_segment;

  int shm_size; // Size of an shm seqment */
  
  /* Reading */
  gavl_packet_source_t * src_int;
  gavl_packet_source_t * src_ext;

  /* Writing */
  gavl_packet_sink_t * sink_ext;
  gavl_packet_sink_t * sink_int;
  
  bg_plug_t * plug;

  gavl_packet_t p_shm;
  gavl_packet_t * p_ext;
  
  int index; // Index inside the program header

  /* Audio stuff */
  gavl_audio_frame_t * aframe;
  gavl_audio_source_t * asrc;
  gavl_audio_sink_t   * asink;
  
  /* Video stuff */
  gavl_video_frame_t * vframe;
  gavl_video_source_t * vsrc;
  gavl_video_sink_t   * vsink;
  
  } stream_t;

struct bg_plug_s
  {
  int wr;
  gavf_t * g;
  gavf_io_t * io;
  gavf_program_header_t * ph;
  
  int num_audio_streams;
  stream_t * audio_streams;

  int num_video_streams;
  stream_t * video_streams;

  int num_text_streams;
  stream_t * text_streams;
  
  int is_local;  

  bg_plugin_registry_t * plugin_reg;
  };

static bg_plug_t * create_common()
  {
  bg_plug_t * ret = calloc(1, sizeof(*ret));
  ret->g = gavf_create();
  ret->ph = gavf_get_program_header(ret->g);
  return ret;
  }
  
bg_plug_t * bg_plug_create_reader(bg_plugin_registry_t * plugin_reg)
  {
  bg_plug_t * ret;
  ret = create_common();
  ret->plugin_reg = plugin_reg;
  return ret;
  }

bg_plug_t * bg_plug_create_writer(bg_plugin_registry_t * plugin_reg)
  {
  bg_plug_t * ret = bg_plug_create_reader(plugin_reg);
  ret->wr = 1;
  return ret;
  }

static void free_streams(stream_t * streams, int num)
  {
  int i;
  stream_t * s;
  for(i = 0; i < num; i++)
    {
    s = streams + i;
    if(s->src_ext && (s->src_ext != s->src_int))
      gavl_packet_source_destroy(s->src_ext);
    if(s->sink_ext && (s->sink_ext != s->sink_int))
      gavl_packet_sink_destroy(s->sink_ext);
    if(s->sp)
      bg_shm_pool_destroy(s->sp);

    if(s->asrc)
      gavl_audio_source_destroy(s->asrc);
    if(s->asink)
      gavl_audio_sink_destroy(s->asink);

    if(s->vsrc)
      gavl_video_source_destroy(s->vsrc);
    if(s->vsink)
      gavl_video_sink_destroy(s->vsink);
    
    if(s->aframe)
      {
      gavl_audio_frame_null(s->aframe);
      gavl_audio_frame_destroy(s->aframe);
      }
    if(s->vframe)
      {
      gavl_video_frame_null(s->vframe);
      gavl_video_frame_destroy(s->vframe);
      }
    }
  if(streams)
    free(streams);
  }

void bg_plug_destroy(bg_plug_t * p)
  {
  free_streams(p->audio_streams, p->num_audio_streams);
  free_streams(p->video_streams, p->num_video_streams);
  free_streams(p->text_streams, p->num_text_streams);
  gavf_close(p->g);
  free(p);
  }

const bg_parameter_info_t *
bg_plug_get_input_parameters(bg_plug_t * p)
  {
  return NULL;
  }

const bg_parameter_info_t *
bg_plug_get_output_parameters(bg_plug_t * p)
  {
  return NULL;
  }

void bg_plug_set_parameter(void * data, const char * name,
                           const bg_parameter_value_t * val)
  {

  }

/* Read/write */

static void init_streams(bg_plug_t * p)
  {
  int i;
  int audio_idx = 0;
  int video_idx = 0;
  int text_idx = 0;
  stream_t * s;

  for(i = 0; i < p->ph->num_streams; i++)
    {
    switch(p->ph->streams[i].type)
      {
      case GAVF_STREAM_AUDIO:
        p->num_audio_streams++;
        break;
      case GAVF_STREAM_VIDEO:
        p->num_video_streams++;
        break;
      case GAVF_STREAM_TEXT:
        p->num_text_streams++;
        break;
      }
    }

  if(p->num_audio_streams)
    p->audio_streams =
      calloc(p->num_audio_streams, sizeof(*p->audio_streams));

  if(p->num_video_streams)
    p->video_streams =
      calloc(p->num_video_streams, sizeof(*p->video_streams));

  if(p->num_text_streams)
    p->text_streams =
      calloc(p->num_text_streams, sizeof(*p->text_streams));

  for(i = 0; i < p->ph->num_streams; i++)
    {
    switch(p->ph->streams[i].type)
      {
      case GAVF_STREAM_AUDIO:
        {
        s = p->audio_streams + audio_idx;
        s->h = p->ph->streams + i;
        s->index = i;
        audio_idx++;
        }
        break;
      case GAVF_STREAM_VIDEO:
        {
        s = p->video_streams + video_idx;
        s->h = p->ph->streams + i;
        s->index = i;
        video_idx++;
        }
        break;
      case GAVF_STREAM_TEXT:
        {
        s = p->text_streams + text_idx;
        s->h = p->ph->streams + i;
        s->index = i;
        text_idx++;
        }
        break;
      }
    }
  }

/* Read support */

static gavl_source_status_t read_packet_shm(void * priv,
                                            gavl_packet_t ** ret)
  {
  shm_info_t si;
  gavl_source_status_t st;
  gavl_packet_t * p = NULL;
  stream_t * s = priv;

  if((st = gavl_packet_source_read_packet(s->src_int, &p)) !=
     GAVL_SOURCE_OK)
    return st;

  /* Sanity check */
  if(p->data_len != sizeof(si))
    return GAVL_SOURCE_EOF;

  memcpy(&si, p->data, sizeof(si));

  /* Unref the old segment if there is one */
  if(s->shm_segment)
    bg_shm_unref(s->shm_segment);
  
  /* Obtain memory segment */
  s->shm_segment = bg_shm_pool_get_read(s->sp, si.id);
  
  /* Copy metadata */
  
  memcpy(&s->p_shm, p, sizeof(*p));
  
  /* Exchange pointers */
  s->p_shm.data       = bg_shm_get_buffer(s->shm_segment, &s->p_shm.data_alloc);
  s->p_shm.data_len   = si.len;
  
  *ret = &s->p_shm;
  return GAVL_SOURCE_OK;
  }

static void init_read_common(gavf_t * g,
                            stream_t * s,
                            const gavl_compression_info_t * ci,
                            const gavl_audio_format_t * afmt,
                            const gavl_video_format_t * vfmt)
  {
  s->src_int = gavf_get_packet_source(g, s->index);
  
  if(gavl_metadata_get_int(&s->h->m, META_SHM_SIZE, &s->shm_size))
    {
    /* Create shared memory handle */
    s->sp = bg_shm_pool_create(s->shm_size, 0);
    /* Clear metadata tags */
    gavl_metadata_set(&s->h->m, META_SHM_SIZE, NULL);

    s->src_ext =
      gavl_packet_source_create(read_packet_shm, s,
                                GAVL_SOURCE_SRC_ALLOC,
                                ci, afmt, vfmt);
    }
  else
    s->src_ext = s->src_int;
  }

/* Uncompressed source funcs */
static gavl_source_status_t read_audio_func(void * priv,
                                            gavl_audio_frame_t ** f)
  {
  gavl_source_status_t st;
  gavl_packet_t * p = NULL;
  stream_t * s = priv;

  if((st = gavl_packet_source_read_packet(s->src_ext, &p))
     != GAVL_SOURCE_OK)
    return st;

  gavf_packet_to_audio_frame(p, s->aframe,
                             &s->h->format.audio);
  *f = s->aframe;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t read_video_func(void * priv,
                                            gavl_video_frame_t ** f)
  {
  gavl_source_status_t st;
  gavl_packet_t * p = NULL;
  stream_t * s = priv;
  
  if((st = gavl_packet_source_read_packet(s->src_ext, &p))
     != GAVL_SOURCE_OK)
    return st;
  
  gavf_packet_to_video_frame(p, s->vframe,
                             &s->h->format.video);
  *f = s->vframe;
  return GAVL_SOURCE_OK;
    
  }

static int init_read(bg_plug_t * p)
  {
  int i; 
  stream_t * s;
  
  init_streams(p);

  for(i = 0; i < p->num_audio_streams; i++)
    {
    s = p->audio_streams + i;

    init_read_common(p->g, s, &s->h->ci,
                     &s->h->format.audio,
                     NULL);
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->asrc = gavl_audio_source_create(read_audio_func, s,
                                        GAVL_SOURCE_SRC_ALLOC,
                                        &s->h->format.audio);
      s->aframe = gavl_audio_frame_create(NULL);
      }
    }

  for(i = 0; i < p->num_video_streams; i++)
    {
    s = p->video_streams + i;

    init_read_common(p->g, s, &s->h->ci,
                     NULL,
                     &s->h->format.video);
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->vsrc =
        gavl_video_source_create(read_video_func, s,
                                 GAVL_SOURCE_SRC_ALLOC,
                                 &s->h->format.video);
      s->vframe = gavl_video_frame_create(NULL);
      }
    }
  
  for(i = 0; i < p->num_text_streams; i++)
    {
    s = p->text_streams + i;
    init_read_common(p->g, s, NULL, NULL, NULL);
    }
  return 1;
  }

/* Uncompressed Sink funcs */

static gavl_audio_frame_t * get_audio_func(void * priv)
  {
  stream_t * as = priv;
  as->p_ext = gavl_packet_sink_get_packet(as->sink_ext);
  as->aframe->valid_samples = as->h->format.audio.samples_per_frame;
  gavl_audio_frame_set_channels(as->aframe, &as->h->format.audio,
                                as->p_ext->data);
  return as->aframe;
  }

static gavl_sink_status_t put_audio_func(void * priv,
                                         gavl_audio_frame_t * f)
  {
  stream_t * as = priv;
  gavf_audio_frame_to_packet_metadata(f, as->p_ext);
  return gavl_packet_sink_put_packet(as->sink_ext, as->p_ext);
  }

static gavl_video_frame_t * get_video_func(void * priv)
  {
  stream_t * vs = priv;
  vs->p_ext = gavl_packet_sink_get_packet(vs->sink_ext);
  gavl_video_frame_set_planes(vs->vframe, &vs->h->format.video,
                              vs->p_ext->data);
  return vs->vframe;
  }

static gavl_sink_status_t put_video_func(void * priv,
                                         gavl_video_frame_t * f)
  {
  stream_t * vs = priv;
  gavf_video_frame_to_packet_metadata(f, vs->p_ext);
  return gavl_packet_sink_put_packet(vs->sink_ext, vs->p_ext);;
  }

/* Packet */

static gavl_packet_t * get_packet_shm(void * priv)
  {
  stream_t * s = priv;

  s->shm_segment = bg_shm_pool_get_write(s->sp);
  
  s->p_shm.data = bg_shm_get_buffer(s->shm_segment, &s->p_shm.data_alloc);
  gavl_packet_reset(&s->p_shm);
  return &s->p_shm;
  }

static gavl_sink_status_t put_packet_shm(void * priv, gavl_packet_t * pp)
  {
  gavl_packet_t * p;
  shm_info_t si;
  stream_t * s = priv;

  /* Exchange pointers */
  si.id = bg_shm_get_id(s->shm_segment);
  si.len = s->p_shm.data_len;

  p = gavl_packet_sink_get_packet(s->sink_int);
  
  gavl_packet_copy_metadata(p, &s->p_shm);
  gavl_packet_alloc(p, sizeof(si));
  memcpy(p->data, &si, sizeof(si));
  p->data_len = sizeof(si);
  return gavl_packet_sink_put_packet(s->sink_int, p);
  }

static int init_write_common(bg_plug_t * p, stream_t * s)
  {
  s->sink_int = gavf_get_packet_sink(p->g, s->index);
  
  if(p->is_local && (s->h->ci.max_packet_size > SHM_THRESHOLD))
    {
    s->shm_size = s->h->ci.max_packet_size + GAVL_PACKET_PADDING;
    s->sp = bg_shm_pool_create(s->shm_size, 1);
    gavl_metadata_set_int(&s->h->m, META_SHM_SIZE, s->shm_size);
    
    s->sink_ext = gavl_packet_sink_create(get_packet_shm,
                                          put_packet_shm,
                                          s);
    }
  else
    s->sink_ext = s->sink_int;
  
  return 1;
  }

static int init_write(bg_plug_t * p)
  {
  int i;
  stream_t * s;

  if(!gavf_start(p->g))
    return 0;
  
  init_streams(p);

  /* Create shared memory instances */
  for(i = 0; i < p->num_audio_streams; i++)
    {
    s = p->audio_streams + i;
    init_write_common(p, s);
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->asink = gavl_audio_sink_create(get_audio_func,
                                        put_audio_func,
                                        s, &s->h->format.audio);
      s->aframe = gavl_audio_frame_create(NULL);
      }
    }
  for(i = 0; i < p->num_video_streams; i++)
    {
    s = p->video_streams + i;
    init_write_common(p, s);
    
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->vsink = gavl_video_sink_create(get_video_func,
                                       put_video_func,
                                       s, &s->h->format.video);
      s->vframe = gavl_video_frame_create(NULL);
      }
    }
  for(i = 0; i < p->num_text_streams; i++)
    {
    s = p->text_streams + i;
    init_write_common(p, s);
    }
  
  return 1;
  }

int bg_plug_start(bg_plug_t * p)
  {
  if(!p->io)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot start %s plug (call open first)",
           (p->wr ? "output" : "input"));
    return 0;
    }
  
  if(p->wr)
    return init_write(p);
  else
    return init_read(p);
  }

int bg_plug_open(bg_plug_t * p, gavf_io_t * io,
                 const gavl_metadata_t * m,
                 const gavl_chapter_list_t * cl)
  {
  p->io = io;
  
  if(p->wr)
    {
    if(!gavf_open_write(p->g, p->io, m, cl))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "gavf_open_write failed");
      return 0;
      }
    return 1;
    }
  if(!gavf_open_read(p->g, p->io))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "gavf_open_read failed");
    return 0;
    }

  if(!init_read(p))
    return 0;
  
  return 1;
  }

int bg_plug_open_location(bg_plug_t * p, const char * location,
                          const gavl_metadata_t * m,
                          const gavl_chapter_list_t * cl)
  {
  gavf_io_t * io = bg_plug_io_open_location(location, p->wr, &p->is_local);
  if(io)
    return bg_plug_open(p, io, m, cl);
  else
    return 0;
  }

gavf_t * bg_plug_get_gavf(bg_plug_t * p)
  {
  return p->g;
  }


const gavf_stream_header_t * bg_plug_next_packet_header(bg_plug_t * p)
  {
  int i;
  const gavf_packet_header_t * ph = gavf_packet_read_header(p->g);

  if(!ph)
    return NULL;

  for(i = 0; i < p->ph->num_streams; i++)
    {
    if(ph->stream_id == p->ph->streams[i].id)
      return &p->ph->streams[i];
    }
  return NULL;
  }

/* Utility functions */

const gavf_stream_header_t *
bg_plug_header_from_index(bg_plug_t * p, int index)
  {
  return &p->ph->streams[index];
  }

const gavf_stream_header_t *
bg_plug_header_from_id(bg_plug_t * p, uint32_t id)
  {
  int i;
  for(i = 0; i < p->ph->num_streams; i++)
    {
    if(p->ph->streams[i].id == id)
      return &p->ph->streams[i];
    }
  return NULL;
  }

/* Get stream sources */

static stream_t * find_stream_by_id(stream_t * streams, int num, int id)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    if(streams[i].h->id == id)
      return &streams[i];
    }
  return NULL;
  }

static stream_t * find_stream_by_id_all(bg_plug_t * p, int id)
  {
  stream_t * ret;
  ret = find_stream_by_id(p->audio_streams, p->num_audio_streams, id);
  if(ret)
    return ret;
  ret = find_stream_by_id(p->video_streams, p->num_video_streams, id);
  if(ret)
    return ret;
  ret = find_stream_by_id(p->text_streams, p->num_text_streams, id);
  if(ret)
    return ret;
  return NULL;
  }
  
int bg_plug_get_stream_source(bg_plug_t * p,
                              const gavf_stream_header_t * h,
                              gavl_audio_source_t ** as,
                              gavl_video_source_t ** vs,
                              gavl_packet_source_t ** ps)
  {
  stream_t * s = NULL;
  if(!(s = find_stream_by_id_all(p, h->id)))
    return 0;
  
  if(s->asrc && as)
    *as = s->asrc;
  else if(s->vsrc && vs)
    *vs = s->vsrc;
  else if(ps)
    *ps = s->src_ext;
  else
    return 0;
  return 1;
  }

int bg_plug_get_stream_sink(bg_plug_t * p,
                            const gavf_stream_header_t * h,
                            gavl_audio_sink_t ** as,
                            gavl_video_sink_t ** vs,
                            gavl_packet_sink_t ** ps)
  {
  stream_t * s = NULL;
  if(!(s = find_stream_by_id_all(p, h->id)))
    return 0;
     
  if(s->asink && as)
    *as = s->asink;
  else if(s->vsink && vs)
    *vs = s->vsink;
  else if(ps)
    *ps = s->sink_ext;
  else
    return 0;
  return 1;
  }

/* Setup writer */

int bg_plug_setup_writer(bg_plug_t * p, bg_mediaconnector_t * conn)
  {
  int i;
  gavl_compression_info_t ci_none;
  const gavl_audio_format_t * afmt;
  const gavl_video_format_t * vfmt;
  const gavl_compression_info_t * ci;
  bg_mediaconnector_stream_t * s;
  const gavf_stream_header_t * h;

  gavl_audio_sink_t * as;
  gavl_video_sink_t * vs;
  gavl_packet_sink_t * ps;
  
  memset(&ci_none, 0, sizeof(ci_none));
  
  for(i = 0; i < conn->num_streams; i++)
    {
    s = conn->streams + i;
    switch(s->type)
      {
      case GAVF_STREAM_AUDIO:
        if(s->psrc)
          {
          ci = gavl_packet_source_get_ci(s->psrc);
          afmt = gavl_packet_source_get_audio_format(s->psrc);
          }
        else if(s->asrc)
          {
          ci = &ci_none;
          afmt = gavl_audio_source_get_src_format(s->asrc);
          }
        else
          return 0;
        
        if(gavf_add_audio_stream(p->g, ci, afmt, &s->m) < 0)
          return 0;
        break;
      case GAVF_STREAM_VIDEO:
        if(s->psrc)
          {
          ci = gavl_packet_source_get_ci(s->psrc);
          vfmt = gavl_packet_source_get_video_format(s->psrc);
          }
        else if(s->vsrc)
          {
          ci = &ci_none;
          vfmt = gavl_video_source_get_src_format(s->vsrc);
          }
        else
          return 0;
        
        if(gavf_add_video_stream(p->g, ci, vfmt, &s->m) < 0)
          return 0;
        break;
      case GAVF_STREAM_TEXT:
        if(gavf_add_text_stream(p->g, s->timescale, &s->m) < 0)
          return 0;
        break;
      }
    }

  if(!bg_plug_start(p))
    return 0;


  /* Get sinks and connect them */

  for(i = 0; i < conn->num_streams; i++)
    {
    s = conn->streams + i;
    h = p->ph->streams + i;

    as = NULL;
    vs = NULL;
    ps = NULL;
    
    if(!bg_plug_get_stream_sink(p, h, &as, &vs, &ps))
      return 0;
    
    if(as && conn->streams->aconn)
      gavl_audio_connector_connect(conn->streams->aconn, as);
    else if(vs && conn->streams->vconn)
      gavl_video_connector_connect(conn->streams->vconn, vs);
    else if(ps && conn->streams->pconn)
      gavl_packet_connector_connect(conn->streams->pconn, ps);
    else
      return 0;
    }
  return 1;
  }
