/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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


#include <config.h>

#include <string.h>
#include <stdio.h>

#include <gavl/gavl.h>

#include <gmerlin/parameter.h>
#include <gmerlin/bggavl.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "bggavl"

#define SP_INT(s) if(!strcmp(name, # s)) \
    { \
    opt->s = val->val_i;                     \
    return 1; \
    }

#define SP_FLOAT(s) if(!strcmp(name, # s))      \
    { \
    opt->s = val->val_f;                     \
    return 1; \
    }

/* Audio stuff */

int bg_gavl_audio_set_parameter(void * data, const char * name, const bg_parameter_value_t * val)
  {
  bg_gavl_audio_options_t * opt = (bg_gavl_audio_options_t *)data;
  if(!name)
    return 1;
  if(!strcmp(name, "conversion_quality"))
    {
    if(val->val_i != gavl_audio_options_get_quality(opt->opt))
      opt->options_changed = 1;
    gavl_audio_options_set_quality(opt->opt, val->val_i);
    return 1;
    }
  SP_INT(fixed_samplerate);

  if(!strcmp(name, "sampleformat"))
    {
    gavl_sample_format_t force_format = GAVL_SAMPLE_NONE;
    if(!strcmp(val->val_str, "8"))
      force_format = GAVL_SAMPLE_S8;
    else if(!strcmp(val->val_str, "16"))
      force_format = GAVL_SAMPLE_S16;
    else if(!strcmp(val->val_str, "32"))
      force_format = GAVL_SAMPLE_S32;
    else if(!strcmp(val->val_str, "f"))
      force_format = GAVL_SAMPLE_FLOAT;
    else if(!strcmp(val->val_str, "d"))
      force_format = GAVL_SAMPLE_DOUBLE;
    if(force_format != opt->force_format)
      {
      opt->force_format = force_format;
      opt->options_changed = 1;
      }
    return 1;
    }
  
  SP_INT(samplerate);
  SP_INT(fixed_channel_setup);
  SP_INT(num_front_channels);
  SP_INT(num_rear_channels);
  SP_INT(num_lfe_channels);
  
  if(!strcmp(name, "front_to_rear"))
    {
    int old_flags, new_flags;
    if(!val->val_str)
      return 1;
    old_flags = gavl_audio_options_get_conversion_flags(opt->opt);
    new_flags = old_flags & ~GAVL_AUDIO_FRONT_TO_REAR_MASK;
    
    if(!strcmp(val->val_str, "copy"))
      new_flags |= GAVL_AUDIO_FRONT_TO_REAR_COPY;
    else if(!strcmp(val->val_str, "mute"))
      new_flags |= GAVL_AUDIO_FRONT_TO_REAR_MUTE;
    else if(!strcmp(val->val_str, "diff"))
      new_flags |= GAVL_AUDIO_FRONT_TO_REAR_DIFF;
    
    if(old_flags != new_flags)
      opt->options_changed = 1;
    
    gavl_audio_options_set_conversion_flags(opt->opt, new_flags);
    
    return 1;
    }

  else if(!strcmp(name, "stereo_to_mono"))
    {
    int old_flags, new_flags = GAVL_AUDIO_STEREO_TO_MONO_MIX;
    if(!val->val_str)
      return 1;
    old_flags = gavl_audio_options_get_conversion_flags(opt->opt);
    new_flags = (old_flags & ~GAVL_AUDIO_STEREO_TO_MONO_MASK);
    
    if(!strcmp(val->val_str, "left"))
      new_flags |= GAVL_AUDIO_STEREO_TO_MONO_LEFT;
    else if(!strcmp(val->val_str, "right"))
      new_flags |= GAVL_AUDIO_STEREO_TO_MONO_RIGHT;
    else if(!strcmp(val->val_str, "mix"))
      new_flags |= GAVL_AUDIO_STEREO_TO_MONO_MIX;

    if(old_flags |= new_flags)
      opt->options_changed = 1;
    
    gavl_audio_options_set_conversion_flags(opt->opt, new_flags);
    return 1;
    }

  else if(!strcmp(name, "dither_mode"))
    {
    gavl_audio_dither_mode_t dither_mode = GAVL_AUDIO_DITHER_AUTO;
    if(!strcmp(val->val_str, "auto"))
      {
      dither_mode = GAVL_AUDIO_DITHER_AUTO;
      }
    else if(!strcmp(val->val_str, "none"))
      {
      dither_mode = GAVL_AUDIO_DITHER_NONE;
      }
    else if(!strcmp(val->val_str, "rect"))
      {
      dither_mode = GAVL_AUDIO_DITHER_RECT;
      }
    else if(!strcmp(val->val_str, "shaped"))
      {
      dither_mode = GAVL_AUDIO_DITHER_SHAPED;
      }

    if(dither_mode != gavl_audio_options_get_dither_mode(opt->opt))
      opt->options_changed = 1;
      
      gavl_audio_options_set_dither_mode(opt->opt, dither_mode);

    return 1;
    }

  
  else if(!strcmp(name, "resample_mode"))
    {
    gavl_resample_mode_t resample_mode = GAVL_RESAMPLE_AUTO;
    if(!strcmp(val->val_str, "auto"))
      gavl_audio_options_set_resample_mode(opt->opt, GAVL_RESAMPLE_AUTO);

    else if(!strcmp(val->val_str, "linear"))
      resample_mode = GAVL_RESAMPLE_LINEAR;
    else if(!strcmp(val->val_str, "zoh"))
      resample_mode = GAVL_RESAMPLE_ZOH;
    else if(!strcmp(val->val_str, "sinc_fast"))
      resample_mode = GAVL_RESAMPLE_SINC_FAST;
    else if(!strcmp(val->val_str, "sinc_medium"))
      resample_mode = GAVL_RESAMPLE_SINC_MEDIUM;
    else if(!strcmp(val->val_str, "sinc_best"))
      resample_mode = GAVL_RESAMPLE_SINC_BEST;
    
    if(resample_mode != gavl_audio_options_get_resample_mode(opt->opt))
      opt->options_changed = 1;
    
    gavl_audio_options_set_resample_mode(opt->opt, resample_mode);
    
    return 1;
    }

  
  return 0;
  }

