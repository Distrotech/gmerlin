/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <string.h>
#include <math.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#include <gmerlin/bggavl.h>

#include "../videofilters/colormatrix.h"

#define LOG_DOMAIN "vis_scope"

typedef struct scope_priv_s
  {
  int threads;
  
  gavl_video_format_t video_format;
  gavl_audio_format_t audio_format;
  
  float fg_float[3];

  uint32_t fg_int;
  
  gavl_audio_frame_t * audio_frame;

  gavl_video_frame_t * last_video_frame;
  gavl_video_frame_t * tmp_video_frame;

  bg_thread_pool_t * thread_pool;
  
  /* Damn simple beat detection */
  float energy;
  int beat;

  /* Foreground drawing function */
  void (*draw_fg)(struct scope_priv_s *, gavl_video_frame_t *);
  int fg_index;

  /* Transforms */
  gavl_image_transform_t * transform_sin;
  gavl_image_transform_t * transform_cos;
  gavl_image_transform_t * transform_rotate_left;
  gavl_image_transform_t * transform_rotate_right;
  gavl_image_transform_t * transform_lens;
  gavl_image_transform_t * transform_ripple;

  gavl_image_transform_t * transform_zoom_in;
  gavl_image_transform_t * transform_zoom_out;

  gavl_image_transform_t * transform_current;
  int transform_index;
  
  double sin_rotate;
  double cos_rotate;

  float half_width;
  float half_height;
  
  /* Colormatrix */
  bg_colormatrix_t * colormatrix;

  int colormatrix_index;
  
  gavl_video_options_t * opt;

  int fg_color_index;
  int bg_color_index;

  gavl_video_scaler_t * scaler;
  
  } scope_priv_t;

/* Random functions */

//static float scope_random(float min, float max);

int scope_random_int(int min, int max);

/* return 1 with a probability of p */

int scope_decide(float p);

/* Foreground drawing functions */

static void draw_fg_scope(scope_priv_t * vp, gavl_video_frame_t * frame);
static void draw_fg_vectorscope(scope_priv_t * vp, gavl_video_frame_t * frame);
static void draw_fg_vectorscope_dots(scope_priv_t * vp, gavl_video_frame_t * frame);
     
static const struct
  {
  void (*func)(struct scope_priv_s *, gavl_video_frame_t *);
  }
fg_funcs[] =
  {
    { draw_fg_scope },
    { draw_fg_vectorscope },
    { draw_fg_vectorscope_dots },
  };

static const int num_fg_funcs =
sizeof(fg_funcs) / sizeof(fg_funcs[0]);

/* Transformations */

static void
transform_sin(void *priv, double xdst, double ydst, double *xsrc, double *ysrc);
static void
transform_cos(void *priv, double xdst, double ydst, double *xsrc, double *ysrc);
static void
transform_rotate_left(void *priv, double xdst, double ydst, double *xsrc, double *ysrc);
static void
transform_rotate_right(void *priv, double xdst, double ydst, double *xsrc, double *ysrc);
static void
transform_lens(void *priv, double xdst, double ydst, double *xsrc, double *ysrc);
static void
transform_ripple(void *priv, double xdst, double ydst, double *xsrc, double *ysrc);

static void
transform_zoom_in(void *priv, double xdst, double ydst, double *xsrc, double *ysrc);
static void
transform_zoom_out(void *priv, double xdst, double ydst, double *xsrc, double *ysrc);

/* */

static void * create_scope()
  {
  scope_priv_t * ret;
  int flags;
  ret = calloc(1, sizeof(*ret));
  
  ret->opt = gavl_video_options_create();
  gavl_video_options_set_scale_mode(ret->opt, GAVL_SCALE_BILINEAR);
  gavl_video_options_set_quality(ret->opt, GAVL_QUALITY_FASTEST);
  
  flags = gavl_video_options_get_conversion_flags(ret->opt);
  flags &= ~GAVL_CONVOLVE_NORMALIZE;
  gavl_video_options_set_conversion_flags(ret->opt, flags);

  
  //  gavl_video_options_set_quality(ret->scaler_opt, 3);
  
  //  ret->draw_fg = draw_fg_scope;
  //  ret->draw_fg = draw_fg_vectorscope;
  ret->draw_fg = draw_fg_vectorscope_dots;
  
  ret->transform_sin = gavl_image_transform_create();
  ret->transform_cos = gavl_image_transform_create();
  ret->transform_rotate_left = gavl_image_transform_create();
  ret->transform_rotate_right = gavl_image_transform_create();
  ret->transform_lens = gavl_image_transform_create();
  ret->transform_zoom_in = gavl_image_transform_create();
  ret->transform_zoom_out = gavl_image_transform_create();
  ret->transform_ripple = gavl_image_transform_create();

  ret->scaler = gavl_video_scaler_create();
  
  ret->colormatrix = bg_colormatrix_create();
  
  ret->sin_rotate = sin(0.05);
  ret->cos_rotate = cos(0.05);


  return ret;
  }

