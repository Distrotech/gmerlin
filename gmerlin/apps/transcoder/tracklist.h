typedef struct track_list_s track_list_t;

track_list_t * track_list_create(bg_plugin_registry_t * plugin_reg,
                                 bg_cfg_section_t * track_defaults_section);

void track_list_destroy(track_list_t *);

GtkWidget * track_list_get_widget(track_list_t *);
GtkWidget * track_list_get_menu(track_list_t *);

bg_transcoder_track_t * track_list_get_track(track_list_t *);

void track_list_prepend_track(track_list_t *, bg_transcoder_track_t *);

void track_list_load(track_list_t * t, const char * filename);
void track_list_save(track_list_t * t, const char * filename);

void track_list_set_display_colors(track_list_t * t, float * fg, float * bg);

void track_list_set_tooltips(track_list_t * t, int show_tooltips);

void track_list_add_files(track_list_t * t, char ** files);
void track_list_add_xml(track_list_t * t, char * xml_string, int len);

bg_parameter_info_t * track_list_get_parameters(track_list_t * t);

void track_list_set_parameter(void * data, char * name, bg_parameter_value_t * val);
int track_list_get_parameter(void * data, char * name, bg_parameter_value_t * val);
