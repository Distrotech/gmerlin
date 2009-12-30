#include <math.h>
#include <string.h>

#include <gtk/gtk.h>
#include <inttypes.h>
#include <gavl/gavl.h>

#include <types.h>

#include <gui_gtk/timerange.h>
#include <gui_gtk/utils.h>
#include <gui_gtk/projectwindow.h>

#define SELECTION_MODE_LEFT  1
#define SELECTION_MODE_RIGHT 2

int64_t bg_nle_pos_2_time(bg_nle_timerange_widget_t * w, double pos)
  {
  int64_t ret;
  double ret_d = (double)pos /
    (double)w->width * (double)(w->visible.end - w->visible.start);
  ret = (int64_t)(ret_d + 0.5) + w->visible.start;

  if(bg_nle_snap_to_frames && w->ti->tab)
    {
    int64_t ret_scaled;
    int64_t frame_time_1;
    int64_t frame_time_2;
    int64_t frame;
    
    ret_scaled = gavl_time_scale(w->ti->scale, ret + 10);

    if(ret_scaled <= w->ti->tab->offset)
      {
      ret_scaled = w->ti->tab->offset;
      goto end;
      }

    frame_time_2 = gavl_frame_table_frame_to_time(w->ti->tab,
                                                  gavl_frame_table_num_frames(w->ti->tab)-1,
                                                  NULL);
    
    if(ret_scaled >= frame_time_2)
      {
      ret_scaled = frame_time_2;
      goto end;
      }
    
    frame = gavl_frame_table_time_to_frame(w->ti->tab, ret_scaled, &frame_time_1);
    frame_time_2 = gavl_frame_table_frame_to_time(w->ti->tab, frame + 1, NULL);

    /* Take the frame we are closer to */
    if(ret_scaled - frame_time_1 < frame_time_2 - ret_scaled)
      ret_scaled = frame_time_1;
    else
      ret_scaled = frame_time_2;

    end:
    ret = gavl_time_unscale(w->ti->scale, ret_scaled);
    }
  
  return ret;
  }

double bg_nle_time_2_pos(bg_nle_timerange_widget_t * w, int64_t time)
  {
  double ret_d = (double)(time - w->visible.start) /
    (double)(w->visible.end - w->visible.start) * (double)w->width;
  return ret_d;
  }

int bg_nle_time_is_near(bg_nle_timerange_widget_t * r, int64_t time, double pos)
  {
  double diff = fabs(bg_nle_time_2_pos(r, time) - pos);
  return diff < 3.0 ? 1 : 0;
  }

