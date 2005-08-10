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

/**
 * @file gavl.h
 * external api header.
 */

#ifndef GAVL_H_INCLUDED
#define GAVL_H_INCLUDED

#include <inttypes.h>
#include <gavlconfig.h>

#include "gavltime.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \ingroup video_format
 *\brief Video format
 */

typedef struct gavl_video_format_s gavl_video_format_t;

/* Quality levels */
  
/** \defgroup quality Quality settings
    \brief Generic quality settings

    This is a portable way to select the conversion routines. Optimized routines often have a worse
    precision, and these defines are a way to choose among them. Quality level
    3 enables the standard ANSI-C versions, which are always available. Qualities
    1 and 2 choose optimized versions, qualities 4 and 5 enable high quality versions.
    Not all routines aren't available for quality levely other than 3. In these cases, the
    quality will be ignored.
 */



/*! 
    \ingroup quality
    \brief Fastest processing
   
    Worst quality.
*/
  
#define GAVL_QUALITY_FASTEST 1

/*! \ingroup quality
    \brief Highest quality
   
    Slowest, may not work in realtime applications
*/
  
#define GAVL_QUALITY_BEST    5 

/*! \ingroup quality
    \brief Default quality
   
    Default quality, most probably the choice for realtime playback applications.
*/

#define GAVL_QUALITY_DEFAULT 2 

/** \defgroup audio Audio
 *  \brief Audio support
 */

/* Sample formats: all multibyte numbers are native endian */

/** \defgroup audio_format Audio format definitions
 *  \ingroup audio
 *
 * \brief Definitions for several variations of audio data
*/


/*! \def GAVL_MAX_CHANNELS
 *  \ingroup audio_format
    \brief Maximum number of audio channels
   
*/
#define GAVL_MAX_CHANNELS 6
  
/*! Format of one audio sample
  \ingroup audio_format
  For multibyte numbers, the byte order is always machine native endian
*/
  
typedef enum
  {
    GAVL_SAMPLE_NONE  = 0, /*!< Undefined */
    GAVL_SAMPLE_U8    = 1, /*!< Unsigned 8 bit */
    GAVL_SAMPLE_S8    = 2, /*!< Signed 8 bit */
    GAVL_SAMPLE_U16   = 3, /*!< Unsigned 16 bit */
    GAVL_SAMPLE_S16   = 4, /*!< Signed 16 bit */
    GAVL_SAMPLE_S32   = 5, /*!< Signed 32 bit */
    GAVL_SAMPLE_FLOAT = 6  /*!< Floating point (-1.0 .. 1.0) */
  } gavl_sample_format_t;

/*! Interleave mode of the channels
  \ingroup audio_format
*/
  
typedef enum
  {
    GAVL_INTERLEAVE_NONE = 0, /*!< No interleaving, all channels separate */
    GAVL_INTERLEAVE_2    = 1, /*!< Interleaved pairs of channels          */ 
    GAVL_INTERLEAVE_ALL  = 2  /*!< Everything interleaved                 */
  } gavl_interleave_mode_t;

/*! Audio channel setup
  \ingroup audio_format
 */
  
typedef enum
  {
    GAVL_CHANNEL_NONE   = 0, /*!< Undefined */
    GAVL_CHANNEL_MONO   = 1, /*!< Mono */
    GAVL_CHANNEL_STEREO = 2, /*!< 2 Front channels (Stereo or Dual channels) */
    GAVL_CHANNEL_3F     = 3, /*!< Front Left, Center, Front Right */
    GAVL_CHANNEL_2F1R   = 4, /*!< Front Left, Front Right, Rear */
    GAVL_CHANNEL_3F1R   = 5, /*!< Front Left, Center, Front Right, Rear */
    GAVL_CHANNEL_2F2R   = 6, /*!< Front Left, Front Right, Rear Left, Rear Right */
    GAVL_CHANNEL_3F2R   = 7  /*!< Front Left, Center, Front Right, Rear Left, Rear Right */
  } gavl_channel_setup_t;

/*! Audio channel setup
  \ingroup audio_format

  These are the channel locations used to identify the channel order for an audio format
 */
  
typedef enum
  {
    GAVL_CHID_NONE         = 0, /*!< Undefined */
    GAVL_CHID_FRONT,            /*!< For mono  */
    GAVL_CHID_FRONT_LEFT,       /*!< Front left */
    GAVL_CHID_FRONT_RIGHT,      /*!< Front right */
    GAVL_CHID_FRONT_CENTER,     /*!< Center */
    GAVL_CHID_REAR,             /*!< Rear */
    GAVL_CHID_REAR_LEFT,        /*!< Rear left */
    GAVL_CHID_REAR_RIGHT,       /*!< Rear right */
    GAVL_CHID_LFE               /*!< Subwoofer */
  } gavl_channel_id_t;

/*! Audio Format
  \ingroup audio_format

  Structure describing an audio format. The samples_per_frame member is used
  exclusively by \ref gavl_audio_frame_create to determine how many bytes to
  allocate.
 */
  
typedef struct gavl_audio_format_s
  {
  int samples_per_frame;  /*!< Maximum number of samples per frame */
  int samplerate;         /*!< Samplerate */
  int num_channels;         /*!< Number of channels */
  gavl_sample_format_t   sample_format; /*!< Sample format */
  gavl_interleave_mode_t interleave_mode; /*!< Interleave mode */
  gavl_channel_setup_t   channel_setup; /*!< Channel setup */
  int lfe;            /*!< Low frequency effect channel present */
  
  float center_level; /*!< linear factor for mixing center to front */
  float rear_level;   /*!< linear factor for mixing rear to front */

  gavl_channel_id_t channel_locations[GAVL_MAX_CHANNELS];   /*!< Which channel is stored where */

  } gavl_audio_format_t;

  
/* Audio format -> string conversions */

/*! 
  \ingroup audio_format
  \brief Convert a gavl_sample_format_t to a human readable string
  \param format A sample format
 */
  
const char * gavl_sample_format_to_string(gavl_sample_format_t format);

/*! 
  \ingroup audio_format
  \brief Convert a gavl_channel_id_t to a human readable string
  \param id A channel id
 */

const char * gavl_channel_id_to_string(gavl_channel_id_t id);

/*! 
  \ingroup audio_format
  \brief Convert a gavl_channel_setup_t to a human readable string
  \param setup A channel setup
 */

const char * gavl_channel_setup_to_string(gavl_channel_setup_t setup);

/*! 
  \ingroup audio_format
  \brief Convert a gavl_interleave_mode_t to a human readable string
  \param mode An interleave mode
 */

const char * gavl_interleave_mode_to_string(gavl_interleave_mode_t mode);

/*! 
  \ingroup audio_format
  \brief Dump a gavl_audio_format_t to stderr
  \param format An audio format
 */

void gavl_audio_format_dump(const gavl_audio_format_t * format);

/*!
  \ingroup audio_format
  \brief Get the index of a particular channel for a given format. 
  \param format An audio format
  \param id A channel id
 */

int gavl_channel_index(const gavl_audio_format_t * format, gavl_channel_id_t id);

/*!
  \ingroup audio_format
  \brief Get number of front channels for a given format
  \param format An audio format
 */
  
int gavl_front_channels(const gavl_audio_format_t * format);

/*!
  \ingroup audio_format
  \brief Get number of rear channels for a given format
  \param format An audio format
 */
  
int gavl_rear_channels(const gavl_audio_format_t * format);

/*!
  \ingroup audio_format
  \brief Get number of LFE channels for a given format
  \param format An audio format
 */

int gavl_lfe_channels(const gavl_audio_format_t * format);

/*!
  \ingroup audio_format
  \brief Get number of channels for a given channel setup
  \param setup A channel setup
 */

int gavl_num_channels(gavl_channel_setup_t setup);

/*!
  \ingroup audio_format
  \brief Copy one audio format to another
  \param dst Destination format 
  \param src Source format 
 */
  
void gavl_audio_format_copy(gavl_audio_format_t * dst,
                            const gavl_audio_format_t * src);


