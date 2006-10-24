/*
  Acceleration flags are NOT part of the public API
  because what the user wants in the end is to choose between
  speed and quality.

  The only place, where these functions are needed from outside
  the library, are the test programs, which must select
  individual routines.
*/

#include <string.h> /* We want the memcpy prototype anyway */

/* Acceleration flags (CPU Specific flags are in the public API) */

#define GAVL_ACCEL_C        (1<<16)
#define GAVL_ACCEL_C_HQ     (1<<17)
#define GAVL_ACCEL_C_SHQ    (1<<18) /* Super high quality, damn slow */

/* CPU Specific flags */

  

void gavl_audio_options_set_accel_flags(gavl_audio_options_t * opt,
                                        int accel_flags);
int gavl_audio_options_get_accel_flags(gavl_audio_options_t * opt);

void gavl_video_options_set_accel_flags(gavl_video_options_t * opt,
                                        int accel_flags);
int gavl_video_options_get_accel_flags(gavl_video_options_t * opt);

/* Optimized memcpy versions */

void gavl_init_memcpy();

extern void * (*gavl_memcpy)(void *to, const void *from, size_t len);
