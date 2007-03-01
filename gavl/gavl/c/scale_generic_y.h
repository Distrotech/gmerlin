static void (FUNC_NAME)(gavl_video_scale_context_t * ctx)
  {
  int i, j;
  uint8_t * _src;

  TYPE *dst, *src;
  
#ifdef INIT
  INIT
#endif
    
  for(i = 0; i < ctx->dst_size; i++)
    {
    dst = (TYPE*)(ctx->dst);

    SCALE_INIT

    _src = ctx->src + ctx->src_stride * ctx->table_v.pixels[ctx->scanline].index +
      i * ctx->offset->src_advance;
    for(j = 0; j < ctx->num_taps_v; j++)
      {
      src = (TYPE*)_src;

      SCALE_ACCUM
      _src += ctx->src_stride;
      }
    
    SCALE_FINISH
    
    ctx->dst += ctx->offset->dst_advance;
    }
  }

#ifdef INIT
#undef INIT
#endif

#undef FUNC_NAME
#undef TYPE

#undef SCALE_INIT
#undef SCALE_ACCUM
#undef SCALE_FINISH

