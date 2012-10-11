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

#define META_SHM_ID   "bgplug_shm_id"
#define META_SHM_NUM  "bgplug_shm_num"

#define NUM_BUFFERS 2
#define SHM_THRESHOLD 1024 // Minimum max_packet_size to switch to shm

typedef struct
  {
  int buffer_number;
  int buffer_len;
  } shm_info_t;

typedef struct
  {
  gavf_stream_header_t * h;
  bg_shm_t * shm;

  int num_buffers;
  int buffer_index;

  gavl_packet_source_t * src_int;
  gavl_packet_sink_t * sink_int;

  gavl_packet_source_t * src_ext;
  gavl_packet_sink_t * sink_ext;
  
  bg_plug_t * plug;
  gavl_packet_t p; // True packet (data points to shm area)
  gavl_packet_t p_shminfo; // Transmitted packet

  int index; // Index inside the program header
  
  } stream_common_t;

typedef struct
  {
  stream_common_t com; // Must be first

  gavl_audio_frame_t * f;
  gavl_audio_source_t * src_int;
  gavl_audio_sink_t   * sink_int;

  gavl_audio_source_t * src_ext;
  gavl_audio_sink_t   * sink_ext;

  } audio_stream_t;

typedef struct
  {
  stream_common_t com; // Must be first
  
  gavl_video_frame_t * f;
  gavl_video_source_t * src_int;
  gavl_video_sink_t   * sink_int;

  gavl_video_source_t * src_ext;
  gavl_video_sink_t   * sink_ext;

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
  
  
  if(s->shm)
    bg_shm_free(s->shm);
  
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
      if(s->src_ext)
        gavl_audio_source_destroy(s->src_ext);
      if(s->sink_ext)
        gavl_audio_sink_destroy(s->sink_ext);

      if(s->f)
        {
        if(s->com.shm)
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
      if(s->src_ext)
        gavl_video_source_destroy(s->src_ext);
      if(s->sink_ext)
        gavl_video_sink_destroy(s->sink_ext);

      if(s->f)
        {
        if(s->com.shm)
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

static void init_shm_read(stream_common_t * s)
  {
  const char * shm_name;

  if((shm_name = gavl_metadata_get(&s->h->m, META_SHM_ID)) &&
     gavl_metadata_get_int(&s->h->m, META_SHM_NUM, &s->num_buffers))
    {
    /* Create shared memory handle */
    s->shm = bg_shm_alloc_read(shm_name,
                               s->h->ci.max_packet_size * s->num_buffers);
    
    /* Clear metadata tags */
    gavl_metadata_set(&s->h->m, META_SHM_ID, NULL);
    gavl_metadata_set(&s->h->m, META_SHM_NUM, NULL);
    }
  }

static gavl_source_status_t
read_shm_packet(stream_common_t * s)
  {
  shm_info_t si;
  gavl_source_status_t st;
  gavl_packet_t * pp = &s->p_shminfo;
  
  if((st = gavl_packet_source_read_packet(s->src_int, &pp)) !=
     GAVL_SOURCE_OK)
    return st;
  
  if(s->p_shminfo.data_len != sizeof(si))
    return GAVL_SOURCE_EOF;
  
  memcpy(&si, s->p_shminfo.data, sizeof(si));
  memcpy(&s->p, &s->p_shminfo, sizeof(s->p));
  
  s->p.data = s->shm->addr + s->h->ci.max_packet_size * si.buffer_number;
  s->p.data_len = si.buffer_len;
  s->p.data_alloc = s->h->ci.max_packet_size;
  
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t
read_video_frame_func_shm(void * priv, gavl_video_frame_t ** f)
  {
  gavl_source_status_t st;
  video_stream_t * s = priv;

  if((st = read_shm_packet(&s->com)) != GAVL_SOURCE_OK)
    return st;

  gavf_packet_to_video_frame(&s->com.p, s->f,
                             &s->com.h->format.video);
  *f = s->f;
  return GAVL_SOURCE_OK;
  }


static gavl_source_status_t
read_audio_frame_func_shm(void * priv, gavl_audio_frame_t ** f)
  {
  gavl_source_status_t st;
  audio_stream_t * s = priv;

  if((st = read_shm_packet(&s->com)) != GAVL_SOURCE_OK)
    return st;

  gavf_packet_to_audio_frame(&s->com.p, s->f,
                             &s->com.h->format.audio);

  /* The frame pointers might be wrong in the shm case when
     valid_samples is smaller than samples_per_frame */

  if(s->f->valid_samples < s->com.h->format.audio.samples_per_frame)
    {
    int valid_samples_save = s->f->valid_samples;
    s->f->valid_samples = s->com.h->format.audio.samples_per_frame;
    gavl_audio_frame_set_channels(s->f, &s->com.h->format.audio,
                                  s->com.p.data);
    s->f->valid_samples = valid_samples_save;
    }
  
  *f = s->f;
  return GAVL_SOURCE_OK;

  }


static gavl_source_status_t
read_packet_func_shm(void * priv, gavl_packet_t ** p)
  {
  gavl_source_status_t st;
  stream_common_t * s = priv;

  if((st = read_shm_packet(s)) != GAVL_SOURCE_OK)
    return st;

  *p = &s->p;
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
    init_shm_read(&s->com);

    s->com.src_int = gavf_get_packet_source(p->g, s->com.index);
    
    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      {
      if(s->com.shm)
        {
        s->src_ext =
          gavl_audio_source_create(read_audio_frame_func_shm, s,
                                   GAVL_SOURCE_SRC_ALLOC,
                                   &s->com.h->format.audio);
        s->f = gavl_audio_frame_create(NULL);
        }
      else
        s->src_int = gavf_get_audio_source(p->g, s->com.index);
      }
    else
      {
      if(s->com.shm)
        s->com.src_ext =
          gavl_packet_source_create(read_packet_func_shm, s,
                                    GAVL_SOURCE_SRC_ALLOC,
                                    &s->com.h->ci,
                                    &s->com.h->format.audio, NULL);
      }
    }

  for(i = 0; i < p->num_video_streams; i++)
    {
    video_stream_t * s;
    s = &p->video_streams[i];
    init_shm_read(&s->com);

    s->com.src_int = gavf_get_packet_source(p->g, s->com.index);
    
    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      {
      if(s->com.shm)
        {
        s->src_ext =
          gavl_video_source_create(read_video_frame_func_shm, s,
                                   GAVL_SOURCE_SRC_ALLOC,
                                   &s->com.h->format.video);
        s->f = gavl_video_frame_create(NULL);
        }
      else
        {
        s->src_int =
          gavf_get_video_source(p->g, s->com.index);
        }
      }
    else
      {
      if(s->com.shm)
        s->com.src_ext =
          gavl_packet_source_create(read_packet_func_shm, s,
                                    GAVL_SOURCE_SRC_ALLOC, &s->com.h->ci,
                                    NULL, &s->com.h->format.video);
      }
    }
  
  for(i = 0; i < p->num_text_streams; i++)
    {
    text_stream_t * s;
    s = &p->text_streams[i];
    init_shm_read(&s->com);

    s->com.src_int = gavf_get_packet_source(p->g, s->com.index);
    
    if(s->com.shm)
      s->com.src_ext =
        gavl_packet_source_create(read_packet_func_shm, s,
                                  GAVL_SOURCE_SRC_ALLOC,
                                  NULL, NULL, NULL);
    }
  return 1;
  }

/* Write support */

static void prepare_packet_shm(stream_common_t * s)
  {
  s->p.data = s->shm->addr + s->buffer_index * s->h->ci.max_packet_size;
  s->p.data_alloc = s->h->ci.max_packet_size;
  }

static gavl_sink_status_t write_packet_shm(stream_common_t * s)
  {
  shm_info_t si;
  int result;
  
  si.buffer_number = s->buffer_index;
  si.buffer_len    = s->p.data_len;

  gavl_packet_alloc(&s->p_shminfo, sizeof(si));
  memcpy(s->p_shminfo.data, &si, sizeof(si));
  s->p_shminfo.data_len = sizeof(si);
  gavl_packet_copy_metadata(&s->p_shminfo, &s->p);
  result = gavl_packet_sink_put_packet(s->sink_int, &s->p_shminfo);

  s->buffer_index++;
  if(s->buffer_index == NUM_BUFFERS)
    s->buffer_index = 0;
  return result;
  }

/* Video */

static gavl_video_frame_t *
get_video_frame_func_shm(void * priv)
  {
  video_stream_t * vs = priv;
  prepare_packet_shm(&vs->com);
  gavl_video_frame_set_planes(vs->f, &vs->com.h->format.video,
                              vs->com.p.data);
  return vs->f;
  }

static gavl_sink_status_t
put_video_frame_func_shm(void * priv, gavl_video_frame_t * f)
  {
  video_stream_t * vs = priv;
  gavf_video_frame_to_packet_metadata(f, &vs->com.p);
  return write_packet_shm(&vs->com);
  }

/* Audio */

static gavl_audio_frame_t *
get_audio_frame_func_shm(void * priv)
  {
  audio_stream_t * as = priv;
  if(!as->f)
    as->f = gavl_audio_frame_create(NULL);
  prepare_packet_shm(&as->com);
  as->f->valid_samples = as->com.h->format.audio.samples_per_frame;
  gavl_audio_frame_set_channels(as->f, &as->com.h->format.audio,
                                as->com.p.data);
  return as->f;
  }

static gavl_sink_status_t
put_audio_frame_func_shm(void * priv, gavl_audio_frame_t * f)
  {
  audio_stream_t * as = priv;
  gavf_audio_frame_to_packet_metadata(f, &as->com.p);
  return write_packet_shm(&as->com);
  }



/* Packet */

static gavl_packet_t * get_packet_func_shm(void * priv)
  {
  stream_common_t * s = priv;
  prepare_packet_shm(s);
  return &s->p;
  }

static gavl_sink_status_t put_packet_func_shm(void * priv, gavl_packet_t * p)
  {
  stream_common_t * s = priv;
  return write_packet_shm(s);
  }


static void create_packet_sink(stream_common_t * s)
  {
  if(s->shm)
    s->sink_ext = gavl_packet_sink_create(get_packet_func_shm,
                                          put_packet_func_shm,
                                          s);
  }


static int init_shm_write(stream_common_t * s)
  {
  if(s->h->ci.max_packet_size > SHM_THRESHOLD)
    {
    s->num_buffers = NUM_BUFFERS;
    s->shm = bg_shm_alloc_write(s->h->ci.max_packet_size * s->num_buffers);
    
    if(!s->shm)
      return 0;
    
    gavl_metadata_set(&s->h->m, META_SHM_ID, s->shm->name);
    gavl_metadata_set_int(&s->h->m, META_SHM_NUM, s->num_buffers);
    }
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
    if(p->is_local && !init_shm_write(&s->com))
      return 0;

    s->com.sink_int = gavf_get_packet_sink(p->g, s->com.index);
                                               
    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      {
      if(s->com.shm)
        {
        s->sink_ext = gavl_audio_sink_create(get_audio_frame_func_shm,
                                             put_audio_frame_func_shm,
                                             s, &s->com.h->format.audio);
        s->f = gavl_audio_frame_create(NULL);
        }
      else
        s->sink_int = gavf_get_audio_sink(p->g, s->com.index);
      }
    else
      create_packet_sink(&s->com);
    }
  for(i = 0; i < p->num_video_streams; i++)
    {
    video_stream_t * s;
    s = &p->video_streams[i];
    if(p->is_local && !init_shm_write(&s->com))
      return 0;
    s->com.sink_int = gavf_get_packet_sink(p->g, s->com.index);

    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      {
      if(s->com.shm)
        {
        s->sink_ext = gavl_video_sink_create(get_video_frame_func_shm,
                                         put_video_frame_func_shm,
                                         s, &s->com.h->format.video);
        s->f = gavl_video_frame_create(NULL);
        }
      else
        s->sink_int = gavf_get_video_sink(p->g, s->com.index);
      }
    else
      create_packet_sink(&s->com);
    }
  for(i = 0; i < p->num_text_streams; i++)
    {
    text_stream_t * s;
    s = &p->text_streams[i];
    if(p->is_local && !init_shm_write(&s->com))
      return 0;

    s->com.sink_int = gavf_get_packet_sink(p->g, s->com.index);
    
    create_packet_sink(&s->com);
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
        if(s->src_ext)
          *as = s->src_ext;
        else if(s->src_int)
          *as = s->src_int;
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
        if(s->src_ext)
          *vs = s->src_ext;
        else if(s->src_int)
          *vs = s->src_int;
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
        if(s->sink_ext)
          *as = s->sink_ext;
        else if(s->sink_int)
          *as = s->sink_int;
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
        if(s->sink_ext)
          *vs = s->sink_ext;
        else if(s->sink_int)
          *vs = s->sink_int;
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