static void destroy_scope(void * priv)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;

  if(vp->audio_frame)
    gavl_audio_frame_destroy(vp->audio_frame);
  if(vp->last_video_frame)
    gavl_video_frame_destroy(vp->last_video_frame);
  if(vp->tmp_video_frame)
    gavl_video_frame_destroy(vp->tmp_video_frame);

  gavl_image_transform_destroy(vp->transform_sin);
  gavl_image_transform_destroy(vp->transform_cos);
  gavl_image_transform_destroy(vp->transform_rotate_left);
  gavl_image_transform_destroy(vp->transform_rotate_right);
  gavl_image_transform_destroy(vp->transform_lens);
  gavl_image_transform_destroy(vp->transform_ripple);
  gavl_image_transform_destroy(vp->transform_zoom_in);
  gavl_image_transform_destroy(vp->transform_zoom_out);

  bg_colormatrix_destroy(vp->colormatrix);

  gavl_video_scaler_destroy(vp->scaler);
  
  gavl_video_options_destroy(vp->opt);
  
  if(vp->thread_pool)
    bg_thread_pool_destroy(vp->thread_pool);
  
  free(vp);
  }

static const float blur_coeffs[] = { 0.05, 0.91, 0.05 };

static int
open_scope(void * priv, gavl_audio_format_t * audio_format,
           gavl_video_format_t * video_format)
  {
  scope_priv_t * vp;
  
  vp = (scope_priv_t *)priv;

  /* Create thread pool */

  if(!vp->thread_pool)
    {
    vp->thread_pool = bg_thread_pool_create(vp->threads);
    gavl_video_options_set_num_threads(vp->opt, vp->threads);
    gavl_video_options_set_run_func(vp->opt, bg_thread_pool_run, vp->thread_pool);
    gavl_video_options_set_stop_func(vp->opt, bg_thread_pool_stop, vp->thread_pool);
    }
  
  /* Adjust video format */
  
  if(video_format->image_width < 16)
    video_format->image_width = 16;
  if(video_format->image_height < 16)
    video_format->image_height = 16;

  if(video_format->frame_width < 16)
    video_format->frame_width = 16;
  if(video_format->frame_height < 16)
    video_format->frame_height = 16;
  
  video_format->pixel_width  = 1;
  video_format->pixel_height  = 1;
  video_format->pixelformat = GAVL_RGB_32;
  
  /* Adjust audio format */
  
  audio_format->sample_format = GAVL_SAMPLE_FLOAT;
  audio_format->interleave_mode = GAVL_INTERLEAVE_NONE;
  audio_format->samples_per_frame = video_format->image_width;

  /* Force stereo */
  audio_format->num_channels = 2;
  audio_format->channel_locations[0] = GAVL_CHID_NONE;
  gavl_set_channel_setup(audio_format);
  
  gavl_video_format_copy(&vp->video_format, video_format);
  gavl_audio_format_copy(&vp->audio_format, audio_format);

  /* Create frames */
  if(vp->audio_frame)
    gavl_audio_frame_destroy(vp->audio_frame);
  if(vp->last_video_frame)
    gavl_video_frame_destroy(vp->last_video_frame);
  if(vp->tmp_video_frame)
    gavl_video_frame_destroy(vp->tmp_video_frame);

  vp->audio_frame = gavl_audio_frame_create(&vp->audio_format);
  gavl_audio_frame_mute(vp->audio_frame, &vp->audio_format);

  vp->last_video_frame = gavl_video_frame_create(&vp->video_format);
  gavl_video_frame_clear(vp->last_video_frame, &vp->video_format);

  vp->tmp_video_frame = gavl_video_frame_create(&vp->video_format);
  gavl_video_frame_clear(vp->tmp_video_frame, &vp->video_format);

  /* Apply options */
  gavl_video_options_copy(gavl_image_transform_get_options(vp->transform_sin),
                          vp->opt);
  gavl_video_options_copy(gavl_image_transform_get_options(vp->transform_cos),
                          vp->opt);
  gavl_video_options_copy(gavl_image_transform_get_options(vp->transform_rotate_left),
                          vp->opt);
  gavl_video_options_copy(gavl_image_transform_get_options(vp->transform_rotate_right),
                          vp->opt);
  gavl_video_options_copy(gavl_image_transform_get_options(vp->transform_lens),
                          vp->opt);
  gavl_video_options_copy(gavl_image_transform_get_options(vp->transform_ripple),
                          vp->opt);
  gavl_video_options_copy(gavl_image_transform_get_options(vp->transform_zoom_in),
                          vp->opt);
  gavl_video_options_copy(gavl_image_transform_get_options(vp->transform_zoom_out),
                          vp->opt);
  gavl_video_options_copy(gavl_video_scaler_get_options(vp->scaler),
                          vp->opt);
  
  /* Init transformations */
  vp->half_width  = (float)vp->video_format.image_width*0.5;
  vp->half_height = (float)vp->video_format.image_height*0.5;
  gavl_image_transform_init(vp->transform_sin,
                            video_format, transform_sin, vp);
  gavl_image_transform_init(vp->transform_cos,
                            video_format, transform_cos, vp);

  gavl_image_transform_init(vp->transform_rotate_left,
                            video_format, transform_rotate_left, vp);

  gavl_image_transform_init(vp->transform_rotate_right,
                            video_format, transform_rotate_right, vp);

  gavl_image_transform_init(vp->transform_zoom_in,
                            video_format, transform_zoom_in, vp);
  gavl_image_transform_init(vp->transform_zoom_out,
                            video_format, transform_zoom_out, vp);
  gavl_image_transform_init(vp->transform_lens,
                            video_format, transform_lens, vp);
  gavl_image_transform_init(vp->transform_ripple,
                            video_format, transform_ripple, vp);

  gavl_video_scaler_init_convolve(vp->scaler,
                                  video_format,
                                  1, blur_coeffs,
                                  1, blur_coeffs);
  
  /* Init colormatrix */
  
  bg_colormatrix_init(vp->colormatrix,
                      video_format, 0,
                      vp->opt);
  vp->colormatrix_index = -1;
  vp->fg_color_index = -1;
  vp->bg_color_index = -1;
  
  return 1;
  }

