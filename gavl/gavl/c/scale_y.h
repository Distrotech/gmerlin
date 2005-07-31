
static void (FUNC_NAME)(gavl_video_scale_context_t * ctx)
  {
  int i;

#ifdef NO_UINT8
  uint8_t * _src_1
#if NUM_TAPS > 1
    , * _src_2
#endif
#if NUM_TAPS > 2
    , * _src_3
#endif
#if NUM_TAPS > 3
    , * _src_4
#endif
    ;

#else
#define _src_1 src_1
#define _src_2 src_2
#define _src_3 src_3
#define _src_4 src_4
#endif

  TYPE * src_1,
#if NUM_TAPS > 1
  * src_2,
#endif
#if NUM_TAPS > 2
  * src_3,
#endif
#if NUM_TAPS > 3
  * src_4,
#endif
  *dst;

#ifdef INIT
  INIT
#endif
  
  _src_1 = ctx->src + ctx->table_v.pixels[ctx->scanline].index * ctx->src_stride;
#if NUM_TAPS > 1
  _src_2 = _src_1 + ctx->src_stride;
#endif
#if NUM_TAPS > 2
  _src_3 = _src_2 + ctx->src_stride;
#endif
#if NUM_TAPS > 3
  _src_4 = _src_3 + ctx->src_stride;
#endif
  
  for(i = 0; i < ctx->dst_size; i++)
    {
    dst = (TYPE*)(ctx->dst);
#ifdef NO_UINT8  
    src_1 = (TYPE*)(_src_1);
#if NUM_TAPS > 1
    src_2 = (TYPE*)(_src_2);
#endif
#if NUM_TAPS > 2
    src_3 = (TYPE*)(_src_3);
#endif
#if NUM_TAPS > 3
    src_4 = (TYPE*)(_src_4);
#endif
    
#endif
    
    SCALE
    
    ctx->dst += ctx->offset->dst_advance;
    _src_1 += ctx->offset->src_advance;
#if NUM_TAPS > 1
    _src_2 += ctx->offset->src_advance;
#endif
#if NUM_TAPS > 2
    _src_3 += ctx->offset->src_advance;
#endif
#if NUM_TAPS > 3
    _src_4 += ctx->offset->src_advance;
#endif
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

#ifdef _src_3
#undef _src_3
#endif

#ifdef _src_4
#undef _src_4
#endif

#ifdef NO_UINT8
#undef NO_UINT8
#endif
