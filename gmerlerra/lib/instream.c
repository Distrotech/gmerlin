#include <stdlib.h>
#include <renderer.h>

struct bg_nle_renderer_instream_s
  {
  bg_nle_track_t * t;
  bg_nle_project_t * p;
  
  gavl_video_format_t format;
  };

bg_nle_renderer_instream_t *
bg_nle_renderer_instream_create(bg_nle_project_t * p,
                                bg_nle_track_t * t)
  {
  bg_nle_renderer_instream_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->p = p;
  ret->t = t;
  
  return ret;
  }

void bg_nle_renderer_instream_destroy(bg_nle_renderer_instream_t * s)
  {
  free(s);
  }

int bg_nle_renderer_instream_read_video(bg_nle_renderer_instream_t * s,
                                        gavl_video_frame_t * ret)
  {
  return 0;
  }

int bg_nle_renderer_instream_read_audio(bg_nle_renderer_instream_t * s,
                                        gavl_audio_frame_t * ret,
                                        int num_samples)
  {
  return 0;
  }
