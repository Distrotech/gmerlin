typedef struct track_list_s track_list_t;

track_list_t * track_list_create(bg_plugin_registry_t * plugin_reg,
                                 bg_cfg_section_t * track_defaults_section);

void track_list_destroy(track_list_t *);

GtkWidget * track_list_get_widget(track_list_t *);

bg_transcoder_track_t * track_list_get_track(track_list_t *);

void track_list_prepend_track(track_list_t *, bg_transcoder_track_t *);
