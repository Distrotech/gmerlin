
static void (FUNC_NAME)(gavl_video_scale_context_t * ctx)
  {
  int i;

#ifdef NO_UINT8
  uint8_t * _src_1, * _src_2;
#else
#define _src_1 src_1
#define _src_2 src_2
#endif

  TYPE * src_1, *src_2, *dst;
#ifdef INIT
  INIT
#endif
  
  _src_1 = ctx->src + ctx->table_v.pixels[ctx->scanline].index * ctx->src_stride;
  _src_2 = _src_1 + ctx->src_stride;
  for(i = 0; i < ctx->dst_size; i++)
    {
    dst = (TYPE*)(ctx->dst);
#ifdef NO_UINT8  
    src_1 = (TYPE*)(_src_1);
    src_2 = (TYPE*)(_src_2);
#endif
    
    SCALE
    
    ctx->dst += ctx->offset->dst_advance;
    _src_1 += ctx->offset->src_advance;
    _src_2 += ctx->offset->src_advance;
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

#ifdef NO_UINT8
#undef NO_UINT8
#endif
