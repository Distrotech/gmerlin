/*****************************************************************
 
  vis_scope.c
 
  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <math.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>
#include <log.h>

#define LOG_DOMAIN "vis_scope"

#define BLUR_MODE_GAUSS      0
#define BLUR_MODE_TRIANGULAR 1
#define BLUR_MODE_BOX        2


typedef struct
  {
  gavl_video_format_t video_format;
  gavl_audio_format_t audio_format;
  
  gavl_video_scaler_t * scaler;
  
  float fg_float[3];
  uint32_t fg_int;
  
  gavl_audio_frame_t * audio_frame;

  gavl_video_frame_t * last_video_frame;

  int blur_mode;
  float blur_radius;
  
  int changed;
  float fade_factor;
  
  gavl_video_options_t * scaler_opt;
  
  } scope_priv_t;

static void * create_scope()
  {
  scope_priv_t * ret;
  int flags;
  ret = calloc(1, sizeof(*ret));
  ret->scaler = gavl_video_scaler_create();
  ret->scaler_opt = gavl_video_scaler_get_options(ret->scaler);

  //  gavl_video_options_set_quality(ret->scaler_opt, 3);

  flags = gavl_video_options_get_conversion_flags(ret->scaler_opt);
  flags &= ~GAVL_CONVOLVE_NORMALIZE;
  gavl_video_options_set_conversion_flags(ret->scaler_opt, flags);
  
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
  free(vp);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "fg_color",
      long_name: TRS("Foreground color"),
      type: BG_PARAMETER_COLOR_RGB,
      flags: BG_PARAMETER_SYNC,
      val_default: { val_color: { 1.0, 1.0, 0.0 } },
    },
    {
      name: "blur_mode",
      long_name: TRS("Blur mode"),
      type: BG_PARAMETER_STRINGLIST,
      flags: BG_PARAMETER_SYNC,
      val_default: { val_str: "gauss" },
      multi_names: (char*[]){ "gauss", "triangular", "box", 
                              (char*)0 },
      multi_labels: (char*[]){ TRS("Gauss"), 
                               TRS("Triangular"), 
                               TRS("Rectangular"),
                              (char*)0 },
    },
    {
      name: "blur_radius",
      long_name: TRS("Blur radius"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min: { val_f:  1.0 },
      val_max: { val_f: 50.0 },
      val_default: { val_f:  0.3 },
      num_digits: 1,
    },
    {
      name: "fade_factor",
      long_name: TRS("Fade factor"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      val_min: { val_f:  0.1 },
      val_max: { val_f:  1.0 },
      val_default: { val_f:  0.98 },
      num_digits: 2,
    },
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_scope(void * priv)
  {
  return parameters;
  }

static void
set_parameter_scope(void * priv, const char * name,
                    const bg_parameter_value_t * val)
  {
  uint8_t fg_r, fg_g, fg_b;
  
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;

  if(!name)
    return;
  
  if(!strcmp(name, "fg_color"))
    {
    vp->fg_float[0] = val->val_color[0];
    vp->fg_float[1] = val->val_color[1];
    vp->fg_float[2] = val->val_color[2];

    /* Set foreground color */
    fg_r = (int)(vp->fg_float[0] * 255.0 + 0.5);
    fg_g = (int)(vp->fg_float[1] * 255.0 + 0.5);
    fg_b = (int)(vp->fg_float[2] * 255.0 + 0.5);
    
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
    vp->fg_int = fg_r | (fg_g << 8) | (fg_b << 16) | 0xff000000;
#else
    vp->fg_int = (fg_r<<24) | (fg_g << 16) | (fg_b << 8) | 0x000000ff;
#endif
    
    }
  else if(!strcmp(name, "blur_mode"))
    {
    if(!strcmp(val->val_str, "gauss"))
      vp->blur_mode = BLUR_MODE_GAUSS;
    else if(!strcmp(val->val_str, "triangular"))
      vp->blur_mode = BLUR_MODE_TRIANGULAR;
    else if(!strcmp(val->val_str, "box"))
      vp->blur_mode = BLUR_MODE_BOX;
    vp->changed = 1;
    }
  else if(!strcmp(name, "blur_radius"))
    {
    if(vp->blur_radius != val->val_f)
      {
      vp->blur_radius = val->val_f;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "fade_factor"))
    {
    if(vp->fade_factor != val->val_f)
      {
      vp->fade_factor = val->val_f;
      vp->changed = 1;
      }
    }
  }

static float get_coeff_rectangular(float radius)
  {
  if(radius <= 1.0)
    return radius;
  return 1.0;
  }

static float get_coeff_triangular(float radius)
  {
  if(radius <= 1.0)
    return 1.0 - (1.0-radius)*(1.0-radius);
  return 1.0;
  }

static float get_coeff_gauss(float radius)
  {
  return erf(radius);
  }


