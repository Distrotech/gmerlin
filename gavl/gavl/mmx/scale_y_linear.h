

static void (FUNC_NAME)(gavl_video_scale_context_t * ctx)
  {
  int i, imax;
  int32_t tmp;
  
  uint8_t * src_1;
  uint8_t * src_2;
  uint8_t * dst;
#if 0  
  if(!ctx->scanline)
    fprintf(stderr, "scale_y_linear_mmx %d %d\n",
            ctx->table_v.pixels[ctx->scanline].factor_i[0],
            ctx->table_v.pixels[ctx->scanline].factor_i[1]);
#endif
  imax = (ctx->dst_size * WIDTH_MUL) / 8;
  //  imax = 0;
  
  src_1 =
    ctx->src + ctx->table_v.pixels[ctx->scanline].index * ctx->src_stride;
  src_2 = src_1 + ctx->src_stride;

  dst = ctx->dst;

  /* Load factors */

  /*
   *  mm0: Input1
   *  mm1: Input2
   *  mm2: Factor1
   *  mm3: Factor1
   *  mm4: Output1
   *  mm5: Output2
   *  mm6: Scratch
   *  mm7: 0
   */

  pxor_r2r(mm7, mm7);
  
  /* Load factor1 */
  movd_m2r(ctx->table_v.pixels[ctx->scanline].factor_i[0], mm2);
  //  psllw_i2r(7, mm2);
  movq_r2r(mm2, mm6);
  psllq_i2r(16, mm6);
  por_r2r(mm6, mm2);
  movq_r2r(mm2, mm6);
  psllq_i2r(32, mm6);
  por_r2r(mm6, mm2);

  /* Load factor2 */
  movd_m2r(ctx->table_v.pixels[ctx->scanline].factor_i[1], mm3);
  //  psllw_i2r(7, mm3);
  movq_r2r(mm3, mm6);
  psllq_i2r(16, mm6);
  por_r2r(mm6, mm3);
  movq_r2r(mm3, mm6);
  psllq_i2r(32, mm6);
  por_r2r(mm6, mm3);
  
  for(i = 0; i < imax; i++)
    {
    /* Load input 1 */
    movq_m2r(*src_1,mm0);
    movq_r2r(mm0,mm1);
    punpcklbw_r2r(mm7, mm0);
    punpckhbw_r2r(mm7, mm1);
    psllw_i2r(7, mm0);
    psllw_i2r(7, mm1);

    /* Accumulate mm0 */
    pmulhw_r2r(mm2, mm0);
    movq_r2r(mm0, mm4);
    /* Accumulate mm1 */ 
    pmulhw_r2r(mm2, mm1);
    movq_r2r(mm1, mm5);

    /* Load input 2 */
    movq_m2r(*(src_2),mm0);
    movq_r2r(mm0,mm1);
    punpcklbw_r2r(mm7, mm0);
    punpckhbw_r2r(mm7, mm1);
    psllw_i2r(7, mm0);
    psllw_i2r(7, mm1);

    /* Accumulate mm0 */
    pmulhw_r2r(mm3, mm0);
    paddsw_r2r(mm0, mm4);
    /* Accumulate mm1 */ 
    pmulhw_r2r(mm3, mm1);
    paddsw_r2r(mm1, mm5);

    psraw_i2r(5, mm4);
    psraw_i2r(5, mm5);
    packuswb_r2r(mm5, mm4);
    MOVQ_R2M(mm4, *dst);
    
    dst += 8;
    src_1 += 8;
    src_2 += 8;
    }

  emms();
  
  imax = (ctx->dst_size * WIDTH_MUL) % 8;
  //  imax = (ctx->dst_size * WIDTH_MUL);
  
  if(!imax)
    return;
  
  for(i = 0; i < imax; i++)
    {
    tmp = (*src_1 * ctx->table_v.pixels[ctx->scanline].factor_i[0] +
           *src_2 * ctx->table_v.pixels[ctx->scanline].factor_i[1]) >> 7;
    *dst = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
    /* Accum */
    dst++;
    src_1++;
    src_2++;
    }
  }

#undef FUNC_NAME
#undef WIDTH_MUL
