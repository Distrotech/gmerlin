/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
#include <stdio.h>
#include <string.h>

#include <gavl/gavl.h>
#include <video.h>
#include <blend.h>

gavl_overlay_blend_context_t * gavl_overlay_blend_context_create()
  {
  gavl_overlay_blend_context_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->ovl_win = gavl_video_frame_create((gavl_video_format_t*)0);
  ret->dst_win = gavl_video_frame_create((gavl_video_format_t*)0);
  
  gavl_video_options_set_defaults(&ret->opt);
  
  return ret;
  }

void gavl_overlay_blend_context_destroy(gavl_overlay_blend_context_t * ctx)
  {
  gavl_video_frame_null(ctx->dst_win);
  gavl_video_frame_destroy(ctx->dst_win);
  
  if(ctx->ovl_win)
    {
    gavl_video_frame_null(ctx->ovl_win);
    gavl_video_frame_destroy(ctx->ovl_win);
    }
  
  free(ctx);
  }

gavl_video_options_t *
gavl_overlay_blend_context_get_options(gavl_overlay_blend_context_t * ctx)
  {
  return &(ctx->opt);
  }

int
gavl_overlay_blend_context_init(gavl_overlay_blend_context_t * ctx,
                                const gavl_video_format_t * dst_format,
                                gavl_video_format_t * ovl_format)
  {
  /* Clean up from previous initializations */

  if(ctx->ovl_win)
    {
    gavl_video_frame_null(ctx->ovl_win);
    gavl_video_frame_destroy(ctx->ovl_win);
    ctx->ovl_win = (gavl_video_frame_t*)0;
    ctx->has_overlay = 0;
    }
  
  /* Check for non alpha capable overlay format */

  if(!gavl_pixelformat_has_alpha(ovl_format->pixelformat))
    return 0;
  
  /* Copy formats */
  gavl_video_format_copy(&(ctx->dst_format), dst_format);
  gavl_video_format_copy(&(ctx->ovl_format), ovl_format);

  /* Get chroma subsampling of the destination */
  gavl_pixelformat_chroma_sub(dst_format->pixelformat,
                              &(ctx->dst_sub_h), &(ctx->dst_sub_v));

  /* Get blend function */

  ctx->func = 
    gavl_find_blend_func_c(ctx,
                           dst_format->pixelformat,
                           &(ctx->ovl_format.pixelformat));
  
  ctx->ovl_win = gavl_video_frame_create((gavl_video_format_t*)0);
  gavl_video_format_copy(ovl_format, &(ctx->ovl_format));
  return 1;
  }

void gavl_overlay_blend_context_set_overlay(gavl_overlay_blend_context_t * ctx,
                                            gavl_overlay_t * ovl)
  {
  int diff;
  /* Save overlay */

  if(!ovl)
    {
    ctx->has_overlay = 0;
    return;
    }
  ctx->has_overlay = 1;
  memcpy(&ctx->ovl, ovl, sizeof(ctx->ovl));

  /* Crop rectangle to destination format */

  if(ctx->ovl.dst_x < 0)
    {
    ctx->ovl.ovl_rect.w += ctx->ovl.dst_x;
    ctx->ovl.ovl_rect.x -= ctx->ovl.dst_x;
    ctx->ovl.dst_x = 0;
    }

  if(ctx->ovl.dst_y < 0)
    {
    ctx->ovl.ovl_rect.h += ctx->ovl.dst_y;
    ctx->ovl.ovl_rect.y -= ctx->ovl.dst_y;
    ctx->ovl.dst_y = 0;
    }
  
  diff = ctx->ovl.dst_x + ctx->ovl.ovl_rect.w - ctx->dst_format.image_width;
  if(diff > 0)
    ctx->ovl.ovl_rect.w -= diff;

  diff = ctx->ovl.dst_y + ctx->ovl.ovl_rect.h - ctx->dst_format.image_height;
  if(diff > 0)
    ctx->ovl.ovl_rect.h -= diff;

  /* Crop rectangle to source format */

  if(ctx->ovl.ovl_rect.x < 0)
    {
    ctx->ovl.ovl_rect.w += ctx->ovl.ovl_rect.x;
    ctx->ovl.dst_x -= ctx->ovl.ovl_rect.x;
    ctx->ovl.ovl_rect.x = 0;
    }

  if(ctx->ovl.ovl_rect.y < 0)
    {
    ctx->ovl.ovl_rect.h += ctx->ovl.ovl_rect.y;
    ctx->ovl.dst_y -= ctx->ovl.ovl_rect.y;
    ctx->ovl.ovl_rect.y = 0;
    }

  diff = ctx->ovl.ovl_rect.x + ctx->ovl.ovl_rect.w - ctx->ovl_format.image_width;
  if(diff > 0)
    ctx->ovl.ovl_rect.w -= diff;

  diff = ctx->ovl.ovl_rect.y + ctx->ovl.ovl_rect.h - ctx->ovl_format.image_height;
  if(diff > 0)
    ctx->ovl.ovl_rect.h -= diff;

  /* Align rectangle */

  ctx->ovl.ovl_rect.w -= ctx->ovl.ovl_rect.w % ctx->dst_sub_h;
  ctx->ovl.ovl_rect.h -= ctx->ovl.ovl_rect.h % ctx->dst_sub_v;
  ctx->ovl.dst_x      -= ctx->ovl.dst_x % ctx->dst_sub_h;
  ctx->ovl.dst_y      -= ctx->ovl.dst_y % ctx->dst_sub_v;

  /* Set destination rectangle for getting the subframe later on */

  ctx->dst_rect.x = ctx->ovl.dst_x;
  ctx->dst_rect.y = ctx->ovl.dst_y;

  ctx->dst_rect.w = ctx->ovl.ovl_rect.w;
  ctx->dst_rect.h = ctx->ovl.ovl_rect.h;
    
  gavl_video_frame_get_subframe(ctx->ovl_format.pixelformat,
                                ovl->frame,
                                ctx->ovl_win,
                                &(ctx->ovl.ovl_rect));
  }

void gavl_overlay_blend(gavl_overlay_blend_context_t * ctx,
                        gavl_video_frame_t * dst_frame)
  {
  if(!ctx->has_overlay)
    return;
  /* Get subframe from destination */
  
  gavl_video_frame_get_subframe(ctx->dst_format.pixelformat,
                                dst_frame,
                                ctx->dst_win,
                                &(ctx->dst_rect));
  /* Fire up blender */

  ctx->func(ctx, ctx->dst_win, ctx->ovl_win);
  }

