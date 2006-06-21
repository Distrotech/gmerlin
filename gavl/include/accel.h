/*
  Acceleration flags are NOT part of the public API
  because what the user wants in the end is to choose between
  speed and quality.

  The only place, where these functions are needed from outside
  the library, are the test programs, which must select
  individual routines.
*/

#include <string.h> /* We want the memcpy prototype anyway */

/* Acceleration flags */

#define GAVL_ACCEL_C        (1<<0)
#define GAVL_ACCEL_C_HQ     (1<<1)
#define GAVL_ACCEL_C_SHQ    (1<<2) /* Super high quality, damn slow */

/* These are returned by gavl_accel_supported() */

#define GAVL_ACCEL_MMX      (1<<3)
#define GAVL_ACCEL_MMXEXT   (1<<4)
#define GAVL_ACCEL_SSE      (1<<5)
#define GAVL_ACCEL_SSE2     (1<<6)
#define GAVL_ACCEL_SSE3     (1<<7)
#define GAVL_ACCEL_3DNOW    (1<<8)
#define GAVL_ACCEL_3DNOWEXT (1<<9)
  
/*
 *   Return supported CPU acceleration flags
 *   Note, that GAVL_ACCEL_C and GAVL_ACCEL_C_HQ are always supported and
 *   aren't returned here
 */
  
int gavl_accel_supported();

void gavl_audio_options_set_accel_flags(gavl_audio_options_t * opt,
                                        int accel_flags);
int gavl_audio_options_get_accel_flags(gavl_audio_options_t * opt);

void gavl_video_options_set_accel_flags(gavl_video_options_t * opt,
                                        int accel_flags);
int gavl_video_options_get_accel_flags(gavl_video_options_t * opt);

/* Optimized memcpy versions */

void gavl_init_memcpy();

extern void * (*gavl_memcpy)(void *to, const void *from, size_t len);
