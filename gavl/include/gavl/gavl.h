/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

/**
 * @file gavl.h
 * external api header.
 */

#ifndef GAVL_H_INCLUDED
#define GAVL_H_INCLUDED

#include <inttypes.h>

#include "gavltime.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \ingroup video_format
 *\brief Video format
 */

typedef struct gavl_video_format_s gavl_video_format_t;

#include "timecode.h"
  
/* Quality levels */
  
/** \defgroup quality Quality settings
    \brief Generic quality settings

    Gavl allows multiple versions of each conversion routine. Optimized routines often
    have a worse precision, while highly accurate routines are too slow for live playback.
    Quality level 3 enables the standard ANSI-C versions, which are always available, or
    an optimized version of the same accuracy. 
    Qualities 1 and 2 choose optimized versions which are less accurate. Qualities 4 and 5
    enable high quality versions.

    Not all routines are available for quality levels other than 3. In these cases, the
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

/** \defgroup accel_flags Acceleration flags
 *  \brief CPU specific acceleration flags
 *
 *  These flags are used internally by gavl to obtain the supported
 *  acceleration mechanisms at runtime. Some applications however might
 *  need them as well for other tasks, so they are exported into the
 *  public API.
 *
 *  @{
 */
  
#define GAVL_ACCEL_MMX      (1<<0) //!< MMX
#define GAVL_ACCEL_MMXEXT   (1<<1) //!< Extended MMX (a.k.a MMX2)
#define GAVL_ACCEL_SSE      (1<<2) //!< Intel SSE
#define GAVL_ACCEL_SSE2     (1<<3) //!< Intel SSE2
#define GAVL_ACCEL_SSE3     (1<<4) //!< Intel SSE3
#define GAVL_ACCEL_3DNOW    (1<<5) //!< AMD 3Dnow
#define GAVL_ACCEL_3DNOWEXT (1<<6) //!< AMD 3Dnow ext
#define GAVL_ACCEL_SSSE3    (1<<7) //!< Intel SSSE3

/** \brief Get the supported acceleration flags
 *  \returns A combination of GAVL_ACCEL_* flags.
 */
  
int gavl_accel_supported();

/**
 *  @}
 */
  
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
#define GAVL_MAX_CHANNELS 128
  
/** \ingroup audio_format
 *  \brief Format of one audio sample
 * 
 * For multibyte numbers, the byte order is always machine native endian
 */
  
typedef enum
  {
    GAVL_SAMPLE_NONE   = 0, /*!< Undefined */
    GAVL_SAMPLE_U8     = 1, /*!< Unsigned 8 bit */
    GAVL_SAMPLE_S8     = 2, /*!< Signed 8 bit */
    GAVL_SAMPLE_U16    = 3, /*!< Unsigned 16 bit */
    GAVL_SAMPLE_S16    = 4, /*!< Signed 16 bit */
    GAVL_SAMPLE_S32    = 5, /*!< Signed 32 bit */
    GAVL_SAMPLE_FLOAT  = 6,  /*!< Floating point (-1.0 .. 1.0) */
    GAVL_SAMPLE_DOUBLE = 7  /*!< Double (-1.0 .. 1.0) */
  } gavl_sample_format_t;

/** \ingroup audio_format
 *
 *  Interleave mode of the channels
 */
  
typedef enum
  {
    GAVL_INTERLEAVE_NONE = 0, /*!< No interleaving, all channels separate */
    GAVL_INTERLEAVE_2    = 1, /*!< Interleaved pairs of channels          */ 
    GAVL_INTERLEAVE_ALL  = 2  /*!< Everything interleaved                 */
  } gavl_interleave_mode_t;

/** \ingroup audio_format
 *  \brief Audio channel setup
 *
 * These are the channel locations used to identify the channel order
 * for an audio format
 */
  
typedef enum
  {
    GAVL_CHID_NONE         = 0,   /*!< Undefined                                 */
    GAVL_CHID_FRONT_CENTER,       /*!< For mono                                  */
    GAVL_CHID_FRONT_LEFT,         /*!< Front left                                */
    GAVL_CHID_FRONT_RIGHT,        /*!< Front right                               */
    GAVL_CHID_FRONT_CENTER_LEFT,  /*!< Left of Center                            */
    GAVL_CHID_FRONT_CENTER_RIGHT, /*!< Right of Center                           */
    GAVL_CHID_REAR_LEFT,          /*!< Rear left                                 */
    GAVL_CHID_REAR_RIGHT,         /*!< Rear right                                */
    GAVL_CHID_REAR_CENTER,        /*!< Rear Center                               */
    GAVL_CHID_SIDE_LEFT,          /*!< Side left                                 */
    GAVL_CHID_SIDE_RIGHT,         /*!< Side right                                */
    GAVL_CHID_LFE,                /*!< Subwoofer                                 */
    GAVL_CHID_AUX,                /*!< Additional channel (can be more than one) */
  } gavl_channel_id_t;

/** \ingroup audio_format
 *  \brief Audio Format
 *
 * Structure describing an audio format. The samples_per_frame member is used
 * exclusively by \ref gavl_audio_frame_create to determine how many bytes to
 * allocate.
 */
  
typedef struct 
  {
  int samples_per_frame;  /*!< Maximum number of samples per frame */
  int samplerate;         /*!< Samplerate */
  int num_channels;         /*!< Number of channels */
  gavl_sample_format_t   sample_format; /*!< Sample format */
  gavl_interleave_mode_t interleave_mode; /*!< Interleave mode */
  
  float center_level; /*!< linear factor for mixing center to front */
  float rear_level;   /*!< linear factor for mixing rear to front */

  gavl_channel_id_t channel_locations[GAVL_MAX_CHANNELS];   /*!< Which channel is stored where */
  
  } gavl_audio_format_t;

  
/* Audio format -> string conversions */

/*! 
  \ingroup audio_format
  \brief Convert a gavl_sample_format_t to a human readable string
  \param format A sample format
  \returns A string describing the format
 */
  
const char * gavl_sample_format_to_string(gavl_sample_format_t format);

/*! 
  \ingroup audio_format
  \brief Convert a string to a sample format
  \param str String

  str must be one of the strings returned by \ref gavl_sample_format_to_string
 */

gavl_sample_format_t gavl_string_to_sample_format(const char * str);

/*! \ingroup audio_format
 * \brief Get total number of supported sample formats
 * \returns total number of supported sample formats
 */

int gavl_num_sample_formats();

/*! \ingroup audio_format
 * \brief Get the sample format from index
 * \param index index (must be between 0 and the result of \ref gavl_num_sample_formats)
 * \returns The sample format corresponding to index or GAVL_SAMPLE_NONE.
 */

gavl_sample_format_t gavl_get_sample_format(int index);
  
/*! 
  \ingroup audio_format
  \brief Convert a gavl_channel_id_t to a human readable string
  \param id A channel id
 */

const char * gavl_channel_id_to_string(gavl_channel_id_t id);


/*! 
  \ingroup audio_format
  \brief Convert a gavl_interleave_mode_t to a human readable string
  \param mode An interleave mode
 */

const char * gavl_interleave_mode_to_string(gavl_interleave_mode_t mode);

/*! 
  \ingroup audio_format
  \brief Dump an audio format to stderr
  \param format An audio format
 */

void gavl_audio_format_dump(const gavl_audio_format_t * format);

/*!
  \ingroup audio_format
  \brief Get the index of a particular channel for a given format. 
  \param format An audio format
  \param id A channel id
  \returns The index of the channel in the format or -1 if such a channel is not present
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
  \brief Get number of side channels for a given format
  \param format An audio format
 */
  
int gavl_side_channels(const gavl_audio_format_t * format);

/*!
  \ingroup audio_format
  \brief Get number of aux channels for a given format
  \param format An audio format
 */

int gavl_aux_channels(const gavl_audio_format_t * format);

  
  
/*!
  \ingroup audio_format
  \brief Get number of LFE channels for a given format
  \param format An audio format
 */

int gavl_lfe_channels(const gavl_audio_format_t * format);

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
  \brief Compare 2 audio formats
  \param format_1 First format
  \param format_2 Second format
  \returns 1 if the formats are equal, 0 else
*/
  
