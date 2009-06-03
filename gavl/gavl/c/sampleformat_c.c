#include <audio.h>
#include <sampleformat.h>
#include <float_cast.h>

#define CONVERSION_FUNC_START \
int i, j;\
for(i = 0; i < ctx->input_format->num_channels; i++)\
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

static void swap_sign_16_oe(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_16[i][j] =
    ctx->input_frame->channels.u_16[i][j] ^ 0x0080;
  CONVERSION_FUNC_END
  }

static void swap_endian_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_16[i][j] =
    SWAP_ENDIAN(ctx->input_frame->channels.u_16[i][j]);
  CONVERSION_FUNC_END
  }

static void swap_endian_sign_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_16[i][j] =
    SWAP_ENDIAN(ctx->input_frame->channels.u_16[i][j]) ^ 0x8000;
  CONVERSION_FUNC_END
  }

static void swap_sign_endian_16(gavl_audio_convert_context_t * ctx)
  {
  uint16_t tmp;
  CONVERSION_FUNC_START
  tmp = ctx->input_frame->channels.u_16[i][j] ^ 0x8000;
  ctx->output_frame->channels.u_16[i][j] = SWAP_ENDIAN(tmp);
  CONVERSION_FUNC_END
  }

static void s_8_to_s_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.s_16[i][j] =
    ctx->input_frame->channels.s_8[i][j] * 0x0101;
  CONVERSION_FUNC_END
  }

static void s_8_to_s_16_oe(gavl_audio_convert_context_t * ctx)
  {
  int16_t tmp;
  CONVERSION_FUNC_START
  tmp = ctx->input_frame->channels.s_8[i][j] * 0x0101;
  ctx->output_frame->channels.u_16[i][j] = SWAP_ENDIAN(tmp);
  CONVERSION_FUNC_END
  }

static void u_8_to_s_16(gavl_audio_convert_context_t * ctx)
  {
  int8_t tmp;
  CONVERSION_FUNC_START
  tmp = ctx->input_frame->channels.u_8[i][j] ^ 0x80;
  ctx->output_frame->channels.s_16[i][j] = tmp * 0x0101;
  CONVERSION_FUNC_END
  }

static void u_8_to_s_16_oe(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.s_16[i][j] =
    (ctx->input_frame->channels.u_8[i][j] |
     (ctx->input_frame->channels.u_8[i][j] << 8)) ^ 0x0080;
    
  CONVERSION_FUNC_END
  }

static void s_8_to_u_16(gavl_audio_convert_context_t * ctx)
  {
  uint8_t tmp;
  CONVERSION_FUNC_START
    tmp = ctx->input_frame->channels.s_8[i][j] ^ 0x80;
  ctx->output_frame->channels.u_16[i][j] = tmp | (tmp << 8);
  CONVERSION_FUNC_END
  }

static void u_8_to_u_16(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_16[i][j] =
    ctx->input_frame->channels.u_8[i][j] |
    (ctx->input_frame->channels.u_8[i][j] << 8);
  CONVERSION_FUNC_END
  }

static void convert_16_to_8(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_8[i][j] =
    ctx->input_frame->channels.u_16[i][j] >> 8;
  CONVERSION_FUNC_END
  }

static void convert_16_to_8_sign(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_8[i][j] =
    (ctx->input_frame->channels.u_16[i][j] >> 8) ^ 0x80;
  CONVERSION_FUNC_END
  }

static void convert_16_to_8_oe(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_8[i][j] =
    ctx->input_frame->channels.u_16[i][j] & 0xff;
  CONVERSION_FUNC_END
  }

