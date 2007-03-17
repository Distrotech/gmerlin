

static void (FUNC_NAME)(gavl_video_scale_context_t * ctx)
  {
  int i, imax;
  int64_t tmp;
  
#if NUM_TAPS <= 0
  int j;
#endif
  uint8_t * src, *src_start;
  uint8_t * dst;

  //  if(!ctx->scanline)
  //    fprintf(stderr, "scale_y_mmx %d %d\n", ctx->src_stride);
  
  imax = (ctx->dst_size * WIDTH_MUL) / 8;
  //  imax = 0;
  
  src_start =
    ctx->src + ctx->table_v.pixels[ctx->scanline].index * ctx->src_stride;

  dst = ctx->dst;

#if BITS == 8
    INIT_8_GLOBAL;
#else
    INIT_16_GLOBAL;
#endif
  
  for(i = 0; i < imax; i++)
    {
#if BITS == 8
    INIT_8;
#else
    INIT_16;
#endif

    src = src_start;
    
    /* Accum */
#if NUM_TAPS <= 0
    for(j = 0; j < ctx->table_v.factors_per_pixel; j++)
      {
#if BITS == 8
      ACCUM_8(j);
#else
      ACCUM_16(j);
#endif
      src += ctx->src_stride;
      }
#else

#if BITS == 8
      ACCUM_8(0);
#else
      ACCUM_16(0);
#endif

    src += ctx->src_stride;
    
#if NUM_TAPS > 1

#if BITS == 8
      ACCUM_8(1);
#else
      ACCUM_16(1);
#endif
    src += ctx->src_stride;
#endif

#if NUM_TAPS > 2

#if BITS == 8
    ACCUM_8(2);
#else
    ACCUM_16(2);
#endif

    src += ctx->src_stride;
#endif

#if NUM_TAPS > 3

#if BITS == 8
    ACCUM_8(3);
#else
    ACCUM_16(3);
#endif

    src += ctx->src_stride;
#endif
    
#endif

#if BITS == 8
    OUTPUT_8;
#else
    OUTPUT_16;
#endif
    dst += 8;
    src_start += 8;
    
    }

  emms();
  
  imax = (ctx->dst_size * WIDTH_MUL) % 8;
  //  imax = (ctx->dst_size * WIDTH_MUL);
  
  if(!imax)
    return;
  
  for(i = 0; i < imax; i++)
    {
    src = src_start;
    tmp = 0;
    
    /* Accum */
#if NUM_TAPS <= 0
    for(j = 0; j < ctx->table_v.factors_per_pixel; j++)
      {
#if BITS == 8
      ACCUM_C_8(j);
#else
      ACCUM_C_16(j);
#endif
      src += ctx->src_stride;
      }
    
#else

#if BITS == 8
      ACCUM_C_8(0);
#else
      ACCUM_C_16(0);
#endif

    src += ctx->src_stride;
    
#if NUM_TAPS > 1
#if BITS == 8
    ACCUM_C_8(1);
#else
    ACCUM_C_16(1);
#endif

    src += ctx->src_stride;
#endif

#if NUM_TAPS > 2
#if BITS == 8
    ACCUM_C_8(2);
#else
    ACCUM_C_16(2);
#endif
    src += ctx->src_stride;
#endif

#if NUM_TAPS > 3
#if BITS == 8
    ACCUM_C_8(3);
#else
    ACCUM_C_16(3);
#endif
    src += ctx->src_stride;
#endif
    
#endif

#if BITS == 8
    OUTPUT_C_8;
#else
    OUTPUT_C_16;
#endif
    dst++;
    src_start++;
    }
  }

#undef FUNC_NAME
#undef NUM_TAPS
#undef BITS

#undef WIDTH_MUL