/*!
  \ingroup audio_format
  \brief Set the default channel setup and indices
  \param format An audio format

  Set a default channel setup and channel indices if only the number of channels
  is known. The result might be wrong, but it provides a fallback if you have
  something else than Mono or Stereo and the file format/codec supports no speaker configurations.
*/
  
void gavl_set_channel_setup(gavl_audio_format_t * format);

/*!
  \ingroup audio_format
  \brief Get the number of bytes per sample for a given sample format
  \param format A sample format
*/
 
int gavl_bytes_per_sample(gavl_sample_format_t format);

/** \defgroup audio_frame Audio frame
 * \ingroup audio
 *
 * \brief Container for audio data
 *
 * 
 */


/*!
  \ingroup audio_frame
  \brief Container for interleaved audio samples
 */
  
typedef union gavl_audio_samples_u
  {
  uint8_t * u_8; /*!< Unsigned 8 bit samples */
  int8_t *  s_8; /*!< Signed 8 bit samples */

  uint16_t * u_16; /*!< Unsigned 16 bit samples */
  int16_t  * s_16; /*!< Signed 16 bit samples */
  
  uint32_t * u_32; /*!< Unsigned 32 bit samples (used internally only) */
  int32_t  * s_32; /*!< Signed 32 bit samples */
  
  float * f; /*!< Floating point samples */
  } gavl_audio_samples_t;


/*!
  \ingroup audio_frame
  \brief Container for noninterleaved audio channels
 */
  
typedef union gavl_audio_channels_u
  {
  uint8_t * u_8[GAVL_MAX_CHANNELS];/*!< Unsigned 8 bit channels */
  int8_t *  s_8[GAVL_MAX_CHANNELS];/*!< Signed 8 bit channels */

  uint16_t * u_16[GAVL_MAX_CHANNELS];/*!< Unsigned 16 bit channels */
  int16_t  * s_16[GAVL_MAX_CHANNELS];/*!< Signed 16 bit channels */
    
  uint32_t * u_32[GAVL_MAX_CHANNELS];/*!< Unsigned 32 bit channels */
  int32_t  * s_32[GAVL_MAX_CHANNELS];/*!< Signed 32 bit channels (used internally only) */

  float * f[GAVL_MAX_CHANNELS];/*!< Floating point channels */
  
  } gavl_audio_channels_t;

/*!
  \ingroup audio_frame
  \brief Container for audio samples.

  This is the main container structure for audio data. The data are stored in unions,
  so you can access the matching pointer type without the need for casts. If you have
  noninterleaved channels, the i'th channel will be in channels[i].f (if you use floating
  point samples). For noninterleaved formats, use the samples member. valid_samples must
  be set by the source to the number of actually valid samples in this frame.
 */
  
typedef struct gavl_audio_frame_s
  {
  gavl_audio_samples_t  samples; /*!< Sample pointer for interleaved formats         */ 
  gavl_audio_channels_t channels;/*!< Channel pointer for non interleaved formats    */
  int valid_samples;             /*!< Number of actually valid samples */
  } gavl_audio_frame_t;

/*!
  \ingroup audio_frame
  \brief Create audio frame
  \param format Format of the data to be stored in this frame or NULL

  Creates an audio frame for a given format and allocates buffers for the audio data. The buffer
  size is determined by the samples_per_frame member of the format. To create an audio frame from
  your custom memory, pass NULL for the format and you'll get an empty frame in which you can set the
  pointers manually.
*/
  
gavl_audio_frame_t * gavl_audio_frame_create(const gavl_audio_format_t* format);

/*!
  \ingroup audio_frame
  \brief Zero all pointers in the audio frame.
  \param frame An audio frame

  Zero all pointers, so \ref gavl_audio_frame_destroy won't free them.
  Call this for audio frames, which were created with a NULL format
  before destroying them.
  
*/
  
void gavl_audio_frame_null(gavl_audio_frame_t * frame);

/*!
  \ingroup audio_frame
  \brief Destroy an audio frame.
  \param frame An audio frame

  Destroys an audio frame and frees all associated memory. If you used your custom memory
  to allocate the frame, call \ref gavl_audio_frame_null before.
*/
  
void gavl_audio_frame_destroy(gavl_audio_frame_t * frame);

/*!
  \ingroup audio_frame
  \brief Mute an audio frame.
  \param frame An audio frame
  \param format The format of the frame

  Fills the frame with digital zero samples according to the audio format
*/
  
void gavl_audio_frame_mute(gavl_audio_frame_t * frame,
                           const gavl_audio_format_t * format);
  

/*!
  \ingroup audio_frame
  \brief Copy audio data from one frame to another.
  \param format Format, must be equal for source and destination frames
  \param dst Destination frame
  \param src Source frame
  \param dst_pos Offset (in samples) in the destination frame
  \param src_pos Offset (in samples) in the source frame
  \param dst_size Available samples in the destination frame
  \param src_size Available samples in the source frame
  \returns The actual number of copied samples

  This function copies audio samples from src (starting at src_offset) to dst
  (starting at dst_offset). The number of copied samples will be the smaller one of
  src_size and dst_size.

  You can use this function for creating a simple but effective audio buffer.
  
*/
  
int gavl_audio_frame_copy(gavl_audio_format_t * format,
                          gavl_audio_frame_t * dst,
                          gavl_audio_frame_t * src,
                          int dst_pos,
                          int src_pos,
                          int dst_size,
                          int src_size);

/** \defgroup audio_options Audio conversion options
    \ingroup audio
    
*/

/** \defgroup audio_conversion_flags Audio conversion flags
    \ingroup audio_options

    Flags for passing to \ref gavl_audio_options_set_conversion_flags
*/
  
/*! \ingroup audio_conversion_flags
 */
  
#define GAVL_AUDIO_FRONT_TO_REAR_COPY (1<<0) /*!< When mixing front to rear, just copy the front channels */

/*! \ingroup audio_conversion_flags
 */

#define GAVL_AUDIO_FRONT_TO_REAR_MUTE (1<<1) /*!< When mixing front to rear, mute the rear channels */

/*! \ingroup audio_conversion_flags
 */
  
#define GAVL_AUDIO_FRONT_TO_REAR_DIFF (1<<2) /*!< When mixing front to rear, send the difference between front to rear */

/*! \ingroup audio_conversion_flags
 */

#define GAVL_AUDIO_FRONT_TO_REAR_MASK           \
(GAVL_AUDIO_FRONT_TO_REAR_COPY | \
GAVL_AUDIO_FRONT_TO_REAR_MUTE | \
 GAVL_AUDIO_FRONT_TO_REAR_DIFF) /*!< Mask for front to rear mode */

/* Options for mixing stereo to mono */
  
/*! \ingroup audio_conversion_flags
 */
#define GAVL_AUDIO_STEREO_TO_MONO_LEFT  (1<<3) /*!< When converting from stereo to mono, choose left channel */
/*! \ingroup audio_conversion_flags
 */
#define GAVL_AUDIO_STEREO_TO_MONO_RIGHT (1<<4) /*!< When converting from stereo to mono, choose right channel      */
/*! \ingroup audio_conversion_flags
 */
#define GAVL_AUDIO_STEREO_TO_MONO_MIX   (1<<5) /*!< When converting from stereo to mono, mix left and right */

/*! \ingroup audio_conversion_flags
 */
#define GAVL_AUDIO_STEREO_TO_MONO_MASK \
(GAVL_AUDIO_STEREO_TO_MONO_LEFT | \
GAVL_AUDIO_STEREO_TO_MONO_RIGHT | \
GAVL_AUDIO_STEREO_TO_MONO_MIX) /*!< Mask for converting stereo to mono */
  
/*! \ingroup audio_options
 *  \brief Opaque container for audio conversion options.
 *
 * You don't want to know what's inside.
 */

typedef struct gavl_audio_options_s gavl_audio_options_t;

/*! \ingroup audio_options
 *  \brief Set the quality level for the converter
 *  \param opt Audio options
 *  \param quality Quality level (see \ref quality)
 */
  
