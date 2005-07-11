/* Acceleration flags */

#define GAVL_ACCEL_C       (1<<0)
#define GAVL_ACCEL_C_HQ    (1<<1)
#define GAVL_ACCEL_C_SHQ   (1<<2) /* Super high quality, damn slow */
  
#define GAVL_ACCEL_MMX     (1<<3)
#define GAVL_ACCEL_MMXEXT  (1<<4)

/* The following ones are unsupported right now */

#define GAVL_ACCEL_SSE     (1<<5)
#define GAVL_ACCEL_SSE2    (1<<6)
#define GAVL_ACCEL_3DNOW   (1<<7)
  
/*
 *   Return supported CPU acceleration flags
 *   Note, that GAVL_ACCEL_C and GAVL_ACCEL_C_HQ are always supported and
 *   aren't returned here
 */
  
int gavl_accel_supported();

/*
 *  This takes a flag of wanted accel flags and ANDs them with
 *  the actually supported flags. Used mostly internally.
 */

uint32_t gavl_real_accel_flags(uint32_t wanted_flags);

/*
 *  Set accel_flags from quality or vice versa
 *  (depending of what is zero)
 */

void gavl_set_conversion_parameters(uint32_t * flags, int * quality);

void gavl_audio_options_set_accel_flags(gavl_audio_options_t * opt,
                                        int accel_flags);
int gavl_audio_options_get_accel_flags(gavl_audio_options_t * opt);

void gavl_video_options_set_accel_flags(gavl_video_options_t * opt,
                                        int accel_flags);
int gavl_video_options_get_accel_flags(gavl_video_options_t * opt);
