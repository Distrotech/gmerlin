typedef struct track_list_s track_list_t;

track_list_t * track_list_create(bg_plugin_registry_t * plugin_reg);

void track_list_destroy(track_list_t *);

void track_list_set_audio_encoder(track_list_t *, bg_plugin_handle_t * h);
void track_list_set_video_encoder(track_list_t *, bg_plugin_handle_t * h);



GtkWidget * track_list_get_widget(track_list_t *);