void bg_gavl_audio_options_init(bg_gavl_audio_options_t *opt)
  {
  opt->opt = gavl_audio_options_create();
  }

void bg_gavl_audio_options_free(bg_gavl_audio_options_t * opt)
  {
  if(opt->opt)
    gavl_audio_options_destroy(opt->opt);
  }


void bg_gavl_audio_options_set_format(const bg_gavl_audio_options_t * opt,
                                      const gavl_audio_format_t * in_format,
                                      gavl_audio_format_t * out_format)
  {
  int channel_index;

  if(in_format)
    gavl_audio_format_copy(out_format, in_format);

  if(opt->fixed_samplerate || !in_format)
    {
    out_format->samplerate = opt->samplerate;
    }
  if(opt->fixed_channel_setup || !in_format)
    {
    out_format->num_channels =
      opt->num_front_channels + opt->num_rear_channels + opt->num_lfe_channels;
    
    channel_index = 0;
    switch(opt->num_front_channels)
      {
      case 1:
        out_format->channel_locations[channel_index] = GAVL_CHID_FRONT_CENTER;
        break;
      case 2:
        out_format->channel_locations[channel_index] = GAVL_CHID_FRONT_LEFT;
        out_format->channel_locations[channel_index+1] = GAVL_CHID_FRONT_RIGHT;
        break;
      case 3:
        out_format->channel_locations[channel_index] = GAVL_CHID_FRONT_LEFT;
        out_format->channel_locations[channel_index+1] = GAVL_CHID_FRONT_RIGHT;
        out_format->channel_locations[channel_index+2] = GAVL_CHID_FRONT_CENTER;
        break;
      case 4:
        out_format->channel_locations[channel_index]   = GAVL_CHID_FRONT_LEFT;
        out_format->channel_locations[channel_index+1] = GAVL_CHID_FRONT_RIGHT;
        out_format->channel_locations[channel_index+2] = GAVL_CHID_FRONT_CENTER_LEFT;
        out_format->channel_locations[channel_index+3] = GAVL_CHID_FRONT_CENTER_LEFT;
        break;
      case 5:
        out_format->channel_locations[channel_index]   = GAVL_CHID_FRONT_LEFT;
        out_format->channel_locations[channel_index+1] = GAVL_CHID_FRONT_RIGHT;
        out_format->channel_locations[channel_index+2] = GAVL_CHID_FRONT_CENTER_LEFT;
        out_format->channel_locations[channel_index+3] = GAVL_CHID_FRONT_CENTER_LEFT;
        out_format->channel_locations[channel_index+4] = GAVL_CHID_FRONT_CENTER;
        break;
      }
    channel_index += opt->num_front_channels;
    
    switch(opt->num_rear_channels)
      {
      case 1:
        out_format->channel_locations[channel_index] = GAVL_CHID_REAR_CENTER;
        break;
      case 2:
        out_format->channel_locations[channel_index] = GAVL_CHID_REAR_LEFT;
        out_format->channel_locations[channel_index+1] = GAVL_CHID_REAR_RIGHT;
        break;
      case 3:
        out_format->channel_locations[channel_index] = GAVL_CHID_REAR_LEFT;
        out_format->channel_locations[channel_index+1] = GAVL_CHID_REAR_RIGHT;
        out_format->channel_locations[channel_index+2] = GAVL_CHID_REAR_CENTER;
        break;
      }
    channel_index += opt->num_rear_channels;
    switch(opt->num_lfe_channels)
      {
      case 1:
        out_format->channel_locations[channel_index] = GAVL_CHID_LFE;
        break;
      }
    channel_index += opt->num_lfe_channels;
    
    }
  if(opt->force_format != GAVL_SAMPLE_NONE)
    out_format->sample_format = opt->force_format;
  }

