#include <gavl/gavl.h>
#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
#include <gmerlin/bggavl.h>

#include <project.h>

#define BG_NLE_OVERLAY_REPLACE 0
#define BG_NLE_OVERLAY_BLEND   1

typedef struct bg_nle_renderer_instream_audio_s
bg_nle_renderer_instream_audio_t;

typedef struct bg_nle_renderer_instream_video_s
bg_nle_renderer_instream_video_t;

bg_nle_renderer_instream_audio_t *
bg_nle_renderer_instream_audio_create(bg_nle_project_t * p,
                                      bg_nle_track_t * t,
                                      const gavl_audio_options_t * opt);

bg_nle_renderer_instream_video_t *
bg_nle_renderer_instream_video_create(bg_nle_project_t * p,
                                      bg_nle_track_t * t,
                                      const gavl_video_options_t * opt);

void bg_nle_renderer_instream_audio_destroy(bg_nle_renderer_instream_audio_t *);
void bg_nle_renderer_instream_video_destroy(bg_nle_renderer_instream_video_t *);

int
bg_nle_renderer_instream_video_connect_output(bg_nle_renderer_instream_video_t *,
                                              gavl_video_format_t * format,
                                              int * overlay_mode);

int
bg_nle_renderer_instream_audio_connect_output(bg_nle_renderer_instream_audio_t *,
                                              gavl_audio_format_t * format);

int bg_nle_renderer_instream_video_request(bg_nle_renderer_instream_video_t *,
                                           gavl_time_t time, int stream,
                                           float * camera, float * projector);

int bg_nle_renderer_instream_video_read(bg_nle_renderer_instream_video_t *,
                                        gavl_video_frame_t * ret, int stream);

int bg_nle_renderer_instream_audio_request(bg_nle_renderer_instream_audio_t *,
                                           int64_t sample_position,
                                           int num_samples, int stream);

int bg_nle_renderer_instream_audio_read(bg_nle_renderer_instream_audio_t *,
                                        gavl_audio_frame_t * ret,
                                        int num_samples, int stream);

int bg_nle_renderer_instream_audio_seek(bg_nle_renderer_instream_audio_t *,
                                        int64_t time);

int bg_nle_renderer_instream_video_seek(bg_nle_renderer_instream_video_t *,
                                        int64_t time);

/* Compositors */

typedef struct bg_nle_audio_compositor_s bg_nle_audio_compositor_t;

bg_nle_audio_compositor_t *
bg_nle_audio_compositor_create(bg_nle_outstream_t * s, bg_gavl_audio_options_t * opt,
                               const gavl_audio_format_t * format);


void bg_nle_audio_compositor_clear(bg_nle_audio_compositor_t *);

void bg_nle_audio_compositor_add_stream(bg_nle_audio_compositor_t *,
                                        bg_nle_renderer_instream_audio_t * s,
                                        bg_nle_track_t * t);

int bg_nle_audio_compositor_read(void * priv, gavl_audio_frame_t* frame, int stream,
                                 int num_samples);

int bg_nle_audio_compositor_seek(bg_nle_audio_compositor_t *,
                                 int64_t time);

void bg_nle_audio_compositor_destroy(bg_nle_audio_compositor_t *);

typedef struct bg_nle_video_compositor_s bg_nle_video_compositor_t;

bg_nle_video_compositor_t *
bg_nle_video_compositor_create(bg_nle_outstream_t * s,
                               bg_gavl_video_options_t * opt,
                               const gavl_video_format_t * format, float * bg_color);

void bg_nle_video_compositor_clear(bg_nle_video_compositor_t *);

void bg_nle_video_compositor_add_stream(bg_nle_video_compositor_t *,
                                        bg_nle_renderer_instream_video_t * s,
                                        bg_nle_track_t * t);

int bg_nle_video_compositor_read(void * priv, gavl_video_frame_t* frame, int stream);

int bg_nle_video_compositor_seek(bg_nle_video_compositor_t *,
                                 int64_t time);

void bg_nle_video_compositor_destroy(bg_nle_video_compositor_t *);

/* Renderer outstream */

typedef struct bg_nle_renderer_outstream_audio_s bg_nle_renderer_outstream_audio_t;
typedef struct bg_nle_renderer_outstream_video_s bg_nle_renderer_outstream_video_t;

bg_nle_renderer_outstream_audio_t *
bg_nle_renderer_outstream_audio_create(bg_nle_project_t * p,
                                       bg_nle_outstream_t * s,
                                       const gavl_audio_options_t * opt,
                                       gavl_audio_format_t * format);

