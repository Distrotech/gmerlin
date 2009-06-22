#include <renderer.h>

struct bg_nle_renderer_s
  {
  bg_track_info_t info;
  
  bg_nle_renderer_outstream_t ** audio_streams;
  bg_nle_renderer_outstream_t ** video_streams;
  
  int num_audio_streams;
  int num_video_streams;
  
  bg_nle_project_t * p;
  };

bg_nle_renderer_t * bg_nle_renderer_create(bg_nle_project_t * p)
  {
  bg_nle_renderer_t * ret = calloc(1, sizeof(*ret));
  ret->p = p;
  return ret;
  }

void bg_nle_renderer_destroy(bg_nle_renderer_t * r)
  {
  free(r);
  }

bg_track_info_t * bg_nle_renderer_get_track_info(bg_nle_renderer_t * r)
  {
  return &r->info;
  }

int bg_nle_renderer_read_video(bg_nle_renderer_t *r ,
                               gavl_video_frame_t * ret, int stream)
  {
  return bg_nle_renderer_outstream_read_video(r->video_streams[stream],
                                              ret);
  }

int bg_nle_renderer_read_audio(bg_nle_renderer_t * r,
                               gavl_audio_frame_t * ret,
                               int stream, int num_samples)
  {
  return bg_nle_renderer_outstream_read_audio(r->video_streams[stream],
                                              ret, num_samples);
  }

void bg_nle_renderer_seek(bg_nle_renderer_t * r, gavl_time_t time)
  {
  int i;

  for(i = 0; i < r->num_audio_streams; i++)
    bg_nle_renderer_outstream_seek(r->audio_streams[i], time);
  for(i = 0; i < r->num_video_streams; i++)
    bg_nle_renderer_outstream_seek(r->video_streams[i], time);
  
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      /* End of parameters */ 
    },
    
  };

const bg_parameter_info_t *
bg_nle_renderer_get_parameters(bg_nle_renderer_t * r)
  {
  return parameters;
  }

void bg_nle_renderer_set_parameters(void * data, const char * name,
                                    const bg_parameter_value_t * val)
  {
  
  }
