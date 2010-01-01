#include <stdlib.h>
#include <renderer.h>

#include <gmerlin/filters.h>

struct bg_nle_renderer_instream_audio_s
  {
  bg_nle_track_t * t;
  bg_nle_project_t * p;
  gavl_audio_format_t format;
  bg_audio_filter_chain_t * fc;
  bg_gavl_audio_options_t opt;

  int num_outputs;
  };

struct bg_nle_renderer_instream_video_s
  {
  bg_nle_track_t * t;
  bg_nle_project_t * p;
  gavl_video_format_t format;
  bg_video_filter_chain_t * fc;
  bg_gavl_video_options_t opt;
  int overlay_mode;

  int num_outputs;
  };


bg_nle_renderer_instream_audio_t *
bg_nle_renderer_instream_audio_create(bg_nle_project_t * p,
                                      bg_nle_track_t * t,
                                      const gavl_audio_options_t * opt)
  {
  bg_nle_renderer_instream_audio_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->p = p;
  ret->t = t;
  bg_gavl_audio_options_init(&ret->opt);
  gavl_audio_options_copy(ret->opt.opt, opt);
  
  return ret;
  }

bg_nle_renderer_instream_video_t *
bg_nle_renderer_instream_video_create(bg_nle_project_t * p,
                                      bg_nle_track_t * t,
                                      const gavl_video_options_t * opt)
  {
  bg_nle_renderer_instream_video_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->p = p;
  ret->t = t;
  bg_gavl_video_options_init(&ret->opt);
  gavl_video_options_copy(ret->opt.opt, opt);
  
  return ret;
  }

void bg_nle_renderer_instream_audio_destroy(bg_nle_renderer_instream_audio_t * s)
  {
  bg_gavl_audio_options_free(&s->opt);
  free(s);
  }

void bg_nle_renderer_instream_video_destroy(bg_nle_renderer_instream_video_t * s)
  {
  bg_gavl_video_options_free(&s->opt);
  free(s);
  }

int bg_nle_renderer_instream_video_request(bg_nle_renderer_instream_video_t * s,
                                           gavl_time_t time, int stream, float * camera, float * projector)
  {
  camera[0] = 0.0;
  camera[1] = 0.0;
  camera[2] = 1.0;
  projector[0] = 0.0;
  projector[1] = 0.0;
  projector[2] = 1.0;
  
  return 0;
  }

int bg_nle_renderer_instream_video_read(bg_nle_renderer_instream_video_t * s,
                                        gavl_video_frame_t * ret, int stream)
  {
  
  return 0;
  }

int bg_nle_renderer_instream_audio_request(bg_nle_renderer_instream_audio_t * s,
                                           int64_t sample_position,
                                           int num_samples, int stream)
  {
  return 0;
  }

int bg_nle_renderer_instream_audio_read(bg_nle_renderer_instream_audio_t * s,
                                        gavl_audio_frame_t * ret,
                                        int num_samples, int stream)
  {
  return 0;
  }

int
bg_nle_renderer_instream_video_connect_output(bg_nle_renderer_instream_video_t * s,
                                              gavl_video_format_t * format,
                                              int * overlay_mode)
  {
  int ret;
  gavl_video_format_copy(format, &s->format);
  *overlay_mode = s->overlay_mode;
  ret = s->num_outputs;
  s->num_outputs++;
  return ret;
  }

int
bg_nle_renderer_instream_audio_connect_output(bg_nle_renderer_instream_audio_t * s,
                                              gavl_audio_format_t * format)
  {
  int ret;
  gavl_audio_format_copy(format, &s->format);
  ret = s->num_outputs;
  s->num_outputs++;
  return ret;
  }
