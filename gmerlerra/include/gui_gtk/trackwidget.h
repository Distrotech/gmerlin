#include <gui_gtk/timeruler.h>

typedef struct bg_nle_track_widget_s bg_nle_track_widget_t;

bg_nle_track_widget_t * bg_nle_track_widget_create(bg_nle_track_t * track,
                                                   bg_nle_time_ruler_t * ruler);

void bg_nle_track_widget_destroy(bg_nle_track_widget_t *);

GtkWidget * bg_nle_track_widget_get_panel(bg_nle_track_widget_t *);
GtkWidget * bg_nle_track_widget_get_preview(bg_nle_track_widget_t *);

void bg_nle_track_widget_redraw(bg_nle_track_widget_t * w);

void bg_nle_track_widget_set_selection(bg_nle_track_widget_t * w,
                                       bg_nle_time_range_t * selection);

void bg_nle_track_widget_set_visible(bg_nle_track_widget_t * w,
                                     bg_nle_time_range_t * visible);
