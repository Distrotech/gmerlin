
#include <audio.h>
#include <mix.h>

#define SWAP_ENDIAN(i) ((i&0xff)<<8|(i)>>16)

#define SWAP_SIGN_16(i) (i^0x8000)
#define SWAP_SIGN_8(i)  (i^0x80)

#define CLAMP(num, min, max) if(num>max)num=max;if(num<min)num=min;

#define SRC_INDEX(i) ctx->mix_context->matrix[output_index].inputs[i]

/* Signed 8 */

#define RENAME(a) a ## _s8
#define SRC(i,j)      ctx->input_frame->channels.s_8[SRC_INDEX(i)][j]
#define SETDST(i,val) ctx->output_frame->channels.s_8[output_index][i]=val
#define FACTOR(i)     ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].s_8
#define SAMPLE_TYPE   int8_t
#define TMP_TYPE int
#define ADJUST_TMP(i) i/=0x100;CLAMP(i, INT8_MIN, INT8_MAX)

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST
#undef FACTOR
#undef SAMPLE_TYPE
#undef ADJUST_TMP

/* Unsigned 8 */

#define RENAME(a) a ## _u8
#define SRC(i,j)      SWAP_SIGN_8(ctx->input_frame->channels.s_8[SRC_INDEX(i)][j])
#define SETDST(i,val) ctx->output_frame->channels.u_8[output_index][i]=SWAP_SIGN_8(val)
#define FACTOR(i)     ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].s_8
#define SAMPLE_TYPE   int8_t
#define TMP_TYPE int
#define ADJUST_TMP(i) i/=0x100;CLAMP(i, INT8_MIN, INT8_MAX)

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST
#undef FACTOR
#undef SAMPLE_TYPE
#undef ADJUST_TMP

/* Signed 16 */

#define RENAME(a) a ## _s16
#define SRC(i,j)      ctx->input_frame->channels.s_16[SRC_INDEX(i)][j]
#define SETDST(i,val) ctx->output_frame->channels.s_16[output_index][i]=val
#define FACTOR(i)     ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].s_16
#define SAMPLE_TYPE   int16_t
#define TMP_TYPE      int
#define ADJUST_TMP(i) i/=0x10000;CLAMP(i, INT16_MIN, INT16_MAX)

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST
#undef FACTOR
#undef SAMPLE_TYPE
#undef TMP_TYPE
#undef ADJUST_TMP

/* Signed 16 (other endianess) */

#define RENAME(a) a ## _s16oe
#define SRC(i,j)      SWAP_ENDIAN(ctx->input_frame->channels.s_16[SRC_INDEX(i)][j])
#define SETDST(i,val) ctx->output_frame->channels.s_16[output_index][i]=SWAP_ENDIAN(val)
#define FACTOR(i)     ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].s_16
#define SAMPLE_TYPE   int16_t
#define TMP_TYPE      int
#define ADJUST_TMP(i) i/=0x10000;CLAMP(i, INT16_MIN, INT16_MAX)

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST
#undef FACTOR
#undef SAMPLE_TYPE
#undef TMP_TYPE
#undef ADJUST_TMP

/* Unsigned 16 */

#define RENAME(a) a ## _u16
#define SRC(i,j)      SWAP_SIGN_16(ctx->input_frame->channels.s_16[SRC_INDEX(i)][j])
#define SETDST(i,val) ctx->output_frame->channels.u_16[output_index][i]=SWAP_SIGN_16(val)
#define FACTOR(i)     ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].s_16
#define SAMPLE_TYPE   int16_t
#define TMP_TYPE int
#define ADJUST_TMP(i) i/=0x10000;CLAMP(i, INT16_MIN, INT16_MAX);

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST
#undef FACTOR
#undef SAMPLE_TYPE
#undef TMP_TYPE
#undef ADJUST_TMP

/* Unsigned 16 (Other endianess) */

