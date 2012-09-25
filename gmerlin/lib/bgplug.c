
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
#define META_SHM_SIZE "bgplug_shm_size"
#define META_SHM_NUM  "bgplug_shm_num"

#define NUM_BUFFERS 2

typedef struct
  {
  int buffer_number;
  int buffer_len;
  } shm_info_t;

typedef struct
  {
  gavf_stream_header_t * h;
  bg_shm_t * shm;

  int buffer_size;
  int num_buffers;
  int buffer_index;

  gavl_packet_source_t * src;
  gavl_packet_sink_t * sink;
  
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

const bg_parameter_info_t * bg_plug_get_parameters(bg_plug_t * p)
  {

  }

void bg_plug_set_parameter(void * data, const char * name,
                           const bg_parameter_value_t * val)
  {

  }

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

static void init_shm_read(stream_common_t * s)
  {
  const char * shm_name;

  if((shm_name = gavl_metadata_get(&s->h->m, META_SHM_ID)) &&
     gavl_metadata_get_int(&s->h->m, META_SHM_SIZE, &s->buffer_size) &&
     gavl_metadata_get_int(&s->h->m, META_SHM_NUM, &s->num_buffers))
    {
    /* Create shared memory handle */
    s->shm = bg_shm_alloc_read(shm_name, s->buffer_size * s->num_buffers);
    
    /* Clear metadata tags */
    gavl_metadata_set(&s->h->m, META_SHM_ID, NULL);
    gavl_metadata_set(&s->h->m, META_SHM_SIZE, NULL);
    gavl_metadata_set(&s->h->m, META_SHM_NUM, NULL);
    }
  }

static int init_read(bg_plug_t * p)
  {
  int i; 
  audio_stream_t * as;
  video_stream_t * vs;
  
  init_streams(p);

  for(i = 0; i < p->num_audio_streams; i++)
    {
    as = &p->audio_streams[i];
    init_shm_read(&as->com);
    }

  for(i = 0; i < p->num_video_streams; i++)
    {
    vs = &p->video_streams[i];
    init_shm_read(&vs->com);
    }
  return 1;
  }

static int init_shm_write(stream_common_t * s)
  {
  if(s->h->ci.id == GAVL_CODEC_ID_NONE)
    {
    const gavl_audio_format_t * fmt =  &s->h->format.audio;
    
    s->buffer_size = gavl_bytes_per_sample(fmt->sample_format) *
      fmt->num_channels * fmt->samples_per_frame;
    s->num_buffers = NUM_BUFFERS;
    s->shm = bg_shm_alloc_write(s->buffer_size * s->num_buffers);
    
    if(!s->shm)
      return 0;
        
    gavl_metadata_set(&s->h->m, META_SHM_ID, s->shm->name);
    gavl_metadata_set_int(&s->h->m, META_SHM_SIZE, s->buffer_size);
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
                                vs->com.buffer_index * vs->com.buffer_size);
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
  
  }

static gavl_source_status_t
read_video_frame_func(void * priv, gavl_video_frame_t ** f)
  {
  
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
                                as->com.buffer_index * as->com.buffer_size);
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

static gavl_source_status_t
read_audio_frame_func(void * priv, gavl_audio_frame_t ** f)
  {
  
  }

/* Packet */


static gavl_packet_t * get_packet_func(void * priv)
  {
  
  }

static gavl_sink_status_t put_packet_func(void * priv, gavl_packet_t * p)
  {
  
  }

static gavl_source_status_t read_packet_func(void * priv, gavl_packet_t ** p)
  {
  
  }
