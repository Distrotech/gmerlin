/* Acceleration flags */

#define GAVL_ACCEL_C        (1<<0)
#define GAVL_ACCEL_C_HQ     (1<<1)
#define GAVL_ACCEL_C_SHQ    (1<<2) /* Super high quality, damn slow */
  
#define GAVL_ACCEL_MMX      (1<<3)
#define GAVL_ACCEL_MMXEXT   (1<<4)

/* The following ones are unsupported right now */

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
