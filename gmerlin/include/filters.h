/*
 *  A/V converters, which can be plugged together with
 *  filters and input plugins
 */

#include <gavl/gavl.h>
#include <bggavl.h>
#include <plugin.h>

typedef struct bg_audio_filter_chain_s bg_audio_filter_chain_t;
typedef struct bg_video_filter_chain_s bg_video_filter_chain_t;

/* Audio */

bg_audio_filter_chain_t *
bg_audio_filter_chain_create(const bg_gavl_audio_options_t * opt,
                             bg_plugin_registry_t * plugin_reg);

bg_parameter_info_t *
bg_audio_filter_chain_get_parameters(bg_audio_filter_chain_t *);

void bg_audio_filter_chain_set_parameter(void * data,
                                         char * name,
                                         bg_parameter_value_t * val);

int bg_audio_filter_chain_init(bg_audio_filter_chain_t * cnv,
                               const gavl_audio_format_t * in_format,
                               gavl_audio_format_t * out_format);

void bg_audio_filter_chain_connect_input(bg_audio_filter_chain_t * cnv,
                                         bg_read_audio_func_t func,
                                         void * priv,
                                         int stream);

int bg_audio_filter_chain_read(void * priv, gavl_audio_frame_t* frame,
                               int stream,
                               int num_samples);

void bg_audio_filter_chain_destroy(bg_audio_filter_chain_t * cnv);

void bg_audio_filter_chain_lock(bg_audio_filter_chain_t * cnv);
void bg_audio_filter_chain_unlock(bg_audio_filter_chain_t * cnv);

/* Video */

bg_video_filter_chain_t *
bg_video_filter_chain_create(const bg_gavl_video_options_t * opt,
                             bg_plugin_registry_t * plugin_reg);

bg_parameter_info_t *
bg_video_filter_chain_get_parameters(bg_video_filter_chain_t *);

void bg_video_filter_chain_set_parameter(void * data, char * name,
                                         bg_parameter_value_t * val);


int bg_video_filter_chain_init(bg_video_filter_chain_t * cnv,
                               const gavl_video_format_t * in_format,
                               gavl_video_format_t * out_format);

void bg_video_filter_chain_connect_input(bg_video_filter_chain_t * cnv,
                                         bg_read_video_func_t func,
                                         void * priv, int stream);

int bg_video_filter_chain_read(void * priv, gavl_video_frame_t* frame,
                               int stream);

void bg_video_filter_chain_destroy(bg_video_filter_chain_t * cnv);

void bg_video_filter_chain_lock(bg_video_filter_chain_t * cnv);
void bg_video_filter_chain_unlock(bg_video_filter_chain_t * cnv);
