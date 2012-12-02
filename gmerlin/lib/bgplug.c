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
  
  } stream_common_t;

typedef struct
  {
  stream_common_t com; // Must be first

  gavl_audio_frame_t * f;

  gavl_audio_source_t * src;
  gavl_audio_sink_t   * sink;
  
  } audio_stream_t;

typedef struct
  {
  stream_common_t com; // Must be first
  
  gavl_video_frame_t * f;
  
  gavl_video_source_t * src;
  gavl_video_sink_t   * sink;
  
  } video_stream_t;

typedef struct
  {
  stream_common_t com; // Must be first
  } text_stream_t;

struct bg_plug_s
  {
  int wr;
  gavf_t * g;
  gavf_io_t * io;
  gavf_program_header_t * ph;
  
  int num_audio_streams;
  audio_stream_t * audio_streams;

  int num_video_streams;
  video_stream_t * video_streams;

  int num_text_streams;
  text_stream_t * text_streams;
  
  int is_local;  
  
  };

static bg_plug_t * create_common()
  {
  bg_plug_t * ret = calloc(1, sizeof(*ret));
  ret->g = gavf_create();
  ret->ph = gavf_get_program_header(ret->g);
  return ret;
  }
  
bg_plug_t * bg_plug_create_reader(void)
  {
  bg_plug_t * ret;
  ret = create_common();
  return ret;
  }

bg_plug_t * bg_plug_create_writer(void)
  {
  bg_plug_t * ret;
  ret = create_common();
  ret->wr = 1;
  return ret;
  }

static void free_stream_common(stream_common_t * s)
  {
  if(s->src_ext)
    gavl_packet_source_destroy(s->src_ext);
  if(s->sink_ext)
    gavl_packet_sink_destroy(s->sink_ext);
  if(s->sp)
    bg_shm_pool_destroy(s->sp);
  }

void bg_plug_destroy(bg_plug_t * p)
  {
  int i;
  gavf_close(p->g);

  if(p->audio_streams)
    {
    for(i = 0; i < p->num_audio_streams; i++)
      {
      audio_stream_t * s = p->audio_streams + i;
      if(s->src)
        gavl_audio_source_destroy(s->src);
      if(s->sink)
        gavl_audio_sink_destroy(s->sink);
      
      if(s->f)
        {
        gavl_audio_frame_null(s->f);
        gavl_audio_frame_destroy(s->f);
        }
      free_stream_common(&s->com);
      }
    }
  if(p->video_streams)
    {
    for(i = 0; i < p->num_video_streams; i++)
      {
      video_stream_t * s = p->video_streams + i;
      if(s->src)
        gavl_video_source_destroy(s->src);
      if(s->sink)
        gavl_video_sink_destroy(s->sink);
      
      if(s->f)
        {
        gavl_video_frame_null(s->f);
        gavl_video_frame_destroy(s->f);
        }
      
      free_stream_common(&s->com);
      }
    }
  if(p->text_streams)
    {
    for(i = 0; i < p->num_video_streams; i++)
      {
      text_stream_t * s = p->text_streams + i;
      free_stream_common(&s->com);
      }
    }
  
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
        audio_stream_t * s = p->audio_streams + audio_idx;
        s->com.h = p->ph->streams + i;
        s->com.index = i;
        
        audio_idx++;
        }
        break;
      case GAVF_STREAM_VIDEO:
        {
        video_stream_t * s = p->video_streams + video_idx;
        s->com.h = p->ph->streams + i;
        s->com.index = i;
        
        video_idx++;
        }
        break;
      case GAVF_STREAM_TEXT:
        {
        text_stream_t * s = p->text_streams + text_idx;
        s->com.h = p->ph->streams + i;
        s->com.index = i;
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
  stream_common_t * s = priv;

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
                            stream_common_t * s,
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
  audio_stream_t * s = priv;

  if((st = gavl_packet_source_read_packet(s->com.src_ext, &p))
     != GAVL_SOURCE_OK)
    return st;

  gavf_packet_to_audio_frame(p, s->f,
                             &s->com.h->format.audio);
  *f = s->f;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t read_video_func(void * priv,
                                            gavl_video_frame_t ** f)
  {
  gavl_source_status_t st;
  gavl_packet_t * p = NULL;
  video_stream_t * s = priv;
  
  if((st = gavl_packet_source_read_packet(s->com.src_ext, &p))
     != GAVL_SOURCE_OK)
    return st;
  
  gavf_packet_to_video_frame(p, s->f,
                             &s->com.h->format.video);
  *f = s->f;
  return GAVL_SOURCE_OK;
    
  }

static int init_read(bg_plug_t * p)
  {
  int i; 
  
  init_streams(p);

  for(i = 0; i < p->num_audio_streams; i++)
    {
    audio_stream_t * s;
    s = &p->audio_streams[i];

    init_read_common(p->g, &s->com, &s->com.h->ci,
                     &s->com.h->format.audio,
                     NULL);
    
    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->src = gavl_audio_source_create(read_audio_func, s,
                                        GAVL_SOURCE_SRC_ALLOC,
                                        &s->com.h->format.audio);
      }
    }

  for(i = 0; i < p->num_video_streams; i++)
    {
    video_stream_t * s;
    s = &p->video_streams[i];

    init_read_common(p->g, &s->com, &s->com.h->ci,
                     NULL,
                     &s->com.h->format.video);
    
    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      {
      s->src =
        gavl_video_source_create(read_video_func, s,
                                 GAVL_SOURCE_SRC_ALLOC,
                                 &s->com.h->format.video);
      }
    }
  
  for(i = 0; i < p->num_text_streams; i++)
    {
    text_stream_t * s;
    s = &p->text_streams[i];
    init_read_common(p->g, &s->com, NULL, NULL, NULL);
    }
  return 1;
  }

