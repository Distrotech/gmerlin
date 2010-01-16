#include <gui_gtk/timeruler.h>

#define bg_nle_clipboard_atom "GMERLERRA_CLIPBOARD"

typedef struct bg_nle_player_widget_s bg_nle_player_widget_t;

bg_nle_player_widget_t *
bg_nle_player_widget_create(bg_plugin_registry_t * plugin_reg,
                            bg_nle_time_ruler_t * ruler);

void
bg_nle_player_widget_destroy(bg_nle_player_widget_t *);

GtkWidget * bg_nle_player_widget_get_widget(bg_nle_player_widget_t *);

void bg_nle_player_set_track(bg_nle_player_widget_t * w,
                             bg_plugin_handle_t * input_plugin,
                             bg_nle_file_t * file, bg_nle_project_t * p);

void bg_nle_player_realize(bg_nle_player_widget_t * w);

void bg_nle_player_set_oa_parameter(void * data,
                                    const char * name, const bg_parameter_value_t * val);

void bg_nle_player_set_ov_parameter(void * data,
                                    const char * name, const bg_parameter_value_t * val);

void bg_nle_player_set_display_parameter(void * data,
                                         const char * name, const bg_parameter_value_t * val);


bg_parameter_info_t * bg_nle_player_get_oa_parameters(bg_plugin_registry_t * plugin_reg);
bg_parameter_info_t * bg_nle_player_get_ov_parameters(bg_plugin_registry_t * plugin_reg);
