#include <audio.h>
#include <sampleformat.h>
#include <float_cast.h>

#define CONVERSION_FUNC_START \
int i, j;\
for(i = 0; i < ctx->input_format.num_channels; i++)\
  { \
  for(j = 0; j < ctx->input_frame->valid_samples; j++) \
    {
    
#define CONVERSION_FUNC_END \
    } \
  }

#define SWAP_ENDIAN(n) ((n >> 8)|(n << 8))
#define CLAMP(i, min, max) if(i<min)i=min;if(i>max)i=max;

static void swap_sign_8(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_8[i][j] =
    ctx->input_frame->channels.u_8[i][j] ^ 0x80;
  CONVERSION_FUNC_END
  }  

static void swap_sign_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_16[i][j] =
    ctx->input_frame->channels.u_16[i][j] ^ 0x8000;
  CONVERSION_FUNC_END
  }

static void s_8_to_s_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.s_16[i][j] =
    ctx->input_frame->channels.s_8[i][j] * 0x0101;
  CONVERSION_FUNC_END
  }

static void u_8_to_s_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.s_16[i][j] =
    (ctx->input_frame->channels.s_8[i][j] ^ 0x80)  * 0x0101;
  CONVERSION_FUNC_END
  }



static void s_8_to_u_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.s_16[i][j] =
    ((ctx->input_frame->channels.s_8[i][j])  * 0x0101) ^ 0x8000;
  CONVERSION_FUNC_END
  }

static void u_8_to_u_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.u_16[i][j] =
    ctx->input_frame->channels.u_8[i][j] |
     ((uint16_t)(ctx->input_frame->channels.u_8[i][j]) << 8);
  CONVERSION_FUNC_END
  }


  /* 8 -> 32 bits */

static void s_8_to_s_32(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.s_32[i][j] =
    ctx->input_frame->channels.s_8[i][j]  * 0x01010101;
  CONVERSION_FUNC_END
  }

static void u_8_to_s_32(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.s_32[i][j] =
    (ctx->input_frame->channels.s_8[i][j] ^ 0x80)  * 0x01010101;
  CONVERSION_FUNC_END
  }
  
/* 16 -> 8 bits */
  
static void convert_16_to_8_swap(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.u_8[i][j] =
    (ctx->input_frame->channels.u_16[i][j] ^ 0x8000) >> 8;
  CONVERSION_FUNC_END
  }

static void convert_16_to_8(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.u_8[i][j] =
    ctx->input_frame->channels.u_16[i][j] >> 8;
  CONVERSION_FUNC_END
  }

/* 16 -> 32 bits */

static void s_16_to_s_32(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.s_32[i][j] =
    ctx->input_frame->channels.s_16[i][j] * 0x00010001;
  CONVERSION_FUNC_END
  }

static void u_16_to_s_32(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.s_32[i][j] =
    (ctx->input_frame->channels.s_16[i][j] ^ 0x8000 ) * 0x00010001;
  CONVERSION_FUNC_END
  }

/* 32 -> 8 bits */
  
static void convert_32_to_8_swap(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.s_8[i][j] =
    (ctx->input_frame->channels.s_32[i][j] >> 24) ^ 0x80;
  CONVERSION_FUNC_END
  }

static void convert_32_to_8(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.s_8[i][j] =
    ctx->input_frame->channels.s_32[i][j] >> 24;
  CONVERSION_FUNC_END
  }

/* 32 -> 16 bits */
  
static void convert_32_to_16_swap(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.s_16[i][j] =
    (ctx->input_frame->channels.s_32[i][j] >> 16) ^ 0x8000;
  
  CONVERSION_FUNC_END
  }

static void convert_32_to_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
    ctx->output_frame->channels.s_16[i][j] =
    ctx->input_frame->channels.s_32[i][j] >> 16;
  CONVERSION_FUNC_END
  }

/* Int to float */

static void convert_s8_to_float(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.f[i][j] =
    (float)(ctx->input_frame->channels.s_8[i][j])/128.0;
  CONVERSION_FUNC_END
  }

static void convert_u8_to_float(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.f[i][j] =
    (float)(ctx->input_frame->channels.u_8[i][j])/128.0-1.0;
  CONVERSION_FUNC_END
  }

static void convert_s16_to_float(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.f[i][j] =
    (float)(ctx->input_frame->channels.s_16[i][j])/32768.0;
  CONVERSION_FUNC_END
  }