/* Video */

/* Definitions for standard resolutions */
  


/* Frame rates */

#define FRAME_RATE_FROM_INPUT  0
#define FRAME_RATE_USER        1
#define FRAME_RATE_23_976      2
#define FRAME_RATE_24          3
#define FRAME_RATE_25          4
#define FRAME_RATE_29_970      5
#define FRAME_RATE_30          6
#define FRAME_RATE_50          7
#define FRAME_RATE_59_940      8
#define FRAME_RATE_60          9
#define NUM_FRAME_RATES       10

static const struct
  {
  int rate;
  char * name;
  }
framerate_strings[NUM_FRAME_RATES] =
  {
    { FRAME_RATE_FROM_INPUT, "from_source"  },
    { FRAME_RATE_USER,       "user_defined" },
    { FRAME_RATE_23_976,     "23_976"       },
    { FRAME_RATE_24,         "24"           },
    { FRAME_RATE_25,         "25"           },
    { FRAME_RATE_29_970,     "29_970"       },
    { FRAME_RATE_30,         "30"           },
    { FRAME_RATE_50,         "50"           },
    { FRAME_RATE_59_940,     "59_940"       },
    { FRAME_RATE_60,         "60"           },
  };

static const struct
  {
  int rate;
  int timescale;
  int frame_duration;
  }
framerate_rates[NUM_FRAME_RATES] =
  {
    { FRAME_RATE_FROM_INPUT,     0,    0 },
    { FRAME_RATE_USER,           0,    0 },
    { FRAME_RATE_23_976,     24000, 1001 },
    { FRAME_RATE_24,            24,    1 },
    { FRAME_RATE_25,            25,    1 },
    { FRAME_RATE_29_970,     30000, 1001 },
    { FRAME_RATE_30,            30,    1 },
    { FRAME_RATE_50,            50,    1 },
    { FRAME_RATE_59_940,     60000, 1001 },
    { FRAME_RATE_60,            60,    1 },
  };