void gavl_audio_options_set_quality(gavl_audio_options_t * opt, int quality);

/*! \ingroup audio_options
 *  \brief Set the conversion flags
 *  \param opt Audio options
 *  \param flags (see \ref audio_conversion_flags)
 */
  
void gavl_audio_options_set_conversion_flags(gavl_audio_options_t * opt,
                                             int flags);

/*! \ingroup audio_options
 *  \brief Get the conversion flags
 *  \param opt Audio options
 *  \returns Flags (see \ref audio_conversion_flags)
 */
  
int gavl_audio_options_get_conversion_flags(gavl_audio_options_t * opt);

/*! \ingroup audio_options
 *  \brief Set all options to their defaults
 *  \param opt Audio options
 */
  
void gavl_audio_options_set_defaults(gavl_audio_options_t * opt);

/* Audio converter */

/** \defgroup audio_converter Audio converter
    \ingroup audio
    \brief Audio format converter.
    
    This is a generic converter, which converts audio frames from one arbitrary format to
    another. Create an audio converter with \ref gavl_audio_converter_create. Call
    \ref gavl_audio_converter_init to initialize the converter for the input and output formats.
    Audio frames are then converted with \ref gavl_audio_convert.
*/
  
/*! \ingroup audio_converter
 *  \brief Opaque audio converter structure
 *
 * You don't want to know what's inside.
 */
  
typedef struct gavl_audio_converter_s gavl_audio_converter_t;
  
/*! \ingroup audio_converter
 *  \brief Creates an audio converter
 *  \returns A newly allocated audio converter
 */

gavl_audio_converter_t * gavl_audio_converter_create();

/*! \ingroup audio_converter
 *  \brief Destroys an audio converter and frees all associated memory
 *  \param cnv An audio converter
 */

void gavl_audio_converter_destroy(gavl_audio_converter_t* cnv);

/*! \ingroup audio_converter
 *  \brief gets options of an audio converter
 *  \param cnv An audio converter
 *
 * After you called this, you can use the gavl_audio_options_set_*() functions to change
 * the options. Options will become valid with the next call to \ref gavl_audio_converter_init
 */

gavl_audio_options_t * gavl_audio_converter_get_options(gavl_audio_converter_t*cnv);

/*! \ingroup audio_converter
 *  \brief Copies audio options
 *  \param src Source
 *  \param dst Destination
 *
 */
void gavl_audio_options_copy(gavl_audio_options_t * dst,
                             const gavl_audio_options_t * src);

/*! \ingroup audio_converter
 *  \brief Initialize an audio converter
 *  \param cnv An audio converter
 *  \param input_format Input format
 *  \param output_format Output format
 *  \returns The number of single conversion steps necessary to perform the
 *           conversion. It may be 0, in this case you don't need the converter and
 *           can pass the audio frames directly. If something goes wrong (should never happen),
 *           -1 is returned.
 *
 * This function can be called multiple times with one instance
 */
  
int gavl_audio_converter_init(gavl_audio_converter_t* cnv,
                              const gavl_audio_format_t * input_format,
                              const gavl_audio_format_t * output_format);

/*! \ingroup audio_converter
 *  \brief Convert audio
 *  \param cnv An audio converter
 *  \param input_frame Input frame
 *  \param output_frame Output frame
 *
 *  Be careful when resampling: gavl will
 *  assume, that your output frame is big enough.
 *  Minimum size is
 *  input_frame_size * output_samplerate / input_samplerate + 10
 *
 */
  
void gavl_audio_convert(gavl_audio_converter_t * cnv,
                        gavl_audio_frame_t * input_frame,
                        gavl_audio_frame_t * output_frame);




/** \defgroup volume_control Volume control
    \ingroup audio
    \brief Simple volume control

    This is a very simple software volume control.
*/

/*! \ingroup volume_control
 *  \brief Opaque structure for a volume control
 *
 * You don't want to know what's inside.
 */

typedef struct gavl_volume_control_s gavl_volume_control_t;
  
/* Create / destroy */

/*! \ingroup volume_control
 *  \brief Create volume control
 *  \returns A newly allocated volume control
 */
  
gavl_volume_control_t * gavl_volume_control_create();

/*! \ingroup volume_control
 *  \brief Destroys a volume control and frees all associated memory
 *  \param ctrl A volume control 
 */

void gavl_volume_control_destroy(gavl_volume_control_t *ctrl);

/*! \ingroup volume_control
 *  \brief Set format for a volume control
 *  \param ctrl A volume control
 *  \param format The format subsequent frames will be passed with
 * This function can be called multiple times with one instance
 */

void gavl_volume_control_set_format(gavl_volume_control_t *ctrl,
                                    gavl_audio_format_t * format);

/*! \ingroup volume_control
 *  \brief Set volume for a volume control
 *  \param ctrl A volume control
 *  \param volume Volume in dB (must be <= 0.0 to prevent overflows)
 */
  
void gavl_volume_control_set_volume(gavl_volume_control_t * ctrl,
                                    float volume);

/*! \ingroup volume_control
 *  \brief Apply a volume control for an audio frame
 *  \param ctrl A volume control
 *  \param frame An audio frame
 */
  
void gavl_volume_control_apply(gavl_volume_control_t *ctrl,
                               gavl_audio_frame_t * frame);
  

/** \defgroup video Video
 *
 * Video support
 */
  
/*! Maximum number of planes
 * \ingroup video
 */
  
#define GAVL_MAX_PLANES 4 /*!< Maximum number of planes */

/** \defgroup rectangle Rectangles
 * \ingroup video
 *
 * Define rectangular areas in a video frame
 */

/*! Integer rectangle
 * \ingroup rectangle
 */
  
typedef struct
  {
  int x; /*!< Horizontal offset from the left border of the frame */
  int y; /*!< Vertical offset from the left border of the frame */
  int w; /*!< Width */
  int h; /*!< Height */
  } gavl_rectangle_i_t;

/*! Floating point rectangle
 * \ingroup rectangle
 */
  
typedef struct
  {
  double x; /*!< Horizontal offset from the left border of the frame */
  double y; /*!< Vertical offset from the left border of the frame */
  double w; /*!< Width */
  double h; /*!< Height */
  } gavl_rectangle_f_t;

/*! \brief Crop a rectangle so it fits into the image size of a video format
 * \ingroup rectangle
 * \param r A rectangle
 * \param format The video format into which the rectangle must fit
 */
  
void gavl_rectangle_crop_to_format(gavl_rectangle_i_t * r,
                                   const gavl_video_format_t * format);

/*! \brief Crop a floating point rectangle so it fits into the image size of a video format
 * \ingroup rectangle
 * \param r A rectangle
 * \param format The video format into which the rectangle must fit
 */
  
void gavl_rectangle_f_crop_to_format(gavl_rectangle_f_t * r,
                                     const gavl_video_format_t * format);

/*! \brief Set 2 rectangles as source and destination when no scaling is available
 * \ingroup rectangle
 * \param src_rect Source rectangle
 * \param dst_rect Destination rectangle
 * \param src_format Source format
 * \param dst_format Destination format
 *
 *  This produces 2 rectangles of the same size centered on src_format and dst_format
 *  respectively
 */
  
void gavl_rectangle_crop_to_format_noscale(gavl_rectangle_i_t * src_rect,
                                           gavl_rectangle_i_t * dst_rect,
                                           const gavl_video_format_t * src_format,
                                           const gavl_video_format_t * dst_format);
  

/*! \brief Let an integer rectangle span the whole image size of a video format
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param format The video format into which the rectangle must fit
 */
  
void gavl_rectangle_i_set_all(gavl_rectangle_i_t * r, const gavl_video_format_t * format);

/*! \brief Let a float rectangle span the whole image size of a video format
 * \ingroup rectangle
 * \param r A float rectangle
 * \param format The video format into which the rectangle must fit
 */

void gavl_rectangle_f_set_all(gavl_rectangle_f_t * r, const gavl_video_format_t * format);

