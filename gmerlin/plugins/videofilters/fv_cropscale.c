/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#define SQUEEZE_MIN -1.0
#define SQUEEZE_MAX 1.0

#define ZOOM_MIN 20.0
#define ZOOM_MAX 180.0


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
  
  float zoom, squeeze;
  
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
  gavl_video_scaler_destroy(vp->scaler);
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "Crop",
      .long_name = TRS("Crop"),
      .type = BG_PARAMETER_SECTION,
    },
    {
      .name =      "crop_left",
      .long_name = TRS("Crop left"),
      .opt =       "crl",
      .type =      BG_PARAMETER_FLOAT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 100000.0 },
      .val_default = { .val_f = 0.0 },
      .num_digits = 3,
      .help_string = TRS("Cut this many pixels from the left border of the source images.")
    },
    {
      .name =      "crop_right",
      .long_name = TRS("Crop right"),
      .opt =       "crr",
      .type =      BG_PARAMETER_FLOAT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 100000.0 },
      .val_default = { .val_f = 0.0 },
      .num_digits = 3,
      .help_string = TRS("Cut this many pixels from the right border of the source images.")
    },
    {
      .name =      "crop_top",
      .long_name = TRS("Crop top"),
      .opt =       "crt",
      .type =      BG_PARAMETER_FLOAT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 100000.0 },
      .val_default = { .val_f = 0.0 },
      .num_digits = 3,
      .help_string = TRS("Cut this many pixels from the top border of the source images.")
    },
    {
      .name =      "crop_bottom",
      .long_name = TRS("Crop bottom"),
      .opt =       "crb",
      .type =      BG_PARAMETER_FLOAT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 100000.0 },
      .val_default = { .val_f = 0.0 },
      .num_digits = 3,
      .help_string = TRS("Cut this many pixels from the bottom border of the source images.")    },
    {
      .name = "frame_size_section",
      .long_name = TRS("Image size"),
      .type = BG_PARAMETER_SECTION,
    },
    {
      .name =        "frame_size",
      .long_name =   TRS("Image size"),
      .opt =         "s",
      .type =        BG_PARAMETER_STRINGLIST,
      .flags =     BG_PARAMETER_SYNC,
      .multi_names = (char const *[]){ "from_input",
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
                              "720",
                              "1080",
                              "vga",
                              "qvga",
                              "sqcif",
                              "qcif",
                              "cif",
                              "4cif",
                              "16cif",
                              (char*)0 },
      .multi_labels =  (char const *[]){ TRS("From Source"),
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
                                TRS("HD 720p/i (1280x720)"),
                                TRS("HD 1080p/i (1920x1080)"),
                                TRS("VGA (640 x 480)"),
                                TRS("QVGA (320 x 240)"),
                                TRS("SQCIF (128 × 96)"),
                                TRS("QCIF (176 × 144)"),
                                TRS("CIF (352 × 288)"),
                                TRS("4CIF (704 × 576)"),
                                TRS("16CIF (1408 × 1152)"),
                                (char*)0 },
      .val_default = { .val_str = "from_input" },
      .help_string = TRS("Set the output image size. For a user defined size, you must specify the width and height as well as the pixel width and pixel height."),
    },
    {
      .name =      "user_image_width",
      .long_name = TRS("User defined width"),
      .opt =       "w",
      .type =      BG_PARAMETER_INT,   
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 640 },
      .help_string = TRS("User defined width in pixels. Only meaningful if you selected \"User defined\" for the image size."),
    },
    {                                       
      .name =      "user_image_height",
      .long_name = TRS("User defined height"),
      .opt =       "h",
      .type =      BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 480 },
      .help_string = TRS("User defined height in pixels. Only meaningful if you selected \"User defined\" for the image size."),
      },
    {
      .name =      "user_pixel_width",
      .long_name = TRS("User defined pixel width"),
      .opt =       "sw",
      .type =      BG_PARAMETER_INT,   
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 1 },
      .help_string = TRS("User defined pixel width. Only meaningful if you selected \"User defined\" for the image size."),
    },
    {                                       
      .name =      "user_pixel_height",
      .long_name = TRS("User defined pixel height"),
      .opt =       "sh",
      .type =      BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 100000 },
      .val_default = { .val_i = 1 },
      .help_string = TRS("User defined pixel height. Only meaningful if you selected \"User defined\" for the image size."),
    },
    {
      .name = "borders_section",
      .long_name = TRS("Image borders"),
      .type = BG_PARAMETER_SECTION,
    },
    {
      .name =      "maintain_aspect",
      .long_name = TRS("Maintain aspect ratio"),
      .opt =       "ka",
      .type =      BG_PARAMETER_CHECKBUTTON,
      .flags =     BG_PARAMETER_SYNC,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Let the aspect ratio appear the same as in the source, probably resulting in additional borders.")
    },
    {
      .name =      "border_color",
      .long_name = TRS("Border color"),
      .opt =       "bc",
      .type =      BG_PARAMETER_COLOR_RGB,
      .flags =     BG_PARAMETER_SYNC,
      .val_default = { .val_color = { 0.0, 0.0, 0.0 } },
      .help_string = TRS("Color of the image borders.")
    },
    {
      BG_LOCALE,
      .name =        "squeeze",
      .long_name =   "Squeeze",
      .type =        BG_PARAMETER_SLIDER_FLOAT,
      .flags =       BG_PARAMETER_SYNC,
      .val_default = { .val_f = 0.0 },
      .val_min =     { .val_f = SQUEEZE_MIN },
      .val_max =     { .val_f = SQUEEZE_MAX },
      .num_digits =  3
    },
    {
      .name =        "zoom",
      .long_name =   "Zoom",
      .type =        BG_PARAMETER_SLIDER_FLOAT,
      .flags =       BG_PARAMETER_SYNC,
      .val_default = { .val_f = 100.0 },
      .val_min =     { .val_f = ZOOM_MIN },
      .val_max =     { .val_f = ZOOM_MAX },
    },
    {
      .name = "scale_mode_section",
      .long_name = TRS("Scale mode"),
      .type = BG_PARAMETER_SECTION,
    },
    {
      .name =        "scale_mode",
      .long_name =   TRS("Scale mode"),
      .opt =       "sm",
      .type =        BG_PARAMETER_STRINGLIST,
      .flags =     BG_PARAMETER_SYNC,
      .multi_names = BG_GAVL_SCALE_MODE_NAMES,
      .multi_labels = BG_GAVL_SCALE_MODE_LABELS,
      .val_default = { .val_str = "auto" },
      .help_string = TRS("Choose scaling method. Auto means to choose based on the conversion quality. Nearest is fastest, Sinc with Lanczos window is slowest."),
    },
    {
      .name =        "scale_order",
      .long_name =   TRS("Scale order"),
      .opt =         "so",
      .type =        BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 4 },
      .val_max =     { .val_i = 1000 },
      .val_default = { .val_i = 4 },
      .help_string = TRS("Order for sinc scaling."),
    },
    {
      .name      =  "downscale_filter",
      .long_name =  "Antialiasing for downscaling",
      .type =        BG_PARAMETER_STRINGLIST,
      .flags =     BG_PARAMETER_SYNC,
      .multi_names = BG_GAVL_DOWNSCALE_FILTER_NAMES,
      .multi_labels = BG_GAVL_DOWNSCALE_FILTER_LABELS,
      .val_default = { .val_str = "auto" },
      .help_string = TRS("Specifies the antialiasing filter to be used when downscaling images."),
    },
    {
      .name      =  "downscale_blur",
      .long_name =  "Blur factor for downscaling",
      .type =        BG_PARAMETER_SLIDER_FLOAT,
      .flags =     BG_PARAMETER_SYNC,
      .val_default = { .val_f = 1.0 },
      .val_min     = { .val_f = 0.0 },
      .val_max     = { .val_f = 2.0 },
      .num_digits  = 2,
      .help_string = TRS("Specifies how much blurring should be applied when downscaling. Smaller values can speed up scaling, but might result in strong aliasing."),
      
    },
    {
      .name =        "quality",
      .long_name =   TRS("Quality"),
      .opt =         "q",
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST    },
      .val_default = { .val_i = GAVL_QUALITY_DEFAULT },
      .help_string = TRS("Lower quality means more speed. Values above 3 enable slow high quality calculations.")
    },
    {
      .name = "deinterlace_section",
      .long_name = TRS("Deinterlace"),
      .type = BG_PARAMETER_SECTION,
    },
    {
      .name = "deinterlace",
      .long_name = TRS("Deinterlace"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags =     BG_PARAMETER_SYNC,
      .val_default = { .val_str = "never" },
      .multi_names = (char const *[]){ "never", "auto", "always", (char*)0 },
      .multi_labels = (char const *[]){ TRS("Never"), TRS("Auto"), TRS("Always"), (char*)0 },
    },
    {
      .name =      "deinterlace_drop_mode",
      .opt =       "ddm",
      .long_name = "Drop mode",
      .type =      BG_PARAMETER_STRINGLIST,
      .flags =     BG_PARAMETER_SYNC,
      .val_default = { .val_str = "top" },
      .multi_names =   (char const *[]){ "top", "bottom", (char*)0 },
      .multi_labels =  (char const *[]){ TRS("Drop top field"), TRS("Drop bottom field"), (char*)0 },
      .help_string = TRS("Specifies which field the deinterlacer should drop.")
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_cropscale(void * priv)
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
    { FRAME_SIZE_720,            "720" }, 
    { FRAME_SIZE_1080,           "1080" }, 
    { FRAME_SIZE_VGA,            "vga"},
    { FRAME_SIZE_QVGA,           "qvga"},
    { FRAME_SIZE_SQCIF,          "sqcif" },
    { FRAME_SIZE_QCIF,           "qcif" },
    { FRAME_SIZE_CIF,            "cif" },
    { FRAME_SIZE_4CIF,           "4cif" },
    { FRAME_SIZE_16CIF,          "16cif" },
  };

static const struct
  {
  int size;
  int image_width;
  int image_height;
  int pixel_width;
  int pixel_height;
  }
frame_size_sizes[NUM_FRAME_SIZES] =
  {
    { FRAME_SIZE_FROM_INPUT,       0,      0,    0,    0 },
    { FRAME_SIZE_USER,             0,      0,    0,    0 },
    { FRAME_SIZE_PAL_D1,           720,  576,   59,   54 },
    { FRAME_SIZE_PAL_D1_WIDE,      720,  576,  118,   81 },
    { FRAME_SIZE_PAL_DV,           720,  576,   59,   54 },
    { FRAME_SIZE_PAL_DV_WIDE,      720,  576,  118,   81 },
    { FRAME_SIZE_PAL_CVD,          352,  576,   59,   27 },
    { FRAME_SIZE_PAL_VCD,          352,  288,   59,   54 },
    { FRAME_SIZE_PAL_SVCD,         480,  576,   59,   36 },
    { FRAME_SIZE_PAL_SVCD_WIDE,    480,  576,   59,   27 },
    { FRAME_SIZE_NTSC_D1,          720,  480,   10,   11 },
    { FRAME_SIZE_NTSC_D1_WIDE,     720,  480,   40,   33 },
    { FRAME_SIZE_NTSC_DV,          720,  480,   10,   11 },
    { FRAME_SIZE_NTSC_DV_WIDE,     720,  480,   40,   33 },
    { FRAME_SIZE_NTSC_CVD,         352,  480,   20,   11 },
    { FRAME_SIZE_NTSC_VCD,         352,  240,   10,   11 },
    { FRAME_SIZE_NTSC_SVCD,        480,  480,   15,   11 },
    { FRAME_SIZE_NTSC_SVCD_WIDE,   480,  480,   20,   11 },
    { FRAME_SIZE_VGA,              640,  480,    1,    1 },
    { FRAME_SIZE_QVGA,             320,  240,    1,    1 },
    { FRAME_SIZE_720,             1280,  720,    1,    1 },
    { FRAME_SIZE_1080,            1920, 1080,    1,    1 },
    { FRAME_SIZE_SQCIF,            128,   96,   12,   11 },
    { FRAME_SIZE_QCIF,             176,  144,   12,   11 },
    { FRAME_SIZE_CIF,              352,  288,   12,   11 },
    { FRAME_SIZE_4CIF,             704,  576,   12,   11 },
    { FRAME_SIZE_16CIF,           1408, 1152,   12,   11 },
  };

static void set_parameter_cropscale(void * priv, const char * name,
                                    const bg_parameter_value_t * val)
  {
  cropscale_priv_t * vp;
  int i;
  int new_deinterlace;
  gavl_deinterlace_drop_mode_t new_drop_mode;
  gavl_scale_mode_t new_scale_mode;
  gavl_downscale_filter_t new_downscale_filter;
  
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
  else if(!strcmp(name, "squeeze"))
    {
    if(vp->squeeze != val->val_f)
      {
      vp->squeeze = val->val_f;
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "zoom"))
    {
    if(vp->zoom != val->val_f)
      {
      vp->zoom = val->val_f;
      vp->need_reinit = 1;
      }
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
    new_scale_mode = bg_gavl_string_to_scale_mode(val->val_str);
    if(new_scale_mode != gavl_video_options_get_scale_mode(vp->opt))
      {
      gavl_video_options_set_scale_mode(vp->opt, new_scale_mode);
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "downscale_filter"))
    {
    new_downscale_filter = bg_gavl_string_to_downscale_filter(val->val_str);
    if(new_downscale_filter != gavl_video_options_get_downscale_filter(vp->opt))
      {
      gavl_video_options_set_downscale_filter(vp->opt, new_downscale_filter);
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "downscale_blur"))
    {
    if(val->val_f != gavl_video_options_get_downscale_blur(vp->opt))
      {
      gavl_video_options_set_downscale_blur(vp->opt, val->val_f);
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
  float zoom_factor;
  gavl_rectangle_f_t in_rect;
  gavl_rectangle_i_t out_rect;
  
  /* Crop input */
  gavl_rectangle_f_set_all(&in_rect, &vp->in_format);

  gavl_rectangle_f_crop_left(&in_rect,   vp->crop_left);
  gavl_rectangle_f_crop_right(&in_rect,  vp->crop_right);
  gavl_rectangle_f_crop_top(&in_rect,    vp->crop_top);
  gavl_rectangle_f_crop_bottom(&in_rect, vp->crop_bottom);

  zoom_factor = vp->zoom * 0.01;
  
  if(vp->maintain_aspect)
    {
    gavl_rectangle_fit_aspect(&out_rect,   // gavl_rectangle_t * r,
                              &vp->in_format,  // gavl_video_format_t * src_format,
                              &in_rect,    // gavl_rectangle_t * src_rect,
                              &vp->out_format, // gavl_video_format_t * dst_format,
                              zoom_factor,        // float zoom,
                              vp->squeeze          // float squeeze
                              );
    gavl_rectangle_crop_to_format_scale(&in_rect,
                                        &out_rect,
                                        &vp->in_format,
                                        &vp->out_format);
    
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

    if(vp->deinterlace != DEINTERLACE_NEVER)
      vp->out_format.interlace_mode = GAVL_INTERLACE_NONE;
    
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
    vp->need_reinit = 0;
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
  
  gavl_video_frame_copy_metadata(frame, vp->frame);
  return 1;
  }

static int need_restart_cropscale(void * priv)
  {
  cropscale_priv_t * vp;
  vp = (cropscale_priv_t *)priv;
  return vp->need_restart;
  }


const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_cropscale",
      .long_name = TRS("Crop & Scale"),
      .description = TRS("Crop and scale video images. Has lots of standard video formats as presets. Can also do chroma placement correction and simple deinterlacing"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_cropscale,
      .destroy =   destroy_cropscale,
      .get_parameters =   get_parameters_cropscale,
      .set_parameter =    set_parameter_cropscale,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_cropscale,
    
    .set_input_format = set_input_format_cropscale,
    .get_output_format = get_output_format_cropscale,

    .read_video = read_video_cropscale,
    .need_restart = need_restart_cropscale,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
