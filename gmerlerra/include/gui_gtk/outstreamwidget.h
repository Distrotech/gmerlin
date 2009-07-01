#include <gui_gtk/timeruler.h>

typedef struct bg_nle_outstream_widget_s bg_nle_outstream_widget_t;

bg_nle_outstream_widget_t *
bg_nle_outstream_widget_create(bg_nle_outstream_t * outstream,
                               bg_nle_time_ruler_t * ruler,
                               void (*play_callback)(bg_nle_outstream_widget_t *, void *),
                               void (*delete_callback)(bg_nle_outstream_widget_t *, void *),
                               void * callback_data);

void bg_nle_outstream_widget_destroy(bg_nle_outstream_widget_t *);

GtkWidget * bg_nle_outstream_widget_get_panel(bg_nle_outstream_widget_t *);
GtkWidget * bg_nle_outstream_widget_get_preview(bg_nle_outstream_widget_t *);

void bg_nle_outstream_widget_redraw(bg_nle_outstream_widget_t * w);
