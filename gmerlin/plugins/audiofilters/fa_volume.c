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

#define LOG_DOMAIN "fa_volume"

typedef struct
  {
  gavl_volume_control_t * vc;

  bg_read_audio_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_audio_format_t format;
  } volume_priv_t;

static void * create_volume()
  {
  volume_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->vc = gavl_volume_control_create();
  return ret;
  }

static void destroy_volume(void * priv)
  {
  volume_priv_t * vp;
  vp = (volume_priv_t *)priv;

  gavl_volume_control_destroy(vp->vc);
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "volume",
      .long_name = TRS("Volume"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .num_digits = 2,
      .val_min = { .val_f = -90.0 },
      .val_max = { .val_f =  20.0 },
      .val_default = { .val_f = 0.0 },
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_volume(void * priv)
  {
  return parameters;
  }

static void set_parameter_volume(void * priv, const char * name,
                                 const bg_parameter_value_t * val)
  {
  volume_priv_t * vp;
  vp = (volume_priv_t *)priv;

  if(!name)
    return;

  
  if(!strcmp(name, "volume"))
    {
    gavl_volume_control_set_volume(vp->vc, val->val_f);
    }
  }

static void connect_input_port_volume(void * priv,
                                      bg_read_audio_func_t func,
                                      void * data, int stream, int port)
  {
  volume_priv_t * vp;
  vp = (volume_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_volume(void * priv, gavl_audio_format_t * format, int port)
  {
  volume_priv_t * vp;
  vp = (volume_priv_t *)priv;

  if(!port)
    {
    gavl_audio_format_copy(&vp->format, format);
    gavl_volume_control_set_format(vp->vc, format);
    }
  }

static void get_output_format_volume(void * priv, gavl_audio_format_t * format)
  {
  volume_priv_t * vp;
  vp = (volume_priv_t *)priv;
  
  gavl_audio_format_copy(format, &vp->format);
  }

static int read_audio_volume(void * priv, gavl_audio_frame_t * frame, int stream, int num_samples)
  {
  volume_priv_t * vp;
  vp = (volume_priv_t *)priv;

  vp->read_func(vp->read_data, frame, vp->read_stream, num_samples);
  gavl_volume_control_apply(vp->vc, frame);
  return frame->valid_samples;
  }

const bg_fa_plugin_t the_plugin = 
  {
    .common = //!< Infos and functions common to all plugin types
    {
      BG_LOCALE,
      .name =      "fa_volume",
      .long_name = TRS("Volume control"),
      .description = TRS("Simple volume control"),
      .type =     BG_PLUGIN_FILTER_AUDIO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_volume,
      .destroy =   destroy_volume,
      .get_parameters =   get_parameters_volume,
      .set_parameter =    set_parameter_volume,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_volume,
    
    .set_input_format = set_input_format_volume,
    .get_output_format = get_output_format_volume,

    .read_audio = read_audio_volume,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
