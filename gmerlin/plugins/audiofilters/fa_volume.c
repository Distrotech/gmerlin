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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "fa_volume"

typedef struct
  {
  gavl_volume_control_t * vc;

  bg_read_audio_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_audio_format_t format;

  gavl_audio_source_t * in_src;
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
  vp = priv;

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
  vp = priv;

  if(!name)
    return;

  
  if(!strcmp(name, "volume"))
    {
    gavl_volume_control_set_volume(vp->vc, val->val_f);
    }
  }

static gavl_source_status_t read_func(void * priv, gavl_audio_frame_t ** frame)
  {
  gavl_source_status_t st;
  volume_priv_t * vp;
  vp = priv;

  if((st = gavl_audio_source_read_frame(vp->in_src, frame)) !=
     GAVL_SOURCE_OK)
    return st;
  gavl_volume_control_apply(vp->vc, *frame);
  return GAVL_SOURCE_OK;
  }

static gavl_audio_source_t * connect_volume(void * priv,
                                            gavl_audio_source_t * src,
                                            const gavl_audio_options_t * opt)
  {
  const gavl_audio_format_t * format;
  volume_priv_t * vp;
  vp = priv;
  vp->in_src = src;

  format = gavl_audio_source_get_src_format(vp->in_src);
  
  gavl_volume_control_set_format(vp->vc, format);

  if(opt)
    gavl_audio_options_copy(gavl_audio_source_get_options(vp->in_src), opt);

  gavl_audio_source_set_dst(vp->in_src, 0, format);
  
  return gavl_audio_source_create(read_func,
                                  vp, 0,
                                  &vp->format);
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
    
    .connect = connect_volume,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
