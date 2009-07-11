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


void bg_nle_time_ruler_update_visible(bg_nle_time_ruler_t * t);
void bg_nle_time_ruler_update_selection(bg_nle_time_ruler_t * t);
void bg_nle_time_ruler_update_zoom(bg_nle_time_ruler_t * t);

#endif // TIMERULER_H
