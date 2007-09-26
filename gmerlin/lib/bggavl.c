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

#include <config.h>

#include <string.h>
#include <stdio.h>

#include <gavl/gavl.h>

#include <parameter.h>
#include <bggavl.h>

#include <log.h>
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
  SP_INT(force_float);
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
  gavl_audio_format_copy(out_format, in_format);

  if(opt->fixed_samplerate)
    {
    out_format->samplerate = opt->samplerate;
    }
  if(opt->fixed_channel_setup)
    {
    out_format->num_channels = opt->num_front_channels + opt->num_rear_channels + opt->num_lfe_channels;

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
  if(opt->force_float)
    out_format->sample_format = GAVL_SAMPLE_FLOAT;
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
                                const bg_parameter_value_t * val)
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

  if(!strcmp(name, "framerate"))
    {
    set_frame_rate_mode(opt, val);
    return 1;
    }
  //  SP_INT(fixed_framerate);
  SP_INT(frame_duration);
  SP_INT(timescale);

  //  SP_INT(user_image_width);
  //  SP_INT(user_image_height);
  //  SP_INT(user_pixel_width);
  //  SP_INT(user_pixel_height);
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
  }


static void set_framerate(const bg_gavl_video_options_t * opt,
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


void bg_gavl_video_options_set_format(const bg_gavl_video_options_t * opt,
                                      const gavl_video_format_t * in_format,
                                      gavl_video_format_t * out_format)
  {
  set_framerate(opt, in_format, out_format);
  set_interlace(opt, in_format, out_format);
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
