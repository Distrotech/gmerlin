#include <gavl/gavl.h>
#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>

#include <project.h>

typedef struct bg_nle_renderer_instream_s bg_nle_renderer_instream_t;

bg_nle_renderer_instream_t *
bg_nle_renderer_instream_create(bg_nle_project_t * p,
                                bg_nle_track_t * t);

void bg_nle_renderer_instream_destroy(bg_nle_renderer_instream_t *);

int bg_nle_renderer_instream_read_video(bg_nle_renderer_instream_t *,
                                        gavl_video_frame_t * ret);

int bg_nle_renderer_instream_read_audio(bg_nle_renderer_instream_t *,
                                        gavl_audio_frame_t * ret,
                                        int num_samples);

int bg_nle_renderer_instream_seek(bg_nle_renderer_instream_t *,
                                  int64_t time);

/* Compositors */

typedef struct bg_nle_audio_compositor_s bg_nle_audio_compositor_t;

bg_nle_audio_compositor_t *
bg_nle_audio_compositor_create();

void bg_nle_audio_compositor_clear(bg_nle_audio_compositor_t *);

void bg_nle_audio_compositor_add_stream(bg_nle_audio_compositor_t *,
                                        bg_nle_renderer_instream_t * s);

int bg_nle_audio_compositor_read(bg_nle_audio_compositor_t *,
                                 gavl_audio_frame_t * ret,
                                 int num_samples);

int bg_nle_audio_compositor_seek(bg_nle_audio_compositor_t *,
                                 int64_t time);

void bg_nle_audio_compositor_destroy(bg_nle_audio_compositor_t *);

typedef struct bg_nle_video_compositor_s bg_nle_video_compositor_t;

bg_nle_video_compositor_t *
bg_nle_video_compositor_create();

void bg_nle_video_compositor_clear(bg_nle_video_compositor_t *);

void bg_nle_video_compositor_add_stream(bg_nle_video_compositor_t *,
                                        bg_nle_renderer_instream_t * s);

int bg_nle_video_compositor_read(bg_nle_video_compositor_t *,
                                 gavl_video_frame_t * ret);

int bg_nle_video_compositor_seek(bg_nle_video_compositor_t *,
                                 int64_t time);

void bg_nle_video_compositor_destroy(bg_nle_video_compositor_t *);

/* Renderer outstream */

typedef struct bg_nle_renderer_outstream_s bg_nle_renderer_outstream_t;

bg_nle_renderer_outstream_t *
bg_nle_renderer_outstream_create(bg_nle_project_t * p,
                                 bg_nle_outstream_t * s);

void bg_nle_renderer_outstream_destroy(bg_nle_renderer_outstream_t *);

int bg_nle_renderer_outstream_read_video(bg_nle_renderer_outstream_t *,
                                         gavl_video_frame_t * ret);

int bg_nle_renderer_outstream_read_audio(bg_nle_renderer_outstream_t *,
                                         gavl_audio_frame_t * ret,
                                         int num_samples);

void bg_nle_renderer_outstream_seek(bg_nle_renderer_outstream_t *,
                                    int64_t time);

/* Renderer */

typedef struct bg_nle_renderer_s bg_nle_renderer_t;

bg_nle_renderer_t * bg_nle_renderer_create(bg_nle_project_t * p);

void bg_nle_renderer_destroy(bg_nle_renderer_t *);

bg_track_info_t * bg_nle_renderer_get_track_info(bg_nle_renderer_t *);

int bg_nle_renderer_read_video(bg_nle_renderer_t *,
                               gavl_video_frame_t * ret, int stream);

int bg_nle_renderer_read_audio(bg_nle_renderer_t *,
                               gavl_audio_frame_t * ret,
                               int stream, int num_samples);

/* Seek to a time. Always call this after editing the project */

void bg_nle_renderer_seek(bg_nle_renderer_t *, gavl_time_t time);

const bg_parameter_info_t *
bg_nle_renderer_get_parameters(bg_nle_renderer_t *);

void bg_nle_renderer_set_parameters(void * data, const char * name,
                                    const bg_parameter_value_t * val);
