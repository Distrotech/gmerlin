
/* Interface for libvisual plugins */

bg_plugin_info_t * bg_lv_get_info(const char * filename);

int bg_lv_load(bg_plugin_handle_t * ret,
               const char * name, int plugin_flags);

void bg_lv_unload(bg_plugin_handle_t *);
