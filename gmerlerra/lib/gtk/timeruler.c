#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>

#include <gui_gtk/timeruler.h>

// 000:00:00.000
#define TIME_STRING_LEN 14

struct bg_nle_time_ruler_s
  {
  GtkWidget * wid;
  int width, height;
  GdkCursor * cursor;

  gavl_time_t start_visible;
  gavl_time_t end_visible;
  gavl_time_t spacing_major;
  gavl_time_t spacing_minor;

  gavl_time_t start_selection;
  gavl_time_t end_selection;
  
  PangoLayout * pl;

  void (*selection_changed_callback)(void*);
  void * selection_changed_data;
  
  int mouse_x;
  
  };

static void print_time(gavl_time_t t, char * str)
  {
  int hours, minutes, seconds, milliseconds;

  milliseconds = (t/1000) % 1000;
  t /= GAVL_TIME_SCALE;
  seconds = t % 60;
  t /= 60;

  minutes = t % 60;
  t /= 60;

  hours = t % 60;
  t /= 60;

  if(hours)
    sprintf(str, "%d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
  else
    sprintf(str, "%02d:%02d.%03d", minutes, seconds, milliseconds);
  }

static void size_allocate_callback(GtkWidget     *widget,
                                   GtkAllocation *allocation,
                                   gpointer       user_data)
  {
  double scale_factor;
  bg_nle_time_ruler_t * r = user_data;

  if(r->width >= 0)
    {
    scale_factor =
      (double)(r->end_visible-r->start_visible)/(double)(r->width);
    r->end_visible = r->start_visible +
      (int)(scale_factor * (double)(allocation->width) + 0.5);
    }
  
  r->width  = allocation->width;
  r->height = allocation->height;
  }

int64_t bg_nle_time_ruler_pos_2_time(bg_nle_time_ruler_t * r, int pos)
  {
  double ret_d = (double)pos /
    (double)r->width * (double)(r->end_visible - r->start_visible);
  return (int64_t)(ret_d + 0.5) + r->start_visible;
  }

int bg_nle_time_ruler_time_2_pos(bg_nle_time_ruler_t * r, int64_t time)
  {
  double ret_d = (double)(time - r->start_visible) /
    (double)(r->end_visible - r->start_visible) * (double)r->width;
  return (int)(ret_d + 0.5);
  }

static void calc_spacing(bg_nle_time_ruler_t * r)
  {
  double dt;
  int64_t tmp1;
  int log_dt;
  
  /* Minor spacing should be at least 10 pixels,
     Spacings can be [1|2|5]*10^n */

  dt = 10.0 * (double)(r->end_visible-r->start_visible) / r->width;

  log_dt = (int)(log10(dt));
  
  tmp1 = 1;
  while(log_dt--)
    tmp1 *= 10;

  if(dt < (double)(tmp1 * 2.0))
    {
    r->spacing_minor =
      ((double)tmp1 * 2.0 - dt) > (dt - (double)tmp1) ?
      tmp1 : 2 * tmp1;
    }
  else if(dt < (double)(tmp1 * 5.0))
    {
    r->spacing_minor =
      ((double)tmp1 * 5.0 - dt) > (dt - (double)tmp1 * 2.0) ?
      2 * tmp1 : 5 * tmp1;
    }
  else
    {
    r->spacing_minor =
      ((double)tmp1 * 10.0 - dt) > (dt - (double)tmp1 * 5.0) ?
      5 * tmp1 : 10 * tmp1;
    }
  r->spacing_major = 10 * r->spacing_minor;
  
  fprintf(stderr, "dt: %f, tmp1: %ld\n",
          dt, tmp1);
  
  }

static void redraw(bg_nle_time_ruler_t * r)
  {
  int64_t time;
  int pos;
  char time_string[TIME_STRING_LEN];
  
  gtk_paint_box(gtk_widget_get_style(r->wid),
                r->wid->window,
                GTK_STATE_NORMAL,
                GTK_SHADOW_OUT,
                (const GdkRectangle *)0,
                r->wid,
                (const gchar *)0,
                0,
                0,
                r->width,
                r->height);

  /* Draw tics */
  
  time = ((r->start_visible / r->spacing_minor)) * r->spacing_minor;
  
  while(time < r->end_visible)
    {
    pos = bg_nle_time_ruler_time_2_pos(r, time);

    if(pos < 0)
      {
      time += r->spacing_minor;
      continue;
      }

    if(time % r->spacing_major)
      {
      gtk_paint_vline(gtk_widget_get_style(r->wid),
                      r->wid->window,
                      GTK_STATE_NORMAL,
                      (const GdkRectangle *)0,
                      r->wid,
                      (const gchar *)0,
                      16, 32, pos);
      }
    else
      {
      gtk_paint_vline(gtk_widget_get_style(r->wid),
                      r->wid->window,
                      GTK_STATE_NORMAL,
                      (const GdkRectangle *)0,
                      r->wid,
                      (const gchar *)0,
                      0, 32, pos);
      print_time(time, time_string);
      pango_layout_set_text(r->pl, time_string, -1);
      
      gtk_paint_layout(gtk_widget_get_style(r->wid),
                       r->wid->window,
                       GTK_STATE_NORMAL,
                       FALSE, // gboolean use_text,
                       (const GdkRectangle *)0,
                       r->wid,
                       (const gchar *)0,
                       pos+ 2,
                       0,
                       r->pl);
      }
    time += r->spacing_minor;
    }

  }


static gboolean expose_callback(GtkWidget *widget,
                                GdkEventExpose * evt,
                                gpointer       user_data)
  {
  redraw(user_data);
  return TRUE;
  }

static gboolean button_press_callback(GtkWidget *widget,
                                      GdkEventButton * evt,
                                      gpointer user_data)
  {
  bg_nle_time_ruler_t * r = user_data;
  
  r->mouse_x = evt->x;
  }

static gboolean motion_callback(GtkWidget *widget,
                                GdkEventButton * evt,
                                gpointer user_data)
  {
  bg_nle_time_ruler_t * r = user_data;
  
  gavl_time_t diff_time =
    bg_nle_time_ruler_pos_2_time(r, r->mouse_x) -
    bg_nle_time_ruler_pos_2_time(r, evt->x);

  if(r->start_visible + diff_time < 0)
    diff_time = -r->start_visible;

  if(!diff_time)
    return;
  
  r->start_visible += diff_time;
  r->end_visible += diff_time;
  r->mouse_x = evt->x;
  redraw(r);
  
  if(r->selection_changed_callback)
    r->selection_changed_callback(r->selection_changed_data);
  }

static void realize_callback(GtkWidget *widget,
                             gpointer       user_data)
  {
  bg_nle_time_ruler_t * r = user_data;
  gdk_window_set_cursor(r->wid->window, r->cursor);
  }

bg_nle_time_ruler_t * bg_nle_time_ruler_create()
  {
  bg_nle_time_ruler_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  ret->start_visible = 0;
  ret->end_visible = 10 * GAVL_TIME_SCALE;
  ret->spacing_major = GAVL_TIME_SCALE;
  ret->spacing_minor = GAVL_TIME_SCALE / 10;

  ret->width = -1;
  ret->height = -1;
  
  ret->wid = gtk_drawing_area_new();

  ret->pl = pango_layout_new(gtk_widget_get_pango_context(ret->wid));
  
  ret->cursor = gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW);
  
  gtk_widget_set_size_request(ret->wid, 100, 32);

  gtk_widget_set_events(ret->wid,
                        GDK_EXPOSURE_MASK |
                        GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
  
  g_signal_connect(ret->wid, "size-allocate", G_CALLBACK(size_allocate_callback),
                   ret);
  g_signal_connect(ret->wid, "expose-event", G_CALLBACK(expose_callback),
                   ret);

  g_signal_connect(ret->wid, "button-press-event",
                   G_CALLBACK(button_press_callback),
                   ret);

  g_signal_connect(ret->wid, "motion-notify-event",
                   G_CALLBACK(motion_callback),
                   ret);

  g_signal_connect(ret->wid, "realize", G_CALLBACK(realize_callback),
                   ret);
  
  gtk_widget_show(ret->wid);
  
  return ret;
  }

void bg_nle_time_ruler_destroy(bg_nle_time_ruler_t * t)
  {
  g_object_unref(t->pl);
  }

GtkWidget * bg_nle_time_ruler_get_widget(bg_nle_time_ruler_t * t)
  {
  return t->wid;
  }

void bg_nle_time_ruler_get_visible(bg_nle_time_ruler_t * t,
                                   gavl_time_t * start, gavl_time_t * end)
  {
  *start = t->start_visible;
  *end = t->end_visible;
  }

void bg_nle_time_ruler_set_visible(bg_nle_time_ruler_t * t,
                                   gavl_time_t start, gavl_time_t end)
  {
  
  }

void bg_nle_time_ruler_get_selection(bg_nle_time_ruler_t * t,
                                     gavl_time_t * start, gavl_time_t * end)
  {
  *start = t->start_selection;
  *end = t->end_selection;
  }

void bg_nle_time_ruler_set_selection(bg_nle_time_ruler_t * t,
                                     gavl_time_t start, gavl_time_t end)
  {
  t->start_selection = start;
  t->end_selection = end;
  if(t->selection_changed_callback)
    t->selection_changed_callback(t->selection_changed_data);
  }

void bg_nle_time_ruler_set_selection_callback(bg_nle_time_ruler_t * t,
                                              void (*selection_changed_callback)(void*),
                                              void* selection_changed_data)
  {
  t->selection_changed_callback = selection_changed_callback;
  t->selection_changed_data     = selection_changed_data;
  }

void bg_nle_time_ruler_handle_button_press(bg_nle_time_ruler_t * r,
                                           GdkEventButton * evt)
  {
  if(evt->state == GDK_SHIFT_MASK)
    {
    gavl_time_t selection_start, selection_end, t;
    
    bg_nle_time_ruler_get_selection(r,
                                    &selection_start,
                                    &selection_end);
    t = bg_nle_time_ruler_pos_2_time(r, evt->x);
    
    if(selection_start < 0)
      {
      bg_nle_time_ruler_set_selection(r, t, -1);
      }
    else if(selection_end < 0)
      {
      if(t > selection_start)
        bg_nle_time_ruler_set_selection(r,
                                        selection_start, t);
      else if(t < selection_start)
        bg_nle_time_ruler_set_selection(r,
                                        t, selection_start);
      else
        bg_nle_time_ruler_set_selection(r,
                                        t, -1);
      }
    else
      {
      bg_nle_time_ruler_set_selection(r,
                                      selection_start, t);
      }
    }
  else if(!evt->state)
    {
    bg_nle_time_ruler_set_selection(r,
                                    bg_nle_time_ruler_pos_2_time(r,
                                                                 evt->x),
                                    -1);
    }

  }

void bg_nle_time_ruler_zoom_in(bg_nle_time_ruler_t * r)
  {
  gavl_time_t center, diff;
  
  diff = r->end_visible - r->start_visible;
  center = (r->end_visible + r->start_visible)/2;
  
  r->start_visible = center - diff / 4;
  r->end_visible   = center + diff / 4;

  if(r->selection_changed_callback)
    r->selection_changed_callback(r->selection_changed_data);
  calc_spacing(r);
  redraw(r);
  
  }

void bg_nle_time_ruler_zoom_out(bg_nle_time_ruler_t * r)
  {
  gavl_time_t center, diff;
  
  diff = r->end_visible - r->start_visible;
  center = (r->end_visible + r->start_visible)/2;
  
  r->start_visible = center - diff * 4;
  if(r->start_visible < 0)
    r->start_visible = 0;
  r->end_visible   = center + diff * 4;
  
  if(r->selection_changed_callback)
    r->selection_changed_callback(r->selection_changed_data);
  calc_spacing(r);
  redraw(r);
  }

void bg_nle_time_ruler_zoom_fit(bg_nle_time_ruler_t * r)
  {
  
  }
