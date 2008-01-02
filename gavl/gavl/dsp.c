/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <config.h>
#include <gavl/gavl.h>
#include <gavl/gavldsp.h>
#include <dsp.h>

static void init_table(gavl_dsp_context_t* ctx)
  {
  gavl_dsp_init_c(&ctx->funcs, ctx->quality);
#ifdef HAVE_MMX
  if(ctx->accel_flags & GAVL_ACCEL_MMX)
    gavl_dsp_init_mmx(&ctx->funcs, ctx->quality);
  if(ctx->accel_flags & GAVL_ACCEL_MMXEXT)
    gavl_dsp_init_mmxext(&ctx->funcs, ctx->quality);
#endif       

#ifdef HAVE_SSE
  if(ctx->accel_flags & GAVL_ACCEL_SSE)
    gavl_dsp_init_sse(&ctx->funcs, ctx->quality);
#endif       

  }

gavl_dsp_context_t * gavl_dsp_context_create()
  {
  gavl_dsp_context_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->accel_flags = gavl_accel_supported();
  ret->quality = GAVL_QUALITY_DEFAULT;
  init_table(ret);
  return ret;
  }

void gavl_dsp_context_set_quality(gavl_dsp_context_t * ctx,
                                  int q)
  {
  ctx->quality = q;
  init_table(ctx);
  }


gavl_dsp_funcs_t * 
gavl_dsp_context_get_funcs(gavl_dsp_context_t * ctx)
  {
  return &ctx->funcs;
  }

void gavl_dsp_context_destroy(gavl_dsp_context_t * ctx)
  {
  free(ctx);
  }

