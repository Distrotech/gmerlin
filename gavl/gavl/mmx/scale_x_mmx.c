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

#include <config.h>
#include <attributes.h>

#include <stdio.h>
#include <gavl/gavl.h>
#include <video.h>
#include <scale.h>

#include "mmx.h"

#ifdef MMXEXT
#define MOVQ_R2M(reg,mem) movntq_r2m(reg, mem)
#else
#define MOVQ_R2M(reg,mem) movq_r2m(reg, mem)
#endif

#if 0
static mmx_t mm_tmp;
#define DUMP_MM(name, reg) MOVQ_R2M(reg, mm_tmp);\
  fprintf(stderr, "%s: %016llx\n", name, mm_tmp.q);
#endif

static const mmx_t factor_mask = { 0x000000000000FFFFLL };

/*
 *  mm0: Input
 *  mm1: Input
 *  mm2: Factor
 *  mm3: Factor
 *  mm4: Output
 *  mm5: Output
 *  mm6: 0
 *  mm7: scratch
 *  
 */

#define LOAD_FACTOR_1


#define LOAD_FACTOR_4

#define RECLIP(a,idx) \
  if(a < ctx->min_values_h[idx]) a = ctx->min_values_h[idx];    \
  if(a > ctx->max_values_h[idx]) a = ctx->max_values_h[idx]



/* scale_uint8_x_1_x_bicubic_mmx */

static void scale_uint8_x_1_x_bicubic_mmx(gavl_video_scale_context_t * ctx)
  {
  int i;
  uint8_t * src, * dst, *src_start;
  int32_t * factors;
  mmx_t tmp_mm;
  int32_t tmp;
  
  //  fprintf(stderr, "scale_uint8_x_1_x_bicubic_mmx\n");

  src_start = ctx->src + ctx->scanline * ctx->src_stride;
  
  pxor_r2r(mm6, mm6);
  dst = ctx->dst;
  for(i = 0; i < ctx->dst_size; i++)
    {
    src = src_start + ctx->table_h.pixels[i].index;
    factors = ctx->table_h.pixels[i].factor_i;
    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    //    DUMP_MM("mm0", mm0);
    /* Load factors */
    movq_m2r(*factors, mm2);
    movq_m2r(*(factors+2), mm3);
    packssdw_r2r(mm3, mm2);
    /* Multiply */
    pmaddwd_r2r(mm2, mm0);
    MOVQ_R2M(mm0, tmp_mm);
    tmp = tmp_mm.d[0] + tmp_mm.d[1];
    RECLIP(tmp, ctx->plane);
    *(dst++) = tmp >> 14;
    }
  emms();
  }

/* scale_uint8_x_1_x_bicubic_noclip_mmx */

static void
scale_uint8_x_1_x_bicubic_noclip_mmx(gavl_video_scale_context_t * ctx)
  {
  int i;
  uint8_t * src, * dst, *src_start;
  int32_t * factors;
  mmx_t tmp_mm;
  
  //  fprintf(stderr, "scale_uint8_x_1_x_bicubic_noclip_mmx\n");
  src_start = ctx->src + ctx->scanline * ctx->src_stride;
  
  pxor_r2r(mm6, mm6);
  dst = ctx->dst;
  for(i = 0; i < ctx->dst_size; i++)
    {
    src = src_start + ctx->table_h.pixels[i].index;
    factors = ctx->table_h.pixels[i].factor_i;
    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    /* Load factors */
    movq_m2r(*factors, mm2);
    movq_m2r(*(factors+2), mm3);
    packssdw_r2r(mm3, mm2);
    /* Multiply */
    pmaddwd_r2r(mm2, mm0);
    psrld_i2r(14, mm0);
    MOVQ_R2M(mm0, tmp_mm);
    *(dst++) = tmp_mm.d[0] + tmp_mm.d[1];
    }
  emms();
  }


/* scale_uint8_x_4_x_bicubic_mmx */

