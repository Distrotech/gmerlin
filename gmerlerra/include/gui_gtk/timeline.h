#ifndef TIMELINE_H
#define TIMELINE_H

#include <gui_gtk/timeruler.h>

typedef struct bg_nle_timeline_s bg_nle_timeline_t;

bg_nle_timeline_t * bg_nle_timeline_create(bg_nle_project_t*);
void bg_nle_timeline_destroy(bg_nle_timeline_t *);

GtkWidget * bg_nle_timeline_get_widget(bg_nle_timeline_t *);

void bg_nle_timeline_add_track(bg_nle_timeline_t *,
                               bg_nle_track_t * track, int pos);

void bg_nle_timeline_add_outstream(bg_nle_timeline_t * t,
                                   bg_nle_outstream_t * os, int pos);

void bg_nle_timeline_delete_track(bg_nle_timeline_t * t, int index);
void bg_nle_timeline_delete_outstream(bg_nle_timeline_t * t, int index);

void bg_nle_timeline_move_track(bg_nle_timeline_t * t, int old_index, int new_index);
void bg_nle_timeline_move_outstream(bg_nle_timeline_t * t, int old_index, int new_index);

void bg_nle_timeline_set_motion_callback(bg_nle_timeline_t *,
                                         void (*callback)(gavl_time_t time, void * data),
                                         void * data);

void bg_nle_timeline_set_selection(bg_nle_timeline_t * t, bg_nle_time_range_t * selection,
                                   int64_t cursor_pos);
void bg_nle_timeline_set_in_out(bg_nle_timeline_t * t, bg_nle_time_range_t * selection);

void bg_nle_timeline_set_cursor_pos(bg_nle_timeline_t * t, int64_t cursor_pos);

void bg_nle_timeline_set_visible(bg_nle_timeline_t * t, bg_nle_time_range_t * visible);
void bg_nle_timeline_set_zoom(bg_nle_timeline_t * t, bg_nle_time_range_t * visible);

void bg_nle_timeline_set_track_flags(bg_nle_timeline_t * t,
                                     bg_nle_track_t * track, int flags);

void bg_nle_timeline_set_outstream_flags(bg_nle_timeline_t * t,
                                         bg_nle_outstream_t * outstream, int flags);

void bg_nle_timeline_outstreams_make_current(bg_nle_timeline_t * t);

void bg_nle_timeline_update_track_parameters(bg_nle_timeline_t * t, int index,
                                             bg_cfg_section_t * s);

void bg_nle_timeline_update_outstream_parameters(bg_nle_timeline_t * t, int index,
                                                 bg_cfg_section_t * s);

bg_nle_time_ruler_t * bg_nle_timeline_get_ruler(bg_nle_timeline_t *);

void
bg_nle_timeline_set_display_parameter(bg_nle_timeline_t *, const char * name,
                                      const bg_parameter_value_t * val);
  

#endif // TIMELINE_H