/* Uncompressed Sink funcs */

static gavl_audio_frame_t * get_audio_func(void * priv)
  {
  audio_stream_t * as = priv;
  as->com.p_ext = gavl_packet_sink_get_packet(as->com.sink_ext);
  as->f->valid_samples = as->com.h->format.audio.samples_per_frame;
  gavl_audio_frame_set_channels(as->f, &as->com.h->format.audio,
                                as->com.p_ext->data);
  return as->f;
  }

static gavl_sink_status_t put_audio_func(void * priv,
                                         gavl_audio_frame_t * f)
  {
  audio_stream_t * as = priv;
  gavf_audio_frame_to_packet_metadata(f, as->com.p_ext);
  return gavl_packet_sink_put_packet(as->com.sink_ext, as->com.p_ext);
  }

static gavl_video_frame_t * get_video_func(void * priv)
  {
  video_stream_t * vs = priv;
  vs->com.p_ext = gavl_packet_sink_get_packet(vs->com.sink_ext);
  gavl_video_frame_set_planes(vs->f, &vs->com.h->format.video,
                              vs->com.p_ext->data);
  
  return NULL;
  }

static gavl_sink_status_t put_video_func(void * priv,
                                         gavl_video_frame_t * f)
  {
  video_stream_t * vs = priv;
  gavf_video_frame_to_packet_metadata(f, vs->com.p_ext);
  return gavl_packet_sink_put_packet(vs->com.sink_ext, vs->com.p_ext);;
  }

/* Packet */

static gavl_packet_t * get_packet_shm(void * priv)
  {
  stream_common_t * s = priv;

  s->shm_segment = bg_shm_pool_get_write(s->sp);
  
  s->p_shm.data = bg_shm_get_buffer(s->shm_segment, &s->p_shm.data_alloc);
  gavl_packet_reset(&s->p_shm);
  return &s->p_shm;
  }

