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
  bg_gavl_audio_options_t * opt = (bg_gavl_audio_options_t *)data;

  if(!strcmp(name, "conversion_quality"))
    {
    opt->opt.quality = val->val_i;
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
    opt->opt.conversion_flags &= ~GAVL_AUDIO_FRONT_TO_REAR_MASK;
    
    if(!strcmp(val->val_str, "Copy"))
      {
      opt->opt.conversion_flags |= GAVL_AUDIO_FRONT_TO_REAR_COPY;
      }
    else if(!strcmp(val->val_str, "Mute"))
      {
      opt->opt.conversion_flags |= GAVL_AUDIO_FRONT_TO_REAR_MUTE;
      }
    else if(!strcmp(val->val_str, "Diff"))
      {
      opt->opt.conversion_flags |= GAVL_AUDIO_FRONT_TO_REAR_DIFF;
      }
    return 1;
    }

  else if(!strcmp(name, "stereo_to_mono"))
    {
    opt->opt.conversion_flags &= ~GAVL_AUDIO_STEREO_TO_MONO_MASK;
                                                                                                        
    if(!strcmp(val->val_str, "Choose left"))
      {
      opt->opt.conversion_flags |= GAVL_AUDIO_STEREO_TO_MONO_LEFT;
      }
    else if(!strcmp(val->val_str, "Choose right"))
      {
      opt->opt.conversion_flags |= GAVL_AUDIO_STEREO_TO_MONO_RIGHT;
      }
    else if(!strcmp(val->val_str, "Mix"))
      {
      opt->opt.conversion_flags |= GAVL_AUDIO_STEREO_TO_MONO_MIX;
      }
    return 1;
    }

  return 0;
  }

void bg_gavl_audio_options_init(bg_gavl_audio_options_t *opt)
  {
  memset(opt, 0, sizeof(*opt));
  gavl_audio_default_options(&(opt->opt));
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

#define FRAME_SIZE_FROM_INPUT  0
#define FRAME_SIZE_USER        1
#define FRAME_SIZE_DVD_PAL_D1  2
#define FRAME_SIZE_DVD_PAL     3
#define FRAME_SIZE_DVD_NTSC_D1 4
#define FRAME_SIZE_DVD_NTSC    5
#define FRAME_SIZE_VCD_PAL     6
#define FRAME_SIZE_VCD_NTSC    7
#define FRAME_SIZE_SVCD_PAL    8
#define FRAME_SIZE_SVCD_NTSC   9
#define NUM_FRAME_SIZES       10

#if 0
static struct
  {
  int res;
  char * name;
  }
resolution_strings[NUM_FRAME_SIZES] =
  {
    { FRAME_SIZE_FROM_INPUT,  "from_input"   },
    { FRAME_SIZE_USER,        "user_defined" },
    { FRAME_SIZE_DVD_PAL_D1,  "dvd_pal_d1"   },
    { FRAME_SIZE_DVD_PAL,     "dvd_pal"      },
    { FRAME_SIZE_DVD_NTSC_D1, "dvd_ntsc_d1"  },
    { FRAME_SIZE_DVD_NTSC,    "dvd_ntsc"     },
    { FRAME_SIZE_VCD_PAL,     "vcd_pal"      },
    { FRAME_SIZE_VCD_NTSC,    "vcd_ntsc"     },
    { FRAME_SIZE_SVCD_PAL,    "svcd_pal"     },
    { FRAME_SIZE_SVCD_NTSC,   "svcd_ntsc"    },
  };

static struct
  {
  int res;
  int width;
  int height;
  }
resolution_sizes[] =
  {
    { FRAME_SIZE_FROM_INPUT,   0,   0 },
    { FRAME_SIZE_USER,         0,   0 },
    { FRAME_SIZE_DVD_PAL_D1, 720, 576 },
    { FRAME_SIZE_DVD_PAL,    704, 576 },
    { FRAME_SIZE_DVD_NTSC_D1,720, 480 },
    { FRAME_SIZE_DVD_NTSC,   704, 480 },
    { FRAME_SIZE_VCD_PAL,    352, 288 },
    { FRAME_SIZE_VCD_NTSC,   352, 240 },
    { FRAME_SIZE_SVCD_PAL,   480, 576 },
    { FRAME_SIZE_SVCD_NTSC,  480, 480 },
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
  int i_tmp;
  bg_gavl_video_options_t * opt = (bg_gavl_video_options_t *)data;

  if(!strcmp(name, "conversion_quality"))
    {
    opt->opt.quality = val->val_i;
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
  SP_INT(user_width);
  SP_INT(user_height);
  SP_INT(maintain_aspect);

  if(!strcmp(name, "alpha_mode"))
    {
    //    fprintf(stderr, "Setting alpha mode\n");
    if(!strcmp(val->val_str, "Ignore"))
      {
      opt->opt.alpha_mode = GAVL_ALPHA_IGNORE;
      }
    else if(!strcmp(val->val_str, "Blend background color"))
      {
      opt->opt.alpha_mode = GAVL_ALPHA_BLEND_COLOR;
      }
    }
  else if(!strcmp(name, "background_color"))
    {
    i_tmp = (int)(val->val_color[0] * 65535.0 + 0.5);
    if(i_tmp < 0)      i_tmp = 0;
    if(i_tmp > 0xffff) i_tmp = 0xffff;
    opt->opt.background_red = i_tmp;

    i_tmp = (int)(val->val_color[1] * 65535.0 + 0.5);
    if(i_tmp < 0)      i_tmp = 0;
    if(i_tmp > 0xffff) i_tmp = 0xffff;
    opt->opt.background_green = i_tmp;

    i_tmp = (int)(val->val_color[2] * 65535.0 + 0.5);
    if(i_tmp < 0)      i_tmp = 0;
    if(i_tmp > 0xffff) i_tmp = 0xffff;
    opt->opt.background_blue = i_tmp;
    }

  
  return 0;
  }

#undef SP_INT


void bg_gavl_video_options_init(bg_gavl_video_options_t * opt)
  {
  gavl_video_default_options(&(opt->opt));
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
    out_format->free_framerate = 0;
    }
  for(i = 0; i < NUM_FRAME_RATES; i++)
    {
    if(opt->framerate_mode == framerate_rates[i].rate)
      {
      out_format->timescale      = framerate_rates[i].timescale;
      out_format->frame_duration = framerate_rates[i].frame_duration;
      return;
      }
    }
  }

void bg_gavl_video_options_set_framesize(bg_gavl_video_options_t * opt,
                                         const gavl_video_format_t * in_format,
                                         gavl_video_format_t * out_format,
                                         gavl_rectangle_t * in_rect,
                                         gavl_rectangle_t * out_rect)
  {
  
  }
