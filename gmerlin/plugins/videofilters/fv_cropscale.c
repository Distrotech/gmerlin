/*****************************************************************
 
  fv_cropscale.c
 
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>
#include <log.h>

#include <bggavl.h>

#define LOG_DOMAIN "fv_cropscale"

#define DEINTERLACE_NEVER  0
#define DEINTERLACE_AUTO   1
#define DEINTERLACE_ALWAYS 2

typedef struct
  {
  /* Parameters */
  int frame_size;
  
  int user_image_width;
  int user_image_height;

  int user_pixel_width;
  int user_pixel_height;

  double crop_left;
  double crop_right;
  double crop_top;
  double crop_bottom;
  int maintain_aspect;

  int deinterlace;

  /* */
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_scaler_t * scaler;
  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  
  gavl_video_frame_t * frame;

  int need_reinit;
  int need_restart;

  gavl_video_options_t * opt;

  float border_color[4];
  
  } cropscale_priv_t;

static void * create_cropscale()
  {
  cropscale_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->scaler = gavl_video_scaler_create();
  ret->opt = gavl_video_scaler_get_options(ret->scaler);
  
  ret->border_color[3] = 1.0;
  return ret;
  }

static void destroy_cropscale(void * priv)
  {
  cropscale_priv_t * vp;
  vp = (cropscale_priv_t *)priv;
  if(vp->frame)
    gavl_video_frame_destroy(vp->frame);
  free(vp);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "Crop",
      long_name: TRS("Crop"),
      type: BG_PARAMETER_SECTION,
    },
    {
      name:      "crop_left",
      long_name: TRS("Crop left"),
      opt:       "crl",
      type:      BG_PARAMETER_FLOAT,
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_f: 0.0 },
      val_max:     { val_f: 100000.0 },
      val_default: { val_f: 0.0 },
      num_digits: 3,
      help_string: TRS("Cut this many pixels from the left border of the source frames.")
    },
    {
      name:      "crop_right",
      long_name: TRS("Crop right"),
      opt:       "crr",
      type:      BG_PARAMETER_FLOAT,
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_f: 0.0 },
      val_max:     { val_f: 100000.0 },
      val_default: { val_f: 0.0 },
      num_digits: 3,
      help_string: TRS("Cut this many pixels from the right border of the source frames.")
    },
    {
      name:      "crop_top",
      long_name: TRS("Crop top"),
      opt:       "crt",
      type:      BG_PARAMETER_FLOAT,
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_f: 0.0 },
      val_max:     { val_f: 100000.0 },
      val_default: { val_f: 0.0 },
      num_digits: 3,
      help_string: TRS("Cut this many pixels from the top border of the source frames.")
    },
    {
      name:      "crop_bottom",
      long_name: TRS("Crop bottom"),
      opt:       "crb",
      type:      BG_PARAMETER_FLOAT,
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_f: 0.0 },
      val_max:     { val_f: 100000.0 },
      val_default: { val_f: 0.0 },
      num_digits: 3,
      help_string: TRS("Cut this many pixels from the bottom border of the source frames.")    },
    {
      name: "frame_size_section",
      long_name: TRS("Frame size"),
      type: BG_PARAMETER_SECTION,
    },
    {
      name:        "frame_size",
      long_name:   TRS("Frame Size"),
      opt:         "s",
      type:        BG_PARAMETER_STRINGLIST,
      flags:     BG_PARAMETER_SYNC,
      multi_names: (char*[]){ "from_input",
                              "user_defined",
                              "pal_d1",
                              "pal_d1_wide",
                              "pal_dv",
                              "pal_dv_wide",
                              "pal_cvd",
                              "pal_vcd",
                              "pal_svcd",
                              "pal_svcd_wide",
                              "ntsc_d1",
                              "ntsc_d1_wide",
                              "ntsc_dv",
                              "ntsc_dv_wide",
                              "ntsc_cvd",
                              "ntsc_vcd",
                              "ntsc_svcd",
                              "ntsc_svcd_wide",
                              "vga",
                              "qvga",
                               (char*)0 },
      multi_labels:  (char*[]){ TRS("From Source"),
                                TRS("User defined"),
                                TRS("PAL DVD D1 4:3 (720 x 576)"),
                                TRS("PAL DVD D1 16:9 (720 x 576)"),
                                TRS("PAL DV 4:3 (720 x 576)"),
                                TRS("PAL DV 16:9 (720 x 576)"),
                                TRS("PAL CVD (352 x 576)"),
                                TRS("PAL VCD (352 x 288)"),
                                TRS("PAL SVCD 4:3 (480 x 576)"),
                                TRS("PAL SVCD 16:9 (480 x 576)"),
                                TRS("NTSC DVD D1 4:3 (720 x 480)"),
                                TRS("NTSC DVD D1 16:9 (720 x 480)"),
                                TRS("NTSC DV 4:3 (720 x 480)"),
                                TRS("NTSC DV 16:9 (720 x 480)"),
                                TRS("NTSC CVD (352 x 480)"),
                                TRS("NTSC VCD (352 x 240)"),
                                TRS("NTSC SVCD 4:3 (480 x 480)"),
                                TRS("NTSC SVCD 16:9 (480 x 480)"),
                                TRS("VGA (640 x 480)"),
                                TRS("QVGA (320 x 240)"),
                                (char*)0 },
      val_default: { val_str: "from_input" },
      help_string: TRS("Set the output frame size. For a user defined size, you must specify the width and height as well as the pixel width and pixel height (for nonsquare pixels)."),
    },
    {
      name:      "user_image_width",
      long_name: TRS("User defined width"),
      opt:       "w",
      type:      BG_PARAMETER_INT,   
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 640 },
      help_string: TRS("User defined width in pixels. Only meaningful if you selected \"User defined\" for the framesize."),
    },
    {                                       
      name:      "user_image_height",
      long_name: TRS("User defined height"),
      opt:       "h",
      type:      BG_PARAMETER_INT,
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 480 },
      help_string: TRS("User defined height in pixels. Only meaningful if you selected \"User defined\" for the framesize."),
      },
    {
      name:      "user_pixel_width",
      long_name: TRS("User defined pixel width"),
      opt:       "sw",
      type:      BG_PARAMETER_INT,   
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 1 },
      help_string: TRS("User defined pixel width. Only meaningful if you selected \"User defined\" for the framesize."),
    },
    {                                       
      name:      "user_pixel_height",
      long_name: TRS("User defined pixel height"),
      opt:       "sh",
      type:      BG_PARAMETER_INT,
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 1 },
      help_string: TRS("User defined pixel height. Only meaningful if you selected \"User defined\" for the framesize."),
    },
    {
      name: "borders_section",
      long_name: TRS("Image borders"),
      type: BG_PARAMETER_SECTION,
    },
    {
      name:      "maintain_aspect",
      long_name: TRS("Maintain aspect ratio"),
      opt:       "ka",
      type:      BG_PARAMETER_CHECKBUTTON,
      flags:     BG_PARAMETER_SYNC,
      val_default: { val_i: 1 },
      help_string: TRS("Let the aspect ratio appear the same as in the source, probably resulting in additional borders.")
    },
    {
      name:      "border_color",
      long_name: TRS("Border_color"),
      opt:       "bc",
      type:      BG_PARAMETER_COLOR_RGB,
      flags:     BG_PARAMETER_SYNC,
      val_default: { val_color: (float[]){ 0.0, 0.0, 0.0 } },
      help_string: TRS("Color of the image borders.")
    },
    {
      name: "scale_mode_section",
      long_name: TRS("Scale mode"),
      type: BG_PARAMETER_SECTION,
    },
    {
      name:        "scale_mode",
      long_name:   TRS("Scale mode"),
      opt:       "sm",
      type:        BG_PARAMETER_STRINGLIST,
      flags:     BG_PARAMETER_SYNC,
      multi_names: (char*[]){ "auto",\
                              "nearest",         \
                              "bilinear", \
                              "quadratic", \
                              "cubic_bspline", \
                              "cubic_mitchell", \
                              "cubic_catmull", \
                              "sinc_lanczos", \
                              (char*)0 },
      multi_labels: (char*[]){ TRS("Auto"), \
                               TRS("Nearest"),            \
                               TRS("Bilinear"), \
                               TRS("Quadratic"), \
                               TRS("Cubic B-Spline"), \
                               TRS("Cubic Mitchell-Netravali"), \
                               TRS("Cubic Catmull-Rom"), \
                               TRS("Sinc with Lanczos window"), \
                               (char*)0 },
      val_default: { val_str: "auto" },
      help_string: TRS("Choose scaling method. Auto means to choose based on the conversion quality. Nearest is fastest, Sinc with Lanczos window is slowest."),
    },
    {
      name:        "scale_order",
      long_name:   TRS("Scale order"),
      opt:         "so",
      type:        BG_PARAMETER_INT,
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_i: 4 },
      val_max:     { val_i: 1000 },
      val_default: { val_i: 4 },
      help_string: TRS("Order for sinc scaling."),
    },
    {
      name:        "quality",
      long_name:   TRS("Quality"),
      opt:         "q",
      type:        BG_PARAMETER_SLIDER_INT,
      flags:     BG_PARAMETER_SYNC,
      val_min:     { val_i: GAVL_QUALITY_FASTEST },
      val_max:     { val_i: GAVL_QUALITY_BEST    },
      val_default: { val_i: GAVL_QUALITY_DEFAULT },
      help_string: TRS("Lower quality means more speed. Values above 3 enable slow high quality calculations.")
    },
    {
      name: "deinterlace_section",
      long_name: TRS("Deinterlace"),
      type: BG_PARAMETER_SECTION,
    },
    {
      name: "deinterlace",
      long_name: TRS("Deinterlace"),
      type: BG_PARAMETER_STRINGLIST,
      flags:     BG_PARAMETER_SYNC,
      val_default: { val_str: "never" },
      multi_names: (char*[]){ "never", "auto", "always", (char*)0 },
      multi_labels: (char*[]){ TRS("Never"), TRS("Auto"), TRS("Always"), (char*)0 },
    },
    {
      name:      "deinterlace_drop_mode",
      opt:       "ddm",
      long_name: "Drop mode",
      type:      BG_PARAMETER_STRINGLIST,
      flags:     BG_PARAMETER_SYNC,
      val_default: { val_str: "top" },
      multi_names:   (char*[]){ "top", "bottom", (char*)0 },
      multi_labels:  (char*[]){ TRS("Drop top field"), TRS("Drop bottom field"), (char*)0 },
      help_string: TRS("Specifies which field the deinterlacer should drop.")
    },
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_cropscale(void * priv)
  {
  return parameters;
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
#define NUM_FRAME_SIZES           20


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
    { FRAME_SIZE_PAL_CVD,        "pal_cvd"},
    { FRAME_SIZE_PAL_VCD,        "pal_vcd"},
    { FRAME_SIZE_PAL_SVCD,       "pal_svcd"},
    { FRAME_SIZE_PAL_SVCD_WIDE,  "pal_svcd_wide"},
    { FRAME_SIZE_NTSC_D1,        "ntsc_d1"},
    { FRAME_SIZE_NTSC_D1_WIDE,   "ntsc_d1_wide"},
    { FRAME_SIZE_NTSC_DV,        "ntsc_dv"},
    { FRAME_SIZE_NTSC_DV_WIDE,   "ntsc_dv_wide"},
    { FRAME_SIZE_NTSC_CVD,       "ntsc_cvd"},
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
    { FRAME_SIZE_PAL_CVD,          352, 576,   59,   27},
    { FRAME_SIZE_PAL_VCD,          352, 288,   59,   54},
    { FRAME_SIZE_PAL_SVCD,         480, 576,   59,   36},
    { FRAME_SIZE_PAL_SVCD_WIDE,    480, 576,   59,   27},
    { FRAME_SIZE_NTSC_D1,          720, 480,   10,   11},
    { FRAME_SIZE_NTSC_D1_WIDE,     720, 480,   40,   33 },
    { FRAME_SIZE_NTSC_DV,          720, 480,   10,   11 },
    { FRAME_SIZE_NTSC_DV_WIDE,     720, 480,   40,   33 },
    { FRAME_SIZE_NTSC_CVD,         352, 480,   20,   11 },
    { FRAME_SIZE_NTSC_VCD,         352, 240,   10,   11 },
    { FRAME_SIZE_NTSC_SVCD,        480, 480,   15,   11 },
    { FRAME_SIZE_NTSC_SVCD_WIDE,   480, 480,   20,   11 },
    { FRAME_SIZE_VGA,              640, 480,    1,    1 },
    { FRAME_SIZE_QVGA,             320, 240,    1,    1 },
  };

