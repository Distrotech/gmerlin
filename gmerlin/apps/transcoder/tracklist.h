typedef struct track_list_s track_list_t;

track_list_t * track_list_create(bg_plugin_registry_t * plugin_reg);

void track_list_destroy(track_list_t *);

GtkWidget * track_list_get_widget(track_list_t *);