static void convert_u16_to_float(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.f[i][j] =
    (float)(ctx->input_frame->channels.u_16[i][j])/32768.0-1.0;
  CONVERSION_FUNC_END
  }

static void convert_s32_to_float(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.f[i][j] =
    (float)(ctx->input_frame->channels.s_32[i][j])/2147483648.0;
  CONVERSION_FUNC_END
  }

/* Float to int */

static void convert_float_to_s8(gavl_audio_convert_context_t * ctx)
  {
  long tmp;
  CONVERSION_FUNC_START
  tmp = lrintf(ctx->input_frame->channels.f[i][j] * 128.0);
  CLAMP(tmp, -128, 127);
  ctx->output_frame->channels.s_8[i][j] = tmp;
  CONVERSION_FUNC_END
  }

static void convert_float_to_u8(gavl_audio_convert_context_t * ctx)
  {
  long tmp;
  CONVERSION_FUNC_START
  tmp = lrintf((ctx->input_frame->channels.f[i][j]+1.0) * 128.0);
  CLAMP(tmp, -128, 127);
  ctx->output_frame->channels.u_8[i][j] = tmp;
  CONVERSION_FUNC_END
  }

static void convert_float_to_s16(gavl_audio_convert_context_t * ctx)
  {
  long tmp;
  CONVERSION_FUNC_START
  tmp = lrintf((ctx->input_frame->channels.f[i][j]) * 32768.0);
  CLAMP(tmp, -32768, 32767);
  ctx->output_frame->channels.s_16[i][j] = tmp;

  CONVERSION_FUNC_END
  }

static void convert_float_to_u16(gavl_audio_convert_context_t * ctx)
  {
  long tmp;
  CONVERSION_FUNC_START
  tmp = lrintf((ctx->input_frame->channels.f[i][j]+1.0) * 32768.0);
  CLAMP(tmp, -32768, 32767);
  ctx->output_frame->channels.u_8[i][j] = tmp;

  CONVERSION_FUNC_END
  }

static void convert_float_to_s32(gavl_audio_convert_context_t * ctx)
  {
  int64_t tmp;
  CONVERSION_FUNC_START
  tmp = llrintf((ctx->input_frame->channels.f[i][j]) * 2147483648.0);
  CLAMP(tmp, -2147483648LL, 2147483647LL);
  ctx->output_frame->channels.s_32[i][j] = tmp;
  CONVERSION_FUNC_END
  }

void gavl_init_sampleformat_funcs_c(gavl_sampleformat_table_t * t)
  {
  t->swap_sign_8 = swap_sign_8;
  t->swap_sign_16 = swap_sign_16;
  
  /* 8 -> 16 bits */

  t->s_8_to_s_16 = s_8_to_s_16;
  t->u_8_to_s_16 = u_8_to_s_16;

  t->s_8_to_u_16 = s_8_to_u_16;
  t->u_8_to_u_16 = u_8_to_u_16;

  /* 8 -> 32 bits */

  t->s_8_to_s_32 = s_8_to_s_32;
  t->u_8_to_s_32 = u_8_to_s_32;
  
  /* 16 -> 8 bits */
  
  t->convert_16_to_8_swap = convert_16_to_8_swap;
  t->convert_16_to_8 = convert_16_to_8;

  /* 16 -> 32 bits */

  t->s_16_to_s_32 = s_16_to_s_32;
  t->u_16_to_s_32 = u_16_to_s_32;
  
  /* 32 -> 8 bits */
  
  t->convert_32_to_8_swap = convert_32_to_8_swap;
  t->convert_32_to_8 = convert_32_to_8;

  /* 32 -> 16 bits */
  
  t->convert_32_to_16_swap = convert_32_to_16_swap;
  t->convert_32_to_16 = convert_32_to_16;
  
  /* Int to Float  */

  t->convert_s8_to_float = convert_s8_to_float;
  t->convert_u8_to_float = convert_u8_to_float;

  t->convert_s16_to_float = convert_s16_to_float;
  t->convert_u16_to_float = convert_u16_to_float;

  t->convert_s32_to_float = convert_s32_to_float;

  /* Float to int */
  
  t->convert_float_to_s8 = convert_float_to_s8;
  t->convert_float_to_u8 = convert_float_to_u8;

  t->convert_float_to_s16 = convert_float_to_s16;
  t->convert_float_to_u16 = convert_float_to_u16;

  t->convert_float_to_s32 = convert_float_to_s32;
  }
