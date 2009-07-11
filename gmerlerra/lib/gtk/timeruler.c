
#include <stdlib.h>
#include <math.h>
#include <config.h>

#include <gtk/gtk.h>
#include <pango/pangocairo.h>


#include <gmerlin/utils.h>
#include <gmerlin/gui_gtk/gtkutils.h>
#include <gmerlin/cfg_registry.h>

#include <types.h>
#include <gui_gtk/timerange.h>
#include <gui_gtk/timeruler.h>
#include <gui_gtk/utils.h>

// 000:00:00.000
// #define TIME_STRING_LEN 14

#define FONT "Sans 10"

#define RULER_HEIGHT 32

struct bg_nle_time_ruler_s
  {
  GtkWidget * wid;
  
  gavl_time_t spacing_major;
  gavl_time_t spacing_minor;
  
  double spacing_minor_f;
  
  bg_nle_timerange_widget_t * tr;
  
  PangoFontDescription *font_desc;
  };

static void calc_spacing(bg_nle_time_ruler_t * r)
  {
  double dt;
  int64_t tmp1;
  int log_dt;

  fprintf(stderr, "Calc spacing\n");
  
  /* Minor spacing should be at least 15 pixels,
     Spacings can be [1|2|5]*10^n */

  dt = 15.0 * (double)(r->tr->visible.end-r->tr->visible.start) / r->tr->width;

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

  r->spacing_minor_f =
    bg_nle_time_2_pos(r->tr, r->tr->visible.start + r->spacing_minor) -
    bg_nle_time_2_pos(r->tr, r->tr->visible.start);
  
  }

static void size_allocate_callback(GtkWidget     *widget,
                                   GtkAllocation *allocation,
                                   gpointer       user_data)
  {

  }

static void redraw(bg_nle_time_ruler_t * r)
  {
  int64_t time;
  double pos;
  char time_string[GAVL_TIME_STRING_LEN_MS];
  float selection_start_pos;
  float selection_end_pos;
  
  PangoLayout * pl;
  cairo_t * c = gdk_cairo_create(r->wid->window);
  
  if(!r->spacing_major || !r->spacing_minor)
    calc_spacing(r);
  
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
                r->tr->width,
                RULER_HEIGHT);

  /* Draw tics */
  
  time = ((r->tr->visible.start / r->spacing_major)) * r->spacing_major;
  pos = bg_nle_time_2_pos(r->tr, time);
  
  cairo_set_line_width(c, 1.0);
  pango_layout_set_font_description(pl, r->font_desc);
    
  while(time < r->tr->visible.end)
    {
    
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
      }

    pos += r->spacing_minor_f;
    
    time += r->spacing_minor;
    }

  /* Draw selection */

  if(bg_nle_time_range_intersect(&r->tr->selection,
                                 &r->tr->visible))
    {
    selection_start_pos = bg_nle_time_2_pos(r->tr,
                                            r->tr->selection.start);
    selection_end_pos = bg_nle_time_2_pos(r->tr,
                                          r->tr->selection.end);
  
    if(r->tr->selection.start >= 0)
      {
      cairo_move_to(c, selection_start_pos, RULER_HEIGHT/2);
      cairo_line_to(c, selection_start_pos, RULER_HEIGHT);
      cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
      cairo_stroke(c);
    
      if(r->tr->selection.end >= 0)
        {
        GdkRectangle r;
        r.x = selection_start_pos;
        r.width = selection_end_pos - selection_start_pos;
        r.y = RULER_HEIGHT/2;
        r.height = RULER_HEIGHT/2;
        gdk_cairo_rectangle(c, &r);

        cairo_set_source_rgba(c, 1.0, 0.0, 0.0, 0.2);
        cairo_fill(c);

        cairo_move_to(c, selection_end_pos, RULER_HEIGHT/2);
        cairo_line_to(c, selection_end_pos, RULER_HEIGHT);
        cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
        cairo_stroke(c);
        }
    
      }

    }
  
  cairo_destroy(c);
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
  bg_nle_timerange_widget_handle_button_press(r->tr, widget, evt);
  return TRUE;
  }

static gboolean button_release_callback(GtkWidget *widget,
                                        GdkEventButton * evt,
                                        gpointer user_data)
  {
  gdk_window_set_cursor(widget->window, bg_nle_cursor_xterm);
  return TRUE;
  }


static gboolean motion_callback(GtkWidget *widget,
                                GdkEventMotion * evt,
                                gpointer user_data)
  {
  bg_nle_time_ruler_t * r = user_data;
  
  bg_nle_timerange_widget_handle_motion(r->tr, evt);
  return FALSE;
  }

static void realize_callback(GtkWidget *widget,
                             gpointer       user_data)
  {
  bg_nle_time_ruler_t * r = user_data;
  gdk_window_set_cursor(r->wid->window, bg_nle_cursor_xterm);
  }

bg_nle_time_ruler_t * bg_nle_time_ruler_create(bg_nle_timerange_widget_t * tr)
  {
  bg_nle_time_ruler_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->tr = tr;
  //  ret->visible.start = 0;
  //  ret->visible.end = 10 * GAVL_TIME_SCALE;
  
  //  ret->spacing_major = GAVL_TIME_SCALE;
  //  ret->spacing_minor = GAVL_TIME_SCALE / 10;
    
  ret->wid = gtk_drawing_area_new();

  ret->font_desc = pango_font_description_from_string(FONT);
  
  gtk_widget_set_size_request(ret->wid, 100, RULER_HEIGHT);

  gtk_widget_set_events(ret->wid,
                        GDK_EXPOSURE_MASK |
                        GDK_BUTTON1_MOTION_MASK | GDK_BUTTON2_MOTION_MASK |
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  
  g_signal_connect(ret->wid, "size-allocate", G_CALLBACK(size_allocate_callback),
                   ret);
  g_signal_connect(ret->wid, "expose-event", G_CALLBACK(expose_callback),
                   ret);

  g_signal_connect(ret->wid, "button-press-event",
                   G_CALLBACK(button_press_callback),
                   ret);
  g_signal_connect(ret->wid, "button-release-event",
                   G_CALLBACK(button_release_callback),
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

void bg_nle_time_ruler_update_visible(bg_nle_time_ruler_t * t)
  {
  if((t->tr->width > 0) && GTK_WIDGET_REALIZED(t->wid)) 
    {
    redraw(t);
    }
  }

void bg_nle_time_ruler_update_zoom(bg_nle_time_ruler_t * t)
  {
  if((t->tr->width > 0) && GTK_WIDGET_REALIZED(t->wid)) 
    {
    calc_spacing(t);
    redraw(t);
    }
  }


void bg_nle_time_ruler_update_selection(bg_nle_time_ruler_t * t)
  {
  if((t->tr->width > 0) && GTK_WIDGET_REALIZED(t->wid)) 
    {
    redraw(t);
    }
  }



