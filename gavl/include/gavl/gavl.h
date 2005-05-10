/*****************************************************************

  gavl.h

  Copyright (c) 2001-2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

/*
 *  Gmerlin audio video library, a library for handling and conversion
 *  of uncompressed audio- and video data
 */

#ifndef GAVL_H_INCLUDED
#define GAVL_H_INCLUDED

#include <inttypes.h>
#include <gavlconfig.h>

#include "gavltime.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  SECTION 1: General stuff
 */

/* Quality levels */

#define GAVL_QUALITY_FASTEST 1
#define GAVL_QUALITY_BEST    5

#define GAVL_QUALITY_DEFAULT 2 /* Faster then standard C */
  
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
  
  
/*********************************************
 *  SECTION 2: Audio stuff
 *********************************************/
  
/* Maximum number of supported channels */
#define GAVL_MAX_CHANNELS 6

/*********************************************
 *  SECTION 2.1: Audio format definitions
 *********************************************/

/* Sample formats: all multibyte numbers are native endian */
  
typedef enum
  {
    GAVL_SAMPLE_NONE  = 0,
    GAVL_SAMPLE_U8    = 1,
    GAVL_SAMPLE_S8    = 2,
    GAVL_SAMPLE_U16   = 3,
    GAVL_SAMPLE_S16   = 4,
    GAVL_SAMPLE_S32   = 5,
    GAVL_SAMPLE_FLOAT = 6
  } gavl_sample_format_t;

/* Interleave modes */
  
typedef enum
  {
    GAVL_INTERLEAVE_NONE = 0, /* No interleaving, all channels separate */
    GAVL_INTERLEAVE_2    = 1, /* Interleaved pairs of channels          */ 
    GAVL_INTERLEAVE_ALL  = 2  /* Everything interleaved                 */
  } gavl_interleave_mode_t;

/*
 *  Audio channel setup: This can be used with
 *  AC3 decoders to support all speaker configurations
 */
  
typedef enum
  {
    GAVL_CHANNEL_NONE   = 0,
    GAVL_CHANNEL_MONO   = 1,
    GAVL_CHANNEL_STEREO = 2, /* 2 Front channels (Stereo or Dual channels) */
    GAVL_CHANNEL_3F     = 3,
    GAVL_CHANNEL_2F1R   = 4,
    GAVL_CHANNEL_3F1R   = 5,
    GAVL_CHANNEL_2F2R   = 6,
    GAVL_CHANNEL_3F2R   = 7
  } gavl_channel_setup_t;

/* Channel IDs */
  
typedef enum
  {
    GAVL_CHID_NONE         = 0,
    GAVL_CHID_FRONT,
    GAVL_CHID_FRONT_LEFT,
    GAVL_CHID_FRONT_RIGHT,
    GAVL_CHID_FRONT_CENTER,
    GAVL_CHID_REAR,
    GAVL_CHID_REAR_LEFT,
    GAVL_CHID_REAR_RIGHT,
    GAVL_CHID_LFE
  } gavl_channel_id_t;
  
/* Structure describing an audio format */
  
typedef struct gavl_audio_format_s
  {
  int samples_per_frame;  /* Maximum number of samples per frame */
  int samplerate;
  int num_channels;
  gavl_sample_format_t   sample_format;
  gavl_interleave_mode_t interleave_mode;
  gavl_channel_setup_t   channel_setup;
  int lfe;            /* Low frequency effect channel present */
  
  float center_level; /* linear factor for mixing center to front */
  float rear_level;   /* linear factor for mixing rear to front */

  /* Which channel is stored where */

  gavl_channel_id_t channel_locations[GAVL_MAX_CHANNELS];
  } gavl_audio_format_t;

/* Audio format -> string conversions */
  
const char * gavl_sample_format_to_string(gavl_sample_format_t);
const char * gavl_channel_id_to_string(gavl_channel_id_t);
const char * gavl_channel_setup_to_string(gavl_channel_setup_t);
const char * gavl_interleave_mode_to_string(gavl_interleave_mode_t);

