/*
 *  A/V converters, which can be plugged together with
 *  filters and input plugins
 */

#include <gavl/gavl.h>
#include <plugin.h>

typedef struct bg_audio_converter_s bg_audio_converter_t;
typedef struct bg_video_converter_s bg_video_converter_t;
typedef struct bg_framerate_converter_s bg_framerate_converter_t;

/* Audio */


bg_audio_converter_t * bg_audio_converter_create(const gavl_audio_options_t * opt);

int bg_audio_converter_init(bg_audio_converter_t * cnv,
                            const gavl_audio_format_t * in_format,
                            const gavl_audio_format_t * out_format);

void bg_audio_converter_connect_input(bg_audio_converter_t * cnv,
                                      bg_read_audio_func_t func, void * priv,
                                      int stream);

int bg_audio_converter_read(void * priv, gavl_audio_frame_t* frame, int stream,
                            int num_samples);

void bg_audio_converter_destroy(bg_audio_converter_t * cnv);

void bg_audio_converter_reset(bg_audio_converter_t * cnv);

/* Video */

bg_video_converter_t * bg_video_converter_create(const gavl_video_options_t * opt);

int bg_video_converter_init(bg_video_converter_t * cnv,
                            const gavl_video_format_t * in_format,
                            const gavl_video_format_t * out_format);

void bg_video_converter_connect_input(bg_video_converter_t * cnv,
                                      bg_read_video_func_t func, void * priv,
                                      int stream);

int bg_video_converter_read(void * priv, gavl_video_frame_t* frame, int stream);

void bg_video_converter_destroy(bg_video_converter_t * cnv);

void bg_video_converter_reset(bg_video_converter_t * cnv, int64_t out_pts);