static gavl_scale_mode_t string_to_scale_mode(const char * str)
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

static void set_parameter_cropscale(void * priv, char * name,
                                    bg_parameter_value_t * val)
  {
  cropscale_priv_t * vp;
  int i;
  int new_deinterlace;
  gavl_deinterlace_drop_mode_t new_drop_mode;
  gavl_scale_mode_t new_scale_mode;
  vp = (cropscale_priv_t *)priv;

  if(!name)
    return;

  if(!strcmp(name, "crop_left"))
    {
    if(vp->crop_left != val->val_f)
      {
      vp->need_reinit = 1;
      vp->crop_left = val->val_f;
      }
    }
  else if(!strcmp(name, "crop_right"))
    {
    if(vp->crop_right != val->val_f)
      {
      vp->need_reinit = 1;
      vp->crop_right = val->val_f;
      }
    }
  else if(!strcmp(name, "crop_top"))
    {
    if(vp->crop_top != val->val_f)
      {
      vp->need_reinit = 1;
      vp->crop_top = val->val_f;
      }
    }
  else if(!strcmp(name, "crop_bottom"))
    {
    if(vp->crop_bottom != val->val_f)
      {
      vp->need_reinit = 1;
      vp->crop_bottom = val->val_f;
      }
    }
  else if(!strcmp(name, "frame_size"))
    {
    for(i = 0; i < NUM_FRAME_SIZES; i++)
      {
      if(!strcmp(val->val_str, framesize_strings[i].name))
        {
        if(vp->frame_size != framesize_strings[i].size)
          {
          vp->need_restart = 1;
          vp->frame_size = framesize_strings[i].size;
          }
        break;
        }
      }
    }
  else if(!strcmp(name, "user_image_width"))
    {
    if(vp->user_image_width != val->val_i)
      {
      vp->user_image_width = val->val_i;
      if(vp->frame_size == FRAME_SIZE_USER)
        vp->need_restart = 1;
      }
    }
  else if(!strcmp(name, "user_image_height"))
    {
    if(vp->user_image_height != val->val_i)
      {
      vp->user_image_height = val->val_i;

      if(vp->frame_size == FRAME_SIZE_USER)
        vp->need_restart = 1;
      }
    }
  else if(!strcmp(name, "user_pixel_width"))
    {
    if(vp->user_pixel_width != val->val_i)
      {
      vp->user_pixel_width = val->val_i;
      if(vp->frame_size == FRAME_SIZE_USER)
        vp->need_restart = 1;
      }
    }
  else if(!strcmp(name, "user_pixel_height"))
    {
    if(vp->user_pixel_height != val->val_i)
      {
      vp->user_pixel_height = val->val_i;
      if(vp->frame_size == FRAME_SIZE_USER)
        vp->need_restart = 1;
      }
    }
  else if(!strcmp(name, "maintain_aspect"))
    {
    if(vp->maintain_aspect != val->val_i)
      {
      vp->maintain_aspect = val->val_i;
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "border_color"))
    {
    vp->border_color[0] = val->val_color[0];
    vp->border_color[1] = val->val_color[1];
    vp->border_color[2] = val->val_color[2];
    }
  else if(!strcmp(name, "quality"))
    {
    if(gavl_video_options_get_quality(vp->opt) != val->val_i)
      {
      gavl_video_options_set_quality(vp->opt, val->val_i);
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "scale_mode"))
    {
    new_scale_mode = string_to_scale_mode(val->val_str);
    if(new_scale_mode != gavl_video_options_get_scale_mode(vp->opt))
      {
      gavl_video_options_set_scale_mode(vp->opt, new_scale_mode);
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "scale_order"))
    {
    if(gavl_video_options_get_scale_order(vp->opt) != val->val_i)
      {
      gavl_video_options_set_scale_order(vp->opt, val->val_i);
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "deinterlace"))
    {
    new_deinterlace = DEINTERLACE_NEVER;
    if(!strcmp(val->val_str, "never"))
      new_deinterlace = DEINTERLACE_NEVER;
    else if(!strcmp(val->val_str, "auto"))
      new_deinterlace = DEINTERLACE_AUTO;
    else if(!strcmp(val->val_str, "always"))
      new_deinterlace = DEINTERLACE_ALWAYS;

    if(new_deinterlace != vp->deinterlace)
      {
      vp->deinterlace = new_deinterlace;
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "deinterlace_drop_mode"))
    {
    if(!strcmp(val->val_str, "top"))
      new_drop_mode = GAVL_DEINTERLACE_DROP_TOP;
    else
      new_drop_mode = GAVL_DEINTERLACE_DROP_BOTTOM;

    if(new_drop_mode != gavl_video_options_get_deinterlace_drop_mode(vp->opt))
      {
      gavl_video_options_set_deinterlace_drop_mode(vp->opt, new_drop_mode);
      vp->need_reinit = 1;
      }
    }
  
  }

