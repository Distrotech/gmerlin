/*****************************************************************

  gavl.h

  Copyright (c) 2001-2002 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

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
 *  Gmerlin audio video library, a library for conversion of uncompressed
 *  audio- and video data
 */

#ifndef GAVL_H_INCLUDED
#define GAVL_H_INCLUDED


#include <inttypes.h>
#include "gavlconfig.h"

#include "gavltime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Acceleration flags */

#define GAVL_ACCEL_C       (1<<0)
#define GAVL_ACCEL_C_HQ    (1<<1)

#define GAVL_ACCEL_MMX     (1<<2)
#define GAVL_ACCEL_MMXEXT  (1<<3)
#define GAVL_ACCEL_SSE     (1<<4)
#define GAVL_ACCEL_SSE2    (1<<5)
#define GAVL_ACCEL_3DNOW   (1<<6)
  
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

int gavl_real_accel_flags(int wanted_flags);
  
/*************************************
 *  Audio conversions
 *************************************/

typedef struct gavl_audio_converter_s gavl_audio_converter_t;

typedef union gavl_audio_samples_u
  {
  uint8_t * u_8;
  int8_t *  s_8;

  uint16_t * u_16;
  int16_t  * s_16;

  /* We don't have 32 bit ints yet, but we need a
     generic 32 bit type if we handle floats for example */
  
  uint32_t * u_32;
  int32_t  * s_32;
  
  float * f;
  } gavl_audio_samples_t;

typedef union gavl_audio_channels_u
  {
  uint8_t ** u_8;
  int8_t **  s_8;

  uint16_t ** u_16;
  int16_t  ** s_16;

  /* We don't have 32 bit ints yet, but we need a
     generic 32 bit type if we handle floats for example */
  
  uint32_t ** u_32;
  int32_t  ** s_32;

  float ** f;
  
  } gavl_audio_channels_t;
  
typedef struct gavl_audio_frame_s
  {
  gavl_audio_samples_t  samples;
  gavl_audio_channels_t channels;
  int valid_samples;              /* Real number of samples */
  } gavl_audio_frame_t;

typedef enum
  {
    GAVL_SAMPLE_NONE  = 0,
    GAVL_SAMPLE_U8    = 1,
    GAVL_SAMPLE_S8    = 2,
    GAVL_SAMPLE_U16LE = 3,
    GAVL_SAMPLE_S16LE = 4,
    GAVL_SAMPLE_U16BE = 5,
    GAVL_SAMPLE_S16BE = 6,
    GAVL_SAMPLE_FLOAT = 7
  } gavl_sample_format_t;

#ifdef  GAVL_PROCESSOR_BIG_ENDIAN
#define GAVL_SAMPLE_U16NE GAVL_SAMPLE_U16BE
#define GAVL_SAMPLE_S16NE GAVL_SAMPLE_S16BE
#define GAVL_SAMPLE_U16OE GAVL_SAMPLE_U16LE
#define GAVL_SAMPLE_S16OE GAVL_SAMPLE_S16LE
#else
#define GAVL_SAMPLE_U16NE GAVL_SAMPLE_U16LE
#define GAVL_SAMPLE_S16NE GAVL_SAMPLE_S16LE
#define GAVL_SAMPLE_U16OE GAVL_SAMPLE_U16BE
#define GAVL_SAMPLE_S16OE GAVL_SAMPLE_S16BE
#endif

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

/*
   This is the GAVL Channel order:
   GAVL_CHANNEL_MONO  |Front  | (lfe) |      |      |      |      |
   GAVL_CHANNEL_1     |Front  | (lfe) |      |      |      |      |
   GAVL_CHANNEL_2     |Front  | (lfe) |      |      |      |      |
   GAVL_CHANNEL_2F    |Front L|Front R| (lfe)|      |      |      |
   GAVL_CHANNEL_3F    |Front L|Front R|Center| (lfe)|      |      |
   GAVL_CHANNEL_2F1R  |Front L|Front R|Rear  | (lfe)|      |      |
   GAVL_CHANNEL_3F1R  |Front L|Front R|Rear  |Center| (lfe)|      |
   GAVL_CHANNEL_2F2R  |Front L|Front R|Rear L|Rear R|Center|      |
   GAVL_CHANNEL_3F2R  |Front L|Front R|Rear L|Rear R|Center| (lfe)|
*/
  
typedef enum
  {
    GAVL_CHANNEL_NONE = 0,
    GAVL_CHANNEL_MONO = 1,
    GAVL_CHANNEL_1    = 2,      /* First (left) channel */
    GAVL_CHANNEL_2    = 3,      /* Second (right) channel */
    GAVL_CHANNEL_2F   = 4,     /* 2 Front channels (Stereo or Dual channels) */
    GAVL_CHANNEL_3F   = 5,
    GAVL_CHANNEL_2F1R = 6,
    GAVL_CHANNEL_3F1R = 7,
    GAVL_CHANNEL_2F2R = 8,
    GAVL_CHANNEL_3F2R = 9
  } gavl_channel_setup_t;