/*! \brief crop an integer rectangle by some pixels from the left border
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */
  
void gavl_rectangle_i_crop_left(gavl_rectangle_i_t * r,   int num_pixels);

/*! \brief crop an integer rectangle by some pixels from the right border
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_i_crop_right(gavl_rectangle_i_t * r,  int num_pixels);

/*! \brief crop an integer rectangle by some pixels from the top border
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_i_crop_top(gavl_rectangle_i_t * r,    int num_pixels);

/*! \brief crop an integer rectangle by some pixels from the bottom border
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_i_crop_bottom(gavl_rectangle_i_t * r, int num_pixels);

/*! \brief crop a float rectangle by some pixels from the left border
 * \ingroup rectangle
 * \param r A float rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */
 
void gavl_rectangle_f_crop_left(gavl_rectangle_f_t * r,   double num_pixels);

/*! \brief crop a float rectangle by some pixels from the right border
 * \ingroup rectangle
 * \param r A float rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_f_crop_right(gavl_rectangle_f_t * r,  double num_pixels);

/*! \brief crop a float rectangle by some pixels from the top border
 * \ingroup rectangle
 * \param r A float rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_f_crop_top(gavl_rectangle_f_t * r,    double num_pixels);

/*! \brief crop a float rectangle by some pixels from the bottom border
 * \ingroup rectangle
 * \param r A float rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_f_crop_bottom(gavl_rectangle_f_t * r, double num_pixels);

/*! \brief Align a rectangle
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param h_align Horizontal alignment
 * \param v_align Vertical alignment
 *
 * This aligns a rectangle such that the horizontal coordinates are multiples of
 * h_align, while the vertical coordinates are multiples of v_align. When dealing
 * with chroma subsampled formats, you must call this function with the
 * return values of \ref gavl_pixelformat_chroma_sub before taking subframes from
 * video frames.
 */
  
void gavl_rectangle_i_align(gavl_rectangle_i_t * r, int h_align, int v_align);

/*! \brief Copy an integer rectangle
 * \ingroup rectangle
 * \param dst Destination rectangle
 * \param src Source rectangle
 */
  
void gavl_rectangle_i_copy(gavl_rectangle_i_t * dst, const gavl_rectangle_i_t * src);

/*! \brief Copy a float rectangle
 * \ingroup rectangle
 * \param dst Destination rectangle
 * \param src Source rectangle
 */

void gavl_rectangle_f_copy(gavl_rectangle_f_t * dst, const gavl_rectangle_f_t * src);

/*! \brief Check if an integer rectangle is empty
 * \ingroup rectangle
 * \param r Rectangle
 * \returns 1 if the rectangle is empty, 0 else.
 *
 * A rectangle is considered to be empty if the width or height are <= 0.
 */

int gavl_rectangle_i_is_empty(const gavl_rectangle_i_t * r);

/*! \brief Check if a float rectangle is empty
 * \ingroup rectangle
 * \param r Rectangle
 * \returns 1 if the rectangle is empty, 0 else.
 *
 * A rectangle is considered to be empty if the width or height are <= 0.
 */
  
int gavl_rectangle_f_is_empty(const gavl_rectangle_f_t * r);

  

/*!\brief Calculate a destination rectangle
 * \ingroup rectangle
 * \param dst_rect Destination rectangle
 * \param src_format Source format
 * \param src_rect Source rectangle
 * \param dst_format Destination format
 * \param zoom Zoom factor
 * \param squeeze Squeeze factor
 *
 * Assuming we take src_rect from a frame in format src_format,
 * calculate the optimal dst_rect in dst_format. The source and destination
 * display aspect ratio will be unchanged unless it is changed with the squeeze
 * parameter.
 * Zoom is a zoom factor (1.0 = 100 %), Squeeze is a value between -1.0 and 1.0,
 * while squeeze changes the apsect ratio in both directions. 0.0 means unchanged
 */
  
void gavl_rectangle_fit_aspect(gavl_rectangle_i_t * dst_rect,
                               const gavl_video_format_t * src_format,
                               const gavl_rectangle_f_t * src_rect,
                               const gavl_video_format_t * dst_format,
                               float zoom, float squeeze);

/*! \brief Dump a rectangle to stderr
 * \ingroup rectangle
 * \param r Rectangle
 */
void gavl_rectangle_i_dump(const gavl_rectangle_i_t * r);

/*! \brief Dump a floating point rectangle to stderr
 * \ingroup rectangle
 * \param r Floating point rectangle
 */
void gavl_rectangle_f_dump(const gavl_rectangle_f_t * r);

  
/** \defgroup video_format Video format definitions
 * \ingroup video
 *
 * \brief Definitions for several variations of video data
 */

/** \ingroup video_format
 * Flag for planar pixelformats
 */
#define GAVL_PIXFMT_PLANAR (1<<8)

/** \ingroup video_format
 * Flag for rgb pixelformats
 */
#define GAVL_PIXFMT_RGB    (1<<9)

/** \ingroup video_format
 * Flag for yuv pixelformats
 */
#define GAVL_PIXFMT_YUV    (1<<10)

/** \ingroup video_format
 * Flag for yuvj pixelformats
 */
#define GAVL_PIXFMT_YUVJ   (1<<11)

/** \ingroup video_format
 * Alpha flag
 */
#define GAVL_PIXFMT_ALPHA  (1<<12)
  
  
/*! \ingroup video_format
 * \brief Pixelformat definition
 */
  
