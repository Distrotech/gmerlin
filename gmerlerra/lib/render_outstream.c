
#include <stdlib.h>
#include <renderer.h>
#include <gmerlin/filters.h>

struct bg_nle_renderer_outstream_audio_s
  {
  bg_nle_outstream_t * s;
  bg_nle_project_t * p;
  
  bg_nle_audio_compositor_t * ac;
  bg_audio_filter_chain_t * fc;
  
  bg_read_audio_func_t read_audio;
  int read_stream;
  void * read_priv;
  bg_gavl_audio_options_t opt;
  };

struct bg_nle_renderer_outstream_video_s
  {
  bg_nle_outstream_t * s;
  bg_nle_project_t * p;
  
  bg_nle_video_compositor_t * vc;
  bg_video_filter_chain_t * fc;

  bg_read_video_func_t read_video;
  int read_stream;
  void * read_priv;
  bg_gavl_video_options_t opt;
  };

bg_nle_renderer_outstream_audio_t *
bg_nle_renderer_outstream_audio_create(bg_nle_project_t * p,
                                       bg_nle_outstream_t * s,
                                       const gavl_audio_options_t * opt)
  {
  bg_nle_renderer_outstream_audio_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->p = p;
  ret->s = s;
  
  bg_gavl_audio_options_init(&ret->opt);
  gavl_audio_options_copy(ret->opt.opt, opt);
  
  ret->ac = bg_nle_audio_compositor_create(s);
  ret->fc = bg_audio_filter_chain_create(&ret->opt, p->plugin_reg);
  
  return ret;
  }

bg_nle_renderer_outstream_video_t *
bg_nle_renderer_outstream_video_create(bg_nle_project_t * p,
                                       bg_nle_outstream_t * s,
                                       const gavl_video_options_t * opt)
  {
  bg_nle_renderer_outstream_video_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->p = p;
  ret->s = s;

  bg_gavl_video_options_init(&ret->opt);
  gavl_video_options_copy(ret->opt.opt, opt);

  ret->vc = bg_nle_video_compositor_create(s);
  
  ret->fc = bg_video_filter_chain_create(&ret->opt, p->plugin_reg);
  
  return ret;
  }

void
bg_nle_renderer_outstream_audio_destroy(bg_nle_renderer_outstream_audio_t * s)
  {
  bg_gavl_audio_options_free(&s->opt);
  if(s->ac)
    bg_nle_audio_compositor_destroy(s->ac);
  if(s->fc)
    bg_audio_filter_chain_destroy(s->fc);
  free(s);
  }

void
bg_nle_renderer_outstream_video_destroy(bg_nle_renderer_outstream_video_t * s)
  {
  bg_gavl_video_options_free(&s->opt);
  if(s->vc)
    bg_nle_video_compositor_destroy(s->vc);
  if(s->fc)
    bg_video_filter_chain_destroy(s->fc);
  free(s);
  }

int
bg_nle_renderer_outstream_video_read(bg_nle_renderer_outstream_video_t * s,
                                     gavl_video_frame_t * ret)
  {
  return bg_nle_video_compositor_read(s->vc, ret);
  }

int
bg_nle_renderer_outstream_audio_read(bg_nle_renderer_outstream_audio_t * s,
                                     gavl_audio_frame_t * ret,
                                     int num_samples)
  {
  return bg_nle_audio_compositor_read(s->ac, ret, num_samples);
  }

void
bg_nle_renderer_outstream_audio_seek(bg_nle_renderer_outstream_audio_t * s,
                                     int64_t time)
  {
  bg_nle_audio_compositor_seek(s->ac, time);
  }

void
bg_nle_renderer_outstream_video_seek(bg_nle_renderer_outstream_video_t * s,
                                     int64_t time)
  {
  bg_nle_video_compositor_seek(s->vc, time);
  }