int gavl_audio_formats_equal(const gavl_audio_format_t * format_1,
                              const gavl_audio_format_t * format_2);

/*!
  \ingroup audio_format
  \brief Set the default channel setup and indices
  \param format An audio format

  Set a default channel setup and channel indices if only the number of channels
  is known. The result might be wrong if you have
  something else than mono or stereo from a stream, which has no informtions about the
  speaker configurations.
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
  
typedef union 
  {
  uint8_t * u_8; /*!< Unsigned 8 bit samples */
  int8_t *  s_8; /*!< Signed 8 bit samples */

  uint16_t * u_16; /*!< Unsigned 16 bit samples */
  int16_t  * s_16; /*!< Signed 16 bit samples */
  
  uint32_t * u_32; /*!< Unsigned 32 bit samples (used internally only) */
  int32_t  * s_32; /*!< Signed 32 bit samples */
  
  float * f; /*!< Floating point samples */
  double * d; /*!< Double samples */
  } gavl_audio_samples_t;

/*!
  \ingroup audio_frame
  \brief Container for noninterleaved audio samples
 */
  
typedef union
  {
  uint8_t * u_8[GAVL_MAX_CHANNELS];/*!< Unsigned 8 bit channels */
  int8_t *  s_8[GAVL_MAX_CHANNELS];/*!< Signed 8 bit channels */

  uint16_t * u_16[GAVL_MAX_CHANNELS];/*!< Unsigned 16 bit channels */
  int16_t  * s_16[GAVL_MAX_CHANNELS];/*!< Signed 16 bit channels */
    
  uint32_t * u_32[GAVL_MAX_CHANNELS];/*!< Unsigned 32 bit channels */
  int32_t  * s_32[GAVL_MAX_CHANNELS];/*!< Signed 32 bit channels (used internally only) */

  float * f[GAVL_MAX_CHANNELS];/*!< Floating point channels */
  double * d[GAVL_MAX_CHANNELS];/*!< Double channels */
  
  } gavl_audio_channels_t;

/*!
  \ingroup audio_frame
  \brief Generic container for audio samples.

  This is the main container structure for audio data. The data are stored in unions,
  so you can access the matching pointer type without the need for casts. If you have
  noninterleaved channels, the i'th channel will be in channels[i].f (if you use floating
  point samples). For noninterleaved formats, use the samples member. valid_samples must
  be set by the source to the number of actually valid samples in this frame.

  Audio frames are created with \ref gavl_audio_frame_create and destroyed with
  \ref gavl_audio_frame_destroy. The memory can either be allocated by gavl (memory
  aligned) or by the caller.
  
 */
  
typedef struct 
  {
  gavl_audio_samples_t  samples; /*!< Sample pointer for interleaved formats         */ 
  gavl_audio_channels_t channels;/*!< Channel pointer for non interleaved formats    */
  int valid_samples;             /*!< Number of actually valid samples */
  int64_t timestamp;             /*!< Timestamp in samplerate tics */
  int channel_stride;            /*!< Byte offset between channels. Total allocated size is always num_channels * channel_stride */
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
  \brief Mute a number of samples at the start of an audio frame.
  \param frame An audio frame
  \param format The format of the frame
  \param num_samples Number of samples to mute
  
  Fills the frame with digital zero samples according to the audio format
*/

void gavl_audio_frame_mute_samples(gavl_audio_frame_t * frame,
                                   const gavl_audio_format_t * format,
                                   int num_samples);

  
  
/*!
  \ingroup audio_frame
  \brief Mute a single channel of an audio frame.
  \param frame An audio frame
  \param format The format of the frame
  \param channel The channel to mute

  Fills the frame with digital zero samples according to the audio format
*/
  
void gavl_audio_frame_mute_channel(gavl_audio_frame_t * frame,
                                   const gavl_audio_format_t * format,
                                   int channel);
  
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
  
int gavl_audio_frame_copy(const gavl_audio_format_t * format,
                          gavl_audio_frame_t * dst,
                          const gavl_audio_frame_t * src,
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
 *  \brief Dither mode
 */

typedef enum
  {
    GAVL_AUDIO_DITHER_NONE   = 0,
    GAVL_AUDIO_DITHER_AUTO   = 1,
    GAVL_AUDIO_DITHER_RECT   = 2,
    GAVL_AUDIO_DITHER_TRI    = 3,
    GAVL_AUDIO_DITHER_SHAPED = 4,
  } gavl_audio_dither_mode_t;

/*! \ingroup audio_options
 *  \brief Resample mode
 */
  
typedef enum
  {
    GAVL_RESAMPLE_AUTO        = 0, /*!< Set from quality */
    GAVL_RESAMPLE_ZOH         = 1, /*!< Zero order hold interpolator, very fast, poor quality. */
    GAVL_RESAMPLE_LINEAR      = 2, /*!< Linear interpolator, very fast, poor quality. */
    GAVL_RESAMPLE_SINC_FAST   = 3, /*!< Band limited sinc interpolation, fastest, 97dB SNR, 80% BW. */
    GAVL_RESAMPLE_SINC_MEDIUM = 4, /*!< Band limited sinc interpolation, medium quality, 97dB SNR, 90% BW. */
    GAVL_RESAMPLE_SINC_BEST   = 5  /*!< Band limited sinc interpolation, best quality, 97dB SNR, 96% BW. */
  } gavl_resample_mode_t;
  
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
 *  \brief Get the quality level for a converter
 *  \param opt Audio options
 *  \returns Quality level (see \ref quality)
 */
  
int gavl_audio_options_get_quality(gavl_audio_options_t * opt);
  
/*! \ingroup audio_options
 *  \brief Set the dither mode for the converter
 *  \param opt Audio options
 *  \param mode A dither mode
 */
  
void gavl_audio_options_set_dither_mode(gavl_audio_options_t * opt, gavl_audio_dither_mode_t mode);

/*! \ingroup audio_options
 *  \brief Get the dither mode for the converter
 *  \param opt Audio options
 *  \returns The dither mode
 */
  
gavl_audio_dither_mode_t gavl_audio_options_get_dither_mode(gavl_audio_options_t * opt);

  
/*! \ingroup audio_options
 *  \brief Set the resample mode for the converter
 *  \param opt Audio options
 *  \param mode A resample mode
 */
  
void gavl_audio_options_set_resample_mode(gavl_audio_options_t * opt, gavl_resample_mode_t mode);

/*! \ingroup audio_options
 *  \brief Get the resample mode for the converter
 *  \param opt Audio options
 *  \returns The resample mode
 */
  
gavl_resample_mode_t gavl_audio_options_get_resample_mode(gavl_audio_options_t * opt);
  
/*! \ingroup audio_options
 *  \brief Set the conversion flags
 *  \param opt Audio options
 *  \param flags Flags (see \ref audio_conversion_flags)
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

/*! \ingroup audio_options
 *  \brief Create an options container
 *  \returns Newly allocated udio options with default values
 *
 *  Use this to store options, which will apply for more than
 *  one converter instance. Applying the options will be done by
 *  gavl_*_get_options() followed by gavl_audio_options_copy().
 */
  
gavl_audio_options_t * gavl_audio_options_create();

/*! \ingroup audio_options
 *  \brief Copy audio options
 *  \param dst Destination
 *  \param src Source
 */

void gavl_audio_options_copy(gavl_audio_options_t * dst,
                             const gavl_audio_options_t * src);

/*! \ingroup audio_options
 *  \brief Destroy audio options
 *  \param opt Audio options
 */

void gavl_audio_options_destroy(gavl_audio_options_t * opt);
  
  
  
/* Audio converter */

/** \defgroup audio_converter Audio converter
    \ingroup audio
    \brief Audio format converter.
    
    This is a generic converter, which converts audio frames from one arbitrary format to
    another. It does:

    - Up-/Downmixing of channel configurations
    - Resampling
    - Conversion of interleave modes
    - Conversion of sample formats

    Quality levels below 3 mainly result if poor but fast resampling methods.
    Quality levels above 3 will enable high quality resampling methods,
    dithering and floating point mixing.

    Create an audio converter with \ref gavl_audio_converter_create. If you want to configure it,
    get the options pointer with \ref gavl_audio_converter_get_options and change the options
    (See \ref audio_options).
    Call \ref gavl_audio_converter_init to initialize the converter for the input and output
    formats. Audio frames are then converted with \ref gavl_audio_convert.

    When you are done, you can either reinitialize the converter or destroy it with
    \ref gavl_audio_converter_destroy.
    
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
 * the options. Options will become valid with the next call to \ref gavl_audio_converter_init or \ref gavl_audio_converter_reinit
 */

gavl_audio_options_t * gavl_audio_converter_get_options(gavl_audio_converter_t*cnv);


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
 *  \brief Reinitialize an audio converter
 *  \param cnv An audio converter
 *  \returns The number of single conversion steps necessary to perform the
 *           conversion. It may be 0, in this case you don't need the converter and
 *           can pass the audio frames directly. If something goes wrong (should never happen),
 *           -1 is returned.
 *
 * This function can be called if the input and output formats didn't
 * change but the options did.
 */

  
int gavl_audio_converter_reinit(gavl_audio_converter_t* cnv);

  
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
                        const gavl_audio_frame_t * input_frame,
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
                                    const gavl_audio_format_t * format);

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

/** \defgroup peak_detection Peak detector
    \ingroup audio
    \brief Detect peaks in the volume for steering normalizers and
      dynamic range compressors

    While normalizers and dynamic range controls are out of the scope
    of gavl, some low-level functionality can be provided
*/
 
/*! \ingroup peak_detection
 *  \brief Opaque structure for peak detector
 *
 * You don't want to know what's inside.
 */

typedef struct gavl_peak_detector_s gavl_peak_detector_t;
  
/* Create / destroy */

/*! \ingroup peak_detection
 *  \brief Create peak detector
 *  \returns A newly allocated peak detector
 */
  
gavl_peak_detector_t * gavl_peak_detector_create();

/*! \ingroup peak_detection
 *  \brief Destroys a peak detector and frees all associated memory
 *  \param pd A peak detector
 */

void gavl_peak_detector_destroy(gavl_peak_detector_t *pd);

/*! \ingroup peak_detection
 *  \brief Set format for a peak detector
 *  \param pd A peak detector
 *  \param format The format subsequent frames will be passed with
 *
 * This function can be called multiple times with one instance. It also
 * calls \ref gavl_peak_detector_reset.
 */

void gavl_peak_detector_set_format(gavl_peak_detector_t *pd,
                                   const gavl_audio_format_t * format);

/*! \ingroup peak_detection
 *  \brief Feed the peak detector with a new frame
 *  \param pd A peak detector
 *  \param frame An audio frame
 */
  
void gavl_peak_detector_update(gavl_peak_detector_t *pd,
                              gavl_audio_frame_t * frame);
  
/*! \ingroup peak_detection
 *  \brief Get the peak volume across all channels
 *  \param pd A peak detector
 *  \param min Returns minimum amplitude
 *  \param max Returns maximum amplitude
 *  \param abs Returns maximum absolute amplitude
 *
 *  The returned amplitudes are normalized such that the
 *  minimum amplitude corresponds to -1.0, the maximum amplitude
 *  corresponds to 1.0.
 */
  
void gavl_peak_detector_get_peak(gavl_peak_detector_t * pd,
                                 double * min, double * max,
                                 double * abs);

/*! \ingroup peak_detection
 *  \brief Get the peak volume for all channels separate
 *  \param pd A peak detector
 *  \param min Returns minimum amplitude
 *  \param max Returns maximum amplitude
 *  \param abs Returns maximum absolute amplitude
 *
 *  The returned amplitudes are normalized such that the
 *  minimum amplitude corresponds to -1.0, the maximum amplitude
 *  corresponds to 1.0.
 */
  
void gavl_peak_detector_get_peaks(gavl_peak_detector_t * pd,
                                  double * min, double * max,
                                  double * abs);
  
/*! \ingroup peak_detection
 *  \brief Reset a peak detector
 *  \param pd A peak detector
 */
  
void gavl_peak_detector_reset(gavl_peak_detector_t * pd);
  
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
  int y; /*!< Vertical offset from the top border of the frame */
  int w; /*!< Width */
  int h; /*!< Height */
  } gavl_rectangle_i_t;

