
#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
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

  gavl_packet_source_t * src;
  gavl_packet_sink_t * sink;

  bg_plug_t * plug;
  gavl_packet_t p_shm;
  gavl_packet_t p_real;
  
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
  if(s->src)
    gavl_packet_source_destroy(s->src);
  if(s->sink)
    gavl_packet_sink_destroy(s->sink);
  if(s->shm)
    bg_shm_free(s->shm);

  gavl_packet_free(&s->p_shm);
  gavl_packet_free(&s->p_real);
  
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
      free_stream_common(&s->com);
      if(s->src)
        gavl_audio_source_destroy(s->src);
      if(s->sink)
        gavl_audio_sink_destroy(s->sink);
      }
    }
  if(p->video_streams)
    {
    for(i = 0; i < p->num_video_streams; i++)
      {
      video_stream_t * s = p->video_streams + i;
      free_stream_common(&s->com);
      if(s->src)
        gavl_video_source_destroy(s->src);
      if(s->sink)
        gavl_video_sink_destroy(s->sink);
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

  }

const bg_parameter_info_t *
bg_plug_get_output_parameters(bg_plug_t * p)
  {
  
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
        p->audio_streams[audio_idx].com.h = p->ph->streams + i;
        audio_idx++;
        break;
      case GAVF_STREAM_VIDEO:
        p->video_streams[video_idx].com.h = p->ph->streams + i;
        video_idx++;
        break;
      case GAVF_STREAM_TEXT:
        p->text_streams[text_idx].com.h = p->ph->streams + i;
        text_idx++;
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
    s->shm = bg_shm_alloc_read(shm_name, s->h->ci.max_packet_size * s->num_buffers);
    
    /* Clear metadata tags */
    gavl_metadata_set(&s->h->m, META_SHM_ID, NULL);
    gavl_metadata_set(&s->h->m, META_SHM_NUM, NULL);
    }
  }