static gavl_sink_status_t put_packet_shm(void * priv, gavl_packet_t * pp)
  {
  gavl_packet_t * p;
  shm_info_t si;
  stream_common_t * s = priv;

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

static int init_write_common(bg_plug_t * p, stream_common_t * s)
  {
  s->sink_int = gavf_get_packet_sink(p->g, s->index);
  
  if(p->is_local && (s->h->ci.max_packet_size > SHM_THRESHOLD))
    {
    s->shm_size = s->h->ci.max_packet_size + GAVL_PACKET_PADDING;
    s->sp = bg_shm_pool_create(s->h->ci.max_packet_size, 1);
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
  init_streams(p);

  /* Create shared memory instances */
  for(i = 0; i < p->num_audio_streams; i++)
    {
    audio_stream_t * s;
    s = &p->audio_streams[i];

    init_write_common(p, &s->com);
    
    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      s->sink = gavl_audio_sink_create(get_audio_func,
                                       put_audio_func,
                                       s, &s->com.h->format.audio);
    }
  for(i = 0; i < p->num_video_streams; i++)
    {
    video_stream_t * s;
    s = &p->video_streams[i];
    
    init_write_common(p, &s->com);
    
    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      s->sink = gavl_video_sink_create(get_video_func,
                                       put_video_func,
                                       s, &s->com.h->format.video);
    }
  for(i = 0; i < p->num_text_streams; i++)
    {
    text_stream_t * s;
    s = &p->text_streams[i];
    init_write_common(p, &s->com);
    }
  
  return 1;
  }

int bg_plug_start(bg_plug_t * p)
  {
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

int bg_plug_get_stream_source(bg_plug_t * p,
                              const gavf_stream_header_t * h,
                              gavl_audio_source_t ** as,
                              gavl_video_source_t ** vs,
                              gavl_packet_source_t ** ps)
  {
  int i;

  switch(h->type)
    {
    case GAVF_STREAM_AUDIO:
      {
      audio_stream_t * s = NULL;
      for(i = 0; i < p->num_audio_streams; i++)
        {
        if(p->audio_streams[i].com.h->id == h->id)
          s = &p->audio_streams[i];
        }
      if(!s)
        return 0;
      
      if((s->com.h->ci.id == GAVL_CODEC_ID_NONE) && as)
        {
        *as = s->src;
        return 1;
        }
      // Compressed
      else if(ps)
        {
        if(s->com.src_ext)
          *ps = s->com.src_ext;
        else if(s->com.src_int)
          *ps = s->com.src_int;
        return 1;
        }
      }
      break;
    case GAVF_STREAM_VIDEO:
      {
      video_stream_t * s = NULL;
      for(i = 0; i < p->num_video_streams; i++)
        {
        if(p->video_streams[i].com.h->id == h->id)
          s = &p->video_streams[i];
        }
      if(!s)
        return 0;

      if((s->com.h->ci.id == GAVL_CODEC_ID_NONE) && as)
        {
        *vs = s->src;
        return 1;
        }
      // Compressed
      else if(ps)
        {
        if(s->com.src_ext)
          *ps = s->com.src_ext;
        else if(s->com.src_int)
          *ps = s->com.src_int;
        return 1;
        }
      }
      break;
    case GAVF_STREAM_TEXT:
      {
      text_stream_t * s = NULL;
      for(i = 0; i < p->num_text_streams; i++)
        {
        if(p->text_streams[i].com.h->id == h->id)
          s = &p->text_streams[i];
        }
      if(!s)
        return 0;
      if(ps)
        {
        if(s->com.src_ext)
          *ps = s->com.src_ext;
        else if(s->com.src_int)
          *ps = s->com.src_int;
        return 1;
        }
      }
      break;
    default:
      return 0;
    }
  return 0;
  }

int bg_plug_get_stream_sink(bg_plug_t * p,
                            const gavf_stream_header_t * h,
                            gavl_audio_sink_t ** as,
                            gavl_video_sink_t ** vs,
                            gavl_packet_sink_t ** ps)
  {
  int i;

  switch(h->type)
    {
    case GAVF_STREAM_AUDIO:
      {
      audio_stream_t * s = NULL;
      for(i = 0; i < p->num_audio_streams; i++)
        {
        if(p->audio_streams[i].com.h->id == h->id)
          s = &p->audio_streams[i];
        }
      if(!s)
        return 0;
      
      if((s->com.h->ci.id == GAVL_CODEC_ID_NONE) && as)
        {
        *as = s->sink;
        return 1;
        }
      // Compressed
      else if(ps)
        {
        if(s->com.sink_ext)
          *ps = s->com.sink_ext;
        else if(s->com.sink_int)
          *ps = s->com.sink_int;
        return 1;
        }
      }
      break;
    case GAVF_STREAM_VIDEO:
      {
      video_stream_t * s = NULL;
      for(i = 0; i < p->num_video_streams; i++)
        {
        if(p->video_streams[i].com.h->id == h->id)
          s = &p->video_streams[i];
        }
      if(!s)
        return 0;

      if((s->com.h->ci.id == GAVL_CODEC_ID_NONE) && as)
        {
        *vs = s->sink;
        return 1;
        }
      // Compressed
      else if(ps)
        {
        if(s->com.sink_ext)
          *ps = s->com.sink_ext;
        else if(s->com.sink_int)
          *ps = s->com.sink_int;
        return 1;
        }
      }
      break;
    case GAVF_STREAM_TEXT:
      {
      text_stream_t * s = NULL;
      for(i = 0; i < p->num_text_streams; i++)
        {
        if(p->text_streams[i].com.h->id == h->id)
          s = &p->text_streams[i];
        }
      if(!s)
        return 0;
      if(ps)
        {
        if(s->com.sink_ext)
          *ps = s->com.sink_ext;
        else if(s->com.sink_int)
          *ps = s->com.sink_int;
        return 1;
        }
      }
      break;
    default:
      return 0;
    }
  return 0;
  
#if 0
  for(i = 0; i < p->num_audio_streams; i++)
    {
    if(p->audio_streams[i].com.h->id == h->id)
      {
      if(p->audio_streams[i].com.h->ci.id == GAVL_CODEC_ID_NONE)
        {
        if(as)
          *as = p->audio_streams[i].sink;
        }
      else
        {
        if(ps)
          *ps = p->audio_streams[i].com.sink;
        }
      return 1;
      }
    }
  for(i = 0; i < p->num_video_streams; i++)
    {
    if(p->video_streams[i].com.h->id == h->id)
      {
      if(p->video_streams[i].com.h->ci.id == GAVL_CODEC_ID_NONE)
        {
        if(vs)
          *vs = p->video_streams[i].sink;
        }
      else
        {
        if(ps)
          *ps = p->video_streams[i].com.sink;
        }
      return 1;
      }
    }
  for(i = 0; i < p->num_text_streams; i++)
    {
    if(p->text_streams[i].com.h->id == h->id)
      {
      if(ps)
        *ps = p->text_streams[i].com.sink;
      return 1;
      }
    }
  return 0;
#endif
  }