static void set_fg_func(scope_priv_t * vp)
  {
  int index;
  index = scope_random_int(0, num_fg_funcs - 2);
  if(index >= vp->fg_index)
    index++;
  vp->fg_index = index;
  vp->draw_fg = fg_funcs[vp->fg_index].func;
  }

static void set_transform(scope_priv_t * vp)
  {
  int index;
  index = scope_random_int(0, 4);
  if(index >= vp->transform_index)
    index++;
  
  vp->transform_index = index;

  switch(vp->transform_index)
    {
    case 0:
      if(scope_decide(0.5))
        vp->transform_current = vp->transform_sin;
      else
        vp->transform_current = vp->transform_cos;
      break;
    case 1:
      if(scope_decide(0.5))
        vp->transform_current = vp->transform_rotate_left;
      else
        vp->transform_current = vp->transform_rotate_right;
      break;
    case 2:
      vp->transform_current = vp->transform_lens;
      break;
    case 3:
      vp->transform_current = vp->transform_zoom_out;
      break;
    case 4:
      vp->transform_current = vp->transform_zoom_in;
      break;
    case 5:
      vp->transform_current = vp->transform_ripple;
      break;
    }
  }

static struct
  {
  float mat[3][4];
  }
colormatrices[] =
  {
#if 0
    {
      {
        { 0.98, 0.00, 0.0,  0.0 },
        { 0.09, 0.89, 0.0,  0.0 },
        { 0.09, 0.00, 0.89, 0.0 },
      },
    },
    {
      {
        { 0.89, 0.09, 0.0,  0.0 },
        { 0.0,  0.98, 0.0,  0.0 },
        { 0.0,  0.09, 0.89, 0.0 },
      },
    },
    {
      {
        { 0.89, 0.00, 0.09,  0.0 },
        { 0.0,  0.89, 0.09,  0.0 },
        { 0.0,  0.00, 0.98, 0.0 },
      },
    },
#endif
    {
      {
        { 0.95, 0.0, 0.0, 0.0 },
        { 0.0,  0.99, 0.0, 0.0 },
        { 0.0,  0.0,  0.99, 0.0 },
      },
    },
    {
      {
        { 0.99, 0.0,  0.0, 0.0 },
        { 0.0, 0.95, 0.0, 0.0 },
        { 0.0, 0.0,  0.99, 0.0 },
      },
    },
    {
      {
        { 0.99, 0.0,  0.0, 0.0 },
        { 0.0, 0.99,  0.0, 0.0 },
        { 0.0, 0.0,  0.95, 0.0 },
      },
    },
    {
      {
        { 0.95, 0.0, 0.0, 0.0 },
        { 0.0,  0.95, 0.0, 0.0 },
        { 0.0,  0.0, 0.99, 0.0 },
      },
    },
    {
      {
        { 0.95, 0.0, 0.0, 0.0 },
        { 0.0,  0.99, 0.0, 0.0 },
        { 0.0,  0.0, 0.95, 0.0 },
      },
    },
    {
      {
        { 0.99, 0.0, 0.0, 0.0 },
        { 0.0, 0.95, 0.0, 0.0 },
        { 0.0, 0.0, 0.95, 0.0 },
      },
    },
  };

