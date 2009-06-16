
#include <stdlib.h>
#include <math.h>
#include <config.h>

#include <gtk/gtk.h>
#include <pango/pangocairo.h>


#include <gmerlin/utils.h>
#include <gmerlin/gui_gtk/gtkutils.h>
#include <gmerlin/cfg_registry.h>

#include <gui_gtk/timeruler.h>

// 000:00:00.000
// #define TIME_STRING_LEN 14

#define FONT "Sans 10"

struct bg_nle_time_ruler_s
  {
  GtkWidget * wid;
  int width, height;
  GdkCursor * cursor;
  
  gavl_time_t spacing_major;
  gavl_time_t spacing_minor;
  
  void (*selection_changed_callback)(void*);
  void * selection_changed_data;
  
  int mouse_x;
  
  bg_nle_project_t * p;
  
  PangoFontDescription *font_desc;
  };

static void calc_spacing(bg_nle_time_ruler_t * r)
  {
  double dt;
  int64_t tmp1;
  int log_dt;
  
  /* Minor spacing should be at least 10 pixels,
     Spacings can be [1|2|5]*10^n */

  dt = 10.0 * (double)(r->p->end_visible-r->p->start_visible) / r->width;

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
  }

static void size_allocate_callback(GtkWidget     *widget,
                                   GtkAllocation *allocation,
                                   gpointer       user_data)
  {
  double scale_factor;
  int init;
  bg_nle_time_ruler_t * r = user_data;

  if(r->width >= 0)
    {
    scale_factor =
      (double)(r->p->end_visible-r->p->start_visible)/(double)(r->width);
    r->p->end_visible = r->p->start_visible +
      (int)(scale_factor * (double)(allocation->width) + 0.5);
    init = 0;
    }
  else
    {
    init = 1;
    }
  
  r->width  = allocation->width;
  r->height = allocation->height;

  if(init)
    calc_spacing(r);
  
  }

int64_t bg_nle_time_ruler_pos_2_time(bg_nle_time_ruler_t * r, int pos)
  {
  double ret_d = (double)pos /
    (double)r->width * (double)(r->p->end_visible - r->p->start_visible);
  return (int64_t)(ret_d + 0.5) + r->p->start_visible;
  }

double bg_nle_time_ruler_time_2_pos(bg_nle_time_ruler_t * r, int64_t time)
  {
  double ret_d = (double)(time - r->p->start_visible) /
    (double)(r->p->end_visible - r->p->start_visible) * (double)r->width;
  return ret_d;
  }


static void redraw(bg_nle_time_ruler_t * r)
  {
  int64_t time;
  double pos;
  char time_string[GAVL_TIME_STRING_LEN_MS];
  PangoLayout * pl;
  cairo_t * c = gdk_cairo_create(r->wid->window);

  pl = pango_cairo_create_layout(c);
  
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
  
  time = ((r->p->start_visible / r->spacing_major)) * r->spacing_major;
  
  cairo_set_line_width(c, 1.0);
  pango_layout_set_font_description(pl, r->font_desc);
    
  while(time < r->p->end_visible)
    {
    pos = bg_nle_time_ruler_time_2_pos(r, time);
    
    if(time % r->spacing_major)
      {
      cairo_move_to(c, pos, 16.0);
      cairo_line_to(c, pos, 32.0);
      cairo_stroke(c);
      }
    else
      {
      cairo_move_to(c, pos, 0.0);
      cairo_line_to(c, pos, 32.0);
      cairo_stroke(c);

      gavl_time_prettyprint_ms(time, time_string);
      pango_layout_set_text(pl, time_string, -1);
      cairo_move_to(c, pos+2.0, 0.0);
      pango_cairo_show_layout(c, pl);
      
#if 0

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
#endif
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
  return TRUE;
  }

static gboolean motion_callback(GtkWidget *widget,
                                GdkEventButton * evt,
                                gpointer user_data)
  {
  bg_nle_time_ruler_t * r = user_data;
  
  gavl_time_t diff_time =
    bg_nle_time_ruler_pos_2_time(r, r->mouse_x) -
    bg_nle_time_ruler_pos_2_time(r, evt->x);

  if(r->p->start_visible + diff_time < 0)
    diff_time = -r->p->start_visible;

  if(!diff_time)
    return TRUE;
  
  r->p->start_visible += diff_time;
  r->p->end_visible += diff_time;
  r->mouse_x = evt->x;
  redraw(r);
  
  if(r->selection_changed_callback)
    r->selection_changed_callback(r->selection_changed_data);
  return TRUE;
  }

static void realize_callback(GtkWidget *widget,
                             gpointer       user_data)
  {
  bg_nle_time_ruler_t * r = user_data;
  gdk_window_set_cursor(r->wid->window, r->cursor);
  }

bg_nle_time_ruler_t * bg_nle_time_ruler_create(bg_nle_project_t * p)
  {
  bg_nle_time_ruler_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->p = p;
  
  //  ret->p->start_visible = 0;
  //  ret->p->end_visible = 10 * GAVL_TIME_SCALE;
  
  //  ret->spacing_major = GAVL_TIME_SCALE;
  //  ret->spacing_minor = GAVL_TIME_SCALE / 10;

  ret->width = -1;
  ret->height = -1;
  
  ret->wid = gtk_drawing_area_new();

  ret->font_desc = pango_font_description_from_string(FONT);
  
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
  pango_font_description_free(t->font_desc);
  }

GtkWidget * bg_nle_time_ruler_get_widget(bg_nle_time_ruler_t * t)
  {
  return t->wid;
  }

#if 0
void bg_nle_time_ruler_get_visible(bg_nle_time_ruler_t * t,
                                   gavl_time_t * start, gavl_time_t * end)
  {
  *start = t->p->start_visible;
  *end = t->p->end_visible;
  }

void bg_nle_time_ruler_set_visible(bg_nle_time_ruler_t * t,
                                   gavl_time_t start, gavl_time_t end)
  {
  
  }
#endif

void bg_nle_time_ruler_get_selection(bg_nle_time_ruler_t * t,
                                     gavl_time_t * start, gavl_time_t * end)
  {
  *start = t->p->start_selection;
  *end = t->p->end_selection;
  }

void bg_nle_time_ruler_set_selection(bg_nle_time_ruler_t * t,
                                     gavl_time_t start, gavl_time_t end)
  {
  t->p->start_selection = start;
  t->p->end_selection = end;
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
  
  diff = r->p->end_visible - r->p->start_visible;
  center = (r->p->end_visible + r->p->start_visible)/2;
  
  r->p->start_visible = center - diff / 4;
  r->p->end_visible   = center + diff / 4;

  if(r->selection_changed_callback)
    r->selection_changed_callback(r->selection_changed_data);
  calc_spacing(r);
  redraw(r);
  
  }

void bg_nle_time_ruler_zoom_out(bg_nle_time_ruler_t * r)
  {
  gavl_time_t center, diff;
  
  diff = r->p->end_visible - r->p->start_visible;
  center = (r->p->end_visible + r->p->start_visible)/2;
  
  r->p->start_visible = center - diff * 4;
  if(r->p->start_visible < 0)
    r->p->start_visible = 0;
  r->p->end_visible   = center + diff * 4;
  
  if(r->selection_changed_callback)
    r->selection_changed_callback(r->selection_changed_data);
  calc_spacing(r);
  redraw(r);
  }

void bg_nle_time_ruler_zoom_fit(bg_nle_time_ruler_t * r)
  {
  
  }