static void set_framesize(cropscale_priv_t * vp)
  {
  int i;
  
  /* Set image- and pixel size for output */
  
  if(vp->frame_size == FRAME_SIZE_FROM_INPUT)
    {
    vp->out_format.image_width  = vp->in_format.image_width;
    vp->out_format.image_height = vp->in_format.image_height;

    vp->out_format.pixel_width =  vp->in_format.pixel_width;
    vp->out_format.pixel_height = vp->in_format.pixel_height;
    }
  else if(vp->frame_size == FRAME_SIZE_USER)
    {
    vp->out_format.image_width  = vp->user_image_width;
    vp->out_format.image_height = vp->user_image_height;

    vp->out_format.pixel_width =  vp->user_pixel_width;
    vp->out_format.pixel_height = vp->user_pixel_height;
    }
  else
    {
    for(i = 0; i < NUM_FRAME_SIZES; i++)
      {
      if(frame_size_sizes[i].size == vp->frame_size)
        {
        vp->out_format.image_width = frame_size_sizes[i].image_width;
        vp->out_format.image_height = frame_size_sizes[i].image_height;
        
        vp->out_format.pixel_width = frame_size_sizes[i].pixel_width;
        vp->out_format.pixel_height = frame_size_sizes[i].pixel_height;
        
        }
      }
    }
  vp->out_format.frame_width = vp->out_format.image_width;
  vp->out_format.frame_height = vp->out_format.image_height;
  }


