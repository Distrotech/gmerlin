
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
  gavl_audio_frame_t * f;
  gavf_stream_header_t * h;
  bg_shm_t * shm;

  int buffer_size;
  int num_buffers;
  int buffer_index;
  } audio_stream_t;

typedef struct
  {
  gavl_video_frame_t * f;
  gavf_stream_header_t * h;
  bg_shm_t * shm;
  
  int buffer_size;
  int num_buffers;
  int buffer_index;
  } video_stream_t;

typedef struct
  {
  gavf_stream_header_t * h;
  } text_stream_t;

struct bg_plug_s
  {
  int wr;
  gavf_t * g;
  gavf_io_t * io;

  int num_audio_streams;
  audio_stream_t * audio_streams;

  int num_video_streams;
  video_stream_t * video_streams;

  int num_text_streams;
  text_stream_t * text_streams;
  
  int is_local;  
  
  gavf_program_header_t * ph;
  };

static bg_plug_t * create_common()
  {
  bg_plug_t * ret = calloc(1, sizeof(*ret));
  ret->g = gavf_create();
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

void bg_plug_reader_destroy(bg_plug_t * p)
  {
  gavf_close(p->g);
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
        p->audio_streams[audio_idx].h = p->ph->streams + i;
        audio_idx++;
        break;
      case GAVF_STREAM_VIDEO:
        p->video_streams[video_idx].h = p->ph->streams + i;
        video_idx++;
        break;
      case GAVF_STREAM_TEXT:
        p->text_streams[text_idx].h = p->ph->streams + i;
        text_idx++;
        break;
      }
    }
  }

static int init_read(bg_plug_t * p)
  {
  int i; 
  const char * shm_name;
  audio_stream_t * as;
  video_stream_t * vs;
  
  init_streams(p);

  for(i = 0; i < p->num_audio_streams; i++)
    {
    as = &p->audio_streams[i];

    if((shm_name = gavl_metadata_get(&as->h->m, META_SHM_ID)) &&
       gavl_metadata_get_int(&as->h->m, META_SHM_SIZE, &as->buffer_size) &&
       gavl_metadata_get_int(&as->h->m, META_SHM_NUM, &as->num_buffers))
      {
      /* Create shared memory handle */
      as->shm = bg_shm_alloc_read(shm_name, as->buffer_size * as->num_buffers);
      
      /* Clear metadata tags */
      gavl_metadata_set(&as->h->m, META_SHM_ID, NULL);
      gavl_metadata_set(&as->h->m, META_SHM_SIZE, NULL);
      gavl_metadata_set(&as->h->m, META_SHM_NUM, NULL);
      }
    }

  for(i = 0; i < p->num_video_streams; i++)
    {
    vs = &p->video_streams[i];

    if((shm_name = gavl_metadata_get(&vs->h->m, META_SHM_ID)) &&
       gavl_metadata_get_int(&vs->h->m, META_SHM_SIZE, &vs->buffer_size) &&
       gavl_metadata_get_int(&vs->h->m, META_SHM_NUM, &vs->num_buffers))
      {
      /* Create shared memory handle */
      vs->shm = bg_shm_alloc_read(shm_name, vs->buffer_size * vs->num_buffers);
      
      /* Clear metadata tags */
      gavl_metadata_set(&vs->h->m, META_SHM_ID, NULL);
      gavl_metadata_set(&vs->h->m, META_SHM_SIZE, NULL);
      gavl_metadata_set(&vs->h->m, META_SHM_NUM, NULL);
      }

    
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
      if(as->h->ci.id == GAVL_CODEC_ID_NONE)
        {
        const gavl_audio_format_t * fmt = 
          &as->h->format.audio;
        
        as->buffer_size =
          gavl_bytes_per_sample(fmt->sample_format) *
          fmt->num_channels * fmt->samples_per_frame;
        as->num_buffers = NUM_BUFFERS;
        as->shm = bg_shm_alloc_write(as->buffer_size * as->num_buffers);
        
        if(!as->shm)
          return 0;
        
        gavl_metadata_set(&as->h->m, META_SHM_ID, as->shm->name);
        gavl_metadata_set_int(&as->h->m, META_SHM_SIZE, as->buffer_size);
        gavl_metadata_set_int(&as->h->m, META_SHM_NUM, as->num_buffers);
        }
      }
    for(i = 0; i < p->num_video_streams; i++)
      {
      vs = &p->video_streams[i];
      if(vs->h->ci.id == GAVL_CODEC_ID_NONE)
        {
        const gavl_video_format_t * fmt = 
          &vs->h->format.video;
        vs->buffer_size = gavl_video_format_get_image_size(fmt);
        vs->num_buffers = NUM_BUFFERS;
        vs->shm = bg_shm_alloc_write(vs->buffer_size * vs->num_buffers);

        if(!vs->shm)
          return 0;
        
        gavl_metadata_set(&vs->h->m, META_SHM_ID, vs->shm->name);
        gavl_metadata_set_int(&vs->h->m, META_SHM_SIZE, vs->buffer_size);
        gavl_metadata_set_int(&vs->h->m, META_SHM_NUM, vs->num_buffers);
        }
      
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
  
  return 1;
  }

gavf_t * bg_plug_reader_get_gavf(bg_plug_t * p)
  {
  return p->g;
  }

/* Optimized audio/video I/O */

gavl_video_frame_t *
bg_plug_writer_get_video_frame(bg_plug_t *p, int stream)
  {
  video_stream_t * vs = &p->video_streams[stream];

  if(vs->shm)
    {
    if(!vs->f)
      vs->f = gavl_video_frame_create(NULL);
    gavl_video_frame_set_planes(vs->f, &vs->h->format.video,
                                vs->shm->addr + vs->buffer_index * vs->buffer_size);
    }
  else
    {
    if(!vs->f)
      vs->f = gavl_video_frame_create_nopad(&vs->h->format.video);
    }
  
  return vs->f;
  }

int bg_plug_writer_write_video_frame(bg_plug_t *p, int stream)
  {
  video_stream_t * vs = &p->video_streams[stream];
    
  return 0;
  }

gavl_audio_frame_t *
bg_plug_writer_get_audio_frame(bg_plug_t *p, int stream)
  {
  return NULL;
  }

int bg_plug_writer_write_audio_frame(bg_plug_t *p, int stream)
  {
  return 0;
  }