static void convert_16_to_8_sign_oe(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.u_8[i][j] =
    (ctx->input_frame->channels.u_16[i][j] & 0xff) ^ 0x80;
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

static void convert_s16ne_to_float(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.f[i][j] =
    (float)(ctx->input_frame->channels.s_16[i][j])/32768.0;
  CONVERSION_FUNC_END
  }

static void convert_u16ne_to_float(gavl_audio_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START
  ctx->output_frame->channels.f[i][j] =
    (float)(ctx->input_frame->channels.u_16[i][j])/32768.0-1.0;
  CONVERSION_FUNC_END
  }

static void convert_s16oe_to_float(gavl_audio_convert_context_t * ctx)
  {
  uint16_t tmp;
  CONVERSION_FUNC_START
  tmp = SWAP_ENDIAN(ctx->input_frame->channels.s_16[i][j]);
  ctx->output_frame->channels.f[i][j] = (float)(tmp)/32768.0;
  CONVERSION_FUNC_END
  }

static void convert_u16oe_to_float(gavl_audio_convert_context_t * ctx)
  {
  int16_t tmp;
  CONVERSION_FUNC_START
  tmp = SWAP_ENDIAN(ctx->input_frame->channels.s_16[i][j]);
  ctx->output_frame->channels.f[i][j] = (float)(tmp)/32768.0-1.0;
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

static void convert_float_to_s16ne(gavl_audio_convert_context_t * ctx)
  {
  long tmp;
  CONVERSION_FUNC_START
  tmp = lrintf((ctx->input_frame->channels.f[i][j]) * 32768.0);
  CLAMP(tmp, -32768, 32767);
  ctx->output_frame->channels.s_16[i][j] = tmp;

  CONVERSION_FUNC_END
  }

static void convert_float_to_u16ne(gavl_audio_convert_context_t * ctx)
  {
  long tmp;
  CONVERSION_FUNC_START
  tmp = lrintf((ctx->input_frame->channels.f[i][j]+1.0) * 32768.0);
  CLAMP(tmp, -32768, 32767);
  ctx->output_frame->channels.u_8[i][j] = tmp;

  CONVERSION_FUNC_END
  }


static void convert_float_to_s16oe(gavl_audio_convert_context_t * ctx)
  {
  long tmp;
  CONVERSION_FUNC_START
  tmp = lrintf((ctx->input_frame->channels.f[i][j]) * 32768.0);
  CLAMP(tmp, -32768, 32767);
  ctx->output_frame->channels.u_8[i][j] = SWAP_ENDIAN(tmp);

  CONVERSION_FUNC_END
  }

static void convert_float_to_u16oe(gavl_audio_convert_context_t * ctx)
  {
  long tmp;
  CONVERSION_FUNC_START
  tmp = lrintf((ctx->input_frame->channels.f[i][j]+1.0) * 32768.0);
  CLAMP(tmp, -32768, 32767);
  ctx->output_frame->channels.u_8[i][j] = SWAP_ENDIAN(tmp);

  CONVERSION_FUNC_END
  }



void gavl_init_sampleformat_funcs_c(gavl_sampleformat_table_t * t)
  {
  t->swap_sign_8 = swap_sign_8;
  t->swap_sign_16 = swap_sign_16;
  t->swap_sign_16_oe = swap_sign_16_oe;

  t->swap_endian_16 = swap_endian_16;
  
  t->swap_sign_endian_16 = swap_sign_endian_16;
  t->swap_endian_sign_16 = swap_endian_sign_16;

  /* 8 -> 16 bits */

  t->s_8_to_s_16 = s_8_to_s_16;
  t->s_8_to_s_16_oe = s_8_to_s_16_oe;

  t->u_8_to_s_16 = u_8_to_s_16;
  t->u_8_to_s_16_oe = u_8_to_s_16_oe;

  t->s_8_to_u_16 = s_8_to_u_16;
  t->u_8_to_u_16 = u_8_to_u_16;

  /* 16 -> 8 bits */
  
  t->convert_16_to_8 = convert_16_to_8;
  t->convert_16_to_8_sign = convert_16_to_8_sign;

  t->convert_16_to_8_oe = convert_16_to_8_oe;
  t->convert_16_to_8_sign_oe = convert_16_to_8_sign_oe;

  /* Int to Float */


  t->convert_s8_to_float    = convert_s8_to_float;
  t->convert_u8_to_float    = convert_u8_to_float;
  
  t->convert_s16ne_to_float = convert_s16ne_to_float;
  t->convert_u16ne_to_float = convert_u16ne_to_float;
  
  t->convert_s16oe_to_float = convert_s16oe_to_float;
  t->convert_u16oe_to_float = convert_u16oe_to_float;

  /* Float to int */
  
  t->convert_float_to_s8    = convert_float_to_s8;
  t->convert_float_to_u8    = convert_float_to_u8;

  t->convert_float_to_s16ne = convert_float_to_s16ne;
  t->convert_float_to_u16ne = convert_float_to_u16ne;
  
  t->convert_float_to_s16oe = convert_float_to_s16oe;
  t->convert_float_to_u16oe = convert_float_to_u16oe;
  

  }