static void set_rectangles(cropscale_priv_t * vp)
  {
  gavl_rectangle_f_t in_rect;
  gavl_rectangle_i_t out_rect;
  
  /* Crop input */
  gavl_rectangle_f_set_all(&in_rect, &vp->in_format);

  gavl_rectangle_f_crop_left(&in_rect,   vp->crop_left);
  gavl_rectangle_f_crop_right(&in_rect,  vp->crop_right);
  gavl_rectangle_f_crop_top(&in_rect,    vp->crop_top);
  gavl_rectangle_f_crop_bottom(&in_rect, vp->crop_bottom);
  
  if(vp->maintain_aspect)
    {
    gavl_rectangle_fit_aspect(&out_rect,   // gavl_rectangle_t * r,
                              &vp->in_format,  // gavl_video_format_t * src_format,
                              &in_rect,    // gavl_rectangle_t * src_rect,
                              &vp->out_format, // gavl_video_format_t * dst_format,
                              1.0,        // float zoom,
                              0.0         // float squeeze
                              );
    }
  else
    {
    gavl_rectangle_i_set_all(&out_rect, &vp->out_format);
    }
  
  /* Set rectangles */
  
  gavl_video_options_set_rectangles(vp->opt, &in_rect, &out_rect);
  }




static void connect_input_port_cropscale(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  cropscale_priv_t * vp;
  vp = (cropscale_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  }

static void set_input_format_cropscale(void * priv, gavl_video_format_t * format, int port)
  {
  cropscale_priv_t * vp;
  vp = (cropscale_priv_t *)priv;

  if(!port)
    {
    gavl_video_format_copy(&vp->in_format, format);
    gavl_video_format_copy(&vp->out_format, format);
    set_framesize(vp);
    vp->need_reinit = 1;
    vp->need_restart = 0;
    }
  if(vp->frame)
    {
    gavl_video_frame_destroy(vp->frame);
    vp->frame = (gavl_video_frame_t*)0;
    }
  }

static void get_output_format_cropscale(void * priv, gavl_video_format_t * format)
  {
  cropscale_priv_t * vp;
  vp = (cropscale_priv_t *)priv;
  
  gavl_video_format_copy(format, &vp->out_format);
  }

static int read_video_cropscale(void * priv, gavl_video_frame_t * frame, int stream)
  {
  cropscale_priv_t * vp;
  vp = (cropscale_priv_t *)priv;
  
  if(vp->need_reinit)
    {
    int conversion_flags;
    set_rectangles(vp);

    switch(vp->deinterlace)
      {
      case DEINTERLACE_NEVER:
        gavl_video_options_set_deinterlace_mode(vp->opt, GAVL_DEINTERLACE_NONE);
        break;
      case DEINTERLACE_AUTO:
        gavl_video_options_set_deinterlace_mode(vp->opt, GAVL_DEINTERLACE_SCALE);
        conversion_flags = gavl_video_options_get_conversion_flags(vp->opt);
        conversion_flags &= ~GAVL_FORCE_DEINTERLACE;
        gavl_video_options_set_conversion_flags(vp->opt, conversion_flags);
        break;
      case DEINTERLACE_ALWAYS:
        gavl_video_options_set_deinterlace_mode(vp->opt, GAVL_DEINTERLACE_SCALE);
        conversion_flags = gavl_video_options_get_conversion_flags(vp->opt);
        conversion_flags |= GAVL_FORCE_DEINTERLACE;
        gavl_video_options_set_conversion_flags(vp->opt, conversion_flags);
        break;
      }
    gavl_video_scaler_init(vp->scaler, &vp->in_format, &vp->out_format);
    }

  if(!vp->frame)
    {
    vp->frame = gavl_video_frame_create(&vp->in_format);
    }
  
  if(!vp->read_func(vp->read_data, vp->frame, vp->read_stream))
    return 0;

  if(vp->maintain_aspect)
    gavl_video_frame_fill(frame, &vp->out_format, vp->border_color);
  
  gavl_video_scaler_scale(vp->scaler, vp->frame, frame);
  
  frame->timestamp = vp->frame->timestamp;
  frame->duration = vp->frame->duration;
  return 1;
  }

static int need_restart_cropscale(void * priv)
  {
  cropscale_priv_t * vp;
  vp = (cropscale_priv_t *)priv;
  return vp->need_restart;
  }


bg_fv_plugin_t the_plugin = 
  {
    common:
    {
      BG_LOCALE,
      name:      "fv_cropscale",
      long_name: TRS("Crop & Scale"),
      description: TRS("Crop and Scale video images. Can also do chroma placement correction and simple deinterlacing"),
      type:     BG_PLUGIN_FILTER_VIDEO,
      flags:    BG_PLUGIN_FILTER_1,
      create:   create_cropscale,
      destroy:   destroy_cropscale,
      get_parameters:   get_parameters_cropscale,
      set_parameter:    set_parameter_cropscale,
      priority:         1,
    },
    
    connect_input_port: connect_input_port_cropscale,
    
    set_input_format: set_input_format_cropscale,
    get_output_format: get_output_format_cropscale,

    read_video: read_video_cropscale,
    need_restart: need_restart_cropscale,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