void bg_nle_timerange_widget_set_width(bg_nle_timerange_widget_t * r,
                                       int width)
  {
  double scale_factor;
  bg_nle_time_range_t range;
  
  if(r->width == width)
    return;
  
  range.start = r->visible.start;
  if(r->width > 0)
    {
    scale_factor =
      (double)(r->visible.end-r->visible.start)/(double)(r->width);

    range.end = r->visible.start +
      (int64_t)(scale_factor * (double)(width) + 0.5);

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
  int64_t t;
  int ret = 0;

  if(r->snap_time)
    {
    t = *r->snap_time;
    r->snap_time = NULL;
    }
  else
    {
    t = bg_nle_pos_2_time(r, evt->x);
    }
  
  r->mouse_x = evt->x;
  
  //  if(r->set_cursor_pos)
  //    r->set_cursor_pos(&selection, r->callback_data);
  
  if(evt->state == GDK_SHIFT_MASK)
    {
    bg_nle_time_range_copy(&selection, &r->selection);
    
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
    selection.start = t;
    selection.end = -1;
    ret = 1;
    r->selection_mode = 0;
    }
  
  if(ret && r->set_selection)
    r->set_selection(&selection, t, r->callback_data);
  return ret;
  }

int bg_nle_timerange_widget_handle_motion(bg_nle_timerange_widget_t * r,
                                          GtkWidget * w,
                                          GdkEventMotion * evt)
  {
  bg_nle_time_range_t range;

  int64_t time = bg_nle_pos_2_time(r, evt->x);

  if(r->motion_callback)
    r->motion_callback(time, r->callback_data);
  
  if(evt->state == GDK_BUTTON2_MASK)
    {
    int64_t diff_time;

    if(!r->visible.start &&
       (r->visible.end > r->media_time.end))
      return TRUE;
    
    diff_time =
      bg_nle_pos_2_time(r, r->mouse_x) - time;
    
    //fprintf(stderr, "Motion callback: %d %f %ld\n", r->mouse_x, evt->x, diff_time);
    
    if(r->visible.start + diff_time < 0)
      diff_time = -r->visible.start;

    if(r->visible.end + diff_time > r->media_time.end + GAVL_TIME_SCALE)
      diff_time = r->media_time.end + GAVL_TIME_SCALE - r->visible.end;
    
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
  else if(evt->state == GDK_BUTTON1_MASK)
    {
    /* Prevent negative values */
    if(time < 0)
      time = 0;
    
    if(r->selection.end < 0)
      {
      if(evt->x > r->mouse_x)
        {
        r->selection_mode = SELECTION_MODE_RIGHT;
        gdk_window_set_cursor(w->window, bg_nle_cursor_right_side);
        bg_nle_time_range_copy(&range, &r->selection);
        range.end = time;
        if(r->set_selection)
          r->set_selection(&range, time, r->callback_data);
        }
      else if(evt->x < r->mouse_x)
        {
        r->selection_mode = SELECTION_MODE_LEFT;
        gdk_window_set_cursor(w->window, bg_nle_cursor_left_side);
        bg_nle_time_range_copy(&range, &r->selection);
        range.end = range.start;
        range.start = time;
        if(r->set_selection)
          r->set_selection(&range, time, r->callback_data);
        }
      }
    else if(r->selection_mode == SELECTION_MODE_RIGHT)
      {
      if(time < r->selection.start)
        {
        r->selection_mode = SELECTION_MODE_LEFT;
        gdk_window_set_cursor(w->window, bg_nle_cursor_left_side);
        bg_nle_time_range_copy(&range, &r->selection);
        range.end = range.start;
        range.start = time;
        if(r->set_selection)
          r->set_selection(&range, time, r->callback_data);
        }
      else
        {
        bg_nle_time_range_copy(&range, &r->selection);
        range.end = time;
        if(r->set_selection)
          r->set_selection(&range, time, r->callback_data);
        }
      }
    else if(r->selection_mode == SELECTION_MODE_LEFT)
      {
      if(time > r->selection.end)
        {
        r->selection_mode = SELECTION_MODE_RIGHT;
        gdk_window_set_cursor(w->window, bg_nle_cursor_right_side);
        bg_nle_time_range_copy(&range, &r->selection);
        range.start = range.end;
        range.end = time;
        if(r->set_selection)
          r->set_selection(&range, time, r->callback_data);
        }
      else
        {
        bg_nle_time_range_copy(&range, &r->selection);
        range.start = time;
        if(r->set_selection)
          r->set_selection(&range, time, r->callback_data);
        }
      }
    
    r->mouse_x = evt->x;
    return 1;
    }
  /* No buttons pressed */
  else if(!(evt->state & (GDK_BUTTON1_MASK|
                          GDK_BUTTON2_MASK|
                          GDK_BUTTON3_MASK|
                          GDK_BUTTON4_MASK|
                          GDK_BUTTON5_MASK)))
    {
    if((r->in_out.start >= 0) && bg_nle_time_is_near(r, r->in_out.start, evt->x))
      {
      r->snap_time = &r->in_out.start;
      gdk_window_set_cursor(w->window, bg_nle_cursor_left_side);
      }
    else if((r->in_out.end >= 0) && bg_nle_time_is_near(r, r->in_out.end, evt->x))
      {
      r->snap_time = &r->in_out.end;
      gdk_window_set_cursor(w->window, bg_nle_cursor_right_side);
      }
    else
      {
      r->snap_time = NULL;
      gdk_window_set_cursor(w->window, bg_nle_cursor_xterm);
      }
    }
  
  
  return 0;
  }

void bg_nle_timerange_widget_handle_scroll(bg_nle_timerange_widget_t * r,
                                           int64_t diff_time)
  {
  bg_nle_time_range_t range;
  if(r->visible.start + diff_time < 0)
    diff_time = -r->visible.start;
  if(!diff_time)
    return;
  
  bg_nle_time_range_copy(&range, &r->visible);
  range.start += diff_time;
  range.end   += diff_time;
  if(r->set_visible)
    r->set_visible(&range, r->callback_data);
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

void bg_nle_timerange_widget_set_cursor_pos(bg_nle_timerange_widget_t * r, int64_t cursor_pos)
  {
  bg_nle_time_range_t range;
  gavl_time_t min_border_diff = (r->visible.end - r->visible.start)/10;
  gavl_time_t diff;
  
  if(cursor_pos > r->visible.end - min_border_diff)
    {
    diff = cursor_pos - (r->visible.end - min_border_diff);

    bg_nle_time_range_copy(&range, &r->visible);
    range.start += diff;
    range.end   += diff;

    if(r->set_visible)
      r->set_visible(&range, r->callback_data);
    }
  if(cursor_pos < r->visible.start + min_border_diff)
    {
    diff = r->visible.start + min_border_diff - cursor_pos;
    if(r->visible.start - diff < 0)
      diff = r->visible.start;

    if(diff)
      {
      bg_nle_time_range_copy(&range, &r->visible);
      
      range.start -= diff;
      range.end   -= diff;
      
      if(r->set_visible)
        r->set_visible(&range, r->callback_data);
      }
    }
  
  if(r->set_cursor_pos)
    r->set_cursor_pos(cursor_pos, r->callback_data);
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

/* */

void bg_nle_timerange_widget_toggle_in(bg_nle_timerange_widget_t * r)
  {
  bg_nle_time_range_t range;
  bg_nle_time_range_copy(&range, &r->in_out);

  if(range.start == r->cursor_pos)
    range.start = -1;
  else
    {
    range.start = r->cursor_pos;
  
    /* Make some sanity checks */
    if(range.end <= range.start)
      range.end = -1;
    }
  
  
  if(r->set_in_out)
    r->set_in_out(&range, r->callback_data);
  }

void bg_nle_timerange_widget_toggle_out(bg_nle_timerange_widget_t * r)
  {
  bg_nle_time_range_t range;
  bg_nle_time_range_copy(&range, &r->in_out);

  if(range.end == r->cursor_pos)
    range.end = -1;
  else
    {
    range.end = r->cursor_pos;
    /* Make some sanity checks */
    if(range.end <= range.start)
      range.start = -1;
    }
  
  if(r->set_in_out)
    r->set_in_out(&range, r->callback_data);
  
  }

void bg_nle_timerange_widget_add_label(bg_nle_timerange_widget_t * r, int64_t label)
  {
  if(r->labels_alloc < r->num_labels+1)
    {
    r->labels_alloc += 16;
    r->labels = realloc(r->labels, sizeof(*r->labels) * r->labels_alloc);
    }
  
  if(!r->num_labels)
    r->labels[0] = label;
  else
    {
    int i;
    for(i = 0; i < r->num_labels; i++)
      {
      if(label < r->labels[i])
        break;
      }
    
    if(i < r->num_labels)
      memmove(r->labels + i + 1, r->labels + i, sizeof(*r->labels) * (r->num_labels - i));
    
    r->labels[i] = label;
    }
  r->num_labels++;
  }

void bg_nle_timerange_widget_delete_label(bg_nle_timerange_widget_t * r, int64_t label)
  {
  int i;
  for(i = 0; i < r->num_labels; i++)
    {
    if(label == r->labels[i])
      break;
    }
  if(i == r->num_labels)
    {
    fprintf(stderr, "Couldn't delete label %ld\n", label);
    return;
    }
  if(i < r->num_labels-1)
    {
    memmove(r->labels + i, r->labels + i + 1, sizeof(*r->labels) * (r->num_labels - i - 1));
    }
  r->num_labels--;
  }

gavl_time_t bg_nle_timerange_widget_next_label(bg_nle_timerange_widget_t * r)
  {
  int i;
  if(!r->num_labels)
    return GAVL_TIME_UNDEFINED;

  for(i = 0; i < r->num_labels; i++)
    {
    if(r->cursor_pos < r->labels[i])
      {
      return r->labels[i];
      break;
      }
    }
  return GAVL_TIME_UNDEFINED;
  }

gavl_time_t bg_nle_timerange_widget_previous_label(bg_nle_timerange_widget_t * r)
  {
  int i;
  if(!r->num_labels)
    return GAVL_TIME_UNDEFINED;

  for(i = 0; i < r->num_labels; i++)
    {
    if(r->cursor_pos <= r->labels[i])
      {
      return r->labels[i-1];
      break;
      }
    }
  return GAVL_TIME_UNDEFINED;
  }
