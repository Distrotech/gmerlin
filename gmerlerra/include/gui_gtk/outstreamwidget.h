#include <gui_gtk/timeruler.h>

typedef struct bg_nle_outstream_widget_s bg_nle_outstream_widget_t;

bg_nle_outstream_widget_t *
bg_nle_outstream_widget_create(bg_nle_outstream_t * outstream,
                               bg_nle_time_ruler_t * ruler,
                               bg_nle_timerange_widget_t * tr,
                               void (*play_callback)(bg_nle_outstream_widget_t *, void *),
                               void * callback_data);

void bg_nle_outstream_widget_destroy(bg_nle_outstream_widget_t *);

GtkWidget * bg_nle_outstream_widget_get_panel(bg_nle_outstream_widget_t *);
GtkWidget * bg_nle_outstream_widget_get_preview(bg_nle_outstream_widget_t *);

void bg_nle_outstream_widget_update_selection(bg_nle_outstream_widget_t * w);
void bg_nle_outstream_widget_update_visible(bg_nle_outstream_widget_t * w);
void bg_nle_outstream_widget_update_zoom(bg_nle_outstream_widget_t * w);


void bg_nle_outstream_widget_redraw(bg_nle_outstream_widget_t * w);