static float * get_coeffs(float radius, int * r_i, int mode, float fade_factor)
  {
  float sum;
  float * ret;
  float coeff, last_coeff;
  int i;
  float (*get_coeff)(float);
  if(radius == 0.0)
    return (float*)0;
  switch(mode)
    {
    case BLUR_MODE_GAUSS:
      get_coeff = get_coeff_gauss;
      *r_i = 2*(int)(radius + 0.4999);
      break;
    case BLUR_MODE_TRIANGULAR:
      get_coeff = get_coeff_triangular;
      *r_i = (int)(radius + 0.4999);
      break;
    case BLUR_MODE_BOX:
      get_coeff = get_coeff_rectangular;
      *r_i = (int)(radius + 0.4999);
      break;
    default:
      return (float*)0;
    }
  if(*r_i < 1)
    return (float*)0;
  /* Allocate and set return values */
  ret = malloc(((*r_i * 2) + 1)*sizeof(*ret));

//  ret[*r_i] =   
  last_coeff = 0.0;
  coeff = get_coeff(0.5 / radius);
  ret[*r_i] = 2.0 * coeff;
  
  for(i = 1; i <= *r_i; i++)
    {
    last_coeff = coeff;
    coeff = get_coeff((i) / radius);
    ret[*r_i+i] = coeff - last_coeff;
    ret[*r_i-i] = ret[*r_i+i];
    }

  sum = 0.0;
  for(i = 0; i < (2 * *r_i) + 1; i++)
    sum += ret[i];

  for(i = 0; i < (2 * *r_i) + 1; i++)
    ret[i] *= fade_factor / sum;
  
  return ret;
  }


static int
open_scope(void * priv, bg_ov_plugin_t * ov_plugin, void * ov_priv,
           gavl_audio_format_t * audio_format, gavl_video_format_t * video_format)
  {
  scope_priv_t * vp;
  float * blur_coeffs;
  int num_blur_coeffs = 0;
  
  vp = (scope_priv_t *)priv;

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
  
  if(audio_format->num_channels > 2)
    {
    audio_format->num_channels = 2;
    audio_format->channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(audio_format);
    }
  gavl_video_format_copy(&vp->video_format, video_format);
  gavl_audio_format_copy(&vp->audio_format, audio_format);

  /* Create frames */
  if(vp->audio_frame)
    gavl_audio_frame_destroy(vp->audio_frame);
  if(vp->last_video_frame)
    gavl_video_frame_destroy(vp->last_video_frame);

  vp->audio_frame = gavl_audio_frame_create(&vp->audio_format);
  gavl_audio_frame_mute(vp->audio_frame, &vp->audio_format);

  vp->last_video_frame = gavl_video_frame_create(&vp->video_format);
  gavl_video_frame_clear(vp->last_video_frame, &vp->video_format);
  

  /* Initialize scaler */
  
  blur_coeffs = get_coeffs(vp->blur_radius, &num_blur_coeffs, vp->blur_mode, vp->fade_factor);
  
  gavl_video_scaler_init_convolve(vp->scaler,
                                  &vp->video_format,
                                  num_blur_coeffs, blur_coeffs,
                                  num_blur_coeffs, blur_coeffs);
  if(blur_coeffs) free(blur_coeffs);
  
  return 1;
  }

static void
draw_point(scope_priv_t * vp, gavl_video_frame_t * f, int x, int y)
  {
  uint32_t * ptr;

  ptr = (uint32_t*)(f->planes[0] + y * f->strides[0] + x * 4);
  *ptr = vp->fg_int;
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

static void draw_frame_scope(void * priv, gavl_video_frame_t * frame)
  {
  scope_priv_t * vp;

  vp = (scope_priv_t *)priv;

  gavl_video_frame_clear(frame, &vp->video_format);
  
  gavl_video_scaler_scale(vp->scaler, vp->last_video_frame, frame);
  
  //  draw_line(vp, frame, 0, vp->video_format.image_height/2,
  //            (vp->video_format.image_width - 1), vp->video_format.image_height/2);
  
#if 1
  if(vp->audio_format.num_channels == 1)
    {
    draw_scope(vp,
               vp->video_format.image_height / 2,
               vp->video_format.image_height / 2,
               vp->audio_frame->channels.f[0],
               frame);
    }
  else
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
#endif
  gavl_video_frame_copy(&vp->video_format, vp->last_video_frame, frame);
  
  //  gavl_video_frame_fill(frame, &vp->video_format, color);
  }


static void update_scope(void * priv, gavl_audio_frame_t * frame)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;
  gavl_audio_frame_copy(&vp->audio_format, vp->audio_frame, frame,
                        0, 0, vp->audio_format.samples_per_frame,
                        frame->valid_samples);
  }



static void close_scope(void * priv)
  {
  scope_priv_t * vp;
  vp = (scope_priv_t *)priv;
  
  }

bg_visualization_plugin_t the_plugin = 
  {
    common:
    {
      BG_LOCALE,
      name:      "vis_scope",
      long_name: TRS("Scope"),
      description: TRS("Scope plugin"),
      type:     BG_PLUGIN_VISUALIZATION,
      flags:    BG_PLUGIN_VISUALIZE_FRAME,
      create:   create_scope,
      destroy:   destroy_scope,
      get_parameters:   get_parameters_scope,
      set_parameter:    set_parameter_scope,
      priority:         1,
    },
    open: open_scope,
    update: update_scope,
    draw_frame: draw_frame_scope,
    close: close_scope
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
