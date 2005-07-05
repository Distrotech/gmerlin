/*****************************************************************
 
  bggavl.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <stdio.h>

#include <gavl/gavl.h>

#include <parameter.h>
#include <bggavl.h>

/* Audio stuff */

int bg_gavl_audio_set_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  int flags;
  bg_gavl_audio_options_t * opt = (bg_gavl_audio_options_t *)data;

  if(!strcmp(name, "conversion_quality"))
    {
    gavl_audio_options_set_quality(opt->opt, val->val_i);
    return 1;
    }
  else if(!strcmp(name, "fixed_samplerate"))
    {
    opt->fixed_samplerate = val->val_i;
    return 1;
    }
  else if(!strcmp(name, "samplerate"))
    {
    opt->samplerate = val->val_i;
    return 1;
    }
  else if(!strcmp(name, "fixed_channel_setup"))
    {
    opt->fixed_channel_setup = val->val_i;
    return 1;
    }
  else if(!strcmp(name, "channel_setup"))
    {
    if(!strcmp(val->val_str, "Mono"))
      opt->channel_setup = GAVL_CHANNEL_MONO;
    else if(!strcmp(val->val_str, "Stereo"))
      opt->channel_setup = GAVL_CHANNEL_STEREO;
    else if(!strcmp(val->val_str, "3 Front"))
      opt->channel_setup = GAVL_CHANNEL_3F;
    else if(!strcmp(val->val_str, "2 Front 1 Rear"))
      opt->channel_setup = GAVL_CHANNEL_2F1R;
    else if(!strcmp(val->val_str, "3 Front 1 Rear"))
      opt->channel_setup = GAVL_CHANNEL_3F1R;
    else if(!strcmp(val->val_str, "2 Front 2 Rear"))
      opt->channel_setup = GAVL_CHANNEL_2F2R;
    else if(!strcmp(val->val_str, "3 Front 2 Rear"))
      opt->channel_setup = GAVL_CHANNEL_3F2R;
    return 1;
    }

  else if(!strcmp(name, "front_to_rear"))
    {
    flags = gavl_audio_options_get_conversion_flags(opt->opt);

    flags &= ~GAVL_AUDIO_FRONT_TO_REAR_MASK;
    
    if(!strcmp(val->val_str, "Copy"))
      {
      flags |= GAVL_AUDIO_FRONT_TO_REAR_COPY;
      }
    else if(!strcmp(val->val_str, "Mute"))
      {
      flags |= GAVL_AUDIO_FRONT_TO_REAR_MUTE;
      }
    else if(!strcmp(val->val_str, "Diff"))
      {
      flags |= GAVL_AUDIO_FRONT_TO_REAR_DIFF;
      }
    gavl_audio_options_set_conversion_flags(opt->opt, flags);
    
    return 1;
    }

  else if(!strcmp(name, "stereo_to_mono"))
    {
    flags = gavl_audio_options_get_conversion_flags(opt->opt);

    flags &= ~GAVL_AUDIO_STEREO_TO_MONO_MASK;
                                                                                                        
    if(!strcmp(val->val_str, "Choose left"))
      {
      flags |= GAVL_AUDIO_STEREO_TO_MONO_LEFT;
      }
    else if(!strcmp(val->val_str, "Choose right"))
      {
      flags |= GAVL_AUDIO_STEREO_TO_MONO_RIGHT;
      }
    else if(!strcmp(val->val_str, "Mix"))
      {
      flags |= GAVL_AUDIO_STEREO_TO_MONO_MIX;
      }
    gavl_audio_options_set_conversion_flags(opt->opt, flags);
    return 1;
    }

  return 0;
  }

void bg_gavl_audio_options_init(bg_gavl_audio_options_t *opt)
  {
  gavl_audio_options_set_defaults(opt->opt);
  }

void bg_gavl_audio_options_set_format(bg_gavl_audio_options_t * opt,
                                      const gavl_audio_format_t * in_format,
                                      gavl_audio_format_t * out_format)
  {
  gavl_audio_format_copy(out_format, in_format);

  if(opt->fixed_samplerate)
    {
    out_format->samplerate = opt->samplerate;
    }
  if(opt->fixed_channel_setup)
    {
    out_format->channel_setup = opt->channel_setup;
    out_format->channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(out_format);
    }
  }

/* Video */

/* Definitions for standard resolutions */
  
#define FRAME_SIZE_FROM_INPUT      0
#define FRAME_SIZE_USER            1
#define FRAME_SIZE_PAL_D1          2
#define FRAME_SIZE_PAL_D1_WIDE     3
#define FRAME_SIZE_PAL_DV          4
#define FRAME_SIZE_PAL_DV_WIDE     5
#define FRAME_SIZE_PAL_VCD         6
#define FRAME_SIZE_PAL_SVCD        7
#define FRAME_SIZE_PAL_SVCD_WIDE   8
#define FRAME_SIZE_NTSC_D1         9
#define FRAME_SIZE_NTSC_D1_WIDE   10
#define FRAME_SIZE_NTSC_DV        11
#define FRAME_SIZE_NTSC_DV_WIDE   12
#define FRAME_SIZE_NTSC_VCD       13
#define FRAME_SIZE_NTSC_SVCD      14
#define FRAME_SIZE_NTSC_SVCD_WIDE 15
#define FRAME_SIZE_VGA            16
#define FRAME_SIZE_QVGA           17
#define NUM_FRAME_SIZES           18

