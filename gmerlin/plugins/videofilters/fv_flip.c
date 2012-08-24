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

#define LOG_DOMAIN "fv_flip"

typedef struct
  {
  int flip_h;
  int flip_v;
  
  gavl_video_format_t format;

  gavl_video_source_t * in_src;
  } flip_priv_t;

static void * create_flip()
  {
  flip_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_flip(void * priv)
  {
  flip_priv_t * vp;
  vp = priv;
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "flip_h",
      .long_name = TRS("Flip horizontally"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
    },
    {
      .name = "flip_v",
      .long_name = TRS("Flip vertically"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_flip(void * priv)
  {
  return parameters;
  }

static void set_parameter_flip(void * priv, const char * name,
                               const bg_parameter_value_t * val)
  {
  flip_priv_t * vp;
  vp = priv;

  if(!name)
    return;

  if(!strcmp(name, "flip_h"))
    vp->flip_h = val->val_i;
  else if(!strcmp(name, "flip_v"))
    vp->flip_v = val->val_i;
  }

static gavl_source_status_t
read_func(void * priv, gavl_video_frame_t ** frame)
  {
  flip_priv_t * vp;
  gavl_source_status_t st;

  gavl_video_frame_t * in_frame = NULL;
  vp = priv;

  if(!vp->flip_h && !vp->flip_v)
    return gavl_video_source_read_frame(vp->in_src, frame);

  if((st = gavl_video_source_read_frame(vp->in_src, &in_frame)) !=
     GAVL_SOURCE_OK)
    return st;
  
  if(vp->flip_h)
    {
    if(vp->flip_v)
      gavl_video_frame_copy_flip_xy(&vp->format, *frame, in_frame);
    else
      gavl_video_frame_copy_flip_x(&vp->format, *frame, in_frame);
    }
  else /* Flip y */
    gavl_video_frame_copy_flip_y(&vp->format, *frame, in_frame);
  gavl_video_frame_copy_metadata(*frame, in_frame);
  return GAVL_SOURCE_OK;
  }

static gavl_video_source_t *
connect_flip(void * priv, gavl_video_source_t * src,
             const gavl_video_options_t * opt)
  {
  flip_priv_t * vp;
  vp = priv;

  vp->in_src = src;
  gavl_video_format_copy(&vp->format,
                         gavl_video_source_get_src_format(vp->in_src));
  if(opt)
    gavl_video_options_copy(gavl_video_source_get_options(vp->in_src), opt);

  gavl_video_source_set_dst(vp->in_src, 0, &vp->format);
  
  return gavl_video_source_create(read_func,
                                   vp, 0,
                                   &vp->format);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_flip",
      .long_name = TRS("Flip image"),
      .description = TRS("Flip video images horizontally and/or vertically"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_flip,
      .destroy =   destroy_flip,
      .get_parameters =   get_parameters_flip,
      .set_parameter =    set_parameter_flip,
      .priority =         1,
    },

    .connect = connect_flip,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
