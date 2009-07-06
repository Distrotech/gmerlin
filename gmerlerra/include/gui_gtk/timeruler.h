#ifndef TIMERULER_H
#define TIMERULER_H

#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>

#include <track.h>
#include <project.h>

typedef struct bg_nle_time_ruler_s bg_nle_time_ruler_t;

bg_nle_time_ruler_t * bg_nle_time_ruler_create(void);

void bg_nle_time_ruler_set_selection_callback(bg_nle_time_ruler_t *,
                                              void (*selection_changed)(void*,
                                                                        bg_nle_time_range_t*),
                                              void*);

void bg_nle_time_ruler_set_visibility_callback(bg_nle_time_ruler_t *,
                                               void (*callback)(void*,
                                                                bg_nle_time_range_t*),
                                               void*);

void bg_nle_time_ruler_destroy(bg_nle_time_ruler_t *);

GtkWidget * bg_nle_time_ruler_get_widget(bg_nle_time_ruler_t *);

void bg_nle_time_ruler_handle_button_press(bg_nle_time_ruler_t * r,
                                           GdkEventButton * evt);

void bg_nle_time_ruler_set_selection(bg_nle_time_ruler_t * t,
                                     const bg_nle_time_range_t * selection);

void bg_nle_time_ruler_set_visible(bg_nle_time_ruler_t * t,
                                   bg_nle_time_range_t * visible,
                                   int recalc_spacing);

/* Change zoom */

void bg_nle_time_ruler_zoom_in(bg_nle_time_ruler_t * r);
void bg_nle_time_ruler_zoom_out(bg_nle_time_ruler_t * r);
void bg_nle_time_ruler_zoom_fit(bg_nle_time_ruler_t * r);



#endif // TIMERULER_H
