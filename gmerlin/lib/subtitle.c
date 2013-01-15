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

#include <config.h>

#include <stdlib.h>

#include <gavl/gavl.h>
#include <gavl/connectors.h>

#include <gmerlin/subtitle.h>

struct bg_subtitle_handler_s
  {
  gavl_video_format_t video_format;
  
  gavl_video_source_t * src;
  gavl_video_sink_t   * sink;

  gavl_video_frame_t * cur;
  gavl_video_frame_t * next;

  gavl_video_frame_t * ovl[2];

  int dummy;	
  };

bg_subtitle_handler_t * bg_subtitle_handler_create()
  {
  bg_subtitle_handler_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_subtitle_handler_destroy(bg_subtitle_handler_t * h)
  {
  free(h);
  }

void bg_subtitle_handler_init(bg_subtitle_handler_t * h,
                              gavl_video_format_t * video_format,
                              gavl_video_source_t * src,
                              gavl_video_sink_t * sink)
  {
  const gavl_video_format_t * ovl_format;
  
  h->src = src;
  h->sink = sink;

  if(!h->src || !h->sink)
    return;

  ovl_format = gavl_video_sink_get_format(h->sink);
  
  gavl_video_source_set_dst(h->src, 0, ovl_format);
  
  gavl_video_format_copy(&h->video_format, video_format);

  h->ovl[0] = gavl_video_frame_create(ovl_format);
  h->ovl[1] = gavl_video_frame_create(ovl_format);
  h->cur = h->ovl[0];
  h->next = h->ovl[1];
  }

void bg_subtitle_handler_update(bg_subtitle_handler_t * h,
                                const gavl_video_frame_t * frame)
  {
  gavl_source_status_t st;
  /* Check if the subtitle is expired */
  
  }

void bg_subtitle_handler_reset(bg_subtitle_handler_t * h)
  {
  
  }