void gavl_audio_format_dump(const gavl_audio_format_t *);

int gavl_channel_index(const gavl_audio_format_t *, gavl_channel_id_t);

int gavl_front_channels(const gavl_audio_format_t *);
int gavl_rear_channels(const gavl_audio_format_t *);
int gavl_lfe_channels(const gavl_audio_format_t *);

int gavl_num_channels(gavl_channel_setup_t);

  
/* Copy audio format */

void gavl_audio_format_copy(gavl_audio_format_t * dst,
                            const gavl_audio_format_t * src);

/*
 *  This sets the channel_setup and lfe of the format according
 *  to num_channels.
 *  The result is not necessarily correct, but helps for file formats
 *  lacking this information. As least it's correct for mono and stereo
 *  streams.
 */
  
void gavl_set_channel_setup(gavl_audio_format_t * dst);

/* Convenience function */

int gavl_bytes_per_sample(gavl_sample_format_t format);

  
/*********************************************
 *  SECTION 2.2: Audio frame definitions
 *********************************************/
    
/* Container for interleaved audio samples */

typedef union gavl_audio_samples_u
  {
  uint8_t * u_8;
  int8_t *  s_8;

  uint16_t * u_16;
  int16_t  * s_16;
  
  uint32_t * u_32;
  int32_t  * s_32;
  
  float * f;
  } gavl_audio_samples_t;

/* Container for noninterleaved audio channels */
  
typedef union gavl_audio_channels_u
  {
  uint8_t * u_8[GAVL_MAX_CHANNELS];
  int8_t *  s_8[GAVL_MAX_CHANNELS];

  uint16_t * u_16[GAVL_MAX_CHANNELS];
  int16_t  * s_16[GAVL_MAX_CHANNELS];
    
  uint32_t * u_32[GAVL_MAX_CHANNELS];
  int32_t  * s_32[GAVL_MAX_CHANNELS];

  float * f[GAVL_MAX_CHANNELS];
  
  } gavl_audio_channels_t;

/* Audio frame */
  
typedef struct gavl_audio_frame_s
  {
  gavl_audio_samples_t  samples;
  gavl_audio_channels_t channels;
  int valid_samples;              /* Real number of samples */
  } gavl_audio_frame_t;

/* Create audio frame. If format is NULL, no memory is allocated,
   so you can set your own pointers here */
  
gavl_audio_frame_t * gavl_audio_frame_create(const gavl_audio_format_t*);

/* Zero all pointers, so gavl_audio_frame_detsroy won't free them.
   Call this for audio frames, which were creates with a NULL format
   before destroying them */
  
void gavl_audio_frame_null(gavl_audio_frame_t * f);

/* Destroy an audio frame */
  
void gavl_audio_frame_destroy(gavl_audio_frame_t *);

/* Make it silent */  
  
void gavl_audio_frame_mute(gavl_audio_frame_t *,
                           const gavl_audio_format_t *);
  
/* Function for copying audio frames, returnes the number of copied samples */

/*
 *  in_size and out_size are the number of input- and output samples,
 *  respectively. The function will copy the smaller number of samples
 *  and return the number of copied samples.
 *
 *  src_pos and dst_pos are offsets from the start of the sample buffer
 *  in samples
 */
  
int gavl_audio_frame_copy(gavl_audio_format_t * format,
                          gavl_audio_frame_t * dst,
                          gavl_audio_frame_t * src,
                          int dst_pos,
                          int src_pos,
                          int dst_size,
                          int src_size);

/*********************************************
 *  SECTION 2.3: Audio converter
 *********************************************/

/* Conversion flags */
  
#define GAVL_AUDIO_DOWNMIX_DOLBY      (1<<0)

/* Options for mixing front to rear channels */

#define GAVL_AUDIO_FRONT_TO_REAR_COPY (1<<1) /* Just copy                                 */
#define GAVL_AUDIO_FRONT_TO_REAR_MUTE (1<<2) /* Mute rear channels                        */
#define GAVL_AUDIO_FRONT_TO_REAR_DIFF (1<<3) /* Send the difference between front to rear */

