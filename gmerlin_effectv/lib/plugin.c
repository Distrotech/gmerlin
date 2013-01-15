/*****************************************************************
 * gmerlin_effectv - EffecTV plugins for gmerlin
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

#include <gmerlin_effectv.h>

static gavl_pixelformat_t agn_formats[] =
  {
    GAVL_RGB_32,
    GAVL_BGR_32,
    GAVL_RGBA_32,
    GAVL_YUVA_32,
    GAVL_GRAYA_32,
#if SIZEOF_FLOAT == 4
    GAVL_GRAY_FLOAT,
#endif
    GAVL_PIXELFORMAT_NONE,
  };


void * bg_effectv_create(effectRegisterFunc * f, int flags)
  {
  bg_effectv_plugin_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->e = f();
  ret->flags = flags;

#ifdef WORDS_BIGENDIAN
  if(!(ret->flags & BG_EFFECTV_COLOR_AGNOSTIC))
    {
    ret->dsp_ctx = gavl_dsp_context_create();
    }
#endif

  return ret;
  }

void bg_effectv_destroy(void*priv)
  {
  bg_effectv_plugin_t * vp = priv;
  
  if(vp->e)
    {
    if(vp->e->stop)              vp->e->stop(vp->e);
    if(vp->e->priv)              free(vp->e->priv);
    if(vp->e->yuv2rgb)           free(vp->e->yuv2rgb);
    if(vp->e->rgb2yuv)           free(vp->e->rgb2yuv);
    if(vp->e->stretching_buffer) free(vp->e->stretching_buffer);
    if(vp->e->background)        free(vp->e->background);
    if(vp->e->diff)              free(vp->e->diff);
    if(vp->e->diff2)             free(vp->e->diff2);
    free(vp->e);
    }
  
  if(vp->out_src)
    gavl_video_source_destroy(vp->out_src);
  
#ifdef WORDS_BIGENDIAN
  if(vp->dsp_ctx)
    gavl_dsp_context_destroy(vp->dsp_ctx);
#endif

  free(vp);
  }

static gavl_source_status_t
read_func(void * priv, gavl_video_frame_t ** frame)
  {
  gavl_source_status_t st;
  bg_effectv_plugin_t * vp;
  vp = priv;

  if(!vp->in_frame)
    {
    vp->in_frame = gavl_video_frame_create_nopad(&vp->format);
    gavl_video_frame_clear(vp->in_frame, &vp->format);
    }

  if((st = gavl_video_source_read_frame(vp->in_src, &vp->in_frame)) !=
     GAVL_SOURCE_OK)
    return st;

#ifdef WORDS_BIGENDIAN
  if(!(vp->flags & BG_EFFECTV_COLOR_AGNOSTIC))
    gavl_dsp_video_frame_swap_endian(ret->dsp_ctx,
                                     vp->in_frame, &vp->format);
#endif

  if(!vp->out_frame)
    {
    vp->out_frame = gavl_video_frame_create_nopad(&vp->format);
    gavl_video_frame_clear(vp->in_frame, &vp->format);
    }
  vp->e->draw(vp->e, (RGB32*)vp->in_frame->planes[0],
                (RGB32*)vp->out_frame->planes[0]);

#ifdef WORDS_BIGENDIAN
  if(!(vp->flags & BG_EFFECTV_COLOR_AGNOSTIC))
    gavl_dsp_video_frame_swap_endian(ret->dsp_ctx,
                                     vp->out_frame, &vp->format);
#endif

  gavl_video_frame_copy_metadata(vp->out_frame, vp->in_frame);
  *frame = vp->out_frame;
  return GAVL_SOURCE_OK;
  }

static void set_format(bg_effectv_plugin_t * vp, const gavl_video_format_t * format)
  {
  gavl_video_format_copy(&vp->format, format);

  if(vp->flags & BG_EFFECTV_COLOR_AGNOSTIC)
    {
    vp->format.pixelformat = gavl_pixelformat_get_best(vp->format.pixelformat,
                                                    agn_formats, NULL);
    }
  else
    {
    vp->format.pixelformat = GAVL_BGR_32;
    }

  if(vp->started)
    {
    vp->e->stop(vp->e);
    vp->started = 0;
    }
  
  vp->e->video_width = vp->format.image_width;
  vp->e->video_height = vp->format.image_height;
  vp->e->video_area = vp->format.image_width * vp->format.image_height;

  vp->e->start(vp->e);
  vp->started = 1;
  
  if(vp->in_frame)
    {
    gavl_video_frame_destroy(vp->in_frame);
    vp->in_frame = NULL;
    }
  if(vp->out_frame)
    {
    gavl_video_frame_destroy(vp->out_frame);
    vp->out_frame = NULL;
    }
  }

gavl_video_source_t * bg_effectv_connect(void * priv,
                                         gavl_video_source_t * src,
                                         const gavl_video_options_t * opt)
  {
  bg_effectv_plugin_t * vp;
  vp = priv;

  vp->in_src = src;
  set_format(vp, gavl_video_source_get_src_format(vp->in_src));
  
  if(vp->out_src)
    gavl_video_source_destroy(vp->out_src);
  
  if(opt)
    {
    gavl_video_options_copy(gavl_video_source_get_options(vp->in_src), opt);
    }
  gavl_video_source_set_dst(vp->in_src, 0, &vp->format);
  
  vp->out_src = gavl_video_source_create_source(read_func,
                                                vp, GAVL_SOURCE_SRC_ALLOC,
                                                vp->in_src);
  return vp->out_src;
  }