/*! Floating point rectangle
 * \ingroup rectangle
 */
  
typedef struct
  {
  double x; /*!< Horizontal offset from the left border of the frame */
  double y; /*!< Vertical offset from the top border of the frame */
  double w; /*!< Width */
  double h; /*!< Height */
  } gavl_rectangle_f_t;

/*! \brief Crop an integer rectangle so it fits into the image size of a video format
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param format The video format into which the rectangle must fit
 */
  
void gavl_rectangle_i_crop_to_format(gavl_rectangle_i_t * r,
                                   const gavl_video_format_t * format);

/*! \brief Crop a floating point rectangle so it fits into the image size of a video format
 * \ingroup rectangle
 * \param r A floating point rectangle
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
 * This shrinks src_rect and dest_rect that neither is outside the image
 * boundaries of the format. Both rectangles will have the same dimensions.
 *
 * This function can be used for fitting a video image into a window
 * for the case, that no scaling is available.
 */
  
void gavl_rectangle_crop_to_format_noscale(gavl_rectangle_i_t * src_rect,
                                           gavl_rectangle_i_t * dst_rect,
                                           const gavl_video_format_t * src_format,
                                           const gavl_video_format_t * dst_format);

/*! \brief Crop 2 rectangles to their formats when scaling is available
 * \ingroup rectangle
 * \param src_rect Source rectangle
 * \param dst_rect Destination rectangle
 * \param src_format Source format
 * \param dst_format Destination format
 *
 * This shrinks src_rect and dest_rect that neither is outside the image
 * boundaries of the format.
 */
  