#define GAVL_AUDIO_FRONT_TO_REAR_MASK \
(GAVL_AUDIO_FRONT_TO_REAR_COPY | \
GAVL_AUDIO_FRONT_TO_REAR_MUTE | \
 GAVL_AUDIO_FRONT_TO_REAR_DIFF)

/* Options for mixing stereo to mono */
  
#define GAVL_AUDIO_STEREO_TO_MONO_LEFT  (1<<4) /* Left channel       */
#define GAVL_AUDIO_STEREO_TO_MONO_RIGHT (1<<5) /* Right channel      */
#define GAVL_AUDIO_STEREO_TO_MONO_MIX   (1<<6) /* Mix left and right */

#define GAVL_AUDIO_STEREO_TO_MONO_MASK \
(GAVL_AUDIO_STEREO_TO_MONO_LEFT | \
GAVL_AUDIO_STEREO_TO_MONO_RIGHT | \
GAVL_AUDIO_STEREO_TO_MONO_MIX)
  
typedef struct gavl_audio_options_s gavl_audio_options_t;

void gavl_audio_options_set_quality(gavl_audio_options_t * opt, int quality);

void gavl_audio_options_set_accel_flags(gavl_audio_options_t * opt,
                                        int accel_flags);

void gavl_audio_options_set_conversion_flags(gavl_audio_options_t * opt,
                                             int conversion_flags);
  
int gavl_audio_options_get_accel_flags(gavl_audio_options_t * opt);
int gavl_audio_options_get_conversion_flags(gavl_audio_options_t * opt);

void gavl_audio_options_set_defaults(gavl_audio_options_t * opt);

/* Audio converter */

typedef struct gavl_audio_converter_s gavl_audio_converter_t;
  
gavl_audio_converter_t * gavl_audio_converter_create();

void gavl_audio_converter_destroy(gavl_audio_converter_t*);

gavl_audio_options_t * gavl_audio_converter_get_options(gavl_audio_converter_t*);

void gavl_audio_options_copy(gavl_audio_options_t * dst,
                             const gavl_audio_options_t * src);

int gavl_audio_converter_init(gavl_audio_converter_t* cnv,
                              const gavl_audio_format_t * input_format,
                              const gavl_audio_format_t * output_format);
  
/* Convert audio  */

/*
 *  Be careful when resampling: gavl will
 *  assume, that your output frame is big enough.
 *  Make it e.q. 10 samples bigger than
 *  input_frame_size * output_samplerate / input_samplerate
 */
  
void gavl_audio_convert(gavl_audio_converter_t * cnv,
                        gavl_audio_frame_t * input_frame,
                        gavl_audio_frame_t * output_frame);

/**********************************************
 * Section 2.4: Volume control
 **********************************************/

typedef struct gavl_volume_control_s gavl_volume_control_t;

/* Create / destroy */
  
gavl_volume_control_t * gavl_volume_control_create();
void gavl_volume_control_destroy(gavl_volume_control_t *);

/* Set format: can be called multiple times with one instance */

void gavl_volume_control_set_format(gavl_volume_control_t *,
                                    gavl_audio_format_t * format);

/* Set volume: volume is in dB */
  
void gavl_volume_control_set_volume(gavl_volume_control_t *,
                                    float volume);

/* Apply the volume control to one audio frame */
  
void gavl_volume_control_apply(gavl_volume_control_t *,
                               gavl_audio_frame_t * frame);
  
/**********************************************
 *  Section 3: Video stuff
 **********************************************/

#define GAVL_MAX_PLANES 4
  
/**********************************************
 *  Section 3.1: Video format definitions
 **********************************************/
  
