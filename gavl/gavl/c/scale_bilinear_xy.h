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

static void (FUNC_NAME)(gavl_video_scale_context_t * ctx)
  {
  int i;

  uint8_t *src_1, *src_2;
  TYPE *src_11, *src_12, *src_21, *src_22, *dst;
#ifdef INIT
  INIT
#endif

  src_1 = ctx->src + ctx->table_v.pixels[ctx->scanline].index * ctx->src_stride;
  src_2 = src_1 + ctx->src_stride;
  
  for(i = 0; i < ctx->dst_size; i++)
    {
    dst = (TYPE*)(ctx->dst);

    src_11 = (TYPE*)(src_1 + ctx->offset->src_advance * ctx->table_h.pixels[i].index);
    src_12 = (TYPE*)((uint8_t*)(src_11) + ctx->offset->src_advance);

    src_21 = (TYPE*)(src_2 + ctx->offset->src_advance * ctx->table_h.pixels[i].index);
    src_22 = (TYPE*)((uint8_t*)(src_21) + ctx->offset->src_advance);
    
    SCALE
      
    ctx->dst += ctx->offset->dst_advance;
    }
  }

#ifdef INIT
#undef INIT
#endif

#undef FUNC_NAME
#undef TYPE
#undef SCALE

#ifdef _src_1
#undef _src_1
#endif

#ifdef _src_2
#undef _src_2
#endif