/* Structure describing an audio format */
  
typedef struct gavl_audio_format_s
  {
  int samples_per_frame; /* Maximum number of samples per frame */
  int samplerate;
  int num_channels;
  gavl_sample_format_t   sample_format;
  gavl_interleave_mode_t interleave_mode;
  gavl_channel_setup_t   channel_setup;
  int lfe;
  } gavl_audio_format_t;

/* Audio format <-> string conversions */
  
const char * gavl_sample_format_to_string(gavl_sample_format_t);
const char * gavl_channel_setup_to_string(gavl_channel_setup_t);
const char * gavl_interleave_mode_to_string(gavl_interleave_mode_t);

void gavl_audio_format_dump(const gavl_audio_format_t *);
  
  
/* Copy audio format */

void gavl_audio_format_copy(gavl_audio_format_t * dst,
                            const gavl_audio_format_t * src);

/*
 *  This sets the channel_setup and lfe of the format according
 *  to num_channels.
 *  The result is not necessarily correct, but helps for file formats
 *  lacking this information. As least it's correct for mono and stereo
 *  strams.
 */
  
void gavl_set_channel_setup(gavl_audio_format_t * dst);
  
  
/* Maximum number of supported channels */
  
#define GAVL_MAX_CHANNELS 6
  
#define GAVL_AUDIO_DO_BUFFER     (1<<0)
#define GAVL_AUDIO_DOWNMIX_DOLBY (1<<1)
  
typedef struct
  {
  int accel_flags;          /* CPU Acceleration flags */
  int conversion_flags;
  } gavl_audio_options_t;

/* Convenience function */

int gavl_bytes_per_sample(gavl_sample_format_t format);
  
/* Create/destroy audio frame */  
  
gavl_audio_frame_t * gavl_audio_frame_create(const gavl_audio_format_t*);

void gavl_audio_frame_destroy(gavl_audio_frame_t *);

void gavl_audio_frame_mute(gavl_audio_frame_t *,
                           const gavl_audio_format_t *);

  /* Zero all pointers, so gavl_audio_frame_detsroy won't free them */
  
void gavl_audio_frame_null(gavl_audio_frame_t * f);

  
/* Function for copying audio frames, returnes the number of copied samples */

/*
 *  in_size are the number of input samples, out_size the number of output samples,
 *  the function will copy the smaller of both samples and return the number of
 *  copied samples
 */
  
int gavl_audio_frame_copy(gavl_audio_format_t * format,
                          gavl_audio_frame_t * dst,
                          gavl_audio_frame_t * src,
                          int dst_pos,
                          int src_pos,
                          int dst_size,
                          int src_size);
  
/* Create/Destroy audio converter */

gavl_audio_converter_t * gavl_audio_converter_create();

void gavl_audio_converter_destroy(gavl_audio_converter_t*);

void gavl_audio_default_options(gavl_audio_options_t * opt);

int gavl_audio_init(gavl_audio_converter_t* cnv,
                    const gavl_audio_options_t * options,
                    const gavl_audio_format_t * input_format,
                    const gavl_audio_format_t * output_format);

/* Convert audio  */
  
int gavl_audio_convert(gavl_audio_converter_t * cnv,
                       gavl_audio_frame_t * input_frame,
                       gavl_audio_frame_t * output_frame);

/* Audio buffer: Can be used if samples_per_frame is different in the
   input and output format, or if the samples, which will be read at
   once, cannot be determined at start */

typedef struct gavl_audio_buffer_s gavl_audio_buffer_t;

/*
 *  Create/destroy the buffer. The samples_per_frame member
 *  should be the desired number at the output
 */
  
gavl_audio_buffer_t * gavl_audio_buffer_create(gavl_audio_format_t *);

void gavl_destroy_audio_buffer(gavl_audio_buffer_t *);

  /*
   *  Buffer audio
   *
   *  output frame is full, if valid_samples == samples_per_frame
   *
   *  If return value is 0, input frame is needed some more times
   */
  
int gavl_buffer_audio(gavl_audio_buffer_t * b,
                      gavl_audio_frame_t * in,
                      gavl_audio_frame_t * out);
  
  
/**********************************************
 *  Video conversions routines
 **********************************************/
  
typedef struct gavl_video_converter_s gavl_video_converter_t;

typedef struct gavl_video_frame_s
  {
  uint8_t * planes[4];
  int strides[4];

  /* For planar formsts */
  
  //  uint8_t * y;
  //  uint8_t * u;
  //  uint8_t * v;

  //  int y_stride;
  //  int u_stride;
  //  int v_stride;
 
  /* For packed formats */
  
  //  uint8_t * pixels;
  //  int pixels_stride;
  
  void * user_data;   /* For storing private data             */

  gavl_time_t time;       /* us timestamp */
  
  } gavl_video_frame_t;

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
    GAVL_YUV_420_P       = 11,
    GAVL_YUV_422_P       = 12,
    /* TODO */
    GAVL_YUV_444_P       = 13,
    //    GAVL_YUV_411_P       = 14,
    //    GAVL_YUV_410_P       = 15,
    
    GAVL_YUVJ_420_P      = 16,
    GAVL_YUVJ_422_P      = 17,
    GAVL_YUVJ_444_P      = 18,
    
  } gavl_colorspace_t;

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

  int framerate_num;
  int framerate_den;
  
  int free_framerate;   /* Framerate will be based on timestamps only */
  } gavl_video_format_t;

