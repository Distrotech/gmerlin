#include <gui_gtk/timeruler.h>

typedef struct bg_nle_player_widget_s bg_nle_player_widget_t;

bg_nle_player_widget_t *
bg_nle_player_widget_create(bg_plugin_registry_t * plugin_reg,
                            bg_nle_time_ruler_t * ruler);

void
bg_nle_player_widget_destroy(bg_nle_player_widget_t *);

GtkWidget * bg_nle_player_widget_get_widget(bg_nle_player_widget_t *);