static int get_frame_rate_mode(const char * str)
  {
  int i;
  for(i = 0; i < NUM_FRAME_RATES; i++)
    {
    if(!strcmp(str, framerate_strings[i].name))
      {
      return framerate_strings[i].rate;
      }
    }
  return FRAME_RATE_USER;
  }
                

#define SP_FLAG(s, flag) if(!strcmp(name, s)) {               \
  flags = gavl_video_options_get_conversion_flags(opt->opt);  \
  if((val->val_i) && !(flags & flag))                         \
    {                                                         \
    opt->options_changed = 1;                                 \
    flags |= flag;                                            \
    }                                                         \
  else if(!(val->val_i) && (flags & flag))                    \
    {                                                         \
    opt->options_changed = 1;                                 \
    flags &= ~flag;                                           \
    }                                                         \
  gavl_video_options_set_conversion_flags(opt->opt, flags);   \
  return 1;                                                   \
  }

gavl_scale_mode_t bg_gavl_string_to_scale_mode(const char * str)
  {
  if(!strcmp(str, "auto"))
    return GAVL_SCALE_AUTO;
  else if(!strcmp(str, "nearest"))
    return GAVL_SCALE_NEAREST;
  else if(!strcmp(str, "bilinear"))
    return GAVL_SCALE_BILINEAR;
  else if(!strcmp(str, "quadratic"))
    return GAVL_SCALE_QUADRATIC;
  else if(!strcmp(str, "cubic_bspline"))
    return GAVL_SCALE_CUBIC_BSPLINE;
  else if(!strcmp(str, "cubic_mitchell"))
    return GAVL_SCALE_CUBIC_MITCHELL;
  else if(!strcmp(str, "cubic_catmull"))
    return GAVL_SCALE_CUBIC_CATMULL;
  else if(!strcmp(str, "sinc_lanczos"))
    return GAVL_SCALE_SINC_LANCZOS;
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Unknown scale mode %s\n", str);
    return GAVL_SCALE_AUTO;
    }
      
  }

gavl_downscale_filter_t bg_gavl_string_to_downscale_filter(const char * str)
  {
  if(!strcmp(str, "auto"))
    return GAVL_DOWNSCALE_FILTER_AUTO;
  else if(!strcmp(str, "none"))
    return GAVL_DOWNSCALE_FILTER_NONE;
  else if(!strcmp(str, "wide"))
    return GAVL_DOWNSCALE_FILTER_WIDE;
  else if(!strcmp(str, "gauss"))
    return GAVL_DOWNSCALE_FILTER_GAUSS;
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Unknown scale mode %s\n", str);
    return GAVL_DOWNSCALE_FILTER_GAUSS;
    }
  }

#define FRAME_SIZE_FROM_INPUT      0
#define FRAME_SIZE_USER            1
#define FRAME_SIZE_PAL_D1          2
#define FRAME_SIZE_PAL_D1_WIDE     3
#define FRAME_SIZE_PAL_DV          4
#define FRAME_SIZE_PAL_DV_WIDE     5
#define FRAME_SIZE_PAL_CVD         6
#define FRAME_SIZE_PAL_VCD         7
#define FRAME_SIZE_PAL_SVCD        8
#define FRAME_SIZE_PAL_SVCD_WIDE   9
#define FRAME_SIZE_NTSC_D1        10
#define FRAME_SIZE_NTSC_D1_WIDE   11
#define FRAME_SIZE_NTSC_DV        12
#define FRAME_SIZE_NTSC_DV_WIDE   13
#define FRAME_SIZE_NTSC_CVD       14
#define FRAME_SIZE_NTSC_VCD       15
#define FRAME_SIZE_NTSC_SVCD      16
#define FRAME_SIZE_NTSC_SVCD_WIDE 17
#define FRAME_SIZE_VGA            18
#define FRAME_SIZE_QVGA           19
#define FRAME_SIZE_720            20
#define FRAME_SIZE_1080           21
#define FRAME_SIZE_SQCIF          22
#define FRAME_SIZE_QCIF           23
#define FRAME_SIZE_CIF            24
#define FRAME_SIZE_4CIF           25
#define FRAME_SIZE_16CIF          26
#define NUM_FRAME_SIZES           27


