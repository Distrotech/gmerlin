#include <gavl.h>
#include <volume.h>
#include <float_cast.h>

#define CLAMP(val, min, max) if(val < min)val=min;if(val>max)val=max

static void set_volume_s8_c(void * samples, float factor,
                            int num_samples)
  {
  int i;
  float sample;
  int32_t tmp;
  int8_t * s = (int8_t*)samples;
  
  for(i = 0; i < num_samples; i++)
    {
    sample = (float)(s[i])/128.0 * factor;
    tmp = lrintf(sample*128.0);
    CLAMP(tmp, -128, 127);
    s[i] = tmp;
    }

  }
      

static void set_volume_u8_c(void * samples, float factor,
                             int num_samples)
  {
  int i;
  float sample;
  int32_t tmp;
  uint8_t * s = (uint8_t*)samples;
  
  for(i = 0; i < num_samples; i++)
    {
    sample = ((float)(s[i])/128.0-1.0) * factor;
    tmp = lrintf((sample+1.0)*128.0);
    CLAMP(tmp, 0, 255);
    s[i] = tmp;
    }

  }

static void set_volume_s16_c(void * samples, float factor,
                              int num_samples)
  {
  int i;
  float sample;
  int32_t tmp;
  int16_t * s = (int16_t*)samples;
  
  for(i = 0; i < num_samples; i++)
    {
    sample = ((float)(s[i])/32768.0) * factor;
    tmp = lrintf(sample*32768.0);
    CLAMP(tmp, -32768, 32767);
    s[i] = tmp;
    }

  }

static void set_volume_u16_c(void * samples, float factor,
                              int num_samples)
  {
  int i;
  float sample;
  int32_t tmp;
  uint16_t * s = (uint16_t*)samples;
  
  for(i = 0; i < num_samples; i++)
    {
    sample = ((float)(s[i])/32768.0 - 1.0) * factor;
    tmp = lrintf((sample+1.0)*32768.0);
    CLAMP(tmp, 0, 65535);
    s[i] = tmp;
    }
  }

static void set_volume_s32_c(void * samples, float factor,
                              int num_samples)
  {
  int i;
  double sample;
  int64_t tmp;
  int32_t * s = (int32_t*)samples;
  
  for(i = 0; i < num_samples; i++)
    {
    sample = ((double)(s[i])/2147483648.0 - 1.0) * factor;
    tmp = llrintf(sample * 2147483648.0);
    CLAMP(tmp, -2147483648LL, 2147483647LL);
    s[i] = tmp;
    }
  }


static void set_volume_float_c(void * samples, float factor,
                               int num_samples)
  {
  int i;
  float * s = (float*)samples;
  for(i = 0; i < num_samples; i++)
    s[i] *= factor;
  }

void gavl_init_volume_funcs_c(gavl_volume_funcs_t * v)
  {
  v->set_volume_s8 = set_volume_s8_c;
  v->set_volume_u8 = set_volume_u8_c;

  v->set_volume_s16 = set_volume_s16_c;
  v->set_volume_u16 = set_volume_u16_c;
  
  v->set_volume_s32 = set_volume_s32_c;

  v->set_volume_float = set_volume_float_c;
  }