#define RENAME(a) a ## _u16oe
#define SRC(i,j)      SWAP_SIGN_16(SWAP_ENDIAN(ctx->input_frame->channels.s_16[SRC_INDEX(i)][j]))
#define SETDST(i,val) ctx->output_frame->channels.u_16[output_index][i]=(SWAP_ENDIAN(val)^0x0080);
#define FACTOR(i)     ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].s_16
#define SAMPLE_TYPE   int16_t
#define TMP_TYPE int
#define ADJUST_TMP(i) i/=0x10000;CLAMP(i, INT16_MIN, INT16_MAX);

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST
#undef FACTOR
#undef SAMPLE_TYPE
#undef TMP_TYPE
#undef ADJUST_TMP

/* Float */

#define RENAME(a) a ## _float
#define SRC(i,j)      ctx->input_frame->channels.f[SRC_INDEX(i)][j]
#define SETDST(i,val) ctx->output_frame->channels.f[output_index][i]=val
#define FACTOR(i)     ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].f
#define SAMPLE_TYPE   float
#define TMP_TYPE      float
#define ADJUST_TMP(i) CLAMP(i, -1.0, 1.0)

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST
#undef FACTOR
#undef SAMPLE_TYPE
#undef TMP_TYPE
#undef ADJUST_TMP

void gavl_setup_mix_funcs_c(gavl_mixer_table_t * t,
                            gavl_audio_format_t * f)
  {
  switch(f->sample_format)
    {
    case GAVL_SAMPLE_U8:
      t->mix_2_to_1 = mix_2_to_1_u8;
      t->mix_3_to_1 = mix_3_to_1_u8;
      t->mix_4_to_1 = mix_4_to_1_u8;
      t->mix_5_to_1 = mix_5_to_1_u8;
      t->mix_6_to_1 = mix_6_to_1_u8;
      break;
    case GAVL_SAMPLE_S8:
      t->mix_2_to_1 = mix_2_to_1_s8;
      t->mix_3_to_1 = mix_3_to_1_s8;
      t->mix_4_to_1 = mix_4_to_1_s8;
      t->mix_5_to_1 = mix_5_to_1_s8;
      t->mix_6_to_1 = mix_6_to_1_s8;
      break;
    case GAVL_SAMPLE_U16NE:
      t->mix_2_to_1 = mix_2_to_1_u16;
      t->mix_3_to_1 = mix_3_to_1_u16;
      t->mix_4_to_1 = mix_4_to_1_u16;
      t->mix_5_to_1 = mix_5_to_1_u16;
      t->mix_6_to_1 = mix_6_to_1_u16;
    case GAVL_SAMPLE_U16OE:
      t->mix_2_to_1 = mix_2_to_1_u16oe;
      t->mix_3_to_1 = mix_3_to_1_u16oe;
      t->mix_4_to_1 = mix_4_to_1_u16oe;
      t->mix_5_to_1 = mix_5_to_1_u16oe;
      t->mix_6_to_1 = mix_6_to_1_u16oe;
      break;
    case GAVL_SAMPLE_S16OE:
      t->mix_2_to_1 = mix_2_to_1_s16oe;
      t->mix_3_to_1 = mix_3_to_1_s16oe;
      t->mix_4_to_1 = mix_4_to_1_s16oe;
      t->mix_5_to_1 = mix_5_to_1_s16oe;
      t->mix_6_to_1 = mix_6_to_1_s16oe;
      break;
    case GAVL_SAMPLE_S16NE:
      t->mix_2_to_1 = mix_2_to_1_s16;
      t->mix_3_to_1 = mix_3_to_1_s16;
      t->mix_4_to_1 = mix_4_to_1_s16;
      t->mix_5_to_1 = mix_5_to_1_s16;
      t->mix_6_to_1 = mix_6_to_1_s16;
      break;
    case GAVL_SAMPLE_FLOAT:
      t->mix_2_to_1 = mix_2_to_1_float;
      t->mix_3_to_1 = mix_3_to_1_float;
      t->mix_4_to_1 = mix_4_to_1_float;
      t->mix_5_to_1 = mix_5_to_1_float;
      t->mix_6_to_1 = mix_6_to_1_float;
      break;
    case GAVL_SAMPLE_NONE:
      break;
    }

  }