typedef enum 
  {
    GAVL_COLORSPACE_NONE =  0,
    GAVL_RGB_15          =  1,
    GAVL_BGR_15          =  2,
    GAVL_RGB_16          =  3,
    GAVL_BGR_16          =  4,
    GAVL_RGB_24          =  5,
    GAVL_BGR_24          =  6,
    GAVL_RGB_32          =  7,
    GAVL_BGR_32          =  8,
    GAVL_RGBA_32         =  9,
    GAVL_YUY2            = 10,
    GAVL_UYVY            = 11,
    GAVL_YUV_420_P       = 12,
    GAVL_YUV_422_P       = 13,
    GAVL_YUV_444_P       = 14,
    GAVL_YUV_411_P       = 15,
    GAVL_YUV_410_P       = 16,
    
    /* JPEG compliant YUV formats */
    GAVL_YUVJ_420_P      = 17,
    GAVL_YUVJ_422_P      = 18,
    GAVL_YUVJ_444_P      = 19,

    /* TODO: High Quality formats (16 Bits/channel) */

    // GAVL_YUV_48       = 20
    // GAVL_YUVA_64      = 21
    // GAVL_RGB_48       = 22
    // GAVL_RGBA_64      = 23
    
  } gavl_colorspace_t;

/*
 *  Colormodel related functions
 */

int gavl_colorspace_is_rgb(gavl_colorspace_t colorspace);
int gavl_colorspace_is_yuv(gavl_colorspace_t colorspace);
int gavl_colorspace_has_alpha(gavl_colorspace_t colorspace);
int gavl_colorspace_is_planar(gavl_colorspace_t colorspace);

/* Get the number of planes (=1 for packet formats) */  
int gavl_colorspace_num_planes(gavl_colorspace_t csp);

/*
 *  Get the horizontal and vertical subsampling factors
 *  (e.g. for 420: sub_h = 2, sub_v = 2)
 */

void gavl_colorspace_chroma_sub(gavl_colorspace_t csp, int * sub_h, int * sub_v);

/* bytes_per_component is only valid for planar formats */
  
int gavl_colorspace_bytes_per_component(gavl_colorspace_t csp);

/* bytes_per_pixel is only valid for packed formats */

int gavl_colorspace_bytes_per_pixel(gavl_colorspace_t csp);

  
const char * gavl_colorspace_to_string(gavl_colorspace_t colorspace);
gavl_colorspace_t gavl_string_to_colorspace(const char *);

/* Query cvolorspaces */
  
int gavl_num_colorspaces();
gavl_colorspace_t gavl_get_colorspace(int index);

/* Video format structure */
  
typedef struct 
  {
  /* Physical size of the frame fuffers */
  
  int frame_width;
  int frame_height;

  /* Width and height of the actual displayed image */
    
  int image_width;
  int image_height;
  
  /* Support for nonsquare pixels */
    
  int pixel_width;
  int pixel_height;
    
  gavl_colorspace_t colorspace;

  int frame_duration;
  int timescale;
  
  int free_framerate;   /* If 1, framerate will be based on timestamps only */
  } gavl_video_format_t;


void gavl_video_format_copy(gavl_video_format_t * dst,
                            const gavl_video_format_t * src);

void gavl_video_format_dump(const gavl_video_format_t *);

void gavl_video_format_fit_to_source(gavl_video_format_t * dst,
                                     const gavl_video_format_t * src);
  
/* Rectangle */

typedef struct
  {
  int x;
  int y;
  int w;
  int h;
  } gavl_rectangle_t;

void gavl_rectangle_dump(const gavl_rectangle_t * r);

  
void gavl_rectangle_crop_to_format(gavl_rectangle_t * r,
                                   const gavl_video_format_t * format);

void gavl_rectangle_crop_to_format_scale(gavl_rectangle_t * src_rect,
                                         gavl_rectangle_t * dst_rect,
                                         const gavl_video_format_t * src_format,
                                         const gavl_video_format_t * dst_format);

/*
 *  This produces 2 rectangles of the same width centered on src_format and dst_format
 *  respectively
 */
  
void gavl_rectangle_crop_to_format_noscale(gavl_rectangle_t * src_rect,
                                           gavl_rectangle_t * dst_rect,
                                           const gavl_video_format_t * src_format,
                                           const gavl_video_format_t * dst_format);
  