void gavl_rectangle_crop_to_format_scale(gavl_rectangle_f_t * src_rect,
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

/*! \brief Crop an integer rectangle by some pixels from the left border
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */
  
void gavl_rectangle_i_crop_left(gavl_rectangle_i_t * r,   int num_pixels);

/*! \brief Crop an integer rectangle by some pixels from the right border
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_i_crop_right(gavl_rectangle_i_t * r,  int num_pixels);

/*! \brief Crop an integer rectangle by some pixels from the top border
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_i_crop_top(gavl_rectangle_i_t * r,    int num_pixels);

/*! \brief Crop an integer rectangle by some pixels from the bottom border
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_i_crop_bottom(gavl_rectangle_i_t * r, int num_pixels);

/*! \brief Crop a float rectangle by some pixels from the left border
 * \ingroup rectangle
 * \param r A float rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */
 
void gavl_rectangle_f_crop_left(gavl_rectangle_f_t * r,   double num_pixels);

/*! \brief Crop a float rectangle by some pixels from the right border
 * \ingroup rectangle
 * \param r A float rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_f_crop_right(gavl_rectangle_f_t * r,  double num_pixels);

/*! \brief Crop a float rectangle by some pixels from the top border
 * \ingroup rectangle
 * \param r A float rectangle
 * \param num_pixels The number of pixels by which the rectangle gets smaller
 */

void gavl_rectangle_f_crop_top(gavl_rectangle_f_t * r,    double num_pixels);

/*! \brief Crop a float rectangle by some pixels from the bottom border
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
 * This aligns a rectangle such that the horizontal and vertical coordinates
 * are multiples of h_align and v_align respectively. When dealing
 * with chroma subsampled formats, you must call this function with the
 * return values of \ref gavl_pixelformat_chroma_sub before taking subframes from
 * video frames.
 */
  
void gavl_rectangle_i_align(gavl_rectangle_i_t * r, int h_align, int v_align);

/*! \brief Align a rectangle to a format
 * \ingroup rectangle
 * \param r An integer rectangle
 * \param format A video format
 *
 * The convenience function does the same as \ref gavl_rectangle_i_align
 * but takes a format as argument.
 */
  
void gavl_rectangle_i_align_to_format(gavl_rectangle_i_t * r,
                                      const gavl_video_format_t * format);

  
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



/*! \brief Convert an integer rectangle to a floating point rectangle
 * \ingroup rectangle
 * \param dst Destination rectangle
 * \param src Source rectangle
 */
  
void gavl_rectangle_i_to_f(gavl_rectangle_f_t * dst, const gavl_rectangle_i_t * src);

/*! \brief Convert a floating point rectangle to an integer rectangle
 * \ingroup rectangle
 * \param dst Destination rectangle
 * \param src Source rectangle
 */
  
void gavl_rectangle_f_to_i(gavl_rectangle_i_t * dst, const gavl_rectangle_f_t * src);
  
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

/*!\brief Calculate a destination rectangle for scaling
 * \ingroup rectangle
 * \param dst_rect Destination rectangle
 * \param src_format Source format
 * \param src_rect Source rectangle
 * \param dst_format Destination format
 * \param zoom Zoom factor
 * \param squeeze Squeeze factor
 *
 * Assuming we take src_rect from a frame in src_format,
 * calculate the optimal dst_rect in dst_format. The source and destination
 * display aspect ratio will be preserved unless it is changed with the squeeze
 * parameter.
 *
 * Zoom is a zoom factor (1.0 = 100 %). Squeeze is a value between -1.0 and 1.0,
 * which changes the apsect ratio in both directions. 0.0 means unchanged.
 *
 * Note that dst_rect might be outside the image dimensions of dst_format. If you
 * don't like this, call \ref gavl_rectangle_crop_to_format_scale afterwards.
 *
 * Furthermore, the chroma subsampling is ignored by this function. If you you use
 * the rectangles to fire up a \ref gavl_video_scaler_t, this is no problem (the
 * scaler will align the rectangles internally). You can align the destination
 * rectangle manually using \ref gavl_rectangle_i_align or
 * \ref gavl_rectangle_i_align_to_format.
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

  /** \ingroup video_format
 * Flag for grayscale pixelformats
 */
#define GAVL_PIXFMT_GRAY   (1<<13)
  
/*! \ingroup video_format
 * \brief Pixelformat definition
 */
  
typedef enum 
  {
    /*! \brief Undefined 
     */
    GAVL_PIXELFORMAT_NONE =  0, 

    /*! 8 bit gray, scaled 0x00..0xff
     */
    GAVL_GRAY_8          =  1 | GAVL_PIXFMT_GRAY,

    /*! 16 bit gray, scaled 0x0000..0xffff
     */
    GAVL_GRAY_16          =  2 | GAVL_PIXFMT_GRAY,
    
    /*! floating point gray, scaled 0.0..1.0
     */
    GAVL_GRAY_FLOAT       =  3 | GAVL_PIXFMT_GRAY,
    
    /*! 8 bit gray + alpha, scaled 0x00..0xff
     */
    GAVL_GRAYA_16          =  1 | GAVL_PIXFMT_GRAY | GAVL_PIXFMT_ALPHA,

    /*! 16 bit gray + alpha, scaled 0x0000..0xffff
     */
    GAVL_GRAYA_32          =  2 | GAVL_PIXFMT_GRAY | GAVL_PIXFMT_ALPHA,
    
    /*! floating point gray + alpha, scaled 0.0..1.0
     */
    GAVL_GRAYA_FLOAT       =  3 | GAVL_PIXFMT_GRAY | GAVL_PIXFMT_ALPHA,
    
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

    /*! 48 bit RGB. Each color is an uint16_t in native byte order. Color order is RGBRGB
     */
    GAVL_RGB_48       = 10 | GAVL_PIXFMT_RGB,
    /*! 64 bit RGBA. Each color is an uint16_t in native byte order. Color order is RGBARGBA
     */
    GAVL_RGBA_64      = 11 | GAVL_PIXFMT_RGB | GAVL_PIXFMT_ALPHA,
        
    /*! float RGB. Each color is a float (0.0 .. 1.0) in native byte order. Color order is RGBRGB
     */
    GAVL_RGB_FLOAT    = 12 | GAVL_PIXFMT_RGB,
    /*! float RGBA. Each color is a float (0.0 .. 1.0) in native byte order. Color order is RGBARGBA
     */
    GAVL_RGBA_FLOAT   = 13 | GAVL_PIXFMT_RGB  | GAVL_PIXFMT_ALPHA,

    /*! Packed YCbCr 4:2:2. Each component is an uint8_t. Component order is Y1 U1 Y2 V1
     */
    GAVL_YUY2            = 1 | GAVL_PIXFMT_YUV,
    /*! Packed YCbCr 4:2:2. Each component is an uint8_t. Component order is U1 Y1 V1 Y2
     */
    GAVL_UYVY            = 2 | GAVL_PIXFMT_YUV,
    /*! Packed YCbCrA 4:4:4:4. Each component is an uint8_t. Component order is YUVA. Luma and chroma are video scaled, alpha is 0..255. */

    GAVL_YUVA_32         = 3 | GAVL_PIXFMT_YUV | GAVL_PIXFMT_ALPHA,
    /*! Packed YCbCrA 4:4:4:4. Each component is an uint16_t. Component order is YUVA. Luma and chroma are video scaled, alpha is 0..65535. */

    GAVL_YUVA_64         = 4 | GAVL_PIXFMT_YUV | GAVL_PIXFMT_ALPHA,
    /*!
     * Packed YCbCr 4:4:4. Each component is a float. Luma is scaled 0.0..1.0, chroma is -0.5..0.5 */
    GAVL_YUV_FLOAT       = 5 | GAVL_PIXFMT_YUV,

    /*! Packed YCbCrA 4:4:4:4. Each component is a float. Luma is scaled 0.0..1.0, chroma is -0.5..0.5
     */
    GAVL_YUVA_FLOAT       = 6 | GAVL_PIXFMT_YUV | GAVL_PIXFMT_ALPHA,
    
    /*! Packed YCbCrA 4:4:4:4. Each component is an uint16_t. Component order is YUVA. Luma and chroma are video scaled, alpha is 0..65535.
     */
    
    GAVL_YUV_420_P       = 1 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
    /*! Planar YCbCr 4:2:2. Each component is an uint8_t
     */
    GAVL_YUV_422_P       = 2 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
    /*! Planar YCbCr 4:4:4. Each component is an uint8_t
     */
    GAVL_YUV_444_P       = 3 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
    /*! Planar YCbCr 4:1:1. Each component is an uint8_t
     */
    GAVL_YUV_411_P       = 4 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
    /*! Planar YCbCr 4:1:0. Each component is an uint8_t
     */
    GAVL_YUV_410_P       = 5 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
    
    /*! Planar YCbCr 4:2:0. Each component is an uint8_t, luma and chroma values are full range (0x00 .. 0xff)
     */
    GAVL_YUVJ_420_P      = 6 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV | GAVL_PIXFMT_YUVJ,
    /*! Planar YCbCr 4:2:2. Each component is an uint8_t, luma and chroma values are full range (0x00 .. 0xff)
     */
    GAVL_YUVJ_422_P      = 7 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV | GAVL_PIXFMT_YUVJ,
    /*! Planar YCbCr 4:4:4. Each component is an uint8_t, luma and chroma values are full range (0x00 .. 0xff)
     */
    GAVL_YUVJ_444_P      = 8 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV | GAVL_PIXFMT_YUVJ,

    /*! 16 bit Planar YCbCr 4:4:4. Each component is an uint16_t in native byte order.
     */
    GAVL_YUV_444_P_16 = 9 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
    /*! 16 bit Planar YCbCr 4:2:2. Each component is an uint16_t in native byte order.
     */
    GAVL_YUV_422_P_16 = 10 | GAVL_PIXFMT_PLANAR | GAVL_PIXFMT_YUV,
    
  } gavl_pixelformat_t;

/*! \ingroup video_format
 *  \brief Pixelformat for storing 1-dimensional integer data with 8 bits each */
#define GAVL_PIXELFORMAT_1D_8 GAVL_GRAY_8
/*! \ingroup video_format
 *  \brief Pixelformat for storing 2-dimensional integer data with 8 bits each */
#define GAVL_PIXELFORMAT_2D_8 GAVL_GRAYA_16
/*! \ingroup video_format
 *  \brief Pixelformat for storing 3-dimensional integer data with 8 bits each */
#define GAVL_PIXELFORMAT_3D_8 GAVL_RGB_24
/*! \ingroup video_format
 *  \brief Pixelformat for storing 4-dimensional integer data with 8 bits each */
#define GAVL_PIXELFORMAT_4D_8 GAVL_RGBA_32

/*! \ingroup video_format
 *  \brief Pixelformat for storing 1-dimensional integer data with 16 bits each */
#define GAVL_PIXELFORMAT_1D_16 GAVL_GRAY_16
/*! \ingroup video_format
 *  \brief Pixelformat for storing 2-dimensional integer data with 16 bits each */
#define GAVL_PIXELFORMAT_2D_16 GAVL_GRAYA_32
/*! \ingroup video_format
 *  \brief Pixelformat for storing 3-dimensional integer data with 16 bits each */
#define GAVL_PIXELFORMAT_3D_16 GAVL_RGB_48
/*! \ingroup video_format
 *  \brief Pixelformat for storing 4-dimensional integer data with 16 bits each */
#define GAVL_PIXELFORMAT_4D_16 GAVL_RGBA_64

/*! \ingroup video_format
 *  \brief Pixelformat for storing 1-dimensional FLOAT data */
#define GAVL_PIXELFORMAT_1D_FLOAT GAVL_GRAY_FLOAT
/*! \ingroup video_format
 *  \brief Pixelformat for storing 2-dimensional FLOAT data */
#define GAVL_PIXELFORMAT_2D_FLOAT GAVL_GRAYA_FLOAT
/*! \ingroup video_format
 *  \brief Pixelformat for storing 3-dimensional FLOAT data */
#define GAVL_PIXELFORMAT_3D_FLOAT GAVL_RGB_FLOAT
/*! \ingroup video_format
 *  \brief Pixelformat for storing 4-dimensional FLOAT data */
#define GAVL_PIXELFORMAT_4D_FLOAT GAVL_RGBA_FLOAT

  
/*
 *  Colormodel related functions
 */

/*! \ingroup video_format
 * \brief Check if a pixelformat is grayscale
 * \param fmt A pixelformat
 * \returns 1 if the pixelformat is grayscale, 0 else
 */
  
#define gavl_pixelformat_is_gray(fmt) ((fmt) & GAVL_PIXFMT_GRAY)

  
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
 *  \brief Get the effective number of bits for one pixel
 *  \param pixelformat A pixelformat
 *  \returns Number of bits per pixel
 */

int gavl_pixelformat_bits_per_pixel(gavl_pixelformat_t pixelformat);

/*! \ingroup video_format
 *  \brief Get the conversion penalty for pixelformat conversions
 *  \param src Source pixelformat
 *  \param dst Destination pixelformat
 *  \returns A number denoting the "cost" of the conversion
 *
 *  The number (the larger the worse) is calculated from several criteria
 *  and considers both speed and quality issues. Don't ever rely on
 *  specific absolute values, since they can change from version 
 *  to version (except 0, which is returned when and only when src and dst 
 *  are equal). Instead, only compare values returned for different 
 *  combinations among each other.
 */

int gavl_pixelformat_conversion_penalty(gavl_pixelformat_t src,
                                        gavl_pixelformat_t dst);

/*! \ingroup video_format
 *  \brief Get the best destination format for a given source format
 *  \param src Source pixelformat
 *  \param dst_supported List of supported destination format
 *  \param penalty If non-null, returns the conversion penalty
 *  \returns The best supported destination pixelformat
 *
 *  This function takes a source format and a list of supported 
 *  destination formats (terminated with \ref GAVL_PIXELFORMAT_NONE)
 *  and returns the format, which will result in the cheapest conversion
 *  (see \ref gavl_pixelformat_conversion_penalty).
 */

gavl_pixelformat_t 
gavl_pixelformat_get_best(gavl_pixelformat_t src,
                          const gavl_pixelformat_t * dst_supported,
                          int * penalty);
  


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
    GAVL_INTERLACE_NONE = 0,    /*!< Progressive */
    GAVL_INTERLACE_TOP_FIRST,   /*!< Top field first */
    GAVL_INTERLACE_BOTTOM_FIRST,/*!< Bottom field first */
    GAVL_INTERLACE_MIXED        /*!< Use interlace_mode of the frames */
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

  gavl_timecode_format_t  timecode_format;/*!< Optional timecode format */
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
  \brief Compare 2 video formats
  \param format_1 First format
  \param format_2 Second format
  \returns 1 if the formats are equal, 0 else
*/
  
int gavl_video_formats_equal(const gavl_video_format_t * format_1,
                             const gavl_video_format_t * format_2);

  
/*!
  \ingroup video_format
  \brief Get the chroma offsets relative to the luma samples
  \param format A video format 
  \param field Index of the field (0 = top, 1 = bottom). For progressive format, this is unused
  \param plane Index of the plane (1 = Cb, 2 = Cr)
  \param off_x Returns the offset in x-direction
  \param off_y Returns the offset in y-direction
*/
  
void gavl_video_format_get_chroma_offset(const gavl_video_format_t * format, int field, int plane,
                                         float * off_x, float * off_y);
  

  
/*! 
  \ingroup video_format
  \brief Dump a video format to stderr
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
 * Video frames are created with \ref gavl_video_frame_create and destroyed with
 * \ref gavl_video_frame_destroy. The memory can either be allocated by gavl (with
 * memory aligned scanlines) or by the caller.
 * 
 * Gavl video frames are always oriented top->bottom left->right. If you must flip frames,
 * use the functions \ref gavl_video_frame_copy_flip_x, \ref gavl_video_frame_copy_flip_y or
 * \ref gavl_video_frame_copy_flip_xy .
 */

/** \ingroup video_frame
 * Video frame
 */
  
typedef struct
  {
  uint8_t * planes[GAVL_MAX_PLANES]; /*!< Pointers to the planes */
  int strides[GAVL_MAX_PLANES];      /*!< For each plane, this stores the byte offset between the scanlines */
  
  void * user_data;    /*!< For storing user data (e.g. the corresponding XImage) */
  int64_t timestamp; /*!< Timestamp in stream specific units (see \ref video_format) */
  int64_t duration; /*!< Duration in stream specific units (see \ref video_format) */
  gavl_interlace_mode_t   interlace_mode;/*!< Interlace mode */
  gavl_timecode_t timecode; /*!< Timecode associated with this frame */
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
  
gavl_video_frame_t * gavl_video_frame_create_nopad(const gavl_video_format_t*format);

  

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
                            const gavl_video_format_t * format);

/*!
  \ingroup video_frame
  \brief Fill the frame with a user spefified color
  \param frame A video frame
  \param format Format of the data in the frame
  \param color Color components in RGBA format scaled 0.0 .. 1.0
 
*/

void gavl_video_frame_fill(gavl_video_frame_t * frame,
                           const gavl_video_format_t * format,
                           float * color);

  
/*!
  \ingroup video_frame
  \brief Copy one video frame to another
  \param format The format of the frames
  \param dst Destination 
  \param src Source

  The source and destination formats must be identical, as this routine does no
  format conversion. The scanlines however can be padded differently in the source and destination.
  This function only copies the image data. For copying the metadata (timestamp etc.) use#
  \ref gavl_video_frame_copy_metadata.
*/

void gavl_video_frame_copy(const gavl_video_format_t * format,
                           gavl_video_frame_t * dst,
                           const gavl_video_frame_t * src);

/*!
  \ingroup video_frame
  \brief Copy a single plane from one video frame to another
  \param format The format of the frames
  \param dst Destination 
  \param src Source
  \param plane The plane to copy

  Like \ref gavl_video_frame_copy but copies only a single plane
 
*/
  
void gavl_video_frame_copy_plane(const gavl_video_format_t * format,
                                 gavl_video_frame_t * dst,
                                 const gavl_video_frame_t * src, int plane);

/*!
  \ingroup video_frame
  \brief Copy one video frame to another with horizontal flipping
  \param format The format of the frames
  \param dst Destination 
  \param src Source

  Like \ref gavl_video_frame_copy but flips the image horizontally
 
*/
  
void gavl_video_frame_copy_flip_x(const gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  const gavl_video_frame_t * src);

/*!
  \ingroup video_frame
  \brief Copy one video frame to another with vertical flipping
  \param format The format of the frames
  \param dst Destination 
  \param src Source

  Like \ref gavl_video_frame_copy but flips the image vertically
 
*/
  
void gavl_video_frame_copy_flip_y(const gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  const gavl_video_frame_t * src);

/*!
  \ingroup video_frame
  \brief Copy one video frame to another with horizontal and vertical flipping
  \param format The format of the frames
  \param dst Destination 
  \param src Source

  Like \ref gavl_video_frame_copy but flips the image both horizontally and vertically
 
*/

void gavl_video_frame_copy_flip_xy(const gavl_video_format_t * format,
                                   gavl_video_frame_t * dst,
                                  const gavl_video_frame_t * src);

/*!
  \ingroup video_frame
  \brief Copy metadata of one video frame to another
  \param dst Destination 
  \param src Source

  This function only copies the metadata (timestamp, duration, timecode). For copying the image data
  use \ref gavl_video_frame_copy.

  Since 1.1.0.
*/

void gavl_video_frame_copy_metadata(gavl_video_frame_t * dst,
                                    const gavl_video_frame_t * src);

  
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
                                   const gavl_video_frame_t * src,
                                   gavl_video_frame_t * dst,
                                   gavl_rectangle_i_t * src_rect);

