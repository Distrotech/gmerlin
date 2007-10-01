
typedef struct plugin_window_s plugin_window_t;

plugin_window_t *
plugin_window_create(bg_plugin_registry_t * plugin_reg,
                     transcoder_window_t * win,
                     void (*close_notify)(plugin_window_t*,void*),
                     void * close_notify_data);

void plugin_window_destroy(plugin_window_t*);

void plugin_window_show(plugin_window_t*);