static const struct
  {
  const char * name;
  int size;
  int image_width;
  int image_height;
  int pixel_width;
  int pixel_height;
  }
framesizes[] =
  {
    { "from_input",     FRAME_SIZE_FROM_INPUT,      0,      0,    0,    0},
    { "user_defined",   FRAME_SIZE_USER,            0,      0,    0,    0},
    { "pal_d1",         FRAME_SIZE_PAL_D1,          720,  576,   59,   54},
    { "pal_d1_wide",    FRAME_SIZE_PAL_D1_WIDE,     720,  576,  118,   81},
    { "pal_dv",         FRAME_SIZE_PAL_DV,          720,  576,   59,   54},
    { "pal_dv_wide",    FRAME_SIZE_PAL_DV_WIDE,     720,  576,  118,   81},
    { "pal_cvd",        FRAME_SIZE_PAL_CVD,         352,  576,   59,   27},
    { "pal_vcd",        FRAME_SIZE_PAL_VCD,         352,  288,   59,   54},
    { "pal_svcd",       FRAME_SIZE_PAL_SVCD,        480,  576,   59,   36},
    { "pal_svcd_wide",  FRAME_SIZE_PAL_SVCD_WIDE,   480,  576,   59,   27},
    { "ntsc_d1",        FRAME_SIZE_NTSC_D1,         720,  480,   10,   11},
    { "ntsc_d1_wide",   FRAME_SIZE_NTSC_D1_WIDE,    720,  480,   40,   33},
    { "ntsc_dv",        FRAME_SIZE_NTSC_DV,         720,  480,   10,   11},
    { "ntsc_dv_wide",   FRAME_SIZE_NTSC_DV_WIDE,    720,  480,   40,   33},
    { "ntsc_cvd",       FRAME_SIZE_NTSC_CVD,        352,  480,   20,   11},
    { "ntsc_vcd",       FRAME_SIZE_NTSC_VCD,        352,  240,   10,   11},
    { "ntsc_svcd",      FRAME_SIZE_NTSC_SVCD,       480,  480,   15,   11},
    { "ntsc_svcd_wide", FRAME_SIZE_NTSC_SVCD_WIDE,  480,  480,   20,   11},
    { "720",            FRAME_SIZE_720,             1280,  720,    1,    1},
    { "1080",           FRAME_SIZE_1080,            1920, 1080,    1,    1},
    { "vga",            FRAME_SIZE_VGA,             640,  480,    1,    1},
    { "qvga",           FRAME_SIZE_QVGA,            320,  240,    1,    1},
    { "sqcif",          FRAME_SIZE_SQCIF,           128,   96,   12,   11},
    { "qcif",           FRAME_SIZE_QCIF,            176,  144,   12,   11},
    { "cif",            FRAME_SIZE_CIF,             352,  288,   12,   11},
    { "4cif",           FRAME_SIZE_4CIF,            704,  576,   12,   11},
    { "16cif",          FRAME_SIZE_16CIF,           1408, 1152,   12,   11},
  };

static void set_frame_size_mode(bg_gavl_video_options_t * opt,
                                const bg_parameter_value_t * val)
  {
  int i;
  for(i = 0; i < NUM_FRAME_SIZES; i++)
    {
    if(!strcmp(val->val_str, framesizes[i].name))
      opt->size = framesizes[i].size;
    }
  }

