

/* Encoder window */

typedef struct encoder_pp_window_s encoder_pp_window_t;

encoder_pp_window_t * encoder_pp_window_create(bg_plugin_registry_t * plugin_reg);
void encoder_pp_window_run(encoder_pp_window_t * win, bg_transcoder_track_global_t * global);
void encoder_pp_window_destroy(encoder_pp_window_t * win);

const bg_plugin_info_t * encoder_pp_window_get_plugin(encoder_pp_window_t * win);