static const int num_colormatrices =
sizeof(colormatrices)/sizeof(colormatrices[0]);

static void set_colormatrix(scope_priv_t * vp)
  {
  int index;
  index = scope_random_int(0, num_colormatrices-2);
  if(index >= vp->colormatrix_index)
    index++;
  
  vp->colormatrix_index = index;

  bg_colormatrix_set_rgb(vp->colormatrix, colormatrices[index].mat);
  }

static const struct
  {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  }
fg_colors[] =
  {
#if 0
    { 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00 },
    { 0x00, 0x00, 0xff },
    { 0xff, 0x00, 0xff },
    { 0x00, 0xff, 0xff },
#endif
    { 0xff, 0x80, 0x80 },
    { 0x80, 0xff, 0x80 },
    { 0xff, 0xff, 0x80 },
    { 0x80, 0x80, 0xff },
    { 0xff, 0x80, 0xff },
    { 0x80, 0xff, 0xff },
    { 0xff, 0xff, 0xff },
  };

static const int num_fg_colors = sizeof(fg_colors)/sizeof(fg_colors[0]);

static void set_fg_color(scope_priv_t * vp)
  {
  int index;
  index = scope_random_int(0, num_fg_colors-2);
  if(index >= vp->fg_color_index)
    index++;

  vp->fg_color_index = index;
  
#ifndef WORDS_BIGENDIAN
  vp->fg_int = fg_colors[index].r |
    (fg_colors[index].g << 8) |
    (fg_colors[index].b << 16) | 0xff000000;
#else
  vp->fg_int = (fg_colors[index].r<<24) |
    (fg_colors[index].g << 16) |
    (fg_colors[index].b << 8) | 0x000000ff;
#endif
  }

static struct
  {
  const float c[4];
  }
bg_colors[] =
  {
    {
      { 0.5, 0.0, 0.0, 1.0 },
    },
    //    {
    //      { 0.0, 0.5, 0.0, 1.0 },
    //    },
    //    {
    //      { 0.5, 0.5, 0.0, 1.0 },
    //    },
    {
      { 0.0, 0.0, 0.5, 1.0 },
    },
    {
      { 0.5, 0.0, 0.5, 1.0 },
    },
    {
      { 0.0, 0.5, 0.5, 1.0 },
    },
    {
      { 0.0, 0.0, 0.0, 1.0 },
    },
    {
      { 0.0, 0.0, 0.0, 1.0 },
    },
    {
      { 0.0, 0.0, 0.0, 1.0 },
    },
    {
      { 0.0, 0.0, 0.0, 1.0 },
    },
  };

static const int num_bg_colors = sizeof(bg_colors)/sizeof(bg_colors[0]);

static void set_bg_color(scope_priv_t * vp)
  {
  int index;
  index = scope_random_int(0, num_bg_colors-2);
  if(index >= vp->bg_color_index)
    index++;

  vp->bg_color_index = index;
  }

