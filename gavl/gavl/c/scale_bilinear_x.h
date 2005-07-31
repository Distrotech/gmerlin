
static void (FUNC_NAME)(gavl_video_scale_context_t * ctx)
  {
  int i;
  uint8_t * src;

  TYPE * src_1, *src_2, *dst;
#ifdef INIT
  INIT
#endif
  
  src = ctx->src + ctx->scanline * ctx->src_stride;
  for(i = 0; i < ctx->dst_size; i++)
    {
    dst = (TYPE*)(ctx->dst);
    src_1 = (TYPE*)(src + ctx->offset->src_advance * ctx->table_h.pixels[i].index);
    src_2 = (TYPE*)((uint8_t*)(src_1) + ctx->offset->src_advance);
        
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