typedef enum 
  {
 /*! \brief Undefined 
  */
    GAVL_PIXELFORMAT_NONE =  0, 

 /*! 15 bit RGB. Each pixel is a uint16_t in native byte order. Color masks are:
  * for red: 0x7C00, for green: 0x03e0, for blue: 0x001f
  */
    GAVL_RGB_15          =  1 | GAVL_PIXFMT_RGB,
 /*! 15 bit BGR. Each pixel is a uint16_t in native byte order. Color masks are:
  * for red: 0x001f, for green: 0x03e0, for blue: 0x7C00
  */
    GAVL_BGR_15          =  2 | GAVL_PIXFMT_RGB,
 /*! 16 bit RGB. Each pixel is a uint16_t in native byte order. Color masks are:
  * for red: 0xf800, for green: 0x07e0, for blue: 0x001f
  */
    GAVL_RGB_16          =  3 | GAVL_PIXFMT_RGB,
 /*! 16 bit BGR. Each pixel is a uint16_t in native byte order. Color masks are:
  * for red: 0x001f, for green: 0x07e0, for blue: 0xf800
  */
    GAVL_BGR_16          =  4 | GAVL_PIXFMT_RGB,
 /*! 24 bit RGB. Each color is an uint8_t. Color order is RGBRGB
  */
    GAVL_RGB_24          =  5 | GAVL_PIXFMT_RGB,
 /*! 24 bit BGR. Each color is an uint8_t. Color order is BGRBGR
  */
    GAVL_BGR_24          =  6 | GAVL_PIXFMT_RGB,
 /*! 32 bit RGB. Each color is an uint8_t. Color order is RGBXRGBX, where X is unused
  */
    GAVL_RGB_32          =  7 | GAVL_PIXFMT_RGB,
 /*! 32 bit BGR. Each color is an uint8_t. Color order is BGRXBGRX, where X is unused
  */
    GAVL_BGR_32          =  8 | GAVL_PIXFMT_RGB,
 /*! 32 bit RGBA. Each color is an uint8_t. Color order is RGBARGBA
  */
    GAVL_RGBA_32         =  9 | GAVL_PIXFMT_RGB | GAVL_PIXFMT_ALPHA,
 /*! Packed YCbCr 4:2:2. Each component is an uint8_t. Component order is Y1 U1 Y2 V1
  */
    GAVL_YUY2            = 10 | GAVL_PIXFMT_YUV,
 /*! Packed YCbCr 4:2:2. Each component is an uint8_t. Component order is U1 Y1 V1 Y2
  */
    GAVL_UYVY            = 11 | GAVL_PIXFMT_YUV,
 /*! Packed YCbCrA 4:4:4:4. Each component is an uint8_t. Component order is YUVAYUVA
  */
    GAVL_YUVA_32         = 26 | GAVL_PIXFMT_YUV | GAVL_PIXFMT_ALPHA,
 /*! Planar YCbCr 4:2:0. Each component is an uint8_t. Chroma placement is defined by \ref gavl_chroma_placement_t
  */
    GAVL_YUV_420_P       = 12 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
 /*! Planar YCbCr 4:2:2. Each component is an uint8_t
  */
    GAVL_YUV_422_P       = 13 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
 /*! Planar YCbCr 4:4:4. Each component is an uint8_t
  */
    GAVL_YUV_444_P       = 14 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
 /*! Planar YCbCr 4:1:1. Each component is an uint8_t
  */
    GAVL_YUV_411_P       = 15 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
 /*! Planar YCbCr 4:1:0. Each component is an uint8_t
  */
    GAVL_YUV_410_P       = 16 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
    
 /*! Planar YCbCr 4:2:0. Each component is an uint8_t, luma and chroma values are full range (0x00 .. 0xff)
  */
    GAVL_YUVJ_420_P      = 17 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV | GAVL_PIXFMT_YUVJ,
 /*! Planar YCbCr 4:2:2. Each component is an uint8_t, luma and chroma values are full range (0x00 .. 0xff)
  */
    GAVL_YUVJ_422_P      = 18 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV | GAVL_PIXFMT_YUVJ,
 /*! Planar YCbCr 4:4:4. Each component is an uint8_t, luma and chroma values are full range (0x00 .. 0xff)
  */
    GAVL_YUVJ_444_P      = 19 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV | GAVL_PIXFMT_YUVJ,

 /*! 16 bit Planar YCbCr 4:4:4. Each component is an uint16_t in native byte order.
  */
    GAVL_YUV_444_P_16 = 20 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
 /*! 16 bit Planar YCbCr 4:2:2. Each component is an uint16_t in native byte order.
  */
    GAVL_YUV_422_P_16 = 21 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,

 /*! 48 bit RGB. Each color is an uint16_t in native byte order. Color order is RGBRGB
  */
    GAVL_RGB_48       = 22 | GAVL_PIXFMT_RGB,
 /*! 64 bit RGBA. Each color is an uint16_t in native byte order. Color order is RGBARGBA
  */
    GAVL_RGBA_64      = 23 | GAVL_PIXFMT_RGB | GAVL_PIXFMT_ALPHA,
        
 /*! float RGB. Each color is a float (0.0 .. 1.0) in native byte order. Color order is RGBRGB
  */
    GAVL_RGB_FLOAT    = 24 | GAVL_PIXFMT_RGB,
 /*! float RGBA. Each color is a float (0.0 .. 1.0) in native byte order. Color order is RGBARGBA
  */
    GAVL_RGBA_FLOAT   = 25 | GAVL_PIXFMT_RGB  | GAVL_PIXFMT_ALPHA
    
  } gavl_pixelformat_t;

/*
 *  Colormodel related functions
 */

/*! \ingroup video_format
 * \brief Check if a pixelformat is RGB based
 * \param fmt A pixelformat
 * \returns 1 if the pixelformat is RGB based, 0 else
 */
  
#define gavl_pixelformat_is_rgb(fmt) ((fmt) & GAVL_PIXFMT_RGB)

/*! \ingroup video_format
 * \brief Check if a pixelformat is YUV based
 * \param fmt A pixelformat
 * \returns 1 if the pixelformat is YUV based, 0 else
 */
  
#define gavl_pixelformat_is_yuv(fmt) ((fmt) & GAVL_PIXFMT_YUV)

/*! \ingroup video_format
 * \brief Check if a pixelformat is jpeg (full range) scaled
 * \param fmt A pixelformat
 * \returns 1 if the pixelformat is jpeg scaled, 0 else
 */

#define gavl_pixelformat_is_jpeg_scaled(fmt) ((fmt) & GAVL_PIXFMT_YUVJ)

/*! \ingroup video_format
 * \brief Check if a pixelformat has a transparency channel
 * \param fmt A pixelformat
 * \returns 1 if the pixelformat has a transparency channel, 0 else
 */
  
#define gavl_pixelformat_has_alpha(fmt) ((fmt) & GAVL_PIXFMT_ALPHA)

/*! \ingroup video_format
 * \brief Check if a pixelformat is planar
 * \param fmt A pixelformat
 * \returns 1 if the pixelformat is planar, 0 else
 */

#define  gavl_pixelformat_is_planar(fmt) ((fmt) & GAVL_PIXFMT_PLANAR)

/*! \ingroup video_format
 * \brief Get the number of planes
 * \param pixelformat A pixelformat
 * \returns The number of planes (1 for packet formats)
 */

int gavl_pixelformat_num_planes(gavl_pixelformat_t pixelformat);

/*! \ingroup video_format
 * \brief Get the horizontal and vertical subsampling factors
 * \param pixelformat A pixelformat
 * \param sub_h returns the horizontal subsampling factor
 * \param sub_v returns the vertical subsampling factor
 *
 * E.g. for 4:2:0 subsampling: sub_h = 2, sub_v = 2
 */

void gavl_pixelformat_chroma_sub(gavl_pixelformat_t pixelformat, int * sub_h, int * sub_v);

/*! \ingroup video_format
 * \brief Get bytes per component for planar formats
 * \param pixelformat A pixelformat
 * \returns The number of bytes per component for planar formats, 0 for packed formats
 */
  
int gavl_pixelformat_bytes_per_component(gavl_pixelformat_t pixelformat);

/*! \ingroup video_format
 * \brief Get bytes per pixel for packed formats
 * \param pixelformat A pixelformat
 * \returns The number of bytes per pixel for packed formats, 0 for planar formats
 */

int gavl_pixelformat_bytes_per_pixel(gavl_pixelformat_t pixelformat);
  
/*! \ingroup video_format
 * \brief Translate a pixelformat into a human readable string
 * \param pixelformat A pixelformat
 * \returns A string describing the pixelformat
 */

const char * gavl_pixelformat_to_string(gavl_pixelformat_t pixelformat);

/*! \ingroup video_format
 * \brief Translate a pixelformat name into a pixelformat
 * \param name A string describing the pixelformat (returnd by \ref gavl_pixelformat_to_string)
 * \returns The pixelformat or GAVL_PIXELFORMAT_NONE if no match.
 */

gavl_pixelformat_t gavl_string_to_pixelformat(const char * name);

/*! \ingroup video_format
 * \brief Get total number of supported pixelformats
 * \returns total number of supported pixelformats
 */

int gavl_num_pixelformats();

/*! \ingroup video_format
 * \brief Get the pixelformat from index
 * \param index index (must be between 0 and the result of \ref gavl_num_pixelformats)
 * \returns The pixelformat corresponding to index or GAVL_PIXELFORMAT_NONE.
 */

gavl_pixelformat_t gavl_get_pixelformat(int index);

/*  */

/*! \ingroup video_format
 * \brief Chroma placement
 *
 * Specification of the 3 variants of 4:2:0 YCbCr as described at
 * http://www.mir.com/DMG/chroma.html . For other pixelformats, it's meaningless
 * and should be set to GAVL_CHROMA_PLACEMENT_DEFAULT.
 */
  
typedef enum
  {
    GAVL_CHROMA_PLACEMENT_DEFAULT = 0, /*!< MPEG-1/JPEG */
    GAVL_CHROMA_PLACEMENT_MPEG2,       /*!< MPEG-2 */
    GAVL_CHROMA_PLACEMENT_DVPAL        /*!< DV PAL */
  } gavl_chroma_placement_t;

/*! \ingroup video_format
 * \brief Translate a chroma placement into a human readable string
 * \param mode A chroma placement
 * \returns A string describing the chroma placement
 */

const char * gavl_chroma_placement_to_string(gavl_chroma_placement_t mode);
  