static void draw_frame_scope(void * priv, gavl_video_frame_t * frame)
  {
  scope_priv_t * vp;

  vp = (scope_priv_t *)priv;

  /* Set foreground */
  
  if((vp->beat && scope_decide(0.05)) || !vp->draw_fg)
    set_fg_func(vp);

  if((vp->beat && scope_decide(0.05)) || !vp->transform_current)
    set_transform(vp);

  if(((vp->beat && scope_decide(0.3)) || (vp->fg_color_index < 0)))
    set_fg_color(vp);

  if(((vp->beat && scope_decide(0.3)) || (vp->bg_color_index < 0)))
    set_bg_color(vp);
  
  if((vp->beat && scope_decide(0.3)))
    set_fg_color(vp);

  if((vp->beat && scope_decide(0.1)) || (vp->colormatrix_index < 0))
    set_colormatrix(vp);
  
  //  gavl_video_frame_clear(frame, &vp->video_format);

  /* Blur */
  //  gavl_video_scaler_scale(vp->scaler, vp->last_video_frame, vp->tmp_video_frame);

  /* Transform */

  gavl_video_frame_fill(vp->tmp_video_frame, &vp->video_format,
                        bg_colors[vp->bg_color_index].c);
  
  gavl_image_transform_transform(vp->transform_current,
                                 vp->last_video_frame, vp->tmp_video_frame);
  
  
  //  gavl_image_transform_transform(vp->transform_ripple,
  // vp->last_video_frame, vp->tmp_video_frame);

  gavl_video_scaler_scale(vp->scaler, vp->tmp_video_frame, frame);
  
  bg_colormatrix_process(vp->colormatrix, frame);

  
  vp->draw_fg(vp, frame);
  
  gavl_video_frame_copy(&vp->video_format, vp->last_video_frame, frame);
  
  //  gavl_video_frame_fill(frame, &vp->video_format, color);
  }


static void update_scope(void * priv, gavl_audio_frame_t * frame)
  {
  int i, j;
  float new_energy;
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;
  gavl_audio_frame_copy(&vp->audio_format, vp->audio_frame, frame,
                        0, 0, vp->audio_format.samples_per_frame,
                        frame->valid_samples);

  /* The probably most simple beat detection */
    
  new_energy = 0.0;

  for(i = 0; i < vp->audio_format.num_channels; i++)
    {
    for(j = 0; j < frame->valid_samples; j++)
      new_energy += frame->channels.f[i][j] * frame->channels.f[i][j];
    }

  new_energy /= (frame->valid_samples * vp->audio_format.num_channels);

  if((new_energy / vp->energy > 2.0) && !vp->beat && (new_energy > 0.001))
    vp->beat = 1;
  else
    vp->beat = 0;

  vp->energy = vp->energy * 0.95 + new_energy * 0.05;
  }

static void close_scope(void * priv)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;
  
  }

static const bg_parameter_info_t parameters[] =
  {
    BG_GAVL_PARAM_THREADS,
    { /* End */ },
  };

static void set_parameter_scope(void * priv, const char * name,
                                const bg_parameter_value_t * val)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;

  if(!name)
    return;
  
  if(!strcmp(name, "threads"))
    vp->threads = val->val_i;
  }

static const bg_parameter_info_t * get_parameters_scope(void * priv)
  {
  return parameters;
  }

const bg_visualization_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "vis_scope",
      .long_name = TRS("Scope"),
      .description = TRS("Scope plugin"),
      .type =     BG_PLUGIN_VISUALIZATION,
      .flags =    BG_PLUGIN_VISUALIZE_FRAME,
      .create =   create_scope,
      .destroy =   destroy_scope,
      .priority =         1,
      .get_parameters = get_parameters_scope,
      .set_parameter = set_parameter_scope,
    },
    .open_ov = open_scope,
    .update = update_scope,
    .draw_frame = draw_frame_scope,
    .close = close_scope
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;

/* Random functions */

#if 0
static float scope_random(float min, float max)
  {
  return min + (float)rand()*(max - min)/(float)(RAND_MAX);
  }
#endif

int scope_random_int(int min, int max)
  {
  int den = max - min + 1;
  return (rand() % den) + min;
  }

/* return 1 with a probability of p */

int scope_decide(float p)
  {
  float rand_f = (float)(rand())/(float)(RAND_MAX) ;
  return rand_f < p ? 1 : 0;
  }

/* Drawing functions */

static void
draw_point(scope_priv_t * vp, gavl_video_frame_t * f, int x, int y)
  {
  uint32_t * ptr;

  ptr = (uint32_t*)(f->planes[0] + y * f->strides[0] + x * 4);
  *ptr = vp->fg_int;
  }