/* Let a rectangle span the whole screen for format */

void gavl_rectangle_set_all(gavl_rectangle_t * r, const gavl_video_format_t * format);

void gavl_rectangle_crop_left(gavl_rectangle_t * r, int num_pixels);
void gavl_rectangle_crop_right(gavl_rectangle_t * r, int num_pixels);
void gavl_rectangle_crop_top(gavl_rectangle_t * r, int num_pixels);
void gavl_rectangle_crop_bottom(gavl_rectangle_t * r, int num_pixels);

void gavl_rectangle_align(gavl_rectangle_t * r, int h_align, int v_align);

void gavl_rectangle_copy(gavl_rectangle_t * dst, const gavl_rectangle_t * src);

int gavl_rectangle_is_empty(const gavl_rectangle_t * r);

/*
  For a Rectangle in the Luminance plane, calculate the corresponding rectangle
  in chroma plane using the given subsampling factors.
  It is wise to call gavl_rectangle_align before.
*/
  
void gavl_rectangle_subsample(gavl_rectangle_t * dst, const gavl_rectangle_t * src,
                              int sub_h, int sub_v);

/*
 * Assuming we take src_rect from a frame in format src_format,
 * calculate the optimal dst_rect in dst_format. The source and destination
 * display aspect ratio will be unchanged
 * Zoom is a zoom factor (1.0 = 100 %), Squeeze is a value between -1.0 and 1.0,
 * while squeeze changes the apsect ratio in both directions. 0.0 means unchanged
 */
  
void gavl_rectangle_fit_aspect(gavl_rectangle_t * r,
                               const gavl_video_format_t * src_format,
                               const gavl_rectangle_t * src_rect,
                               gavl_video_format_t * dst_format,
                               float zoom, float squeeze);
  
/**********************************************
 *  Section 3.2: Video frame definitions
 **********************************************/

typedef struct gavl_video_frame_s
  {
  uint8_t * planes[GAVL_MAX_PLANES];
  int strides[GAVL_MAX_PLANES];
  
  void * user_data;    /* For storing private data             */
  
  gavl_time_t time;    /* Timestamp */
  int64_t time_scaled; /* Timestamp in stream specific units   */
  int duration_scaled; /* Duration in stream specific units */
  } gavl_video_frame_t;

/* Create a video frame. Passing NULL for the format doesn't allocate any
   memory for the scanlines, so you can fill in your custom pointers then */
  
gavl_video_frame_t * gavl_video_frame_create(const gavl_video_format_t*);

/* Destroy video frame */

void gavl_video_frame_destroy(gavl_video_frame_t*);

/* Zero all pointers of a video frame call this before destroying
   the frame, if you used your custom memory */

void gavl_video_frame_null(gavl_video_frame_t*);
  
/* Clear video frame (make it black) */

void gavl_video_frame_clear(gavl_video_frame_t * frame,
                            gavl_video_format_t * format);
  
/* Copy video frame */

void gavl_video_frame_copy(gavl_video_format_t * format,
                           gavl_video_frame_t * dst,
                           gavl_video_frame_t * src);

/* Copy with flipping */
  
void gavl_video_frame_copy_flip_x(gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  gavl_video_frame_t * src);

void gavl_video_frame_copy_flip_y(gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  gavl_video_frame_t * src);

void gavl_video_frame_copy_flip_xy(gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  gavl_video_frame_t * src);

/* Get a subframe (without copying) */

void gavl_video_frame_get_subframe(gavl_colorspace_t colorspace,
                                   gavl_video_frame_t * src,
                                   gavl_video_frame_t * dst,
                                   gavl_rectangle_t * src_rect);
  
  
/*
  This is purely for debugging purposes:
  Dump all planes into files names <filebase>.p1,
  <filebase>.p2 etc
*/

void gavl_video_frame_dump(gavl_video_frame_t *,
                           gavl_video_format_t *,
                           const char * namebase);

/* Conversion options */
  
