
typedef struct transcoder_window_s transcoder_window_t;

transcoder_window_t * transcoder_window_create();

void transcoder_window_destroy(transcoder_window_t*);
void transcoder_window_run(transcoder_window_t *);
transcoder_window_t * transcoder_window_create();

void transcoder_window_destroy(transcoder_window_t*);
void transcoder_window_run(transcoder_window_t *);

/* Set the default encoder plugins */

void transcoder_window_set_audio_encoder(transcoder_window_t *,
                                         bg_plugin_handle_t *);

void transcoder_window_set_video_encoder(transcoder_window_t *,
                                         bg_plugin_handle_t *);