void bg_gavl_video_options_set_frame_size(const bg_gavl_video_options_t * opt,
                                          const gavl_video_format_t * in_format,
                                          gavl_video_format_t * out_format)
  {
  int i;
  
  if(opt->size == FRAME_SIZE_FROM_INPUT)
    {
    out_format->image_width =  in_format->image_width;
    out_format->image_height = in_format->image_height;
    out_format->frame_width =  in_format->frame_width;
    out_format->frame_height = in_format->frame_height;
    out_format->pixel_width  = in_format->pixel_width;
    out_format->pixel_height = in_format->pixel_height;
    return;
    }
  else if(opt->size == FRAME_SIZE_USER)
    {
    out_format->image_width = opt->user_image_width;
    out_format->image_height = opt->user_image_height;
    out_format->frame_width = opt->user_image_width;
    out_format->frame_height = opt->user_image_height;
    out_format->pixel_width = opt->user_pixel_width;
    out_format->pixel_height = opt->user_pixel_height;
    return;
    }
  
  for(i = 0; i < NUM_FRAME_SIZES; i++)
    {
    if(opt->size == framesizes[i].size)
      {
      out_format->image_width =  framesizes[i].image_width;
      out_format->image_height = framesizes[i].image_height;
      out_format->frame_width =  framesizes[i].image_width;
      out_format->frame_height = framesizes[i].image_height;
      out_format->pixel_width  = framesizes[i].pixel_width;
      out_format->pixel_height = framesizes[i].pixel_height;
      }
    }
  }

int bg_gavl_video_set_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val)
  {
  int flags;  
  bg_gavl_video_options_t * opt = (bg_gavl_video_options_t *)data;

  if(!name)
    return 1;
  if(!strcmp(name, "conversion_quality"))
    {
    if(val->val_i != gavl_video_options_get_quality(opt->opt))
      opt->options_changed = 1;
    
    gavl_video_options_set_quality(opt->opt, val->val_i);
    return 1;
    }
  else if(!strcmp(name, "framerate"))
    {
    opt->framerate_mode = get_frame_rate_mode(val->val_str);
    return 1;
    }
  else if(!strcmp(name, "frame_size"))
    {
    set_frame_size_mode(opt, val);
    return 1;
    }
  else if(!strcmp(name, "pixelformat"))
    {
    opt->pixelformat = gavl_string_to_pixelformat(val->val_str);
    return 1;
    }

  //  SP_INT(fixed_framerate);
  SP_INT(frame_duration);
  SP_INT(timescale);

  SP_INT(user_image_width);
  SP_INT(user_image_height);
  SP_INT(user_pixel_width);
  SP_INT(user_pixel_height);
  //  SP_INT(maintain_aspect);
  
  SP_FLAG("force_deinterlacing", GAVL_FORCE_DEINTERLACE);
  SP_FLAG("resample_chroma", GAVL_RESAMPLE_CHROMA);
  
  if(!strcmp(name, "alpha_mode"))
    {
    if(!strcmp(val->val_str, "ignore"))
      {
      gavl_video_options_set_alpha_mode(opt->opt, GAVL_ALPHA_IGNORE);
      }
    else if(!strcmp(val->val_str, "blend_color"))
      {
      gavl_video_options_set_alpha_mode(opt->opt, GAVL_ALPHA_BLEND_COLOR);
      }
    return 1;
    }
  else if(!strcmp(name, "background_color"))
    {
    gavl_video_options_set_background_color(opt->opt, val->val_color);
    return 1;
    }
  else if(!strcmp(name, "scale_mode"))
    {
    gavl_video_options_set_scale_mode(opt->opt, bg_gavl_string_to_scale_mode(val->val_str));
    return 1;
    }
  else if(!strcmp(name, "scale_order"))
    {
    gavl_video_options_set_scale_order(opt->opt, val->val_i);
    }
  else if(!strcmp(name, "deinterlace_mode"))
    {
    if(!strcmp(val->val_str, "none"))
      gavl_video_options_set_deinterlace_mode(opt->opt, GAVL_DEINTERLACE_NONE);
    else if(!strcmp(val->val_str, "copy"))
      gavl_video_options_set_deinterlace_mode(opt->opt, GAVL_DEINTERLACE_COPY);
    else if(!strcmp(val->val_str, "scale"))
      gavl_video_options_set_deinterlace_mode(opt->opt, GAVL_DEINTERLACE_SCALE);
    }
  else if(!strcmp(name, "deinterlace_drop_mode"))
    {
    if(!strcmp(val->val_str, "top"))
      gavl_video_options_set_deinterlace_drop_mode(opt->opt, GAVL_DEINTERLACE_DROP_TOP);
    else if(!strcmp(val->val_str, "bottom"))
      gavl_video_options_set_deinterlace_drop_mode(opt->opt, GAVL_DEINTERLACE_DROP_BOTTOM);
    }
  else if(!strcmp(name, "threads"))
    {
    opt->num_threads = val->val_i;
    if(!opt->thread_pool)
      {
      opt->thread_pool = bg_thread_pool_create(opt->num_threads);
      gavl_video_options_set_num_threads(opt->opt, opt->num_threads);
      gavl_video_options_set_run_func(opt->opt, bg_thread_pool_run, opt->thread_pool);
      gavl_video_options_set_stop_func(opt->opt, bg_thread_pool_stop, opt->thread_pool);
      }
    return 1;
    }
  return 0;
  }