/*!
  \ingroup video_frame
  \brief Get a field from a frame
  \param pixelformat Pixelformat of the frames
  \param dst Destination
  \param src Source
  \param field Field index (0 = top field, 1 = bottom field)

  This fills the pointers and strides of the destination frame such that it
  will represent the speficied field of the source frame.
  Note that no data are copied here. This means that
  dst must be created with NULL as the format argument and \ref gavl_video_frame_null
  must be called before destroying dst.
*/

void gavl_video_frame_get_field(gavl_pixelformat_t pixelformat,
                                const gavl_video_frame_t * src,
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
                           const gavl_video_format_t * format,
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
 * \brief Force deinterlacing
 *
 * This option turns on deinterlacing by the converter in any case (i.e. also if the input format
 * pretends to be progressive or if the ouput format is interlaced).
 */

#define GAVL_FORCE_DEINTERLACE (1<<0)

/** \ingroup video_conversion_flags
 * \brief Pass chroma planes to the convolver
 */

#define GAVL_CONVOLVE_CHROMA   (1<<1)

/** \ingroup video_conversion_flags
 * \brief Normalize convolution matrices passed to the scaler
 */

#define GAVL_CONVOLVE_NORMALIZE (1<<2)

/** \ingroup video_conversion_flags
 * \brief Force chroma placement correction
 *
 *  Force chroma placement correction and resampling using a scaler.
 *  Usually this is done only for qualities above 3.
 */

#define GAVL_RESAMPLE_CHROMA    (1<<3)
  
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
    GAVL_DEINTERLACE_SCALE     = 2, /*!< Take one field and scale it vertically by 2 */
    GAVL_DEINTERLACE_BLEND     = 3  /*!< Linear blend fields together */
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
    GAVL_SCALE_NONE,          /*!< Used internally when the scaler is used as a convolver */
  } gavl_scale_mode_t;

