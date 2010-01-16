
#include <string.h>
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

  gavl_audio_format_t format;
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

  float bg_color[4];
  
  bg_gavl_video_options_t opt;
  gavl_video_format_t format;
  };

static void set_audio_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val)
  {
  bg_nle_renderer_outstream_audio_t * s = data;

  if(bg_gavl_audio_set_parameter(&s->opt, name, val))
    return;
  }

bg_nle_renderer_outstream_audio_t *
bg_nle_renderer_outstream_audio_create(bg_nle_project_t * p,
                                       bg_nle_outstream_t * s,
                                       const gavl_audio_options_t * opt,
                                       gavl_audio_format_t * out_format)
  {
  bg_nle_renderer_outstream_audio_t * ret;
  
  ret = calloc(1, sizeof(*ret));
  ret->p = p;
  ret->s = s;
  
  bg_gavl_audio_options_init(&ret->opt);
  gavl_audio_options_copy(ret->opt.opt, opt);
  
  bg_cfg_section_apply(s->section, bg_nle_outstream_audio_parameters,
                       set_audio_parameter, ret);
  
  bg_gavl_audio_options_set_format(&ret->opt, NULL, &ret->format);
  
  ret->ac = bg_nle_audio_compositor_create(s, &ret->opt, &ret->format);
  ret->fc = bg_audio_filter_chain_create(&ret->opt, p->plugin_reg);

  bg_audio_filter_chain_connect_input(ret->fc,
                                      bg_nle_audio_compositor_read, ret->ac, 0);

  bg_audio_filter_chain_init(ret->fc, &ret->format, out_format);
  
  ret->read_audio = bg_audio_filter_chain_read;
  ret->read_priv  = ret->fc;
  ret->read_stream = 0;
  
  return ret;
  }

static void set_video_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val)
  {
  bg_nle_renderer_outstream_video_t * s = data;

  if(!name)
    {
    bg_gavl_video_set_parameter(&s->opt, NULL, NULL);
    return;
    }
  
  if(bg_gavl_video_set_parameter(&s->opt, name, val))
    return;
  else if(!strcmp(name, "background"))
    {
    memcpy(s->bg_color, val->val_color, 4 * sizeof(float));
    }
  }

bg_nle_renderer_outstream_video_t *
bg_nle_renderer_outstream_video_create(bg_nle_project_t * p,
                                       bg_nle_outstream_t * s,
                                       const gavl_video_options_t * opt,
                                       gavl_video_format_t * out_format)
  {
  bg_nle_renderer_outstream_video_t * ret;

  ret = calloc(1, sizeof(*ret));
  ret->p = p;
  ret->s = s;

  bg_gavl_video_options_init(&ret->opt);
  gavl_video_options_copy(ret->opt.opt, opt);

  bg_cfg_section_apply(s->section, bg_nle_outstream_video_parameters,
                       set_video_parameter, ret);

  bg_gavl_video_options_set_framerate(&ret->opt, NULL, &ret->format);
  bg_gavl_video_options_set_frame_size(&ret->opt, NULL, &ret->format);
  bg_gavl_video_options_set_pixelformat(&ret->opt, NULL, &ret->format);
  
  ret->vc = bg_nle_video_compositor_create(s, &ret->opt, &ret->format, ret->bg_color);
  
  ret->fc = bg_video_filter_chain_create(&ret->opt, p->plugin_reg);
  bg_video_filter_chain_connect_input(ret->fc, bg_nle_video_compositor_read, ret->vc, 0);

  /* TODO: Apply filter parameters */

  bg_video_filter_chain_init(ret->fc, &ret->format, out_format);
  
  ret->read_video = bg_video_filter_chain_read;
  ret->read_priv  = ret->fc;
  ret->read_stream = 0;
  
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

void bg_nle_renderer_outstream_audio_add_istream(bg_nle_renderer_outstream_audio_t * os,
                                                 bg_nle_renderer_instream_audio_t * s,
                                                 bg_nle_track_t * t)
  {
  bg_nle_audio_compositor_add_stream(os->ac, s, t);
  }

void bg_nle_renderer_outstream_video_add_istream(bg_nle_renderer_outstream_video_t * os,
                                                 bg_nle_renderer_instream_video_t * s,
                                                 bg_nle_track_t * t)
  {
  bg_nle_video_compositor_add_stream(os->vc, s, t);
  }


int
bg_nle_renderer_outstream_video_read(bg_nle_renderer_outstream_video_t * s,
                                     gavl_video_frame_t * ret)
  {
  return s->read_video(s->read_priv, ret, s->read_stream);
  }

int
bg_nle_renderer_outstream_audio_read(bg_nle_renderer_outstream_audio_t * s,
                                     gavl_audio_frame_t * ret,
                                     int num_samples)
  {
  return s->read_audio(s->read_priv, ret, s->read_stream, num_samples);
  }

void
bg_nle_renderer_outstream_audio_seek(bg_nle_renderer_outstream_audio_t * s,
                                     int64_t time)
  {
  bg_nle_audio_compositor_seek(s->ac, time);
  bg_audio_filter_chain_reset(s->fc);
  }

void
bg_nle_renderer_outstream_video_seek(bg_nle_renderer_outstream_video_t * s,
                                     int64_t time)
  {
  bg_nle_video_compositor_seek(s->vc, time);
  bg_video_filter_chain_reset(s->fc);
  }