bg_nle_renderer_outstream_video_t *
bg_nle_renderer_outstream_video_create(bg_nle_project_t * p,
                                       bg_nle_outstream_t * s,
                                       const gavl_video_options_t * opt,
                                       gavl_video_format_t * format);

void bg_nle_renderer_outstream_audio_destroy(bg_nle_renderer_outstream_audio_t *);
void bg_nle_renderer_outstream_video_destroy(bg_nle_renderer_outstream_video_t *);

void bg_nle_renderer_outstream_audio_add_istream(bg_nle_renderer_outstream_audio_t *,
                                                 bg_nle_renderer_instream_audio_t * s,
                                                 bg_nle_track_t * t);

void bg_nle_renderer_outstream_video_add_istream(bg_nle_renderer_outstream_video_t *,
                                                 bg_nle_renderer_instream_video_t * s,
                                                 bg_nle_track_t * t);


int bg_nle_renderer_outstream_video_read(bg_nle_renderer_outstream_video_t *,
                                         gavl_video_frame_t * ret);

int bg_nle_renderer_outstream_audio_read(bg_nle_renderer_outstream_audio_t *,
                                         gavl_audio_frame_t * ret,
                                         int num_samples);

void bg_nle_renderer_outstream_audio_seek(bg_nle_renderer_outstream_audio_t *,
                                    int64_t time);
void bg_nle_renderer_outstream_video_seek(bg_nle_renderer_outstream_video_t *,
                                          int64_t time);

/* Renderer */

typedef struct bg_nle_renderer_s bg_nle_renderer_t;

bg_nle_renderer_t * bg_nle_renderer_create(bg_plugin_registry_t * plugin_reg);

int bg_nle_renderer_open(bg_nle_renderer_t * r, bg_nle_project_t * p);

void bg_nle_renderer_destroy(bg_nle_renderer_t *);

bg_track_info_t * bg_nle_renderer_get_track_info(bg_nle_renderer_t *);

int bg_nle_renderer_set_audio_stream(bg_nle_renderer_t * r, int stream,
                                     bg_stream_action_t action);

int bg_nle_renderer_set_video_stream(bg_nle_renderer_t * r, int stream,
                                     bg_stream_action_t action);
int bg_nle_renderer_start(bg_nle_renderer_t * r);


int bg_nle_renderer_read_video(bg_nle_renderer_t *,
                               gavl_video_frame_t * ret, int stream);

int bg_nle_renderer_read_audio(bg_nle_renderer_t *,
                               gavl_audio_frame_t * ret,
                               int stream, int num_samples);



/* Seek to a time. Always call this after editing the project */

void bg_nle_renderer_seek(bg_nle_renderer_t *, gavl_time_t time);

const bg_parameter_info_t *
bg_nle_renderer_get_parameters(bg_nle_renderer_t *);

void bg_nle_renderer_set_parameter(void * data, const char * name,
                                   const bg_parameter_value_t * val);

/* Renderer plugin */

const char * bg_nle_plugin_get_extensions(void * priv);

const bg_parameter_info_t *
bg_nle_plugin_get_parameters(void*);

void bg_nle_plugin_set_parameter(void * data, const char * name,
                                 const bg_parameter_value_t * val);


void bg_nle_plugin_set_callbacks(void * priv, bg_input_callbacks_t * callbacks);
  
int bg_nle_plugin_open(void * priv, const char * arg);

//  const bg_edl_t * (*get_edl)(void * priv);
    
int bg_nle_plugin_get_num_tracks(void * priv);

bg_track_info_t * bg_nle_plugin_get_track_info(void * priv, int track);
  
int bg_nle_plugin_set_track(void * priv, int track);

int bg_nle_plugin_set_audio_stream(void * priv, int stream, bg_stream_action_t action);

int bg_nle_plugin_set_video_stream(void * priv, int stream, bg_stream_action_t action);

// int bg_plugin_nle_set_subtitle_stream(void * priv, int stream, bg_stream_action_t action);

int bg_nle_plugin_start(void * priv);

gavl_frame_table_t * bg_nle_plugin_get_frame_table(void * priv, int stream);

int bg_nle_plugin_read_audio(void * priv, gavl_audio_frame_t* frame, int stream,
                             int num_samples);

int bg_nle_plugin_read_video(void * priv, gavl_video_frame_t* frame, int stream);

void bg_nle_plugin_skip_video(void * priv, int stream, int64_t * time, int scale, int exact);

void bg_nle_plugin_seek(void * priv, int64_t * time, int scale);

void bg_nle_plugin_stop(void * priv);

void bg_nle_plugin_destroy(void * priv);

void * bg_nle_plugin_create(bg_nle_project_t * p,
                            bg_plugin_registry_t * plugin_reg);
