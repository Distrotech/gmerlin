#include <types.h>
#include <project.h>
#include <thumbnailfactory.h>

typedef struct job_s
  {
  int thread_index;
  struct job_s * next;

  bg_nle_stream_type_t type;

  gavl_audio_converter_t * audio_cnv;
  gavl_audio_converter_t * video_cnv;

  gavl_video_frame_t * video_frame;
  gavl_audio_frame_t * audio_frame;
  
  gavl_video_frame_t * out_frame;
  bg_nle_id_t file_id;
  int job_id;
  
  int stream;
  
  bg_nle_file_handle_t * h;
  
  union
    {
    struct
      {
      gavl_audio_format_t in_format;
      gavl_audio_format_t out_format;
      } audio;
    struct
      {
      gavl_video_format_t in_format;
      gavl_video_format_t out_format;
      } video;
    } data;
  } job_t;

struct bg_nle_thumbnail_factory_s
  {
  bg_nle_project_t * p;
  bg_thread_pool_t * tp;

  int max_id;

  job_t * idle;
  job_t * wait_start;
  job_t * running;
  job_t * wait_finish;
  
  };

job_t * create_job(bg_nle_thumbnail_factory_t *,
                   bg_nle_file_id_t id,
                   bg_nle_stream_type_t type,
                   int stream)
  {
  
  }

bg_nle_thumbnail_factory_t *
bg_nle_thumbnail_factory_create(bg_nle_project_t * p)
  {
  bg_nle_thumbnail_factory_t * ret = calloc(1, sizeof(*ret));
  ret->p = p;
  return ret;
  }

void bg_nle_thumbnail_factory_destroy(bg_nle_thumbnail_factory_t * f)
  {
  free(f);
  }

static const_bg_parameter_info_t * parameters[] =
  {
    { /* End */ },
  };

const bg_parameter_info_t * bg_nle_thumbnail_factory_get_parameters(void)
  {
  return parameters;
  }

void bg_nle_thumbnail_factory_set_parameter(void * data, const char * name,
                                            const bg_parameter_value_t * val)
  {
  if(!name)
    return;
  }

int
bg_nle_thumbnail_factory_request_audio(bg_nle_thumbnail_factory_t *,
                                       bg_nle_id_t id, int stream,
                                       gavl_time_t start_time,
                                       gavl_time_t end_time,
                                       gavl_video_format_t * format,
                                       gavl_video_frame_t * frame)
  {
  
  }

void nle_thumbnail_factory_wait_audio(bg_nle_thumbnail_factory_t *, int id)
  {
  
  }

int
bg_nle_thumbnail_factory_request_video(bg_nle_thumbnail_factory_t *,
                                       bg_nle_id_t id, int stream,
                                       gavl_time_t start_time,
                                       gavl_time_t end_time,
                                       gavl_video_format_t * format,
                                       gavl_video_frame_t * frame)
  {
  
  }

void nle_thumbnail_factory_wait_video(bg_nle_thumbnail_factory_t *, int id)
  {
  
  }
