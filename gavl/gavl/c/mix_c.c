
#include <audio.h>
#include <mix.h>

#define SRC_INDEX(i) ctx->mix_context->matrix[output_index].inputs[i]

#define RENAME(a) a ## _s8
#define SRC(i)      ctx->input_frame->channels.s_8[SRC_INDEX(i)]
#define DST()       ctx->output_frame->channels.s_8[output_index]
#define FACTOR(i)   ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].s_8
#define SAMPLE_TYPE int8_t
#define TMP_TYPE int
#define ADJUST_TMP(i) 

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef DST
#undef FACTOR
#undef SAMPLE_TYPE
#undef ADJUST_TMP

#define RENAME(a) a ## _s16
#define SRC(i)      ctx->input_frame->channels.s_16[SRC_INDEX(i)]
#define DST()       ctx->output_frame->channels.s_16[output_index]
#define FACTOR(i)   ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].s_16
#define SAMPLE_TYPE int16_t
#define TMP_TYPE int
#define ADJUST_TMP(i) 

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef DST
#undef FACTOR
#undef SAMPLE_TYPE
#undef TMP_TYPE
#undef ADJUST_TMP

#define RENAME(a) a ## _float
#define SRC(i)      ctx->input_frame->channels.f[SRC_INDEX(i)]
#define DST()       ctx->output_frame->channels.f[output_index]
#define FACTOR(i)   ctx->mix_context->matrix[output_index].factors[SRC_INDEX(i)].f
#define SAMPLE_TYPE float
#define TMP_TYPE float
#define ADJUST_TMP(i) 

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef DST
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
      break;
    case GAVL_SAMPLE_S8:
      t->mix_2_to_1 = mix_2_to_1_s8;
      t->mix_3_to_1 = mix_3_to_1_s8;
      t->mix_4_to_1 = mix_4_to_1_s8;
      t->mix_5_to_1 = mix_5_to_1_s8;
      t->mix_6_to_1 = mix_6_to_1_s8;
      break;
    case GAVL_SAMPLE_U16NE:
    case GAVL_SAMPLE_U16OE:
    case GAVL_SAMPLE_S16OE:
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
    }

  }