#if 0
      multi_names: (char*[]){ "from_input", \
                              "user_defined", \
                              "pal_d1", \
                              "pal_d1_wide", \
                              "pal_dv", \
                              "pal_dv_wide", \
                              "pal_vcd", \
                              "pal_svcd", \
                              "pal_svcd_wide", \
                              "ntsc_d1", \
                              "ntsc_d1_wide", \
                              "ntsc_dv", \
                              "ntsc_dv_wide", \
                              "ntsc_vcd", \
                              "ntsc_svcd", \
                              "ntsc_svcd_wide", \
                              "vga", \
                              "qvga", \

#endif


#if 1
static struct
  {
  int size;
  char * name;
  }
framesize_strings[NUM_FRAME_SIZES] =
  {
    { FRAME_SIZE_FROM_INPUT,     "from_input"},
    { FRAME_SIZE_USER,           "user_defined"},
    { FRAME_SIZE_PAL_D1,         "pal_d1"},
    { FRAME_SIZE_PAL_D1_WIDE,    "pal_d1_wide"},
    { FRAME_SIZE_PAL_DV,         "pal_dv"},
    { FRAME_SIZE_PAL_DV_WIDE,    "pal_dv_wide"},
    { FRAME_SIZE_PAL_VCD,        "pal_vcd"},
    { FRAME_SIZE_PAL_SVCD,       "pal_svcd"},
    { FRAME_SIZE_PAL_SVCD_WIDE,  "pal_svcd_wide"},
    { FRAME_SIZE_NTSC_D1,        "ntsc_d1"},
    { FRAME_SIZE_NTSC_D1_WIDE,   "ntsc_d1_wide"},
    { FRAME_SIZE_NTSC_DV,        "ntsc_dv"},
    { FRAME_SIZE_NTSC_DV_WIDE,   "ntsc_dv_wide"},
    { FRAME_SIZE_NTSC_VCD,       "ntsc_vcd"},
    { FRAME_SIZE_NTSC_SVCD,      "ntsc_svcd"},
    { FRAME_SIZE_NTSC_SVCD_WIDE, "ntsc_svcd_wide"},
    { FRAME_SIZE_VGA,            "vga"},
    { FRAME_SIZE_QVGA,           "qvga"},
  };

static struct
  {
  int size;
  int image_width;
  int image_height;
  int pixel_width;
  int pixel_height;
  }
frame_size_sizes[NUM_FRAME_SIZES] =
  {
    { FRAME_SIZE_FROM_INPUT,       0,   0,   0,     0 },
    { FRAME_SIZE_USER,             0,   0,   0,     0 },
    { FRAME_SIZE_PAL_D1,           720, 576,   59,   54},
    { FRAME_SIZE_PAL_D1_WIDE,      720, 576,  118,   81},
    { FRAME_SIZE_PAL_DV,           720, 576,   59,   54},
    { FRAME_SIZE_PAL_DV_WIDE,      720, 576,  118,   81},
    { FRAME_SIZE_PAL_VCD,          352, 288,   59,   54},
    { FRAME_SIZE_PAL_SVCD,         480, 576,   59,   36},
    { FRAME_SIZE_PAL_SVCD_WIDE,    480, 576,   59,   27},
    { FRAME_SIZE_NTSC_D1,          720, 480,   10,   11},
    { FRAME_SIZE_NTSC_D1_WIDE,     720, 480,   40,   33 },
    { FRAME_SIZE_NTSC_DV,          720, 480,   10,   11 },
    { FRAME_SIZE_NTSC_DV_WIDE,     720, 480,   40,   33 },
    { FRAME_SIZE_NTSC_VCD,         352, 240,   10,   11 },
    { FRAME_SIZE_NTSC_SVCD,        480, 480,   15,   11 },
    { FRAME_SIZE_NTSC_SVCD_WIDE,   480, 480,   20,   11 },
    { FRAME_SIZE_VGA,            640, 480,    1,    1 },
    { FRAME_SIZE_QVGA,           320, 240,    1,    1 },
  };
#endif

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

static struct
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

static struct
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

static void set_frame_rate_mode(bg_gavl_video_options_t * opt,
                                bg_parameter_value_t * val)
  {
  int i;
  for(i = 0; i < NUM_FRAME_RATES; i++)
    {
    if(!strcmp(val->val_str, framerate_strings[i].name))
      {
      opt->framerate_mode = framerate_strings[i].rate;
      return;
      }
    }
  }

#define SP_INT(s) if(!strcmp(name, # s)) \
    { \
    opt->s = val->val_i;                     \
    return 1; \
    }

int bg_gavl_video_set_parameter(void * data, char * name,
                                bg_parameter_value_t * val)
  {
  int i;
  bg_gavl_video_options_t * opt = (bg_gavl_video_options_t *)data;

  if(!strcmp(name, "conversion_quality"))
    {
    gavl_video_options_set_quality(opt->opt, val->val_i);
    return 1;
    }

  if(!strcmp(name, "framerate"))
    {
    set_frame_rate_mode(opt, val);
    return 1;
    }
  //  SP_INT(fixed_framerate);
  SP_INT(frame_duration);
  SP_INT(timescale);
  SP_INT(crop_left);
  SP_INT(crop_right);
  SP_INT(crop_top);
  SP_INT(crop_bottom);
  SP_INT(user_image_width);
  SP_INT(user_image_height);
  SP_INT(user_pixel_width);
  SP_INT(user_pixel_height);
  SP_INT(maintain_aspect);

  if(!strcmp(name, "alpha_mode"))
    {
    //    fprintf(stderr, "Setting alpha mode\n");
    if(!strcmp(val->val_str, "Ignore"))
      {
      gavl_video_options_set_alpha_mode(opt->opt, GAVL_ALPHA_IGNORE);
      }
    else if(!strcmp(val->val_str, "Blend background color"))
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
    if(!strcmp(val->val_str, "nearest"))
      gavl_video_options_set_scale_mode(opt->opt, GAVL_SCALE_NEAREST);

    else if(!strcmp(val->val_str, "bilinear"))
      gavl_video_options_set_scale_mode(opt->opt, GAVL_SCALE_BILINEAR);
    return 1;
    }
  else if(!strcmp(name, "frame_size"))
    {
    for(i = 0; i < NUM_FRAME_SIZES; i++)
      {
      if(!strcmp(val->val_str, framesize_strings[i].name))
        {
        fprintf(stderr, "Frame size: %s\n", framesize_strings[i].name);
        opt->frame_size = framesize_strings[i].size;
        break;
        }
      }
    return 1;
    }
  return 0;
  }

#undef SP_INT

void bg_gavl_video_options_init(bg_gavl_video_options_t * opt)
  {
  gavl_video_options_set_defaults(opt->opt);
  }

void bg_gavl_video_options_set_framerate(bg_gavl_video_options_t * opt,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format)
  {
  int i;
  if(opt->framerate_mode == FRAME_RATE_FROM_INPUT)
    return;
    
  if(opt->framerate_mode == FRAME_RATE_USER)
    {
    out_format->frame_duration = opt->frame_duration;
    out_format->timescale =      opt->timescale;
    out_format->framerate_mode = GAVL_FRAMERATE_CONSTANT;
    }
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

void bg_gavl_video_options_set_framesize(bg_gavl_video_options_t * opt,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format)
  {
  int i;
  
  /* Set image- and pixel size for output */
  
  if(opt->frame_size == FRAME_SIZE_FROM_INPUT)
    {
    out_format->image_width  = in_format->image_width;
    out_format->image_height = in_format->image_height;
    }
  else if(opt->frame_size == FRAME_SIZE_USER)
    {
    out_format->image_width  = opt->user_image_width;
    out_format->image_height = opt->user_image_height;

    out_format->pixel_width =  opt->user_pixel_width;
    out_format->pixel_height = opt->user_pixel_height;
    }
  else
    {
    for(i = 0; i < NUM_FRAME_SIZES; i++)
      {
      if(frame_size_sizes[i].size == opt->frame_size)
        {
        out_format->image_width = frame_size_sizes[i].image_width;
        out_format->image_height = frame_size_sizes[i].image_height;
        
        out_format->pixel_width = frame_size_sizes[i].pixel_width;
        out_format->pixel_height = frame_size_sizes[i].pixel_height;
        
        }
      }
    }
  out_format->frame_width = out_format->image_width;
  out_format->frame_height = out_format->image_height;
  }

void bg_gavl_video_options_set_rectangles(bg_gavl_video_options_t * opt,
                                          const gavl_video_format_t * in_format,
                                          const gavl_video_format_t * out_format)
  {
  gavl_rectangle_t in_rect;
  gavl_rectangle_t out_rect;
  
  /* Crop input */
  gavl_rectangle_set_all(&in_rect, in_format);

  gavl_rectangle_crop_left(&in_rect,   opt->crop_left);
  gavl_rectangle_crop_right(&in_rect,  opt->crop_right);
  gavl_rectangle_crop_top(&in_rect,    opt->crop_top);
  gavl_rectangle_crop_bottom(&in_rect, opt->crop_bottom);

  if(opt->maintain_aspect)
    {
    // fprintf(stderr, "FIT ASPECT\n");
    gavl_rectangle_fit_aspect(&out_rect,   // gavl_rectangle_t * r,
                              in_format,  // gavl_video_format_t * src_format,
                              &in_rect,    // gavl_rectangle_t * src_rect,
                              out_format, // gavl_video_format_t * dst_format,
                              1.0,        // float zoom,
                              0.0         // float squeeze
                              );
    }
  else
    {
    gavl_rectangle_set_all(&out_rect, out_format);
    }

  /* Set rectangles */

  gavl_video_options_set_rectangles(opt->opt, &in_rect, &out_rect);
  }
