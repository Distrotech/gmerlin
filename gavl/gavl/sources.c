/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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


#include <gavl/gavl.h>

struct gavl_video_source_s
  {
  gavl_video_format_t src_format;
  gavl_video_format_t dst_format;
  int src_flags;
  int dst_flags;

  gavl_video_source_func_t func;
  gavl_video_converter_t * cnv;
  int do_convert;
  
  void * priv;
  int stream;

  gavl_video_frame_pool_t * fp;
  };

gavl_video_source_t *
gavl_video_source_create(gavl_video_source_func_t func,
                         void * priv, int stream,
                         int src_flags,
                         const gavl_video_format_t * src_format)
  {
  gavl_video_source_t * ret = calloc(1, sizeof(*ret));

  ret->func = func;
  ret->priv = priv;
  ret->stream = stream;
  ret->src_flags = src_flags;
  gavl_video_format_copy(&ret->src_format, src_format);
  ret->cnv = gavl_video_converter_create();
  
  return ret;
  }

/* Called by the destination */

const gavl_video_format_t *
gavl_video_source_get_src_format(gavl_video_source_t * s)
  {
  return &s->src_format;
  }
  
const gavl_video_format_t *
gavl_video_source_get_dst_format(gavl_video_source_t * s)
  {
  return &s->dst_format;
  }

gavl_video_options_t * gavl_video_source_get_options(gavl_video_source_t * s)
  {
  return gavl_video_converter_get_options(s->cnv);
  }

void gavl_video_source_set_dst(gavl_video_source_t * s, int dst_flags,
                               const gavl_video_format_t * dst_format)
  {
  s->dst_flags = dst_flags;
  gavl_video_format_copy(&s->dst_format, dst_format);

  s->do_convert =
    gavl_video_converter_init(s->cnv, &s->src_format, &s->dst_format);
  }

int gavl_video_source_read_frame(void * sp, int stream,
                                 gavl_video_frame_t ** frame)
  {
  gavl_video_source_t * s = sp;

  if(*frame)
    {
    /* Need to copy into this frame */
    if(s->src_flags & GAVL_SOURCE_ALLOC)
      {
      gavl_video_frame_t * in_frame;
      if(!s->func(s->priv, &in_frame, s->stream))
        return 0;
      gavl_video_frame_copy(&s->dst_format, *frame, in_frame);
      }
    else
      {
      /* Pass through */
      if(!s->func(s->priv, frame, s->stream))
        return 0;
      }
    }
  else
    {
    if(s->src_flags & GAVL_SOURCE_ALLOC)
      {
      /* Pass through */
      if(!s->func(s->priv, frame, s->stream))
        return 0;
      }
    else
      {
      /* Need to take one of our frames */
      gavl_video_frame_t * out_frame;

      if(!s->fp)
        s->fp = gavl_video_frame_pool_create(NULL, &s->dst_format);

      out_frame = gavl_video_frame_pool_get(s->fp);
      
      *frame = out_frame;
      if(!s->func(s->priv, frame, s->stream))
        return 0;
      }
    }
  return 1;
  }

