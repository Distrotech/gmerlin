/*****************************************************************

  blend.c

  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

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
  
  ret->cnv = calloc(1, sizeof(*ret->cnv));
  gavl_video_options_set_defaults(&ret->opt);
  
  return ret;
  }

void gavl_overlay_blend_context_destroy(gavl_overlay_blend_context_t * ctx)
  {
  if(ctx->cnv->input_frame)
    {
    gavl_video_frame_null(ctx->cnv->input_frame);
    gavl_video_frame_destroy(ctx->cnv->input_frame);
    }
  
  gavl_video_frame_null(ctx->dst_win);
  gavl_video_frame_destroy(ctx->dst_win);

  if(ctx->ovl_win)
    {
    if(!ctx->do_convert)
      gavl_video_frame_null(ctx->ovl_win);
    gavl_video_frame_destroy(ctx->ovl_win);
    }
  
  free(ctx->cnv);
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
                                const gavl_video_format_t * ovl_format)
  {
  /* Clean up from previous initializations */

  if(ctx->ovl_win)
    {
    if(!ctx->do_convert)
      gavl_video_frame_null(ctx->ovl_win);
    gavl_video_frame_destroy(ctx->ovl_win);
    ctx->ovl_win = (gavl_video_frame_t*)0;
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

  /* Check whether to convert the pixelformat */
  
  if(ovl_format->pixelformat !=
     ctx->ovl_format.pixelformat) /* Need pixelformat conversion */
    {
    ctx->do_convert = 1;

    /* Copy formats to the convert context */
    gavl_video_format_copy(&ctx->cnv->input_format, ovl_format);
    gavl_video_format_copy(&ctx->cnv->output_format,
                           &ctx->ovl_format);

    /*
     *  HACK: We choose a prime number for the frame size such that
     *  we always get pixelformat convertors, which don't require a
     *  specific alignment. This must be done, because the actual
     *  sizes of overlays can change
     */
    
    ctx->cnv->func =
      gavl_find_pixelformat_converter(&(ctx->opt),
                                      ctx->cnv->input_format.pixelformat,
                                      ctx->cnv->output_format.pixelformat,
                                      127, 127);
    if(!ctx->cnv->input_frame)
      ctx->cnv->input_frame  =
        gavl_video_frame_create((gavl_video_format_t*)0);
    
    ctx->ovl_win = gavl_video_frame_create(&ctx->ovl_format);

    ctx->cnv->output_frame = ctx->ovl_win;
    }
  else
    {
    ctx->ovl_win = gavl_video_frame_create((gavl_video_format_t*)0);
    ctx->do_convert = 0;
    }
  
  return 1;
  }

void gavl_overlay_blend_context_set_overlay(gavl_overlay_blend_context_t * ctx,
                                            gavl_overlay_t * ovl)
  {
  int diff;
  /* Save overlay */

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
    
  /* Check if we must convert */
  if(ctx->do_convert)
    {
    /* Set source and destination windows */

    gavl_video_frame_get_subframe(ctx->cnv->input_format.pixelformat,
                                  ovl->frame,
                                  ctx->cnv->input_frame,
                                  &(ctx->ovl.ovl_rect));
    
    /* Adjust overlay rectangle (We are at [0,0] now) */
    ctx->ovl.ovl_rect.x = 0;
    ctx->ovl.ovl_rect.y = 0;

    /* Set image dimensions */

    ctx->cnv->input_format.image_width = ctx->ovl.ovl_rect.w;
    ctx->cnv->output_format.image_width = ctx->ovl.ovl_rect.w;

    ctx->cnv->input_format.image_height = ctx->ovl.ovl_rect.h;
    ctx->cnv->output_format.image_height = ctx->ovl.ovl_rect.h;
    
    ctx->cnv->func(ctx->cnv);

    }
  else
    {
    gavl_video_frame_get_subframe(ctx->ovl_format.pixelformat,
                                  ovl->frame,
                                  ctx->ovl_win,
                                  &(ctx->ovl.ovl_rect));
    }
  }

void gavl_overlay_blend(gavl_overlay_blend_context_t * ctx,
                        gavl_video_frame_t * dst_frame)
  {
  /* Get subframe from destination */
  
  gavl_video_frame_get_subframe(ctx->dst_format.pixelformat,
                                dst_frame,
                                ctx->dst_win,
                                &(ctx->dst_rect));
  /* Fire up blender */

  ctx->func(ctx, ctx->dst_win, ctx->ovl_win);
  }

int gavl_overlay_blend_context_need_new(gavl_overlay_blend_context_t * ctx)
  {
  return 0;
  }