#undef SP_INT

void bg_gavl_video_options_init(bg_gavl_video_options_t * opt)
  {
  opt->opt = gavl_video_options_create();
  }

void bg_gavl_video_options_free(bg_gavl_video_options_t * opt)
  {
  if(opt->opt)
    gavl_video_options_destroy(opt->opt);
  if(opt->thread_pool)
    bg_thread_pool_destroy(opt->thread_pool);
  }


void bg_gavl_video_options_set_framerate(const bg_gavl_video_options_t * opt,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format)
  {
  int i;
  if(opt->framerate_mode == FRAME_RATE_FROM_INPUT)
    {
    out_format->frame_duration = in_format->frame_duration;
    out_format->timescale =      in_format->timescale;
    out_format->framerate_mode = in_format->framerate_mode;
    return;
    }
  else if(opt->framerate_mode == FRAME_RATE_USER)
    {
    out_format->frame_duration = opt->frame_duration;
    out_format->timescale =      opt->timescale;
    out_format->framerate_mode = GAVL_FRAMERATE_CONSTANT;
    return;
    }
  else
    {
    for(i = 0; i < NUM_FRAME_RATES; i++)
      {
      if(opt->framerate_mode == framerate_rates[i].rate)
        {
        out_format->timescale      = framerate_rates[i].timescale;
        out_format->frame_duration = framerate_rates[i].frame_duration;
        out_format->framerate_mode = GAVL_FRAMERATE_CONSTANT;
        return;
        }
      }
    }
  }

static void set_interlace(const bg_gavl_video_options_t * opt,
                          const gavl_video_format_t * in_format,
                          gavl_video_format_t * out_format)
  {
  int flags = gavl_video_options_get_conversion_flags(opt->opt);
  if(flags & GAVL_FORCE_DEINTERLACE)
    out_format->interlace_mode = GAVL_INTERLACE_NONE;
  else
    out_format->interlace_mode = in_format->interlace_mode;
  }

void bg_gavl_video_options_set_pixelformat(const bg_gavl_video_options_t * opt,
                                           const gavl_video_format_t * in_format,
                                           gavl_video_format_t * out_format)
  {
  if(opt->pixelformat == GAVL_PIXELFORMAT_NONE)
    out_format->pixelformat = in_format->pixelformat;
  else
    out_format->pixelformat = opt->pixelformat;
  }

void bg_gavl_video_options_set_format(const bg_gavl_video_options_t * opt,
                                      const gavl_video_format_t * in_format,
                                      gavl_video_format_t * out_format)
  {
  bg_gavl_video_options_set_framerate(opt, in_format, out_format);
  set_interlace(opt, in_format, out_format);
  bg_gavl_video_options_set_frame_size(opt, in_format, out_format);
  }



int bg_overlay_too_old(gavl_time_t time, gavl_time_t ovl_time,
                       gavl_time_t ovl_duration)
  {
  if((ovl_duration >= 0) && (time > ovl_time + ovl_duration))
    return 1;
  return 0;
  }

int bg_overlay_too_new(gavl_time_t time, gavl_time_t ovl_time)
  {
  if(time < ovl_time)
    return 1;
  return 0;
  }
