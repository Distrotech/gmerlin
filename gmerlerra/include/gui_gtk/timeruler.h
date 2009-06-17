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
                                                                        int64_t start,
                                                                        int64_t end),
                                              void*);

void bg_nle_time_ruler_set_visibility_callback(bg_nle_time_ruler_t *,
                                               void (*callback)(void*,
                                                                int64_t start,
                                                                int64_t end),
                                               void*);

void bg_nle_time_ruler_destroy(bg_nle_time_ruler_t *);

GtkWidget * bg_nle_time_ruler_get_widget(bg_nle_time_ruler_t *);

int64_t bg_nle_time_ruler_pos_2_time(bg_nle_time_ruler_t * r, int pos);
double bg_nle_time_ruler_time_2_pos(bg_nle_time_ruler_t * r, int64_t time);


void bg_nle_time_ruler_get_range(bg_nle_time_ruler_t *,
                                 gavl_time_t * start, gavl_time_t * end);

void bg_nle_time_ruler_set_range(bg_nle_time_ruler_t *,
                                 gavl_time_t start, gavl_time_t end);

void bg_nle_time_ruler_handle_button_press(bg_nle_time_ruler_t * r,
                                           GdkEventButton * evt);

void bg_nle_time_ruler_get_selection(bg_nle_time_ruler_t * t,
                                     gavl_time_t * start, gavl_time_t * end);

void bg_nle_time_ruler_set_selection(bg_nle_time_ruler_t * t,
                                     gavl_time_t start, gavl_time_t end);

void bg_nle_time_ruler_set_visible(bg_nle_time_ruler_t * t,
                                   gavl_time_t start, gavl_time_t end);



/* Change zoom */

void bg_nle_time_ruler_zoom_in(bg_nle_time_ruler_t * r);
void bg_nle_time_ruler_zoom_out(bg_nle_time_ruler_t * r);
void bg_nle_time_ruler_zoom_fit(bg_nle_time_ruler_t * r);



#endif // TIMERULER_H
