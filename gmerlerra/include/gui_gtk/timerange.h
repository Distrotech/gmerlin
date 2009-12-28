#ifndef TIMERANGE_H
#define TIMERANGE_H

#include <gmerlin/parameter.h>

/* Common class for various
   timeline associated widgets */

typedef struct
  {
  int width;
  bg_nle_time_range_t selection;
  bg_nle_time_range_t visible;
  bg_nle_time_range_t in_out;
  
  int64_t cursor_pos;
  
  int selection_mode;
  
  int64_t * snap_time;
  
  int mouse_x;

  gavl_frame_table_t * frametable;
  
  void (*set_selection)(bg_nle_time_range_t * selection, int64_t cursor_pos, void * priv);
  void (*set_visible)(bg_nle_time_range_t * visible, void * priv);
  void (*set_zoom)(bg_nle_time_range_t * visible, void * priv);
  void (*set_cursor_pos)(int64_t time, void * priv);
  void (*set_in_out)(bg_nle_time_range_t * selection, void * priv);
  
  void (*motion_callback)(int64_t time, void * priv);
  void * callback_data;
  
  } bg_nle_timerange_widget_t;

int64_t bg_nle_pos_2_time(bg_nle_timerange_widget_t * w, double pos);
double bg_nle_time_2_pos(bg_nle_timerange_widget_t * w, int64_t time);

void bg_nle_timerange_widget_set_width(bg_nle_timerange_widget_t * w, int width);

void bg_nle_timerange_widget_set_cursor_pos(bg_nle_timerange_widget_t * w, int64_t cursor_pos);


int bg_nle_timerange_widget_handle_button_press(bg_nle_timerange_widget_t * w,
                                                GtkWidget * widget,
                                                GdkEventButton * evt);

int bg_nle_timerange_widget_handle_motion(bg_nle_timerange_widget_t * w,
                                          GtkWidget * widget,
                                          GdkEventMotion * evt);

void bg_nle_timerange_widget_handle_scroll(bg_nle_timerange_widget_t * r,
                                           int64_t diff_time);

void bg_nle_timerange_widget_zoom_in(bg_nle_timerange_widget_t * r);
void bg_nle_timerange_widget_zoom_out(bg_nle_timerange_widget_t * r);
void bg_nle_timerange_widget_zoom_fit(bg_nle_timerange_widget_t * r);

void bg_nle_timerange_widget_toggle_in(bg_nle_timerange_widget_t * r);
void bg_nle_timerange_widget_toggle_out(bg_nle_timerange_widget_t * r);

int bg_nle_time_is_near(bg_nle_timerange_widget_t * r, int64_t time, double pos);

/* More time related stuff */

int bg_nle_set_time_unit(const char * name,
                         const bg_parameter_value_t * val,
                         int * mode);

typedef struct
  {
  int scale;
  gavl_frame_table_t * tab;
  gavl_timecode_format_t fmt;
  int mode;
  } bg_nle_time_info_t;


void bg_nle_convert_time(gavl_time_t t,
                         int64_t * ret,
                         bg_nle_time_info_t * info);

#endif // TIMERANGE_H