void gavl_video_format_copy(gavl_video_format_t * dst,
                            const gavl_video_format_t * src);

void gavl_video_format_dump(const gavl_video_format_t *);
  
#define GAVL_SCANLINE (1<<0)

typedef struct
  {
  int accel_flags; /* CPU Acceleration flags */
  int conversion_flags;

  float crop_factor; /* Not used yet (for scaling) */
  
  /* Background color (0x0000 - 0xFFFF) */

  uint16_t background_red;
  uint16_t background_green;
  uint16_t background_blue;
  
  } gavl_video_options_t;

/***************************************************
 * Create and destroy video converters
 ***************************************************/

gavl_video_converter_t * gavl_video_converter_create();

void gavl_video_converter_destroy(gavl_video_converter_t*);

/***************************************************
 * Default Options
 ***************************************************/

void gavl_video_default_options(gavl_video_options_t * opt);

/***************************************************
 * Set formats
 ***************************************************/

int gavl_video_init(gavl_video_converter_t* cnv,
                    const gavl_video_options_t * options,
                    const gavl_video_format_t * input_format,
                    const gavl_video_format_t * output_format);

/***************************************************
 * Convert a frame
 * Returns 1 if the input frame is no longer used
 * If the timestamp of the output frame is > 0
 ***************************************************/

void gavl_video_convert(gavl_video_converter_t * cnv,
                        gavl_video_frame_t * input_frame,
                        gavl_video_frame_t * output_frame);

/*******************************************************
 * Create a video frame with memory aligned scanlines
 *******************************************************/

gavl_video_frame_t * gavl_video_frame_create(const gavl_video_format_t*);

/*******************************************************
 * Destroy video frame 
 *******************************************************/

void gavl_video_frame_destroy(gavl_video_frame_t*);

/*****************************************************
 *  Zero all pointers of a video frame
 *******************************************************/

void gavl_video_frame_null(gavl_video_frame_t*);

/*****************************************************
 *  Allocate a video frame for a given format
 *****************************************************/

void gavl_video_frame_alloc(gavl_video_frame_t * frame,
                            const gavl_video_format_t * format);

/*****************************************************
 *  Free memory of a video frame
 *****************************************************/

void gavl_video_frame_free(gavl_video_frame_t*);
  
/*******************************************************
 * Clear video frame (make it black)
 *******************************************************/

void gavl_clear_video_frame(gavl_video_frame_t * frame,
                            gavl_video_format_t * format);

/********************************************************
 *  Copy one video frame to another as quickly as possible
 **********************************************************/

void gavl_video_frame_copy(gavl_video_format_t * format,
                           gavl_video_frame_t * dst,
                           gavl_video_frame_t * src);

/**********************************************************
  This is purely for optimizing purposes:
  Dump all planes into files names <filebase>.p1,
  <filebase>.p2 etc
***********************************************************/

void gavl_video_frame_dump(gavl_video_frame_t *,
                           gavl_video_format_t *,
                           const char * namebase);
  
  

/*
 *  Video framerate converter
 *  Simple converter, which is works by
 *  repeating/dropping pictures
 */

typedef struct gavl_framerate_converter_s gavl_framerate_converter_t;

gavl_framerate_converter_t * gavl_framerate_converter_create();

void
gavl_framerate_converter_init(gavl_framerate_converter_t *,
                              double output_framerate);

void gavl_framerate_converter_reset(gavl_framerate_converter_t*);

void gavl_framerate_converter_destroy(gavl_framerate_converter_t*);

/*
 * Convert framerate:
 * input are 2 subsequent frames with valid timestamps
 * output is one of the input frames with corrected timestamps
 *
 * The timestamps of the input frames should be considered
 * undefined afterwards
 */

#define GAVL_FRAMERATE_INPUT_DONE  (1<<0)
#define GAVL_FRAMERATE_OUTPUT_DONE (1<<1)
  
int gavl_framerate_convert(gavl_framerate_converter_t *,
                           gavl_video_frame_t *  input_frame_1,
                           gavl_video_frame_t *  input_frame_2,
                           gavl_video_frame_t ** output_frame);
  
/*******************************************************
 * Colorspace related functions
 *******************************************************/

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
    
const char * gavl_colorspace_to_string(gavl_colorspace_t colorspace);
gavl_colorspace_t gavl_string_to_colorspace(const char *);

int gavl_num_colorspaces();
gavl_colorspace_t gavl_get_colorspace(int index);

#ifdef __cplusplus
}
#endif

#endif /* GAVL_H_INCLUDED */