/*! \ingroup video_format
 * \brief Framerate mode
 */
  
typedef enum
  {
    GAVL_FRAMERATE_CONSTANT    = 0, /*!< Constant framerate */
    GAVL_FRAMERATE_VARIABLE    = 1, /*!< Variable framerate */
    GAVL_FRAMERATE_STILL       = 2, /*!< Still image */
  } gavl_framerate_mode_t;

/*! \ingroup video_format
 * \brief Interlace mode
 */

typedef enum
  {
    GAVL_INTERLACE_NONE = 0,      /*!< Progressive */
    GAVL_INTERLACE_TOP_FIRST,     /*!< Top field first */
    GAVL_INTERLACE_BOTTOM_FIRST     /*!< Bottom field first */
  } gavl_interlace_mode_t;

/*! \ingroup video_format
 * \brief Translate an interlace mode into a human readable string
 * \param mode An interlace mode
 * \returns A string describing the interlace mode
 */

const char * gavl_interlace_mode_to_string(gavl_interlace_mode_t mode);
  
  
/* Video format structure */
  
/*! \ingroup video_format
 * \brief Video format
 */
  
struct gavl_video_format_s
  {
  int frame_width;/*!< Width of the frame buffer in pixels, might be larger than image_width */
  int frame_height;/*!< Height of the frame buffer in pixels, might be larger than image_height */
   
  int image_width;/*!< Width of the image in pixels */
  int image_height;/*!< Height of the image in pixels */
  
  /* Support for nonsquare pixels */
    
  int pixel_width;/*!< Relative width of a pixel (pixel aspect ratio is pixel_width/pixel_height) */
  int pixel_height;/*!< Relative height of a pixel (pixel aspect ratio is pixel_width/pixel_height) */
    
  gavl_pixelformat_t pixelformat;/*!< Pixelformat */

  int frame_duration;/*!< Duration of a frame in timescale tics. Meaningful only if framerate_mode is
                       GAVL_FRAMERATE_CONSTANT */
  int timescale;/*!< Timescale in tics per second */

  gavl_framerate_mode_t   framerate_mode;/*!< Framerate mode */
  gavl_chroma_placement_t chroma_placement;/*!< Chroma placement */
  gavl_interlace_mode_t   interlace_mode;/*!< Interlace mode */
  };

/*!
  \ingroup video_format
  \brief Copy one video format to another
  \param dst Destination format 
  \param src Source format 
 */
  
void gavl_video_format_copy(gavl_video_format_t * dst,
                            const gavl_video_format_t * src);

/*!
  \ingroup video_format
  \brief Get the chroma offsets relative to the luma samples
  \param format A video format 
  \param field Index of the field (0 = top, 1 = bottom). For progressive format, this is unused
  \param plane Index of the plane (1 = Cb, 2 = Cr)
  \param off_x Returns the offset in x-direction
  \param off_y Returns the offset in y-direction
*/
  
void gavl_video_format_get_chroma_offset(gavl_video_format_t * format, int field, int plane,
                                         float * off_x, float * off_y);
  

  
/*! 
  \ingroup video_format
  \brief Dump a gavl_video_format_t to stderr
  \param format A video format
 */
  
void gavl_video_format_dump(const gavl_video_format_t * format);

/*! 
  \ingroup video_format
  \brief Set the image size of a destination format from a source format
  \param dst Destination format
  \param src Source format

  Sets the image size of dst according src. Before you call this function,
  you must set the pixel_width and pixel_height of dst. This function will
  preserve the display aspect ratio, i.e. when the pixel aspect ratios are different
  in source and destination, the images will be scaled.
 */
  
void gavl_video_format_fit_to_source(gavl_video_format_t * dst,
                                     const gavl_video_format_t * src);

  
/** \defgroup video_frame Video frames
 * \ingroup video
 * \brief Container for video images
 *
 * This is the standardized method of storing one frame with video data. For planar
 * formats, the first scanline starts at planes[0], subsequent scanlines start in
 * intervalls of strides[0] bytes. For planar formats, planes[0] will contain the
 * luminance channel, planes[1] contains Cb (aka U), planes[2] contains Cr (aka V).
 *
 * Gavl video frames are always oriented top->bottom left->right. If you must flip frames,
 * use the functions \ref gavl_video_frame_copy_flip_x, \ref gavl_video_frame_copy_flip_y or
 * \ref gavl_video_frame_copy_flip_xy .
 */

/** \ingroup video_frame
 * Video frame
 */
  
typedef struct gavl_video_frame_s
  {
  uint8_t * planes[GAVL_MAX_PLANES]; /*!< Pointers to the planes */
  int strides[GAVL_MAX_PLANES];      /*!< For each plane, this stores the byte offset between the scanlines */
  
  void * user_data;    /*!< For storing user data (e.g. the corresponding XImage) */
  int64_t time_scaled; /*!< Timestamp in stream specific units (see \ref video_format) */
  int duration_scaled; /*!< Duration in stream specific units (see \ref video_format) */
  } gavl_video_frame_t;


/*!
  \ingroup video_frame
  \brief Create video frame
  \param format Format of the data to be stored in this frame or NULL

  Creates a video frame for a given format and allocates buffers for the scanlines. To create a
  video frame from your custom memory, pass NULL for the format and you'll get an empty frame in
  which you can set the pointers manually. If scanlines are allocated, they are padded to that
  scanlines start at certain byte boundaries (currently 8).
*/
  
gavl_video_frame_t * gavl_video_frame_create(const gavl_video_format_t*format);

/*!
  \ingroup video_frame
  \brief Create video frame without padding
  \param format Format of the data to be stored in this frame or NULL

  Same as \ref gavl_video_frame_create but omits padding, so scanlines will always be
  adjacent in memory.
  
*/
  
gavl_video_frame_t * gavl_video_frame_create_nopadd(const gavl_video_format_t*format);

  

/*!
  \ingroup video_frame
  \brief Destroy a video frame.
  \param frame A video frame

  Destroys a video frame and frees all associated memory. If you used your custom memory
  to allocate the frame, call \ref gavl_video_frame_null before.
*/

void gavl_video_frame_destroy(gavl_video_frame_t*frame);

/*!
  \ingroup video_frame
  \brief Zero all pointers in the video frame.
  \param frame A video frame

  Zero all pointers, so \ref gavl_video_frame_destroy won't free them.
  Call this for video frames, which were created with a NULL format
  before destroying them.
  
*/
  
void gavl_video_frame_null(gavl_video_frame_t*frame);
  
/*!
  \ingroup video_frame
  \brief Fill the frame with black color
  \param frame A video frame
  \param format Format of the data in the frame
 
*/

void gavl_video_frame_clear(gavl_video_frame_t * frame,
                            gavl_video_format_t * format);
  
/*!
  \ingroup video_frame
  \brief Copy one video frame to another
  \param format The format of the frames
  \param dst Destination 
  \param src Source

  The source and destination formats must be identical, as this routine does no
  format conversion. The scanlines however can be padded differently in the source and destination.
 
*/

void gavl_video_frame_copy(gavl_video_format_t * format,
                           gavl_video_frame_t * dst,
                           gavl_video_frame_t * src);

/*!
  \ingroup video_frame
  \brief Copy a single plane from one video frame to another
  \param format The format of the frames
  \param dst Destination 
  \param src Source
  \param plane The plane to copy

  Like \ref gavl_video_frame_copy but copies only a single plane
 
*/
  
void gavl_video_frame_copy_plane(gavl_video_format_t * format,
                                 gavl_video_frame_t * dst,
                                 gavl_video_frame_t * src, int plane);

/*!
  \ingroup video_frame
  \brief Copy one video frame to another with horizontal flipping
  \param format The format of the frames
  \param dst Destination 
  \param src Source

  Like \ref gavl_video_frame_copy but flips the image horizontally
 
*/
  
void gavl_video_frame_copy_flip_x(gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  gavl_video_frame_t * src);

