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
#include <string.h>

#include <gavl/gavl.h>
#include <gavl/connectors.h>

#include <gmerlin/subtitle.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "subtitle"

// #define DUMP_SUBTITLE

struct bg_subtitle_handler_s
  {
  gavl_video_format_t video_format;
  gavl_video_format_t ovl_format;
  
  gavl_video_source_t * src;
  gavl_video_sink_t   * sink;

  gavl_video_frame_t * cur;
  gavl_video_frame_t * next;

  gavl_video_frame_t * ovl[2];

//  gavl_video_frame_t * out_ovl;
  
  int eof;
  int active; // Current subtitle is active
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
  if(h->ovl[0])
    gavl_video_frame_destroy(h->ovl[0]);
  if(h->ovl[1])
    gavl_video_frame_destroy(h->ovl[1]);
 
  h->src = src;
  h->sink = sink;

  if(!h->src || !h->sink)
    return;

  gavl_video_format_copy(&h->ovl_format,
                         gavl_video_sink_get_format(h->sink));
  
  gavl_video_source_set_dst(h->src, 0, &h->ovl_format);
  
  gavl_video_format_copy(&h->video_format, video_format);

  h->ovl[0] = gavl_video_frame_create(&h->ovl_format);
  h->ovl[1] = gavl_video_frame_create(&h->ovl_format);
  h->cur = h->ovl[0];
  h->next = h->ovl[1];

  bg_subtitle_handler_reset(h);
  }

static void read_subtitle(bg_subtitle_handler_t * h,
                          gavl_video_frame_t * frame)
  {
  gavl_source_status_t st;
  if((frame->src_rect.w > 0) || h->eof)
    return;
  
  st = gavl_video_source_read_frame(h->src, &frame);
  if(st == GAVL_SOURCE_EOF)
    {
    h->eof = 1;
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Subtitle stream finished");
    }
  if(st == GAVL_SOURCE_OK)
    {
#ifdef DUMP_SUBTITLE
    bg_dprintf("Got subtitle\n");
    gavl_video_frame_dump_metadata(gavl_video_source_get_dst_format(h->src),
                                   frame);
#endif
    }
  }

static void put_overlay(bg_subtitle_handler_t * h)
  {
  h->active = 1;
  gavl_video_sink_put_frame(h->sink, h->cur);
  }

static void clear_overlay(bg_subtitle_handler_t * h)
  {
  h->active = 0;
  h->cur->src_rect.w = 0;
  gavl_video_sink_put_frame(h->sink, h->cur);
  }

void bg_subtitle_handler_update(bg_subtitle_handler_t * h,
                                const gavl_video_frame_t * frame)
  {
  gavl_video_frame_t * swp;
  gavl_time_t frame_start;
  gavl_time_t frame_end;
  gavl_time_t overlay_start;
  gavl_time_t overlay_end;
  
  frame_start = gavl_time_unscale(h->video_format.timescale, frame->timestamp);
  frame_end = gavl_time_unscale(h->video_format.timescale, frame->timestamp + frame->duration);
  
  /* Check if the subtitle is expired */
  if(h->active && (h->cur->duration > 0))
    {
    overlay_end = gavl_time_unscale(h->ovl_format.timescale,
                                    h->cur->timestamp + h->cur->duration);
    if(frame_start > overlay_end)
      {
      clear_overlay(h);
      
      /* Swap */
      swp = h->cur;
      h->cur = h->next;
      h->next = swp;
      }
    }

  /* Read as many subtitles as possible */
  read_subtitle(h, h->cur);
  read_subtitle(h, h->next);

  /* Check if the current subtitle became valid */
  if(!h->active && h->cur->src_rect.w)
    {
    overlay_start = gavl_time_unscale(h->ovl_format.timescale,
                                      h->cur->timestamp);

    if(overlay_start < frame_end)
      put_overlay(h);
    }
  /* Check if the next subtitle became valid */
  else if(h->next->src_rect.w)
    {
    overlay_start = gavl_time_unscale(h->ovl_format.timescale,
                                      h->next->timestamp);
    if(overlay_start < frame_end)
      {
      h->cur->src_rect.w = 0;
      
      /* Swap */
      swp = h->cur;
      h->cur = h->next;
      h->next = swp;
      
      put_overlay(h);
      }
    }
  }

void bg_subtitle_handler_reset(bg_subtitle_handler_t * h)
  {
  h->cur->src_rect.w = 0;
  h->cur->src_rect.h = 0;

  h->next->src_rect.w = 0;
  h->next->src_rect.h = 0;
  h->active = 0;
  h->eof = 0;
  }