// #define GAVL_SCANLINE (1<<0)
#define GAVL_KEEP_APSPECT (1<<0)

typedef enum
  {
    GAVL_ALPHA_IGNORE      = 0, /* Ignore alpha channel      */
    GAVL_ALPHA_BLEND_COLOR      /* Blend in background color */
    //    GAVL_ALPHA_BLEND_IMAGE /* Blend over an image       */
  } gavl_alpha_mode_t;

typedef enum
  {
    GAVL_SCALE_NONE = 0,
    GAVL_SCALE_AUTO,    /* Take from conversion quality */
    GAVL_SCALE_NEAREST,
    GAVL_SCALE_BILINEAR,
  } gavl_scale_mode_t;

typedef struct gavl_video_options_s gavl_video_options_t;
  

/* Default Options */

// gavl_video_options_t * gavl_video_options_create();
// void gavl_video_options_destroy(gavl_video_options_t *);
void gavl_video_options_set_defaults(gavl_video_options_t * opt);
  
void gavl_video_options_set_rectangles(gavl_video_options_t * opt,
                                       const gavl_rectangle_t * src_rect,
                                       const gavl_rectangle_t * dst_rect);

void gavl_video_options_set_quality(gavl_video_options_t * opt, int quality);

void gavl_video_options_set_accel_flags(gavl_video_options_t * opt,
                                        int accel_flags);
  
void gavl_video_options_set_conversion_flags(gavl_video_options_t * opt,
                                             int conversion_flags);

void gavl_video_options_set_alpha_mode(gavl_video_options_t * opt,
                                       gavl_alpha_mode_t alpha_mode);

void gavl_video_options_set_scale_mode(gavl_video_options_t * opt,
                                       gavl_scale_mode_t scale_mode);

void gavl_video_options_set_background_color(gavl_video_options_t * opt,
                                             float * color);

int gavl_video_options_get_accel_flags(gavl_video_options_t * opt);
int gavl_video_options_get_conversion_flags(gavl_video_options_t * opt);
  
/***************************************************
 * Create and destroy video converters
 ***************************************************/

typedef struct gavl_video_converter_s gavl_video_converter_t;

gavl_video_converter_t * gavl_video_converter_create();

void gavl_video_converter_destroy(gavl_video_converter_t*);


/**************************************************
 * Get options. Change the options with the gavl_video_options_set_*
 * functions above
 **************************************************/

gavl_video_options_t *
gavl_video_converter_get_options(gavl_video_converter_t*);

/***************************************************
 * Set formats
 ***************************************************/

int gavl_video_converter_init(gavl_video_converter_t* cnv,
                              const gavl_video_format_t * input_format,
                              const gavl_video_format_t * output_format);

/***************************************************
 * Convert a frame
 ***************************************************/

void gavl_video_convert(gavl_video_converter_t * cnv,
                        gavl_video_frame_t * input_frame,
                        gavl_video_frame_t * output_frame);

/***************************************************
 * Video scaling support
 ***************************************************/

typedef struct gavl_video_scaler_s gavl_video_scaler_t;

gavl_video_scaler_t * gavl_video_scaler_create();
void gavl_video_scaler_destroy(gavl_video_scaler_t *);

gavl_video_options_t *
gavl_video_scaler_get_options(gavl_video_scaler_t*);
  
/*
 *  Initialize scaler. src_rect and dst_rect might be changed by this
 *  call. Return -1 if no conversion function was found for the selected
 *  accel flags
 */

int gavl_video_scaler_init(gavl_video_scaler_t * scaler,
                            gavl_colorspace_t colorspace,
                            gavl_rectangle_t * src_rect,
                            gavl_rectangle_t * dst_rect,
                            const gavl_video_format_t * src_format,
                            const gavl_video_format_t * dst_format);

void gavl_video_scaler_scale(gavl_video_scaler_t * scaler,
                             gavl_video_frame_t * src,
                             gavl_video_frame_t * dst);

  
#ifdef __cplusplus
}
#endif

#endif /* GAVL_H_INCLUDED */