/*!
  \ingroup video_frame
  \brief Copy one video frame to another with vertical flipping
  \param format The format of the frames
  \param dst Destination 
  \param src Source

  Like \ref gavl_video_frame_copy but flips the image vertically
 
*/
  
void gavl_video_frame_copy_flip_y(gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  gavl_video_frame_t * src);

/*!
  \ingroup video_frame
  \brief Copy one video frame to another with horizontal and vertical flipping
  \param format The format of the frames
  \param dst Destination 
  \param src Source

  Like \ref gavl_video_frame_copy but flips the image both horizontally and vertically
 
*/

void gavl_video_frame_copy_flip_xy(gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  gavl_video_frame_t * src);

/*!
  \ingroup video_frame
  \brief Get a subframe of another frame
  \param pixelformat Pixelformat of the frames
  \param dst Destination
  \param src Source
  \param src_rect Rectangular area in the source, which will be in the destination frame

  This fills the pointers of dst from src such that the dst will represent the
  speficied rectangular area. Note that no data are copied here. This means that
  dst must be created with NULL as the format argument and \ref gavl_video_frame_null
  must be called before destroying dst.

  When dealing with chroma subsampled pixelformats, you must call
  \ref gavl_rectangle_i_align on src_rect before.
*/

void gavl_video_frame_get_subframe(gavl_pixelformat_t pixelformat,
                                   gavl_video_frame_t * src,
                                   gavl_video_frame_t * dst,
                                   gavl_rectangle_i_t * src_rect);

/*!
  \ingroup video_frame
  \brief Get a field from a frame
  \param pixelformat Pixelformat of the frames
  \param dst Destination
  \param src Source
  \param field Field index (0 = top field, 1 = bottom field)

  This fills the pointers of dst from src such that the dst will represent the
  speficied field. Note that no data are copied here. This means that
  dst must be created with NULL as the format argument and \ref gavl_video_frame_null
  must be called before destroying dst.
*/

void gavl_video_frame_get_field(gavl_pixelformat_t pixelformat,
                                gavl_video_frame_t * src,
                                gavl_video_frame_t * dst,
                                int field);

  

/*!
  \ingroup video_frame
  \brief Dump a video frame to files
  \param frame Video frame to dump
  \param format Format of the video data in the frame
  \param namebase Base for the output filenames

  This is purely for debugging purposes:
  It dumps each plane of the frame into files \<namebase\>.p1,
  \<namebase\>.p2 etc
*/
 
void gavl_video_frame_dump(gavl_video_frame_t * frame,
                           gavl_video_format_t * format,
                           const char * namebase);
  
/*****************************
 Conversion options
******************************/

/** \defgroup video_options Video conversion options
 * \ingroup video
 */

/** \defgroup video_conversion_flags Video conversion flags
 * \ingroup video_options
 */

/** \ingroup video_conversion_flags
 * \brief Keep the display aspect ratio
 *
 * This will cause the video converter to scale the
 * image to keep the display aspect ratio. Set this to zero if you want to prevent
 * scaling even if the pixel aspect ratios are different in the source and destination
 * formats.
 */
  
#define GAVL_KEEP_APSPECT (1<<0)

/** \ingroup video_options
 * Alpha handling mode
 *
 * Set the desired behaviour if the source format has a transparency channel but the
 * destination doesn't.
 */
  
typedef enum
  {
    GAVL_ALPHA_IGNORE      = 0, /*!< Ignore alpha channel      */
    GAVL_ALPHA_BLEND_COLOR      /*!< Blend over a specified background color */
  } gavl_alpha_mode_t;

/** \ingroup video_options
 * Deinterlace mode
 *
 * Specifies a deinterlacing mode
 */
  
typedef enum
  {
    GAVL_DEINTERLACE_NONE      = 0, /*!< Don't care about interlacing                */
    GAVL_DEINTERLACE_COPY      = 1, /*!< Take one field and copy it to the other     */
    GAVL_DEINTERLACE_SCALE     = 2  /*!< Take one field and scale it vertically by 2 */
  } gavl_deinterlace_mode_t;

/** \ingroup video_options
 * \brief Specifies which field to drop when deinterlacing
 *
 * This is used for deinterlacing with GAVL_DEINTERLACE_COPY and GAVL_DEINTERLACE_SCALE.
 */
  
typedef enum
  {
    GAVL_DEINTERLACE_DROP_TOP,    /*!< Drop top field, use bottom field */
    GAVL_DEINTERLACE_DROP_BOTTOM, /*!< Drop bottom field, use top field */
  } gavl_deinterlace_drop_mode_t;
  
/** \ingroup video_options
 * Scaling algorithm
 */
  
typedef enum
  {
    GAVL_SCALE_AUTO,          /*!< Take mode from conversion quality */
    GAVL_SCALE_NEAREST,       /*!< Nearest neighbor                  */
    GAVL_SCALE_BILINEAR,      /*!< Bilinear                          */
    GAVL_SCALE_QUADRATIC,     /*!< Quadratic                         */
    GAVL_SCALE_CUBIC_BSPLINE, /*!< Cubic B-Spline */
    GAVL_SCALE_CUBIC_MITCHELL,/*!< Cubic Mitchell-Netravali */
    GAVL_SCALE_CUBIC_CATMULL, /*!< Cubic Catmull-Rom */
    GAVL_SCALE_SINC_LANCZOS,  /*!< Sinc with Lanczos window. Set order with \ref gavl_video_options_set_scale_order */
  } gavl_scale_mode_t;

/** \ingroup video_options
 * Opaque container for video conversion options
 *
 * You don't want to know what's inside.
 */

typedef struct gavl_video_options_s gavl_video_options_t;

/* Default Options */

/*! \ingroup video_options
 *  \brief Set all options to their defaults
 *  \param opt Video options
 */
  
void gavl_video_options_set_defaults(gavl_video_options_t * opt);

/*! \ingroup video_options
 *  \brief Set source and destination rectangles
 *  \param opt Video options
 *  \param src_rect Rectangular area in the source frame or NULL
 *  \param dst_rect Rectangular area in the destination frame or NULL
 *
 *  Set the source and destination rectangles the converter or scaler will operate on.
 *  If you don't call this function, the rectangles will be set to the full image dimensions
 *  of the source and destination formats respectively. If one rectangle is NULL, BOTH rectangles
 *  will be cleared as if you never called this function.
 */
  
void gavl_video_options_set_rectangles(gavl_video_options_t * opt,
                                       const gavl_rectangle_f_t * src_rect,
                                       const gavl_rectangle_i_t * dst_rect);

/*! \ingroup video_options
 *  \brief Set the quality level for the converter
 *  \param opt Video options
 *  \param quality Quality level (see \ref quality)
 */

void gavl_video_options_set_quality(gavl_video_options_t * opt, int quality);

/*! \ingroup video_options
 *  \brief Set the conversion flags
 *  \param opt Video options
 *  \param conversion_flags Conversion flags (see \ref video_conversion_flags)
 */
  
void gavl_video_options_set_conversion_flags(gavl_video_options_t * opt,
                                             int conversion_flags);

/*! \ingroup Video options
 *  \brief Set the alpha mode
 *  \param opt Video options
 *  \param alpha_mode Alpha mode
 */

void gavl_video_options_set_alpha_mode(gavl_video_options_t * opt,
                                       gavl_alpha_mode_t alpha_mode);

/*! \ingroup Video options
 *  \brief Set the scale mode
 *  \param opt Video options
 *  \param scale_mode Scale mode
 */
  
void gavl_video_options_set_scale_mode(gavl_video_options_t * opt,
                                       gavl_scale_mode_t scale_mode);

/*! \ingroup Video options
 *  \brief Set the scale order for GAVL_SCALE_SINC_LANCZOS
 *  \param opt Video options
 *  \param order Order (must be at least 4)
 */
  
void gavl_video_options_set_scale_order(gavl_video_options_t * opt,
                                        int order);

  
/*! \ingroup Video options
 *  \brief Set the background color for alpha blending
 *  \param opt Video options
 *  \param color Array of 3 float values (0.0 .. 1.0) in RGB order
 */

