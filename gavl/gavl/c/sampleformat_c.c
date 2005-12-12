#include <audio.h>
#include <sampleformat.h>
#include <float_cast.h>

#include <stdio.h>


#define SWAP_ENDIAN(n) ((n >> 8)|(n << 8))
#define CLAMP(i, min, max) if(i<min)i=min;if(i>max)i=max;

#define CONVERSION_FUNC_START \
int i, j;\
for(i = 0; i < ctx->input_format.num_channels; i++)\
  { \
  for(j = 0; j < ctx->input_frame->valid_samples; j++) \
    {
    
#define CONVERSION_FUNC_END \
    } \
  }

#define U_8 channels.u_8[i][j]
#define S_8 channels.s_8[i][j]
#define U_16 channels.u_16[i][j]
#define S_16 channels.s_16[i][j]
#define S_32 channels.s_32[i][j]
#define FLOAT channels.f[i][j]

#define RENAME(a) a ## _ni

#include "_sampleformat_c.c"

#undef CONVERSION_FUNC_START
#undef CONVERSION_FUNC_END

#undef U_8
#undef S_8
#undef U_16
#undef S_16
#undef S_32
#undef FLOAT

#undef RENAME

/* */

#define CONVERSION_FUNC_START \
int i, imax;\
imax = ctx->input_format.num_channels * ctx->input_frame->valid_samples; \
for(i = 0; i < imax; i++)\
  {
    
#define CONVERSION_FUNC_END \
  }

#define U_8 samples.u_8[i]
#define S_8 samples.s_8[i]
#define U_16 samples.u_16[i]
#define S_16 samples.s_16[i]
#define S_32 samples.s_32[i]
#define FLOAT samples.f[i]

#define RENAME(a) a ## _i

#include "_sampleformat_c.c"

void gavl_init_sampleformat_funcs_c(gavl_sampleformat_table_t * t, gavl_interleave_mode_t interleave_mode)
  {
  if(interleave_mode == GAVL_INTERLEAVE_NONE)
    return gavl_init_sampleformat_funcs_c_ni(t);
  else if(interleave_mode == GAVL_INTERLEAVE_ALL)
    return gavl_init_sampleformat_funcs_c_i(t);
  else
    {
    fprintf(stderr, "BUUUG: Unsupported interleave mode for sampleformat conversion\n");
    }
  }