static gavl_source_status_t
read_shm_packet(stream_common_t * s)
  {
  shm_info_t si;
  if(!gavf_packet_read_packet(s->plug->g, &s->p_shm))
    return GAVL_SOURCE_EOF;
  
  if(s->p_shm.data_len != sizeof(si))
    return GAVL_SOURCE_EOF;

  memcpy(&si, s->p_shm.data, sizeof(si));
  memcpy(&s->p_real, &s->p_shm, sizeof(s->p_real));
  
  s->p_real.data = s->shm->addr + s->h->ci.max_packet_size * si.buffer_number;
  s->p_real.data_len = si.buffer_len;
  s->p_real.data_alloc = s->h->ci.max_packet_size;
  
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t
read_video_frame_func_shm(void * priv, gavl_video_frame_t ** f)
  {
  gavl_source_status_t st;
  video_stream_t * s = priv;

  if((st = read_shm_packet(&s->com)) != GAVL_SOURCE_OK)
    return st;

  if(!s->f)
    s->f = gavl_video_frame_create(NULL);
  gavf_packet_to_video_frame(&s->com.p_real, s->f,
                             &s->com.h->format.video);
  *f = s->f;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t
read_video_frame_func(void * priv, gavl_video_frame_t ** f)
  {
  video_stream_t * s = priv;
  if(!gavf_packet_read_packet(s->com.plug->g, &s->com.p_real))
    return GAVL_SOURCE_EOF;

  if(!s->f)
    s->f = gavl_video_frame_create(NULL);
  gavf_packet_to_video_frame(&s->com.p_real, s->f,
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

  if(!s->f)
    s->f = gavl_audio_frame_create(NULL);
  gavf_packet_to_audio_frame(&s->com.p_real, s->f,
                             &s->com.h->format.audio);
  *f = s->f;
  return GAVL_SOURCE_OK;

  }

static gavl_source_status_t
read_audio_frame_func(void * priv, gavl_audio_frame_t ** f)
  {
  audio_stream_t * s = priv;
  if(!gavf_packet_read_packet(s->com.plug->g, &s->com.p_real))
    return GAVL_SOURCE_EOF;
  if(!s->f)
    s->f = gavl_audio_frame_create(NULL);
  gavf_packet_to_audio_frame(&s->com.p_real, s->f,
                             &s->com.h->format.audio);
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

  *p = &s->p_real;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t
read_packet_func(void * priv, gavl_packet_t ** p)
  {
  stream_common_t * s = priv;
  if(!gavf_packet_read_packet(s->plug->g, &s->p_real))
    return GAVL_SOURCE_EOF;
  
  *p = &s->p_real;
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

    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      {
      if(s->com.shm)
        s->src =
          gavl_audio_source_create(read_audio_frame_func_shm, s,
                                   GAVL_SOURCE_SRC_ALLOC,
                                   &s->com.h->format.audio);
      else
        s->src =
          gavl_audio_source_create(read_audio_frame_func, s,
                                   GAVL_SOURCE_SRC_ALLOC,
                                   &s->com.h->format.audio);
      }
    else
      {
      if(s->com.shm)
        s->com.src =
          gavl_packet_source_create(read_packet_func_shm, s,
                                    GAVL_SOURCE_SRC_ALLOC, &s->com.h->ci,
                                    &s->com.h->format.audio, NULL);
      else
        s->com.src =
          gavl_packet_source_create(read_packet_func, s,
                                    GAVL_SOURCE_SRC_ALLOC, &s->com.h->ci,
                                    &s->com.h->format.audio, NULL);
      }
    
    }

  for(i = 0; i < p->num_video_streams; i++)
    {
    video_stream_t * s;
    s = &p->video_streams[i];
    init_shm_read(&s->com);

    if(s->com.h->ci.id == GAVL_CODEC_ID_NONE)
      {
      if(s->com.shm)
        s->src =
          gavl_video_source_create(read_video_frame_func_shm, s,
                                   GAVL_SOURCE_SRC_ALLOC,
                                   &s->com.h->format.video);
      else
        s->src =
          gavl_video_source_create(read_video_frame_func, s,
                                   GAVL_SOURCE_SRC_ALLOC,
                                   &s->com.h->format.video);
      }
    else
      {
      if(s->com.shm)
        s->com.src =
          gavl_packet_source_create(read_packet_func_shm, s,
                                    GAVL_SOURCE_SRC_ALLOC, &s->com.h->ci,
                                    NULL, &s->com.h->format.video);
      else
        s->com.src =
          gavl_packet_source_create(read_packet_func, s,
                                    GAVL_SOURCE_SRC_ALLOC, &s->com.h->ci,
                                    NULL, &s->com.h->format.video);
      }
    }
  
  for(i = 0; i < p->num_text_streams; i++)
    {
    text_stream_t * s;
    s = &p->text_streams[i];
    init_shm_read(&s->com);

    if(s->com.shm)
      s->com.src =
        gavl_packet_source_create(read_packet_func_shm, s,
                                  GAVL_SOURCE_SRC_ALLOC,
                                  NULL, NULL, NULL);
    else
      s->com.src =
        gavl_packet_source_create(read_packet_func, s,
                                  GAVL_SOURCE_SRC_ALLOC,
                                  NULL, NULL, NULL);
    }
  
  return 1;
  }

/* Write support */

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

int bg_plug_start_write(bg_plug_t * p)
  {
  int i;
  audio_stream_t * as;
  video_stream_t * vs;
  
  init_streams(p);

  /* Create shared memory instances */
  if(p->is_local)
    {
    for(i = 0; i < p->num_audio_streams; i++)
      {
      as = &p->audio_streams[i];
      if(!init_shm_write(&as->com))
        return 0;
      }
    for(i = 0; i < p->num_video_streams; i++)
      {
      vs = &p->video_streams[i];
      if(!init_shm_write(&vs->com))
        return 0;
      }
    }
  return 1;
  }

int bg_plug_open(bg_plug_t * p, const char * location)
  {
  if(!strcmp(location, "-"))
    {
    struct stat st;
    int fd;
    FILE * f;
    
    /* Stdin/stdout */
    if(p->wr)
      f = stdout;
    else
      f = stdin;
    
    fd = fileno(f);

    if(isatty(fd))
      {
      if(p->wr)
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Not writing to a TTY");
      else
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Not reading from a TTY");
      return 0;
      }
    
    if(fstat(fd, &st))
      {
      if(p->wr)
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot stat stdout");
      else
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot stat stdin");
      return 0;
      }
    if(S_ISFIFO(st.st_mode))
      {
      /* Pipe: Use local connection */
      p->is_local = 1;
      }
    p->io = gavf_io_create_file(f, p->wr, 0);
    }
  else if(!strncmp(location, "tcp://", 6))
    {
    /* Remote TCP socket */
    
    }
  else if(!strncmp(location, "unix://", 7))
    {
    /* Local UNIX domain socket */
    
    }
  else
    {
    /* Regular file */
    
    }

  if(p->io)
    return 0;

  if(p->wr)
    return 1;
  
  if(!gavf_open_read(p->g, p->io))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "gavf_open_read failed");
    return 0;
    }

  if(!init_read(p))
    return 0;
  
  return 1;
  }

gavf_t * bg_plug_reader_get_gavf(bg_plug_t * p)
  {
  return p->g;
  }

/* Optimized audio/video I/O */

/* Video */

static gavl_video_frame_t *
get_video_frame_func(void * priv)
  {
  video_stream_t * vs = priv;
  
  if(vs->com.shm)
    {
    if(!vs->f)
      vs->f = gavl_video_frame_create(NULL);

    gavl_video_frame_set_planes(vs->f, &vs->com.h->format.video,
                                vs->com.shm->addr +
                                vs->com.buffer_index * vs->com.h->ci.max_packet_size);
    }
  else
    {
    if(!vs->f)
      vs->f = gavl_video_frame_create_nopad(&vs->com.h->format.video);
    }
  return vs->f;
  }

static gavl_sink_status_t
put_video_frame_func(void * priv, gavl_video_frame_t * f)
  {
  video_stream_t * vs = priv;
  
  if(vs->com.shm)
    {
    vs->com.buffer_index++;
    if(vs->com.buffer_index == NUM_BUFFERS)
      vs->com.buffer_index = 0;
    }
  else
    {
    
    }
  }


/* Audio */

static gavl_audio_frame_t *
get_audio_frame_func(void * priv)
  {
  audio_stream_t * as = priv;
  
  if(as->com.shm)
    {
    if(!as->f)
      as->f = gavl_audio_frame_create(NULL);
    gavl_audio_frame_set_channels(as->f, &as->com.h->format.audio,
                                as->com.shm->addr +
                                as->com.buffer_index * as->com.h->ci.max_packet_size);
    }
  else
    {
    //    if(!vs->f)
      //      vs->f = gavl_audio_frame_create_nopad(&vs->h->format.audio);
    }
  return as->f;
  }

static gavl_sink_status_t
put_audio_frame_func(void * priv, gavl_audio_frame_t * f)
  {
  
  }

/* Packet */


static gavl_packet_t * get_packet_func(void * priv)
  {
  
  }

static gavl_sink_status_t put_packet_func(void * priv, gavl_packet_t * p)
  {
  
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
      for(i = 0; i < p->num_audio_streams; i++)
        {
        if(p->audio_streams[i].com.h->id == h->id)
          {
          if(p->audio_streams[i].com.h->ci.id == GAVL_CODEC_ID_NONE)
            *as = p->audio_streams[i].src;
          else
            *ps = p->audio_streams[i].com.src;
          return 1;
          }
        }
      break;
    case GAVF_STREAM_VIDEO:
      for(i = 0; i < p->num_video_streams; i++)
        {
        if(p->video_streams[i].com.h->id == h->id)
          {
          if(p->video_streams[i].com.h->ci.id == GAVL_CODEC_ID_NONE)
            *vs = p->video_streams[i].src;
          else
            *ps = p->video_streams[i].com.src;
          return 1;
          }
        }
      break;
    case GAVF_STREAM_TEXT:
      for(i = 0; i < p->num_text_streams; i++)
        {
        if(p->text_streams[i].com.h->id == h->id)
          {
          *ps = p->text_streams[i].com.src;
          return 1;
          }
        }
      break;
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
      for(i = 0; i < p->num_audio_streams; i++)
        {
        if(p->audio_streams[i].com.h->id == h->id)
          {
          if(p->audio_streams[i].com.h->ci.id == GAVL_CODEC_ID_NONE)
            *as = p->audio_streams[i].sink;
          else
            *ps = p->audio_streams[i].com.sink;
          return 1;
          }
        }
      break;
    case GAVF_STREAM_VIDEO:
      for(i = 0; i < p->num_video_streams; i++)
        {
        if(p->video_streams[i].com.h->id == h->id)
          {
          if(p->video_streams[i].com.h->ci.id == GAVL_CODEC_ID_NONE)
            *vs = p->video_streams[i].sink;
          else
            *ps = p->video_streams[i].com.sink;
          return 1;
          }
        }
      break;
    case GAVF_STREAM_TEXT:
      for(i = 0; i < p->num_text_streams; i++)
        {
        if(p->text_streams[i].com.h->id == h->id)
          {
          *ps = p->text_streams[i].com.sink;
          return 1;
          }
        }
      break;
    }
  return 0;
  }


