static void (FUNC_NAME)(gavl_video_scale_context_t * ctx)
  {
  int i, j;
  uint8_t * _src, * src_start;

  TYPE *dst, *src;
    
#ifdef INIT
  INIT
#endif
        
  src_start = ctx->src + ctx->scanline * ctx->src_stride;
  for(i = 0; i < ctx->dst_size; i++)
    {
    dst = (TYPE*)(ctx->dst);

    SCALE_INIT

    _src = src_start + ctx->offset->src_advance * ctx->table_h.pixels[i].index;
    for(j = 0; j < ctx->num_taps_h; j++)
      {
      src = (TYPE*)_src;

      SCALE_ACCUM
      
      _src += ctx->offset->src_advance;
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