void gavl_video_options_set_background_color(gavl_video_options_t * opt,
                                             float * color);

/*! \ingroup video_options
 *  \brief Get the conversion flags
 *  \param opt Video options
 *  \returns Flags (see \ref video_conversion_flags)
 */

int gavl_video_options_get_conversion_flags(gavl_video_options_t * opt);

/*! \ingroup Video options
 *  \brief Set the deinterlace mode
 *  \param opt Video options
 *  \param deinterlace_mode Deinterlace mode
 */
  
void gavl_video_options_set_deinterlace_mode(gavl_video_options_t * opt,
                                             gavl_deinterlace_mode_t deinterlace_mode);

/*! \ingroup Video options
 *  \brief Set the deinterlace drop mode
 *  \param opt Video options
 *  \param deinterlace_drop_mode Deinterlace drop mode
 */
  
void gavl_video_options_set_deinterlace_drop_mode(gavl_video_options_t * opt,
                                                  gavl_deinterlace_drop_mode_t deinterlace_drop_mode);

  
/***************************************************
 * Create and destroy video converters
 ***************************************************/
  
/** \defgroup video_converter Video converter
 * \ingroup video
 * \brief Video format converter
 *
 * This is a generic converter, which converts video frames from one arbitrary format to
 * another. It's main purpose is to convert pixelformats and to scale images. For quality levels
 * below 4, pixelformats are converted in one single step, without the need for intermediate
 * frames. Furthermore, it can handle nonsquare pixels. This means, that if the pixel aspect ratio
 * is different for the input and output formats, the image will be scaled to preserve the
 * overall aspect ratio of the video.
 *
 * Create a video converter with \ref gavl_video_converter_create. Call
 * \ref gavl_video_converter_init to initialize the converter for the input and output formats.
 * Video frames are then converted with \ref gavl_video_convert.
 */

/*! \ingroup video_converter
 * \brief Opaque video converter structure
 *
 * You don't want to know what's inside.
 */
  
typedef struct gavl_video_converter_s gavl_video_converter_t;

/*! \ingroup video_converter
 *  \brief Creates a video converter
 *  \returns A newly allocated video converter
 */

gavl_video_converter_t * gavl_video_converter_create();

/*! \ingroup video_converter
 *  \brief Destroys a video converter and frees all associated memory
 *  \param cnv A video converter
 */
  
void gavl_video_converter_destroy(gavl_video_converter_t*cnv);

/**************************************************
 * Get options. Change the options with the gavl_video_options_set_*
 * functions above
 **************************************************/

/*! \ingroup video_converter
 *  \brief gets options of a video converter
 *  \param cnv A video converter
 *
 * After you called this, you can use the gavl_video_options_set_*() functions to change
 * the options. Options will become valid with the next call to \ref gavl_video_converter_init
 */
  
gavl_video_options_t *
gavl_video_converter_get_options(gavl_video_converter_t*cnv);


/*! \ingroup video_converter
 *  \brief Initialize a video converter
 *  \param cnv A video converter
 *  \param input_format Input format
 *  \param output_format Output format
 *  \returns The number of single conversion steps necessary to perform the
 *           conversion. It may be 0, in this case you don't need the converter and
 *           can pass the video frames directly. If something goes wrong (should never happen),
 *           -1 is returned.
 *
 * This function can be called multiple times with one instance
 */
  
int gavl_video_converter_init(gavl_video_converter_t* cnv,
                              const gavl_video_format_t * input_format,
                              const gavl_video_format_t * output_format);

/***************************************************
 * Convert a frame
 ***************************************************/

/*! \ingroup video_converter
 *  \brief Convert video
 *  \param cnv A video converter
 *  \param input_frame Input frame
 *  \param output_frame Output frame
 */
  
void gavl_video_convert(gavl_video_converter_t * cnv,
                        gavl_video_frame_t * input_frame,
                        gavl_video_frame_t * output_frame);

/*! \defgroup video_scaler Scaler
 *  \ingroup video
 *  \brief Video scaler
 *
 *  The video scaler can resize images, resample chroma planes
 *  and perform simple deinterlacing. You can use it either through the
 *  \ref gavl_video_converter_t or directly through the functions in this module.
 *
 *  The scaler does the elementary operation to take a rectangular area
 *  of the source image and scale it into a specified rectangular area of the
 *  destination image. The source rectangle has floating point coordinates, the destination
 *  rectangle must have integer coordinates, which are aligned to chroma subsampling factors.
 *  When scaling with arbitrary ratios, it makes sense to use a destination format without
 *  chroma subsampling.
 *
 *  Internally, the source and destination rectangles can be set
 *  independently for all planes and fields, which means, that the scaler can also do colospace
 *  conversions for pixelformats which differ only in the chroma subsampling modes.
 */

/*! \ingroup video_scaler
 *  \brief Opaque scaler structure.
 *
 *  You don't want to know what's inside.
 */
  
typedef struct gavl_video_scaler_s gavl_video_scaler_t;

/*! \ingroup video_scaler
 *  \brief Create a video scaler
 *  \returns A newly allocated video scaler
 */

gavl_video_scaler_t * gavl_video_scaler_create();

/*! \ingroup video_scaler
 *  \brief Destroy a video scaler
 *  \param scaler A video scaler
 */

void gavl_video_scaler_destroy(gavl_video_scaler_t * scaler);

/*! \ingroup video_scaler
 *  \brief gets options of a scaler
 *  \param scaler A video scaler
 *
 * After you called this, you can use the gavl_video_options_set_*() functions to change
 * the options. Options will become valid with the next call to \ref gavl_video_scaler_init
 */
  
gavl_video_options_t *
gavl_video_scaler_get_options(gavl_video_scaler_t * scaler);

/*! \ingroup video_scaler
 *  \brief Initialize a video scaler
 *  \param scaler A video scaler
 *  \param src_format Input format
 *  \param dst_format Output format
 *  \returns If something goes wrong (should never happen), -1 is returned.
 *
 * You should have equal pixelformats in the source and destination.
 * This function can be called multiple times with one instance. 
 */


int gavl_video_scaler_init(gavl_video_scaler_t * scaler,
                           const gavl_video_format_t * src_format,
                           const gavl_video_format_t * dst_format);

/*! \ingroup video_scaler
 *  \brief Scale video
 *  \param scaler A video scaler
 *  \param input_frame Input frame
 *  \param output_frame Output frame
 */
  
void gavl_video_scaler_scale(gavl_video_scaler_t * scaler,
                             gavl_video_frame_t * input_frame,
                             gavl_video_frame_t * output_frame);

/**************************************************
 * Transparent overlays 
 **************************************************/
#if 0
/* Overlay format */
  
typedef struct
  {
  gavl_video_format_t overlay_format;
  gavl_video_format_t frame_format;
  } gavl_overlay_format_t;
    
/* Overlay struct */

typedef struct
  {
  gavl_video_frame_t * frame;
  gavl_rectangle_i_t dst_rectangle;
  } gavl_overlay_t;

/*
 *  Blend context
 */

typedef struct gavl_overlay_blend_context_s gavl_overlay_blend_context_t;

gavl_overlay_blend_context_t * gavl_overlay_blend_context_create();

void gavl_overlay_blend_context_destroy(gavl_overlay_blend_context_t *);
void gavl_overlay_blend_context_init(gavl_overlay_blend_context_t *,
                                     gavl_overlay_format_t *);

gavl_video_options_t *
gavl_overlay_blend_context_get_options(gavl_overlay_blend_context_t *);

void gavl_overlay_blend(gavl_overlay_blend_context_t *,
                        gavl_video_frame_t * dst_frame);

int gavl_overlay_blend_context_need_new(gavl_overlay_blend_context_t *);
void gavl_overlay_blend_context_set_overlay(gavl_overlay_blend_context_t *,
                                            gavl_overlay_t *);

#endif  
#ifdef __cplusplus
}
#endif

#endif /* GAVL_H_INCLUDED */
