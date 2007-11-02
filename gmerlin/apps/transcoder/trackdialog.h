
typedef struct track_dialog_s track_dialog_t;

track_dialog_t * track_dialog_create(bg_transcoder_track_t * t,
                                     void (*update_callback)(void * priv),
                                     void * update_priv, int show_tooltips,
                                     bg_plugin_registry_t * plugin_reg);

void track_dialog_run(track_dialog_t *, GtkWidget * parent);
void track_dialog_destroy(track_dialog_t *);