/** \ingroup video_options
 *  Antialiasing filters 
 *
 *  Specifies the antialiasing filter to be used
 *  when downscaling images.
 *
 *  Since 1.1.0
 */
  
typedef enum
  {
    GAVL_DOWNSCALE_FILTER_AUTO = 0, //!< Auto selection based on quality
    GAVL_DOWNSCALE_FILTER_NONE, //!< Fastest method, might produce heavy aliasing artifacts
    GAVL_DOWNSCALE_FILTER_WIDE, //!< Widen the filter curve according to the scaling ratio. 
    GAVL_DOWNSCALE_FILTER_GAUSS, //!< Do a Gaussian preblur
  } gavl_downscale_filter_t;
  
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
 *  \brief Create an options container
 *  \returns Newly allocated udio options with default values
 *
 *  Use this to store options, which will apply for more than
 *  one converter instance. Applying the options will be done by
 *  gavl_*_get_options() followed by gavl_video_options_copy().
 */
  
gavl_video_options_t * gavl_video_options_create();

/*! \ingroup video_options
 *  \brief Copy video options
 *  \param dst Destination
 *  \param src Source
 */

void gavl_video_options_copy(gavl_video_options_t * dst,
                             const gavl_video_options_t * src);

/*! \ingroup video_options
 *  \brief Destroy video options
 *  \param opt Video options
 */

void gavl_video_options_destroy(gavl_video_options_t * opt);
  
  
/*! \ingroup video_options
 *  \brief Set source and destination rectangles
 *  \param opt Video options
 *  \param src_rect Rectangular area in the source frame or NULL
 *  \param dst_rect Rectangular area in the destination frame or NULL
 *
 *  Set the source and destination rectangles the converter or scaler will operate on.
 *  If you don't call this function, the rectangles will be set to the full image dimensions
 *  of the source and destination formats respectively. If one rectangle is NULL, BOTH rectangles
 *  will be cleared as if you never called this function. See \ref rectangle for convenience
 *  functions, which calculate the proper rectangles in some typical playback or transcoding
 *  situations.
 */
  
void gavl_video_options_set_rectangles(gavl_video_options_t * opt,
                                       const gavl_rectangle_f_t * src_rect,
                                       const gavl_rectangle_i_t * dst_rect);

/*! \ingroup video_options
 *  \brief Get source and destination rectangles
 *  \param opt Video options
 *  \param src_rect Returns the rectangular area in the source frame
 *  \param dst_rect Returns the rectangular area in the destination frame
 */
void gavl_video_options_get_rectangles(gavl_video_options_t * opt,
                                       gavl_rectangle_f_t * src_rect,
                                       gavl_rectangle_i_t * dst_rect);
  
/*! \ingroup video_options
 *  \brief Set the quality level for the converter
 *  \param opt Video options
 *  \param quality Quality level (see \ref quality)
 */

void gavl_video_options_set_quality(gavl_video_options_t * opt, int quality);

/*! \ingroup video_options
 *  \brief Get the quality level for the converter
 *  \param opt Video options
 *  \returns Quality level (see \ref quality)
 */

int gavl_video_options_get_quality(gavl_video_options_t * opt);

  
/*! \ingroup video_options
 *  \brief Set the conversion flags
 *  \param opt Video options
 *  \param conversion_flags Conversion flags (see \ref video_conversion_flags)
 */
  
void gavl_video_options_set_conversion_flags(gavl_video_options_t * opt,
                                             int conversion_flags);

/*! \ingroup video_options
 *  \brief Get the conversion flags
 *  \param opt Video options
 *  \returns Flags (see \ref video_conversion_flags)
 */

int gavl_video_options_get_conversion_flags(gavl_video_options_t * opt);
  
/*! \ingroup video_options
 *  \brief Set the alpha mode
 *  \param opt Video options
 *  \param alpha_mode Alpha mode
 */

void gavl_video_options_set_alpha_mode(gavl_video_options_t * opt,
                                       gavl_alpha_mode_t alpha_mode);

/*! \ingroup video_options
 *  \brief Get the alpha mode
 *  \param opt Video options
 *  \returns Alpha mode
 */

gavl_alpha_mode_t
gavl_video_options_get_alpha_mode(gavl_video_options_t * opt);

  
/*! \ingroup video_options
 *  \brief Set the scale mode
 *  \param opt Video options
 *  \param scale_mode Scale mode
 */
  
void gavl_video_options_set_scale_mode(gavl_video_options_t * opt,
                                       gavl_scale_mode_t scale_mode);

/*! \ingroup video_options
 *  \brief Get the scale mode
 *  \param opt Video options
 *  \returns Scale mode
 */
  
gavl_scale_mode_t
gavl_video_options_get_scale_mode(gavl_video_options_t * opt);

  
/*! \ingroup video_options
 *  \brief Set the scale order for GAVL_SCALE_SINC_LANCZOS
 *  \param opt Video options
 *  \param order Order (must be at least 4)
 */
  
void gavl_video_options_set_scale_order(gavl_video_options_t * opt,
                                        int order);

/*! \ingroup video_options
 *  \brief Get the scale order for GAVL_SCALE_SINC_LANCZOS
 *  \param opt Video options
 *  \returns Order
 */
  
