#include <stdlib.h>

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