static void
draw_point_large(scope_priv_t * vp, gavl_video_frame_t * f, int x, int y)
  {
  draw_point(vp, f, x, y);

  if(x > 0)
    draw_point(vp, f, x-1, y);
  if(y > 0)
    draw_point(vp, f, x, y-1);

  if(x < vp->video_format.image_width - 1)
    draw_point(vp, f, x+1, y);
  if(y < vp->video_format.image_height - 1)
    draw_point(vp, f, x, y+1);
  }

static void
draw_line(scope_priv_t * vp, gavl_video_frame_t * f,
          int x1, int y1, int x2, int y2)
  {
  int i;
  int abs_x, abs_y, x, y;
  int sign;

  abs_x = abs(x2 - x1);
  abs_y = abs(y2 - y1);

  if(abs_x == 0 && abs_y == 0)
    {
    draw_point(vp, f, x1, y1);
    return;
    }
  
  if(abs_x > abs_y)
    {
    /* X-loop */
    if(x1 > x2)
      sign = -1;
    else
      sign = 1;
    
    for(i = 0; i <= abs_x; i++)
      {
      x = x1 + sign * i;
      y = y1 + (i * (y2 - y1)) / abs_x;
      draw_point(vp, f, x, y);
      }
    }
  else
    {
    /* Y-loop */
    if(y1 > y2)
      sign = -1;
    else
      sign = 1;

    for(i = 0; i <= abs_y; i++)
      {
      y = y1 + sign * i;
      x = x1 + (i * (x2 - x1)) / abs_y;
      draw_point(vp, f, x, y);
      }
    }
  }

#define CLAMP(x,min,max) x = (x>max) ? (max) : ((x < (min)) ? (min) : x)

static void draw_scope(scope_priv_t * vp, int y_off, int y_ampl, float * samples,
                       gavl_video_frame_t * f)
  {
  int i, x1, y1, x2, y2;

  x1 = 0;
  y1 = y_off + (int)(samples[0] * y_ampl + 0.5);
  
  CLAMP(y1, 0, vp->video_format.image_height - 1);
  
  for(i = 1; i < vp->audio_format.samples_per_frame; i++)
    {
    x2 = (i * vp->video_format.image_width) / vp->audio_format.samples_per_frame;
    y2 = y_off + (int)(samples[i] * y_ampl + 0.5);

    CLAMP(x2, 0, vp->video_format.image_width  - 1);
    CLAMP(y2, 0, vp->video_format.image_height - 1);
    
    draw_line(vp, f, x1, y1, x2, y2);
    x1 = x2;
    y1 = y2;
    }
  }

static void draw_fg_scope(scope_priv_t * vp, gavl_video_frame_t * frame)
  {
  draw_scope(vp,
             vp->video_format.image_height / 4,
             vp->video_format.image_height / 4,
             vp->audio_frame->channels.f[0],
             frame);
  draw_scope(vp,
             (3 * vp->video_format.image_height) / 4,
             vp->video_format.image_height / 4,
             vp->audio_frame->channels.f[1],
             frame);
  }

/* Vectorscope */

#define SQRT_2 0.707106871

#define GET_COORDS_VECTORSCOPE(index, dstx, dsty) \
  tmpx = SQRT_2*(vp->audio_frame->channels.f[0][index]-vp->audio_frame->channels.f[1][index])*vp->half_width/vp->half_height;\
  tmpy = SQRT_2*(vp->audio_frame->channels.f[0][index]+vp->audio_frame->channels.f[1][index]);\
  dstx = (int)((tmpx + 1.0) * vp->half_width);\
  dsty = (int)((tmpy + 1.0) * vp->half_height);\
  CLAMP(dstx, 0, vp->video_format.image_width  - 1);\
  CLAMP(dsty, 0, vp->video_format.image_height - 1);

static void draw_fg_vectorscope(scope_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;

  float tmpx, tmpy;
  
  int x1, y1, x2, y2;
  
  GET_COORDS_VECTORSCOPE(0, x1, y1);
  
  for(i = 1; i < vp->audio_frame->valid_samples; i++)
    {
    GET_COORDS_VECTORSCOPE(i, x2, y2);
    draw_line(vp, frame, x1, y1, x2, y2);
    
    x1 = x2;
    y1 = y2;
    }
  }

