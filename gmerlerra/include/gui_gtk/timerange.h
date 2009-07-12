#ifndef TIMERANGE_H
#define TIMERANGE_H

/* Common class for various
   timeline associated widgets */

typedef struct
  {
  int width;
  bg_nle_time_range_t selection;
  bg_nle_time_range_t visible;
  
  int selection_mode;
  
  int mouse_x;

  void (*set_selection)(bg_nle_time_range_t * selection, void * priv);
  void (*set_visible)(bg_nle_time_range_t * visible, void * priv);
  void (*set_zoom)(bg_nle_time_range_t * visible, void * priv);
  void (*motion_callback)(int64_t time, void * priv);
  void * callback_data;
  
  } bg_nle_timerange_widget_t;

int64_t bg_nle_pos_2_time(bg_nle_timerange_widget_t * w, double pos);
double bg_nle_time_2_pos(bg_nle_timerange_widget_t * w, int64_t time);

void bg_nle_timerange_widget_set_width(bg_nle_timerange_widget_t * w, int width);


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


#endif // TIMERANGE_H
