#include <gtk/gtk.h>
#include <inttypes.h>
#include <gavl/gavl.h>

#include <types.h>

#include <gui_gtk/timerange.h>
#include <gui_gtk/utils.h>

int64_t bg_nle_pos_2_time(bg_nle_timerange_widget_t * w, double pos)
  {
  double ret_d = (double)pos /
    (double)w->width * (double)(w->visible.end - w->visible.start);
  return (int64_t)(ret_d + 0.5) + w->visible.start;
  }

double bg_nle_time_2_pos(bg_nle_timerange_widget_t * w, int64_t time)
  {
  double ret_d = (double)(time - w->visible.start) /
    (double)(w->visible.end - w->visible.start) * (double)w->width;
  return ret_d;
  }

void bg_nle_timerange_widget_set_width(bg_nle_timerange_widget_t * r,
                                       int width)
  {
  double scale_factor;
  bg_nle_time_range_t range;
  
  range.start = r->visible.start;
  if(r->width > 0)
    {
    scale_factor =
      (double)(r->visible.end-r->visible.start)/(double)(r->width);

    range.end = r->visible.start +
      (int)(scale_factor * (double)(width) + 0.5);

    if(r->set_visible)
      r->set_visible(&range, r->callback_data);
    }
  
  r->width  = width;
  
  }

int bg_nle_timerange_widget_handle_button_press(bg_nle_timerange_widget_t * r,
                                                GtkWidget * w,
                                                GdkEventButton * evt)
  {
  bg_nle_time_range_t selection;
  int ret = 0;

  r->mouse_x = evt->x;
  
  if(evt->state == GDK_SHIFT_MASK)
    {
    int64_t t;

    bg_nle_time_range_copy(&selection, &r->selection);
    
    t = bg_nle_pos_2_time(r, evt->x);
    
    if(selection.start < 0)
      {
      selection.start = t;
      selection.end = -1;
      ret = 1;
      }
    else if(selection.end < 0)
      {
      if(t > selection.start)
        {
        selection.end = t;
        ret = 1;
        }
      else if(t < selection.start)
        {
        selection.end = selection.start;
        selection.start = t;
        ret = 1;
        }
      else
        {
        selection.start = t;
        selection.end   = -1;
        ret = 1;
        }
      }
    else
      {
      selection.end = t;
      ret = 1;
      }
    }
  else if(!evt->state && evt->button == 2)
    {
    gdk_window_set_cursor(w->window, bg_nle_cursor_doublearrow);
    }
  else if(!evt->state && evt->button == 1)
    {
    bg_nle_time_range_copy(&selection, &r->selection);
    selection.start = bg_nle_pos_2_time(r, evt->x);
    selection.end = -1;
    ret = 1;
    }
  
  if(ret && r->set_selection)
    r->set_selection(&selection, r->callback_data);
  return ret;
  }

int bg_nle_timerange_widget_handle_motion(bg_nle_timerange_widget_t * r,
                                          GdkEventMotion * evt)
  {
  bg_nle_time_range_t range;
  
  if(evt->state == GDK_BUTTON2_MASK)
    {
    int64_t diff_time =
      bg_nle_pos_2_time(r, r->mouse_x) -
      bg_nle_pos_2_time(r, evt->x);

    fprintf(stderr, "Motion callback: %d %f %ld\n", r->mouse_x, evt->x, diff_time);
    
    if(r->visible.start + diff_time < 0)
      diff_time = -r->visible.start;
    
    if(!diff_time)
      return TRUE;
    
    bg_nle_time_range_copy(&range, &r->visible);
    range.start += diff_time;
    range.end   += diff_time;

    if(r->set_visible)
      r->set_visible(&range, r->callback_data);
    
    r->mouse_x = evt->x;
    return 1;
    }
  
  return 0;
  }

void bg_nle_timerange_widget_zoom_in(bg_nle_timerange_widget_t * r)
  {
  int64_t center, diff;
  bg_nle_time_range_t range;
  
  diff = r->visible.end - r->visible.start;
  center = (r->visible.end + r->visible.start)/2;

  if(diff < GAVL_TIME_SCALE / 5)
    diff = GAVL_TIME_SCALE / 5;
  
  range.start = center - diff / 4;
  range.end   = center + diff / 4;

  if(r->set_zoom)
    r->set_zoom(&range, r->callback_data);
  }

void bg_nle_timerange_widget_zoom_out(bg_nle_timerange_widget_t * r)
  {
  int64_t center, diff;
  bg_nle_time_range_t range;
  
  diff = r->visible.end - r->visible.start;
  center = (r->visible.end + r->visible.start)/2;

  range.start = center - diff;
  if(range.start < 0)
    range.start = 0;
  range.end = center + diff;

  if(r->set_zoom)
    r->set_zoom(&range, r->callback_data);
  }

void bg_nle_timerange_widget_zoom_fit(bg_nle_timerange_widget_t * r)
  {
  int64_t center, diff;
  bg_nle_time_range_t range;
  diff = ((r->selection.end - r->selection.start) / 9) * 10;
  
  if(diff < GAVL_TIME_SCALE / 10)
    diff = GAVL_TIME_SCALE / 10;
  
  center = (r->selection.end + r->selection.start) / 2;
  range.start = center - diff / 2;
  if(range.start < 0)
    range.start = 0;
  range.end = center + diff / 2;
  
  if(r->set_zoom)
    r->set_zoom(&range, r->callback_data);
  }
