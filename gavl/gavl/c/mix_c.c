#include "c_macros.h"
#include <audio.h>
#include <mix.h>

#define SWAP_SIGN_16(i) (i^0x8000)
#define SWAP_SIGN_8(i)  (i^0x80)

#define CLAMP(num, min, max) if(num>max)num=max;if(num<min)num=min;

#define SRC_INDEX(i) channel->inputs[i].index

/* Signed 8 */

#define RENAME(a) a ## _s8
#define SRC(i,j)      input_frame->channels.s_8[SRC_INDEX(i)][j]
#define SETDST(i,val) output_frame->channels.s_8[channel->index][i]=val
#define FACTOR(i)     channel->inputs[i].factor.f_8
#define SAMPLE_TYPE   int8_t
#define TMP_TYPE      int
#define ADJUST_TMP(i) i/=0x100;CLAMP(i, INT8_MIN, INT8_MAX)

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST

/* Unsigned 8 */

#define RENAME(a) a ## _u8
#define SRC(i,j)      SWAP_SIGN_8(input_frame->channels.s_8[SRC_INDEX(i)][j])
#define SETDST(i,val) output_frame->channels.s_8[channel->index][i]= SWAP_SIGN_8(val)

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST
#undef FACTOR
#undef SAMPLE_TYPE
#undef TMP_TYPE
#undef ADJUST_TMP

/* Signed 16 */

#define RENAME(a) a ## _s16
#define SRC(i,j)      input_frame->channels.s_16[SRC_INDEX(i)][j]
#define SETDST(i,val) output_frame->channels.s_16[channel->index][i]=val
#define FACTOR(i)     channel->inputs[i].factor.f_16
#define SAMPLE_TYPE   int16_t
#define TMP_TYPE      int
#define ADJUST_TMP(i) i/=0x10000;CLAMP(i, INT16_MIN, INT16_MAX)

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST

/* Unsigned 16 */

#define RENAME(a) a ## _u16
#define SRC(i,j)      SWAP_SIGN_16(input_frame->channels.s_16[SRC_INDEX(i)][j])
#define SETDST(i,val) output_frame->channels.s_16[channel->index][i]= SWAP_SIGN_16(val)

#include "_mix_c.c"

#undef RENAME
#undef SRC
#undef SETDST
#undef FACTOR
#undef SAMPLE_TYPE
#undef TMP_TYPE
#undef ADJUST_TMP

/* Signed 32 */

#define RENAME(a) a ## _s32
#define SRC(i,j)      input_frame->channels.s_32[SRC_INDEX(i)][j]
#define SETDST(i,val) output_frame->channels.s_32[channel->index][i]=val
#define FACTOR(i)     channel->inputs[i].factor.f_32
#define SAMPLE_TYPE   int32_t
#define TMP_TYPE      int64_t
#define ADJUST_TMP(i) i/=0x100000000LL;CLAMP(i, INT32_MIN, INT32_MAX)

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
#define SRC(i,j)      input_frame->channels.f[SRC_INDEX(i)][j]
#define SETDST(i,val) output_frame->channels.f[channel->index][i]=val
#define FACTOR(i)     channel->inputs[i].factor.f_float
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

/* Copy routines */

static void copy_8(gavl_mix_output_channel_t * channel,
                   gavl_audio_frame_t * input_frame,
                   gavl_audio_frame_t * output_frame)
  {
  GAVL_MEMCPY(output_frame->channels.s_8[channel->index],
              input_frame->channels.s_8[SRC_INDEX(0)],
              input_frame->valid_samples);
  }

static void copy_16(gavl_mix_output_channel_t * channel,
                    gavl_audio_frame_t * input_frame,
                    gavl_audio_frame_t * output_frame)
  {
  GAVL_MEMCPY(output_frame->channels.s_16[channel->index],
              input_frame->channels.s_16[SRC_INDEX(0)],
              input_frame->valid_samples*2);
  }

static void copy_32(gavl_mix_output_channel_t * channel,
                    gavl_audio_frame_t * input_frame,
                    gavl_audio_frame_t * output_frame)
  {
  GAVL_MEMCPY(output_frame->channels.s_32[channel->index],
              input_frame->channels.s_32[SRC_INDEX(0)],
              input_frame->valid_samples*4);
  }

void gavl_setup_mix_funcs_c(gavl_mixer_table_t * t,
                            gavl_audio_format_t * f)
  {
  int bytes_per_sample;

  bytes_per_sample = gavl_bytes_per_sample(f->sample_format);
  switch(bytes_per_sample)
    {
    case 1:
      t->copy_func = copy_8;
      break;
    case 2:
      t->copy_func = copy_16;
      break;
    case 4:
      t->copy_func = copy_32;
      break;
    }
  switch(f->sample_format)
    {
    case GAVL_SAMPLE_U8:
      t->mix_1_to_1 = mix_1_to_1_u8;
      t->mix_2_to_1 = mix_2_to_1_u8;
      t->mix_3_to_1 = mix_3_to_1_u8;
      t->mix_4_to_1 = mix_4_to_1_u8;
      t->mix_5_to_1 = mix_5_to_1_u8;
      t->mix_6_to_1 = mix_6_to_1_u8;
      break;
    case GAVL_SAMPLE_S8:
      t->mix_1_to_1 = mix_1_to_1_s8;
      t->mix_2_to_1 = mix_2_to_1_s8;
      t->mix_3_to_1 = mix_3_to_1_s8;
      t->mix_4_to_1 = mix_4_to_1_s8;
      t->mix_5_to_1 = mix_5_to_1_s8;
      t->mix_6_to_1 = mix_6_to_1_s8;
      break;
    case GAVL_SAMPLE_U16:
      t->mix_1_to_1 = mix_1_to_1_u16;
      t->mix_2_to_1 = mix_2_to_1_u16;
      t->mix_3_to_1 = mix_3_to_1_u16;
      t->mix_4_to_1 = mix_4_to_1_u16;
      t->mix_5_to_1 = mix_5_to_1_u16;
      t->mix_6_to_1 = mix_6_to_1_u16;
    case GAVL_SAMPLE_S16:
      t->mix_1_to_1 = mix_1_to_1_s16;
      t->mix_2_to_1 = mix_2_to_1_s16;
      t->mix_3_to_1 = mix_3_to_1_s16;
      t->mix_4_to_1 = mix_4_to_1_s16;
      t->mix_5_to_1 = mix_5_to_1_s16;
      t->mix_6_to_1 = mix_6_to_1_s16;
      break;
    case GAVL_SAMPLE_S32:
      t->mix_1_to_1 = mix_1_to_1_s32;
      t->mix_2_to_1 = mix_2_to_1_s32;
      t->mix_3_to_1 = mix_3_to_1_s32;
      t->mix_4_to_1 = mix_4_to_1_s32;
      t->mix_5_to_1 = mix_5_to_1_s32;
      t->mix_6_to_1 = mix_6_to_1_s32;
      break;
    case GAVL_SAMPLE_FLOAT:
      t->mix_1_to_1 = mix_1_to_1_float;
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
