/*****************************************************************
 * gavl - a general purpose audio/video processing library
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
#include <gavl/connectors.h>

#define FLAG_GET_CALLED (1<<0)

struct gavl_video_sink_s
  {
  gavl_video_sink_get_func get_func;
  gavl_video_sink_put_func put_func;
  void * priv;
  gavl_video_format_t format;
  int flags;
  };

gavl_video_sink_t *
gavl_video_sink_create(gavl_video_sink_get_func get_func,
                       gavl_video_sink_put_func put_func,
                       void * priv,
                       const gavl_video_format_t * format)
  {
  gavl_video_sink_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->get_func = get_func;
  ret->put_func = put_func;
  ret->priv = priv;
  gavl_video_format_copy(&ret->format, format);
  return ret;
  }

const gavl_video_format_t *
gavl_video_sink_get_format(gavl_video_sink_t * s)
  {
  return &s->format;
  }

gavl_video_frame_t *
gavl_video_sink_get_frame(gavl_video_sink_t * s)
  {
  s->flags |= FLAG_GET_CALLED;
  if(s->get_func)
    return s->get_func(s->priv);
  else
    return NULL;
  }

gavl_sink_status_t
gavl_video_sink_put_frame(gavl_video_sink_t * s,
                          gavl_video_frame_t * f)
  {
  gavl_video_frame_t * df;

  if(!(s->flags & FLAG_GET_CALLED) &&
     s->get_func &&
     (df = s->get_func(s->priv)))
    {
    gavl_video_frame_copy(&s->format, df, f);
    gavl_video_frame_copy_metadata(df, f);
    return s->put_func(s->priv, df);
    }
  else
    return s->put_func(s->priv, f);
  }

void
gavl_video_sink_destroy(gavl_video_sink_t * s)
  {
  free(s);
  }