static void scale_uint8_x_4_x_bicubic_mmx(gavl_video_scale_context_t * ctx)
  {
  int i;
  uint8_t * src, * dst, *src_start;
  int32_t * factors;
  //  mmx_t tmp_mm;

/*
 *  mm0: Input
 *  mm1: factor_mask
 *  mm2: Factor
 *  mm3: Output
 *  mm4: 
 *  mm5: 
 *  mm6: 0
 *  mm7: scratch
 *  
 */
  
  //  fprintf(stderr, "scale_uint8_x_1_x_bicubic_noclip_mmx\n");
  src_start = ctx->src + ctx->scanline * ctx->src_stride;
  
  pxor_r2r(mm6, mm6);
  movq_m2r(factor_mask, mm1);
  dst = ctx->dst;
  for(i = 0; i < ctx->dst_size; i++)
    {
    src = src_start + 4*ctx->table_h.pixels[i].index;
    factors = ctx->table_h.pixels[i].factor_i;
    
    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    psllw_i2r(7, mm0);
    /* Load factors */
    movd_m2r(*factors, mm2);
    pand_r2r(mm1, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(16, mm7);
    por_r2r(mm7, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(32, mm7);
    por_r2r(mm7, mm2);
    /* Multiply */
    pmulhw_r2r(mm2, mm0);
    movq_r2r(mm0, mm3);
    //    DUMP_MM("mm3_1", mm3);
    src += 4;
    factors++;
    
    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    psllw_i2r(7, mm0);
    /* Load factors */
    movd_m2r(*factors, mm2);
    pand_r2r(mm1, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(16, mm7);
    por_r2r(mm7, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(32, mm7);
    por_r2r(mm7, mm2);
    /* Multiply */
    pmulhw_r2r(mm2, mm0);
    paddw_r2r(mm0, mm3);
    //    DUMP_MM("mm3_2", mm3);
    src += 4;
    factors++;

    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    psllw_i2r(7, mm0);
    /* Load factors */
    movd_m2r(*factors, mm2);
    pand_r2r(mm1, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(16, mm7);
    por_r2r(mm7, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(32, mm7);
    por_r2r(mm7, mm2);
    /* Multiply */
    pmulhw_r2r(mm2, mm0);
    paddw_r2r(mm0, mm3);
    //    DUMP_MM("mm3_3", mm3);
    src += 4;
    factors++;

    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    psllw_i2r(7, mm0);
    /* Load factors */
    movd_m2r(*factors, mm2);
    pand_r2r(mm1, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(16, mm7);
    por_r2r(mm7, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(32, mm7);
    por_r2r(mm7, mm2);
    /* Multiply */
    pmulhw_r2r(mm2, mm0);
    paddw_r2r(mm0, mm3);
    src += 4;
    factors++;

    psraw_i2r(5, mm3);
    packuswb_r2r(mm6, mm3);
    movd_r2m(mm3, *dst);
    
    dst+=4;
    }
  emms();
  }


/* scale_uint8_x_4_x_bicubic_mmx */

static void scale_uint8_x_4_x_quadratic_mmx(gavl_video_scale_context_t * ctx)
  {
  int i;
  uint8_t * src, * dst, *src_start;
  int32_t * factors;
  //  mmx_t tmp_mm;

/*
 *  mm0: Input
 *  mm1: factor_mask
 *  mm2: Factor
 *  mm3: Output
 *  mm4: 
 *  mm5: 
 *  mm6: 0
 *  mm7: scratch
 *  
 */
  
  //  fprintf(stderr, "scale_uint8_x_1_x_bicubic_noclip_mmx\n");
  src_start = ctx->src + ctx->scanline * ctx->src_stride;
  
  pxor_r2r(mm6, mm6);
  movq_m2r(factor_mask, mm1);
  dst = ctx->dst;
  for(i = 0; i < ctx->dst_size; i++)
    {
    src = src_start + 4*ctx->table_h.pixels[i].index;
    factors = ctx->table_h.pixels[i].factor_i;
    
    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    psllw_i2r(7, mm0);
    /* Load factors */
    movd_m2r(*factors, mm2);
    pand_r2r(mm1, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(16, mm7);
    por_r2r(mm7, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(32, mm7);
    por_r2r(mm7, mm2);
    /* Multiply */
    pmulhw_r2r(mm2, mm0);
    movq_r2r(mm0, mm3);
    //    DUMP_MM("mm3_1", mm3);
    src += 4;
    factors++;
    
    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    psllw_i2r(7, mm0);
    /* Load factors */
    movd_m2r(*factors, mm2);
    pand_r2r(mm1, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(16, mm7);
    por_r2r(mm7, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(32, mm7);
    por_r2r(mm7, mm2);
    /* Multiply */
    pmulhw_r2r(mm2, mm0);
    paddw_r2r(mm0, mm3);
    //    DUMP_MM("mm3_2", mm3);
    src += 4;
    factors++;

    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    psllw_i2r(7, mm0);
    /* Load factors */
    movd_m2r(*factors, mm2);
    pand_r2r(mm1, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(16, mm7);
    por_r2r(mm7, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(32, mm7);
    por_r2r(mm7, mm2);
    /* Multiply */
    pmulhw_r2r(mm2, mm0);
    paddw_r2r(mm0, mm3);
    //    DUMP_MM("mm3_3", mm3);
    src += 4;
    factors++;
    
    psraw_i2r(5, mm3);
    packuswb_r2r(mm6, mm3);
    movd_r2m(mm3, *dst);
    
    dst+=4;
    }
  emms();
  }


/* scale_uint8_x_1_x_generic_mmx */

static void scale_uint8_x_1_x_generic_mmx(gavl_video_scale_context_t * ctx)
  {
  int i, j, jmax;
  uint8_t * src, * dst, *src_start;
  int32_t * factors;
  mmx_t tmp_mm;
  int tmp;
  
  //  fprintf(stderr, "scale_uint8_x_1_x_generic_mmx\n");

  src_start = ctx->src + ctx->scanline * ctx->src_stride;
  
  pxor_r2r(mm6, mm6);
  dst = ctx->dst;
  for(i = 0; i < ctx->dst_size; i++)
    {
    src = src_start + ctx->table_h.pixels[i].index;
    factors = ctx->table_h.pixels[i].factor_i;

    jmax = ctx->table_h.factors_per_pixel / 4;
    tmp = 0;

    pxor_r2r(mm4, mm4);

    for(j = 0; j < jmax; j++)
      {
      /* Load pixels */
      movd_m2r(*(src), mm0);
      punpcklbw_r2r(mm6, mm0);
      //    DUMP_MM("mm0", mm0);
      /* Load factors */
      movq_m2r(*factors, mm2);
      movq_m2r(*(factors+2), mm3);
      packssdw_r2r(mm3, mm2);
      /* Multiply */
      pmaddwd_r2r(mm2, mm0);
      paddd_r2r(mm0, mm4);
      src += 4;
      factors += 4;
      }

    MOVQ_R2M(mm4, tmp_mm);
    tmp = tmp_mm.d[0] + tmp_mm.d[1];

    
    jmax = ctx->table_h.factors_per_pixel % 4;

    for(j = 0; j < jmax; j++)
      {
      tmp += *factors * *src;
      factors++;
      src++;
      }
    
    RECLIP(tmp, ctx->plane);
    *(dst++) = tmp >> 14;
    }
  emms();
  }

/* scale_uint8_x_4_x_generic_mmx */

static void scale_uint8_x_4_x_generic_mmx(gavl_video_scale_context_t * ctx)
  {
  int i, j;
  uint8_t * src, * dst, *src_start;
  int32_t * factors;
  //  mmx_t tmp_mm;

/*
 *  mm0: Input
 *  mm1: factor_mask
 *  mm2: Factor
 *  mm3: Output
 *  mm4: 
 *  mm5: 
 *  mm6: 0
 *  mm7: scratch
 *  
 */
  
  src_start = ctx->src + ctx->scanline * ctx->src_stride;
  
  pxor_r2r(mm6, mm6);
  movq_m2r(factor_mask, mm1);
  dst = ctx->dst;
  for(i = 0; i < ctx->dst_size; i++)
    {
    src = src_start + 4*ctx->table_h.pixels[i].index;
    factors = ctx->table_h.pixels[i].factor_i;
    pxor_r2r(mm3, mm3);

    for(j = 0; j < ctx->table_h.factors_per_pixel; j++)
      {
      /* Load pixels */
      movd_m2r(*(src), mm0);
      punpcklbw_r2r(mm6, mm0);
      psllw_i2r(7, mm0);
      /* Load factors */
      movd_m2r(*factors, mm2);
      pand_r2r(mm1, mm2);
      movq_r2r(mm2, mm7);
      psllq_i2r(16, mm7);
      por_r2r(mm7, mm2);
      movq_r2r(mm2, mm7);
      psllq_i2r(32, mm7);
      por_r2r(mm7, mm2);
      /* Multiply */
      pmulhw_r2r(mm2, mm0);
      paddw_r2r(mm0, mm3);
      //    DUMP_MM("mm3_2", mm3);
      src += 4;
      factors++;
      
      }
    
    psraw_i2r(5, mm3);
    packuswb_r2r(mm6, mm3);
    movd_r2m(mm3, *dst);
    
    dst+=4;
    }
  emms();
  
  }

/* scale_uint8_x_1_x_bilinear_mmx */

static void scale_uint8_x_1_x_bilinear_mmx(gavl_video_scale_context_t * ctx)
  {
  int i, imax, index;
  uint8_t * src, * dst, *src_start;
  mmx_t tmp_mm;

  
/*
 *  mm0: Input1 Input2
 *  mm1: Factor
 *  mm2:
 *  mm3: 
 *  mm4: 
 *  mm5: 
 *  mm6: 0
 *  mm7: scratch
 *  
 */
  
  src_start = ctx->src + ctx->scanline * ctx->src_stride;
  
  pxor_r2r(mm6, mm6);
  dst = ctx->dst;

  imax = ctx->dst_size / 4;
  //  imax = 0;
  index = 0;
  
  for(i = 0; i < imax; i++)
    {
    
    /* Load pixels */
    src = src_start + ctx->table_h.pixels[index].index;
    tmp_mm.uw[0] = *src;
    tmp_mm.uw[1] = *(src+1);
    
    src = src_start + ctx->table_h.pixels[index+1].index;
    tmp_mm.uw[2] = *src;
    tmp_mm.uw[3] = *(src+1);
    
    movq_m2r(tmp_mm, mm0);
    /* Load factors */
    movq_m2r(ctx->table_h.pixels[index].factor_i[0], mm1);
    movq_m2r(ctx->table_h.pixels[index+1].factor_i[0], mm7);

    packssdw_r2r(mm7, mm1);
    pmaddwd_r2r(mm0, mm1);

    index += 2;
    
    /* Load pixels */
    src = src_start + ctx->table_h.pixels[index].index;
    tmp_mm.uw[0] = *src;
    tmp_mm.uw[1] = *(src+1);
    
    src = src_start + ctx->table_h.pixels[index+1].index;
    tmp_mm.uw[2] = *src;
    tmp_mm.uw[3] = *(src+1);
    
    movq_m2r(tmp_mm, mm0);
    /* Load factors */
    movq_m2r(ctx->table_h.pixels[index].factor_i[0], mm3);
    movq_m2r(ctx->table_h.pixels[index+1].factor_i[0], mm7);
    packssdw_r2r(mm7, mm3);
    pmaddwd_r2r(mm0, mm3);
    
    psrld_i2r(7, mm3);
    psrld_i2r(7, mm1);
    packssdw_r2r(mm3, mm1);
    psrlw_i2r(7, mm1);
    index += 2;
    
    packuswb_r2r(mm6, mm1);
    
    movd_r2m(mm1, *dst);
    //    *dst      = tmp_mm.ub[0];
    //    *(dst+1) = tmp_mm.ub[4];
    dst+=4;
    }
  emms();

  imax = ctx->dst_size % 4;
  //  imax = ctx->dst_size;
  
  for(i = 0; i < imax; i++)
    {
    src = (src_start + ctx->table_h.pixels[index].index);
    *dst = (ctx->table_h.pixels[index].factor_i[0] * *src +
      ctx->table_h.pixels[index].factor_i[1] * *(src+1)) >> 14;
    dst++;
    index++;
    }
  
  }

/* scale_uint8_x_4_x_bilinear_mmx */

static void scale_uint8_x_4_x_bilinear_mmx(gavl_video_scale_context_t * ctx)
  {
  int i;
  uint8_t * src, * dst, *src_start;
  int32_t * factors;
  //  mmx_t tmp_mm;

/*
 *  mm0: Input1
 *  mm1: Input2
 *  mm2: Factor
 *  mm3: Output
 *  mm4: 
 *  mm5: 
 *  mm6: 0
 *  mm7: scratch
 *  
 */

//  fprintf(stderr, "scale_uint8_x_4_x_bilinear_mmx\n");
  
  src_start = ctx->src + ctx->scanline * ctx->src_stride;
  
  pxor_r2r(mm6, mm6);
  movq_m2r(factor_mask, mm1);
  dst = ctx->dst;
  for(i = 0; i < ctx->dst_size; i++)
    {
    src = src_start + 4*ctx->table_h.pixels[i].index;
    factors = ctx->table_h.pixels[i].factor_i;
    
    /* Load pixels */
    movd_m2r(*(src), mm0);
    punpcklbw_r2r(mm6, mm0);
    psllw_i2r(7, mm0);
    /* Load pixels */
    movd_m2r(*(src+4), mm1);
    punpcklbw_r2r(mm6, mm1);
    psllw_i2r(7, mm1);

    /* Load factors */
    movd_m2r(*factors, mm2);
    pand_r2r(mm1, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(16, mm7);
    por_r2r(mm7, mm2);
    movq_r2r(mm2, mm7);
    psllq_i2r(32, mm7);
    por_r2r(mm7, mm2);
    /* Subtract */
    movq_r2r(mm1, mm7);
    psubsw_r2r(mm0, mm7);
    pmulhw_r2r(mm2, mm7);
    
    psrlw_i2r(2, mm1);
    
    paddsw_r2r(mm7, mm1);
    
    psraw_i2r(5, mm1);
    packuswb_r2r(mm6, mm1);
    movd_r2m(mm1, *dst);
    
    dst+=4;
    }
  emms();
  
  }

#ifdef MMXEXT
void gavl_init_scale_funcs_bicubic_x_mmxext(gavl_scale_funcs_t * tab,
                                         int src_advance, int dst_advance)
#else
void gavl_init_scale_funcs_bicubic_x_mmx(gavl_scale_funcs_t * tab,
                                         int src_advance, int dst_advance)
#endif
  {
  if((src_advance == 1) && (dst_advance == 1))
    {
    tab->funcs_x.scale_uint8_x_1_noadvance =  scale_uint8_x_1_x_bicubic_mmx;
    tab->funcs_x.bits_uint8_noadvance = 14;
    }
  else if((src_advance == 4) && (dst_advance == 4))
    {
    tab->funcs_x.scale_uint8_x_3 =  scale_uint8_x_4_x_bicubic_mmx;
    tab->funcs_x.scale_uint8_x_4 =  scale_uint8_x_4_x_bicubic_mmx;
    tab->funcs_x.bits_uint8_noadvance  = 14;
    }
  }

#ifdef MMXEXT
void gavl_init_scale_funcs_quadratic_x_mmxext(gavl_scale_funcs_t * tab,
                                              int src_advance, int dst_advance)
#else
void gavl_init_scale_funcs_quadratic_x_mmx(gavl_scale_funcs_t * tab,
                                           int src_advance, int dst_advance)
#endif
  {
  if((src_advance == 4) && (dst_advance == 4))
    {
    tab->funcs_x.scale_uint8_x_3 =  scale_uint8_x_4_x_quadratic_mmx;
    tab->funcs_x.scale_uint8_x_4 =  scale_uint8_x_4_x_quadratic_mmx;
    tab->funcs_x.bits_uint8_noadvance  = 14;
    }
  }

#ifdef MMXEXT
void gavl_init_scale_funcs_bicubic_noclip_x_mmxext(gavl_scale_funcs_t * tab,
                                                   int src_advance, int dst_advance)
#else
void gavl_init_scale_funcs_bicubic_noclip_x_mmx(gavl_scale_funcs_t * tab,
                                                int src_advance, int dst_advance)
#endif
  {
  if((src_advance == 1) && (dst_advance == 1))
    {
    tab->funcs_x.scale_uint8_x_1_noadvance =  scale_uint8_x_1_x_bicubic_noclip_mmx;
    tab->funcs_x.bits_uint8_noadvance = 14;
    }
#if 1  
  else if((src_advance == 4) && (dst_advance == 4))
    {
    tab->funcs_x.scale_uint8_x_3 =  scale_uint8_x_4_x_bicubic_mmx;
    tab->funcs_x.scale_uint8_x_4 =  scale_uint8_x_4_x_bicubic_mmx;
    tab->funcs_x.bits_uint8_noadvance  = 14;
    }
#endif
  }

#ifdef MMXEXT
void gavl_init_scale_funcs_generic_x_mmxext(gavl_scale_funcs_t * tab,
                                            int src_advance, int dst_advance)
#else
void gavl_init_scale_funcs_generic_x_mmx(gavl_scale_funcs_t * tab,
                                         int src_advance, int dst_advance)
#endif
  {
  if((src_advance == 1) && (dst_advance == 1))
    {
    tab->funcs_x.scale_uint8_x_1_noadvance =  scale_uint8_x_1_x_generic_mmx;
    tab->funcs_x.bits_uint8_noadvance = 14;
    }
  else if((src_advance == 4) && (dst_advance == 4))
    {
    tab->funcs_x.scale_uint8_x_3 =  scale_uint8_x_4_x_generic_mmx;
    tab->funcs_x.scale_uint8_x_4 =  scale_uint8_x_4_x_generic_mmx;
    tab->funcs_x.bits_uint8_noadvance  = 14;
    }
  }

#ifdef MMXEXT
void gavl_init_scale_funcs_bilinear_x_mmxext(gavl_scale_funcs_t * tab,
                                             int src_advance, int dst_advance)
#else
void gavl_init_scale_funcs_bilinear_x_mmx(gavl_scale_funcs_t * tab,
                                         int src_advance, int dst_advance)
#endif
  {
  if((src_advance == 1) && (dst_advance == 1))
    {
    tab->funcs_x.scale_uint8_x_1_noadvance =  scale_uint8_x_1_x_bilinear_mmx;
    tab->funcs_x.bits_uint8_noadvance = 14;
    }
  else if((src_advance == 4) && (dst_advance == 4))
    {
    tab->funcs_x.scale_uint8_x_3 =  scale_uint8_x_4_x_bilinear_mmx;
    tab->funcs_x.scale_uint8_x_4 =  scale_uint8_x_4_x_bilinear_mmx;
    tab->funcs_x.bits_uint8_noadvance  = 14;
    }
  
  }