static void draw_fg_vectorscope_dots(scope_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;

  float tmpx, tmpy;
  
  int x, y;
  
  for(i = 0; i < vp->audio_frame->valid_samples; i++)
    {
    GET_COORDS_VECTORSCOPE(i, x, y);
    draw_point_large(vp, frame, x, y);
    }
  }

static void transform_sin(void *priv, double xdst, double ydst,
                          double *xsrc, double *ysrc)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;
  
  *xsrc = xdst;
  *ysrc = ydst + (5.0 * vp->half_height / 120.0) * 
    sin(*xsrc / vp->video_format.image_width * 4.0 * M_PI);
  }

static void transform_cos(void *priv, double xdst, double ydst, double *xsrc, double *ysrc)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;
  
  *xsrc = xdst;
  *ysrc = ydst + (5.0 * vp->half_height / 120.0) *
    cos(*xsrc / vp->video_format.image_width * 4.0 * M_PI);
  }

static void
transform_rotate_left(void *priv, double xdst, double ydst,
                      double *xsrc, double *ysrc)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;

  xdst -= vp->half_width;
  ydst -= vp->half_height;
  
  *xsrc = xdst * vp->cos_rotate - ydst * vp->sin_rotate;
  *ysrc = xdst * vp->sin_rotate + ydst * vp->cos_rotate;

  *xsrc += vp->half_width;
  *ysrc += vp->half_height;
  }

static void
transform_rotate_right(void *priv, double xdst, double ydst,
                       double *xsrc, double *ysrc)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;

  xdst -= vp->half_width;
  ydst -= vp->half_height;
  
  *xsrc = xdst * vp->cos_rotate + ydst * vp->sin_rotate;
  *ysrc = -xdst * vp->sin_rotate + ydst * vp->cos_rotate;

  *xsrc += vp->half_width;
  *ysrc += vp->half_height;
  }

static void transform_zoom_in(void * priv,
                              double xdst,
                              double ydst,
                              double *xsrc,
                              double *ysrc)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;

  xdst -= vp->half_width;
  ydst -= vp->half_height;
  
  *xsrc = xdst * 1.1;
  *ysrc = ydst * 1.1;
  
  *xsrc += vp->half_width;
  *ysrc += vp->half_height;
  }

static void transform_zoom_out(void * priv,
                               double xdst,
                               double ydst,
                               double *xsrc,
                               double *ysrc)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;

  xdst -= vp->half_width;
  ydst -= vp->half_height;
  
  *xsrc = xdst * 0.9;
  *ysrc = ydst * 0.9;
  
  *xsrc += vp->half_width;
  *ysrc += vp->half_height;
  }


static void transform_lens(void * priv,
                           double x,
                           double y,
                           double *newx,
                           double *newy)
  {
#if 1
  double distance;
  double shift;
  double x1, y1;

  double lens_zoom = 1.5;
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;
  
  x1 = x - vp->half_width;
  y1 = y - vp->half_height;
  
  distance = (x1 * x1 + y1 * y1 - vp->half_height * vp->half_height) / (vp->half_height * vp->half_height);
  
  if(distance > 0)
    {
    *newx = (x - vp->half_width) * 1.01 + vp->half_width;
    *newy = (y - vp->half_height) * 1.01 + vp->half_height;

    //    *newx = x;
    //    *newy = y;

    return;
    }
  else
    {
    shift = lens_zoom /
      sqrt(lens_zoom * lens_zoom - distance);
    
    *newx = vp->half_width  + x1 * shift;
    *newy = vp->half_height + y1 * shift;
    }
#else
  static int printed = 0;
  if(!printed)
    {
    fprintf(stderr, "Transform: %f %f\n", x, y);
    printed = 1;
    }
  *newx = x;
  *newy = y;
#endif
  }

static void transform_ripple(void * priv,
                             double x,
                             double y,
                             double *newx,
                             double *newy)
  {
  float distance;
  float phi;
  float shift;
  scope_priv_t * vp;
  double x1, y1;
  vp = (scope_priv_t *)priv;
  x1 = x - vp->half_width;
  y1 = y - vp->half_height;
  
  distance = 10.0 * hypot(x1, y1) / vp->half_height;

  phi = atan2(y1, x1);
  
  shift = 0.01 * cos(distance * distance * 0.1) * vp->half_height;
  //  shift = 0;
  
  *newx = vp->half_width  + 1.01 * (x1 + shift * cos(phi));
  *newy = vp->half_height + 1.01 * (y1 + shift * sin(phi));
  }
