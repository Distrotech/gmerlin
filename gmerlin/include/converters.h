/*
 */

#include <gavl/gavl.h>
#include <plugin.h>

/** \defgroup converters A/V Converters
 * 
 *  A/V converters, which can be plugged together with
 *  filters and input plugins. These are basically wrappers around
 *  the \ref gavl_audio_converter_t and the \ref gavl_video_converter_t
 *  with the difference, that they work asynchronous with the
 *  advantage, that the video converter can do framerate conversion as well.
 *
 *  The functions for reading A/V frames are compatible with the read functions
 *  of input plugins and filters for easy generation of processing pipelines.
 *
 *  @{
 */

/** \brief Audio converter
 *
 *  Opaque audio converter structure. You don't want to know, what's inside.
 */

typedef struct bg_audio_converter_s bg_audio_converter_t;

/** \brief Video converter
 *
 *  Opaque video converter structure. You don't want to know, what's inside.
 */
typedef struct bg_video_converter_s bg_video_converter_t;

/* Audio */

/** \brief Create an audio converter
 *  \param opt Audio options
 *
 *  The options should be valid for the whole lifetime of the converter.
 */

bg_audio_converter_t * bg_audio_converter_create(const gavl_audio_options_t * opt);

/** \brief Initialize an audio converter
 *  \param cnv An audio converter
 *  \param in_format Input format
 *  \param out_format Output format
 *  \returns The number of conversion steps
 *
 *  When this function returns 0 it means, that the converter can be bypassed.
 *  It should not be used then until the next initialization, which returns nonzero.
 */

int bg_audio_converter_init(bg_audio_converter_t * cnv,
                            const gavl_audio_format_t * in_format,
                            const gavl_audio_format_t * out_format);

/** \brief Set input callback of an audio converter
 *  \param cnv An audio converter
 *  \param func The function to call
 *  \param priv The private handle to pass to func
 *  \param stream The stream argument to pass to func
 */

void bg_audio_converter_connect_input(bg_audio_converter_t * cnv,
                                      bg_read_audio_func_t func, void * priv,
                                      int stream);

/** \brief Read samples from an audio converter
 *  \param priv An audio converter
 *  \param frame An audio frame
 *  \param stream Stream number (must be 0)
 *  \param num_samples Number of samples to read
 *  \returns The number of samples read. 0 means EOF.
 *
 *  This function can be used as an input callback for audio filters.
 */

int bg_audio_converter_read(void * priv, gavl_audio_frame_t* frame, int stream,
                            int num_samples);

/** \brief Destroy an audio converter and free all associated memory
 *  \param cnv An audio converter
 */

void bg_audio_converter_destroy(bg_audio_converter_t * cnv);

/** \brief Reset an audio converter as if no samples have been processed since initialization
 *  \param cnv An audio converter
 *
 *  Reset an audio converter as if no samples have been processed since initialization.
 */

void bg_audio_converter_reset(bg_audio_converter_t * cnv);

/* Video */

/** \brief Create a video converter
 *  \param opt Video options
 *
 *  The options should be valid for the whole lifetime of the converter.
 */

bg_video_converter_t * bg_video_converter_create(const gavl_video_options_t * opt);

/** \brief Initialize a video converter
 *  \param cnv A video converter
 *  \param in_format Input format
 *  \param out_format Output format
 *  \returns The number of conversion steps
 *
 *  When this function returns 0 it means, that the converter can be bypassed.
 *  It should not be used then until the next initialization, which returns nonzero.
 */

int bg_video_converter_init(bg_video_converter_t * cnv,
                            const gavl_video_format_t * in_format,
                            const gavl_video_format_t * out_format);

/** \brief Set input callback of a video converter
 *  \param cnv A video converter
 *  \param func The function to call
 *  \param priv The private handle to pass to func
 *  \param stream The stream argument to pass to func
 */

void bg_video_converter_connect_input(bg_video_converter_t * cnv,
                                      bg_read_video_func_t func, void * priv,
                                      int stream);

/** \brief Read a video frame from a video converter
 *  \param priv A video converter
 *  \param frame A video frame
 *  \param stream Stream number (must be 0)
 *  \returns 1 if a frame was read, 0 means EOF.
 *
 *  This function can be used as an input callback for video filters.
 */

int bg_video_converter_read(void * priv, gavl_video_frame_t* frame, int stream);

/** \brief Destroy a video converter and free all associated memory
 *  \param cnv A video converter
 */

void bg_video_converter_destroy(bg_video_converter_t * cnv);

/** \brief Reset a video converter as if no samples have been processed since initialization
 *  \param cnv     A video converter
 *  \param out_pts The next pts, this filter should output
 *
 *  Reset a video converter as if no frame has been processed since initialization.
 */

void bg_video_converter_reset(bg_video_converter_t * cnv);

/**
 * @}
 */
