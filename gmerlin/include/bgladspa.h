/* Make gmerlin audio filters from ladspa plugins.
 * See http://www.ladspa.org for information about ladspa.
 *
 * For plugins, go to
 * http://tap-plugins.sourceforge.net
 * http://plugin.org.uk/
 */

bg_plugin_info_t * bg_ladspa_get_info(void * dll_handle, const char * filename);

int bg_ladspa_load(bg_plugin_handle_t * ret,
                   const bg_plugin_info_t * info);

void bg_ladspa_unload(bg_plugin_handle_t *);

