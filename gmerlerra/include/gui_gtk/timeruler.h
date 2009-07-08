#ifndef TIMERULER_H
#define TIMERULER_H

#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>

#include <track.h>
#include <project.h>

typedef struct bg_nle_time_ruler_s bg_nle_time_ruler_t;

bg_nle_time_ruler_t * bg_nle_time_ruler_create(bg_nle_timerange_widget_t * tr);


void bg_nle_time_ruler_destroy(bg_nle_time_ruler_t *);

GtkWidget * bg_nle_time_ruler_get_widget(bg_nle_time_ruler_t *);


void bg_nle_time_ruler_set_visible(bg_nle_time_ruler_t * t,
                                   bg_nle_time_range_t * visible,
                                   int recalc_spacing);

/* Change zoom */

void bg_nle_time_ruler_zoom_in(bg_nle_time_ruler_t * r);
void bg_nle_time_ruler_zoom_out(bg_nle_time_ruler_t * r);
void bg_nle_time_ruler_zoom_fit(bg_nle_time_ruler_t * r);



#endif // TIMERULER_H
