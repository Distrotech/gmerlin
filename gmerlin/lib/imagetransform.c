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

#include <parameter.h>
#include <imagetransform.h>

struct bg_image_transform_s
  {
  gavl_video_format_t format;
  int need_restart;
  };


bg_image_transform_t * bg_image_transform_create()
  {
  bg_image_transform_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_image_transform_destroy(bg_image_transform_t * t)
  {
  free(t);
  }

/* Format can get changed */

void bg_image_transform_init(bg_image_transform_t * t,
                             gavl_video_format_t * format,
                             bg_image_transform_func func, void * priv)
  {

  }

static const bg_parameter_info_t parameters[] =
  {
    { /* End of parameters */ },
  };

const bg_parameter_info_t * bg_image_transform_get_parameters(bg_image_transform_t * t)
  {
  return parameters;
  }

void bg_image_transform_set_parameter(void * priv, const char * name,
                                      const bg_parameter_value_t * val)
  {
  bg_image_transform_t * t = (bg_image_transform_t *)priv;
  
  }

void bg_image_transform_transform(bg_image_transform_t * t,
                                  gavl_video_frame_t * in_frame,
                                  gavl_video_frame_t * out_frame)
  {

  }

int bg_image_transform_need_restart(bg_image_transform_t * t)
  {
  return t->need_restart;
  }