int gavl_video_options_get_scale_order(gavl_video_options_t * opt);

  
/*! \ingroup video_options
 *  \brief Set the background color for alpha blending
 *  \param opt Video options
 *  \param color Array of 3 float values (0.0 .. 1.0) in RGB order
 */

void gavl_video_options_set_background_color(gavl_video_options_t * opt,
                                             const float * color);

/*! \ingroup video_options
 *  \brief Get the background color for alpha blending
 *  \param opt Video options
 *  \param color Returns 3 float values (0.0 .. 1.0) in RGB order
 */

void gavl_video_options_get_background_color(gavl_video_options_t * opt,
                                             float * color);
  
/*! \ingroup video_ptions
 *  \brief Set the deinterlace mode
 *  \param opt Video options
 *  \param deinterlace_mode Deinterlace mode
 */
  
void gavl_video_options_set_deinterlace_mode(gavl_video_options_t * opt,
                                             gavl_deinterlace_mode_t deinterlace_mode);

/*! \ingroup video_ptions
 *  \brief Get the deinterlace mode
 *  \param opt Video options
 *  \returns Deinterlace mode
 */
  
gavl_deinterlace_mode_t
gavl_video_options_get_deinterlace_mode(gavl_video_options_t * opt);

/*! \ingroup video_options
 *  \brief Set the deinterlace drop mode
 *  \param opt Video options
 *  \param deinterlace_drop_mode Deinterlace drop mode
 */
  
void gavl_video_options_set_deinterlace_drop_mode(gavl_video_options_t * opt,
                                                  gavl_deinterlace_drop_mode_t deinterlace_drop_mode);

/*! \ingroup video_options
 *  \brief Get the deinterlace drop mode
 *  \param opt Video options
 *  \returns Deinterlace drop mode
 */
  
gavl_deinterlace_drop_mode_t
gavl_video_options_get_deinterlace_drop_mode(gavl_video_options_t * opt);

/*!  \ingroup video_options
 *   \brief Set antialiasing filter for downscaling
 *   \param opt Video options
 *   \param f Filter type (see \ref gavl_downscale_filter_t)
 *
 *  Since 1.1.0
 */

void gavl_video_options_set_downscale_filter(gavl_video_options_t * opt,
                                             gavl_downscale_filter_t f);
  

/*! \ingroup video_options
 *  \brief Get the antialiasing filter for downscaling
 *  \param opt Video options
 *  \returns antialiasing filter for downscaling
 *
 *  Since 1.1.0
 */
  
gavl_downscale_filter_t
gavl_video_options_get_downscale_filter(gavl_video_options_t * opt);

/*!  \ingroup video_options
 *   \brief Set blur factor for downscaling
 *   \param opt Video options
 *   \param f Factor
 *
 *   Specifies an additional blur-factor for downscaling. The
 *   default value of 1.0 calculates the preblur coefficients
 *   according the the downsample factor. Larger values mean
 *   more blurring (and slower scaling), smaller values mean
 *   less blurring (and probably more aliasing artifacts),
 *   0 is equivalent to
 *   calling \ref gavl_video_options_set_downscale_filter
 *   with GAVL_DOWNSCALE_FILTER_NONE as argument.
 *
 *  Since 1.1.0
 */

void gavl_video_options_set_downscale_blur(gavl_video_options_t * opt,
                                           float f);

/*!  \ingroup video_options
 *   \brief Get blur factor for downscaling
 *   \param opt Video options
 *   \returns Factor
 *
 *  Since 1.1.0
 */
  
float gavl_video_options_get_downscale_blur(gavl_video_options_t * opt);
  
/***************************************************
 * Create and destroy video converters
 ***************************************************/
  
/** \defgroup video_converter Video converter
 * \ingroup video
 * \brief Video format converter
 *
 * This is a generic converter, which converts video frames from one arbitrary format to
 * another. It can convert pixelformats, scale images and deinterlace.
 *
 * For quality levels of 3 and below, pixelformats are converted in one single step,
 * without the need for intermediate frames.
 * Quality levels of 4 and 5 will take care of chroma placement. For this, a
 * \ref gavl_video_scaler_t will be used.
 *
 * Deinterlacing is enabled if the input format is interlaced and the output format is
 * progressive and the deinterlace mode is something else than \ref GAVL_DEINTERLACE_NONE.
 * You can also force deinterlacing (\ref GAVL_FORCE_DEINTERLACE).
 *
 * Create a video converter with \ref gavl_video_converter_create. If you want to configure it,
 * get the options pointer with \ref gavl_video_converter_get_options and change the options
 * (See \ref video_options).
 * Call \ref gavl_video_converter_init to initialize the converter for the input and output
 * formats. Video frames are then converted with \ref gavl_video_convert.
 *
 * When you are done, you can either reinitialize the converter or destroy it with
 * \ref gavl_video_converter_destroy.
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
 * the options. Options will become valid with the next call to \ref gavl_video_converter_init or \ref gavl_video_converter_reinit.
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

/*! \ingroup video_converter
 *  \brief Reinitialize a video converter
 *  \param cnv A video converter
 *  \returns The number of single conversion steps necessary to perform the
 *           conversion. It may be 0, in this case you don't need the converter and
 *           can pass the video frames directly. If something goes wrong (should never happen),
 *           -1 is returned.
 *
 * This function can be called if the input and output formats didn't
 * change but the options did.
 */
  
int gavl_video_converter_reinit(gavl_video_converter_t* cnv);
 
  
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
                        const gavl_video_frame_t * input_frame,
                        gavl_video_frame_t * output_frame);

/*! \defgroup video_scaler Scaler
 *  \ingroup video
 *  \brief Video scaler
 *
 *  The scaler does the elementary operation to take a rectangular area
 *  of the source image and scale it into a specified rectangular area of the
 *  destination image. The source rectangle has floating point coordinates, the destination
 *  rectangle must have integer coordinates, which are aligned to chroma subsampling factors.
 *
 *  The internal scale tables are created for each plane and field separately. This
 *  means that:
 *
 *  - We handle all flavors of chroma placement correctly
 *  - We can convert chroma subsampling by scaling the chroma planes and copying
 *    the luminance plane.
 *  - Since the scaler knows about fields, it will scale interlaced frames field-wise
 *    (not a good idea to scale interlaced frames vertically though).
 *  - Simple deinterlacing (See \ref GAVL_DEINTERLACE_SCALE)
 *    can be done by taking one source field and scale it vertically to the entire
 *    destination frame.
 *
 *  You can use the scaler directly (through \ref gavl_video_scaler_t). The generic video 
 *  converter (\ref gavl_video_converter_t) will create an internal scaler if necessary.
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
 *  \brief Initialize a video scaler as a generic convolver
 *  \param scaler A video scaler
 *  \param format Format (must be the same for input and output)
 *  \param h_radius Horizontal radius
 *  \param h_coeffs Horizontal coefficients
 *  \param v_radius Vertical radius
 *  \param v_coeffs Vertical coefficients
 *  \returns If something goes wrong (should never happen), -1 is returned.
 *
 * This initialized a scaler for use as a generic convolver.
 * The h_radius and v_radius arguments denote the numbers of pixels,
 * which are taken in both left and right from the center pixel, i.e.
 * a value of 1 will result in a 3-tap filter. The coefficients
 * must be given for ALL taps (the convolver does not assume the
 * coeffitients to be symmetric)
 *
 * This function can be called multiple times with one instance. 
 */


int gavl_video_scaler_init_convolve(gavl_video_scaler_t * scaler,
                                    const gavl_video_format_t * format,
                                    int h_radius, float * h_coeffs,
                                    int v_radius, float * v_coeffs);
  
/*! \ingroup video_scaler
 *  \brief Scale video
 *  \param scaler A video scaler
 *  \param input_frame Input frame
 *  \param output_frame Output frame
 */
  
void gavl_video_scaler_scale(gavl_video_scaler_t * scaler,
                             const gavl_video_frame_t * input_frame,
                             gavl_video_frame_t * output_frame);

/*! \defgroup video_deinterlacer Deinterlacer
 *  \ingroup video
 *  \brief Deinterlacer
 *
 *  Deinterlacing is supported either through the \ref gavl_video_converter_t
 *  or using a low level deinterlacer
 */

/*! \ingroup video_deinterlacer
 *  \brief Opaque deinterlacer structure.
 *
 *  You don't want to know what's inside.
 */


