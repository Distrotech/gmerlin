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

#define LOG_DOMAIN "fv_tctweak"

typedef struct
  {
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;
  
  } tc_priv_t;

static void * create_tctweak()
  {
  tc_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_tctweak(void * priv)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .multi_names = (char*[]){ TRS("interpolate"),
                                TRS("add"),
                                TRS("delete"),
                                (char*)0 },
      .multi_labels = (char*[]){ TRS("Interpolate existing"),
                                TRS("Add new"),
                                TRS("Erase"),
                                (char*)0 },
    },
    {
      .name = "zoom_v",
      .long_name = TRS("Zoom vertically"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .num_digits = 2,
      .val_min = { .val_f = 0.5 },
      .val_max = { .val_f = 2.0 },
      .val_default = { .val_f = 1.0 },

    },
    {
    .name =        "scale_mode",
    .long_name =   TRS("Scale mode"),
    .opt =         "sm",
    .type =        BG_PARAMETER_STRINGLIST,
    .flags = BG_PARAMETER_SYNC,
    .multi_names = BG_GAVL_SCALE_MODE_NAMES,
    .multi_labels = BG_GAVL_SCALE_MODE_LABELS,
    .val_default = { .val_str = "auto" },
    .help_string = TRS("Choose scaling method. Auto means to choose based on the conversion quality. Nearest is fastest, Sinc with Lanczos window is slowest."),
    },
    {
      .name =        "scale_order",
      .long_name =   TRS("Scale order"),
      .opt =       "so",
      .type =        BG_PARAMETER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 4 },
      .val_max =     { .val_i = 1000 },
      .val_default = { .val_i = 4 },
      .help_string = TRS("Order for sinc scaling."),
    },
    {
      .name = "quality",
      .long_name = TRS("Quality"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST },
      .val_default = { .val_i = GAVL_QUALITY_DEFAULT },
    },
    {
      .name = "bg_color",
      .long_name = TRS("Background color"),
      .type = BG_PARAMETER_COLOR_RGBA,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_color = { 0.0, 0.0, 0.0, 1.0 } },
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_tctweak(void * priv)
  {
  return parameters;
  }

static void
set_parameter_tctweak(void * priv, const char * name,
                      const bg_parameter_value_t * val)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;

  if(!name)
    return;

  }

static void connect_input_port_tctweak(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_tctweak(void * priv,
                                gavl_video_format_t * format, int port)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;
  
  if(!port)
    gavl_video_format_copy(&vp->format, format);
  }

static void get_output_format_tctweak(void * priv,
                                 gavl_video_format_t * format)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;
  
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_tctweak(void * priv, gavl_video_frame_t * frame,
                         int stream)
  {
  tc_priv_t * vp;
  vp = (tc_priv_t *)priv;

  if(!vp->read_func(vp->read_data, frame, vp->read_stream))
    return 0;
  
  return 1;
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_tctweak",
      .long_name = TRS("Tweak timecodes"),
      .description = TRS("Add/Remove/Interpolate timecodes"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_tctweak,
      .destroy =   destroy_tctweak,
      .get_parameters =   get_parameters_tctweak,
      .set_parameter =    set_parameter_tctweak,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_tctweak,
    
    .set_input_format = set_input_format_tctweak,
    .get_output_format = get_output_format_tctweak,

    .read_video = read_video_tctweak,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