typedef struct gavl_video_deinterlacer_s gavl_video_deinterlacer_t;

/*! \ingroup video_deinterlacer
 *  \brief Create a video deinterlacer
 *  \returns A newly allocated video deinterlacer
 */

gavl_video_deinterlacer_t * gavl_video_deinterlacer_create();

/*! \ingroup video_deinterlacer
 *  \brief Destroy a video deinterlacer
 *  \param deinterlacer A video deinterlacer
 */

void gavl_video_deinterlacer_destroy(gavl_video_deinterlacer_t * deinterlacer);

/*! \ingroup video_deinterlacer
 *  \brief gets options of a deinterlacer
 *  \param deinterlacer A video deinterlacer
 *
 * After you called this, you can use the gavl_video_options_set_*() functions to change
 * the options. Options will become valid with the next call to \ref gavl_video_deinterlacer_init
 */
  
gavl_video_options_t *
gavl_video_deinterlacer_get_options(gavl_video_deinterlacer_t * deinterlacer);

/*! \ingroup video_deinterlacer
 *  \brief Initialize a video deinterlacer
 *  \param deinterlacer A video deinterlacer
 *  \param src_format Input format
 *  \returns If something goes wrong (should never happen), -1 is returned.
 *
 * You should have equal pixelformats in the source and destination.
 * This function can be called multiple times with one instance. 
 */
  
int gavl_video_deinterlacer_init(gavl_video_deinterlacer_t * deinterlacer,
                                 const gavl_video_format_t * src_format);

  
/*! \ingroup video_deinterlacer
 *  \brief Deinterlace video
 *  \param deinterlacer A video deinterlacer
 *  \param input_frame Input frame
 *  \param output_frame Output frame
 */
  
void gavl_video_deinterlacer_deinterlace(gavl_video_deinterlacer_t * deinterlacer,
                                         const gavl_video_frame_t * input_frame,
                                         gavl_video_frame_t * output_frame);

  
  
/**************************************************
 * Transparent overlays 
 **************************************************/

/* Overlay struct */

/*! \defgroup video_blend Overlay blending
 * \ingroup video
 *
 *  Overlay blending does one elemental operation:
 *  Take a partly transparent overlay (in an alpha
 *  capable pixelformat) and blend it onto a video frame.
 *  Blending can be used for subtitles or OSD in playback applications,
 *  and also for lots of weird effects.
 *  In the current implementation, there is only one overlay
 *  pixelformat, which can be blended onto a cetrtain destination format.
 *  Therefore, the incoming overlay will be converted to the pixelformat
 *  necessary for the conversion. For OSD and Subtitle applications,
 *  this happens only once for each overlay, since the converted overlay is
 *  remembered by the blend context.
 *
 *  Note that gavl doesn't (and never will) support text subtitles. To
 *  blend text strings onto a video frame, you must render it into a
 *  gavl_overlay_t with some typesetting library (e.g. freetype) first.
 */
  
/*! \ingroup video_blend
 *  \brief Overlay structure.
 *
 *  Structure, which holds an overlay. If the sizes of source and destination
 *  rectangles differ, the smaller one will be used.
 */
 
typedef struct
  {
  gavl_video_frame_t * frame;    //!< Video frame in an alpha capable format */
  gavl_rectangle_i_t ovl_rect;   //!< Rectangle in the source frame     */
  int dst_x;                     //!< x offset in the destination frame. */
  int dst_y;                     //!< y offset in the destination frame. */
  } gavl_overlay_t;

/*! \ingroup video_blend
 *  \brief Opaque blend context.
 *
 *  You don't want to know what's inside.
 */
  
typedef struct gavl_overlay_blend_context_s gavl_overlay_blend_context_t;

/*! \ingroup video_blend
 *  \brief Create a blend context
 *  \returns A newly allocated blend context.
 */
  
gavl_overlay_blend_context_t * gavl_overlay_blend_context_create();

/*! \ingroup video_blend
 *  \brief Destroy a blend context and free all associated memory
 *  \param ctx A blend context
 */

void gavl_overlay_blend_context_destroy(gavl_overlay_blend_context_t * ctx);

/*! \ingroup video_blend
 *  \brief Get options from a blend context
 *  \param ctx A blend context
 *  \returns Options (See \ref video_options)
 */
  
gavl_video_options_t *
gavl_overlay_blend_context_get_options(gavl_overlay_blend_context_t * ctx);

/*! \ingroup video_blend
 *  \brief Initialize the blend context
 *  \param ctx A blend context
 *  \param frame_format The format of the destination frames
 *  \param overlay_format The format of the overlays
 * 
 *  Initialize a blend context for a given frame- and overlayformat.
 *  The image_width and image_height members for the overlay format represent
 *  the maximum overlay size. The actual displayed size will be determined
 *  by the ovl_rect of the overlay.
 *  The overlay_format might be changed to something, which can directly be blended.
 *  Make sure you have a \ref gavl_video_converter_t nearby.
 *  
 */

int gavl_overlay_blend_context_init(gavl_overlay_blend_context_t * ctx,
                                    const gavl_video_format_t * frame_format,
                                    gavl_video_format_t * overlay_format);

/*! \ingroup video_blend
 *  \brief Set a new overlay
 *  \param ctx A blend context
 *  \param ovl An overlay
 * 
 *  This function sets a new overlay, regardless of whether the last one has expired
 *  or not.
 */
  
void gavl_overlay_blend_context_set_overlay(gavl_overlay_blend_context_t * ctx,
                                            gavl_overlay_t * ovl);

/*! \ingroup video_blend
 *  \brief Blend overlay onto video frame
 *  \param ctx A blend context
 *  \param dst_frame Destination frame
 */
  
void gavl_overlay_blend(gavl_overlay_blend_context_t * ctx,
                        gavl_video_frame_t * dst_frame);

/*! \defgroup video_transform Image transformation
 * \ingroup video
 *
 * gavl includes a generic image transformation engine.
 * You pass a function pointer to the init function, which
 * transforms the destination coordinates into source
 * coordinates.
 *
 * The interpolation method is set with
 * \ref gavl_video_options_set_scale_mode, but not all modes
 * are supported. When initialized with an invalid scale mode,
 * the transformation engine will silently choose the closest one.
 *
 * @{
 */

/** \brief Opaque image transformation engine.
 *
 * You don't want to know what's inside.
 */

typedef struct gavl_image_transform_s gavl_image_transform_t;

/** \brief Function describing the method
 *  \param priv User data
 *  \param xdst X-coordinate of the destination
 *  \param ydst Y-coordinate of the destination
 *  \param xsrc Returns X-coordinate of the source
 *  \param ysrc Returns Y-coordinate of the source
 *
 *  All coordinates are in fractional pixels. 0,0 is the
 *  upper left corner. Return negative values or values
 *  larger than the dimesion to signal that a pixel is outside
 *  the source image.
 */
 
typedef void (*gavl_image_transform_func)(void * priv,
                                          double xdst,
                                          double ydst,
                                          double * xsrc,
                                          double * ysrc);


/** \brief Create a transformation engine
 *  \returns A newly allocated transformation engine
 */
  
gavl_image_transform_t * gavl_image_transform_create();

/** \brief Destroy a transformation engine
 *  \param t A transformation engine
 */

void gavl_image_transform_destroy(gavl_image_transform_t * t);

/** \brief Destroy a transformation engine
 *  \param t A transformation engine
 *  \param Format (can be changed)
 *  \param func Coordinate transform function
 *  \param priv The priv argument for func
 */

  
void gavl_image_transform_init(gavl_image_transform_t * t,
                               gavl_video_format_t * format,
                               gavl_image_transform_func func, void * priv);

/** \brief Transform an image
 *  \param t A transformation engine
 *  \param Input frame
 *  \param Output frame
 */
  
void gavl_image_transform_transform(gavl_image_transform_t * t,
                                    gavl_video_frame_t * in_frame,
                                    gavl_video_frame_t * out_frame);

gavl_video_options_t *
gavl_image_transform_get_options(gavl_image_transform_t * t);
  
/**
 * @}
 */

  
  
#ifdef __cplusplus
}
#endif

#endif /* GAVL_H_INCLUDED */
