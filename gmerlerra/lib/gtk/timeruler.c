
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
#include <gmerlin/gui_gtk/display.h>

// 000:00:00.000
// #define TIME_STRING_LEN 14

#define FONT "Sans 10"

#define RULER_HEIGHT 32

struct bg_nle_time_ruler_s
  {
  GtkWidget * wid;
  
  gavl_time_t spacing_major;
  gavl_time_t spacing_minor;
  
  
  bg_nle_timerange_widget_t * tr;
  
  PangoFontDescription *font_desc;

  bg_nle_time_info_t * ti;
  
  int time_unit;
  };

static void calc_spacing_hmsms(bg_nle_time_ruler_t * r)
  {
  double dt;
  int64_t tmp1;
  int log_dt;

  //  fprintf(stderr, "Calc spacing\n");
  
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
  }

static double get_frame_spacing(bg_nle_time_ruler_t * r, int frames,
                                int64_t frame1, double pos1)
  {
  double pos2;
  int64_t frame2_time;
  frame2_time = gavl_frame_table_frame_to_time(r->ti->tab, frame1 + frames, NULL);
  pos2 = bg_nle_time_2_pos(r->tr, gavl_time_unscale(r->ti->scale, frame2_time));

  fprintf(stderr, "Frame spacing: %d %f\n", frames, pos2 - pos1);

  return pos2 - pos1;
  }

static void calc_spacing_framecount(bg_nle_time_ruler_t * r)
  {
  double pos1;
  int64_t frame1;
  int64_t frame1_time;
  /* spacing_major and spacing_minor are frame counts */

  r->spacing_minor = 1;

  frame1 = gavl_frame_table_time_to_frame(r->ti->tab,
                                          gavl_time_scale(r->ti->scale, r->tr->visible.start),
                                          &frame1_time);
  
  if(frame1 < 0)
    {
    frame1 = 0;
    frame1_time = gavl_frame_table_frame_to_time(r->ti->tab, 0, NULL);
    }
  pos1 = bg_nle_time_2_pos(r->tr, gavl_time_unscale(r->ti->scale, frame1_time));

  while(1)
    {
    if(get_frame_spacing(r, r->spacing_minor, frame1, pos1) >= 5.0)
      {
      break;
      }
    else if(get_frame_spacing(r, r->spacing_minor * 2, frame1, pos1) >= 5.0)
      {
      r->spacing_minor *= 2;
      break;
      }
    else if(get_frame_spacing(r, r->spacing_minor * 5, frame1, pos1) >= 5.0)
      {
      r->spacing_minor *= 5;
      break;
      }
    r->spacing_minor *= 10;
    }
  
  r->spacing_major = 20 * r->spacing_minor;
  }


static void calc_spacing_timecode(bg_nle_time_ruler_t * r)
  {
  
  
  }

static void calc_spacing(bg_nle_time_ruler_t * r)
  {
  switch(r->time_unit)
    {
    case BG_GTK_DISPLAY_MODE_HMSMS:
      calc_spacing_hmsms(r);
      break;
    case BG_GTK_DISPLAY_MODE_TIMECODE:
      calc_spacing_timecode(r);
      break;
    case BG_GTK_DISPLAY_MODE_FRAMECOUNT:
      calc_spacing_framecount(r);
      break;
    }
  }


static void size_allocate_callback(GtkWidget     *widget,
                                   GtkAllocation *allocation,
                                   gpointer       user_data)
  {

  }

static void draw_tics_hmsms(bg_nle_time_ruler_t * r, PangoLayout * pl, cairo_t * c)
  {
  double spacing_minor_f;
  int64_t time;
  double pos;
  char time_string[GAVL_TIME_STRING_LEN_MS];
  
  spacing_minor_f =
    bg_nle_time_2_pos(r->tr, r->tr->visible.start + r->spacing_minor) -
    bg_nle_time_2_pos(r->tr, r->tr->visible.start);
  
  time = ((r->tr->visible.start / r->spacing_major)) * r->spacing_major;
  pos = bg_nle_time_2_pos(r->tr, time);
  
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

    pos += spacing_minor_f;
    
    time += r->spacing_minor;
    }
  
  }

static void draw_tics_timecode(bg_nle_time_ruler_t * r, PangoLayout * pl, cairo_t * c)
  {
  
  }

static void draw_tics_framecount(bg_nle_time_ruler_t * r, PangoLayout * pl, cairo_t * c)
  {
  double pos;
  char str[128];
  int64_t frame, frame_time;
  int64_t total_frames;
  
  frame = gavl_frame_table_time_to_frame(r->ti->tab,
                                         gavl_time_scale(r->ti->scale, r->tr->visible.start),
                                         &frame_time);

  total_frames = gavl_frame_table_num_frames(r->ti->tab);
  
  if(frame < 0)
    {
    frame = 0;
    }
  
  frame /= r->spacing_major;
  frame *= r->spacing_major;
  
  while(1)
    {
    if(frame >= total_frames)
      break;

    frame_time = gavl_time_unscale(r->ti->scale,
                                   gavl_frame_table_frame_to_time(r->ti->tab, frame, NULL));
    
    if(frame_time > r->tr->visible.end)
      break;
    
    pos = bg_nle_time_2_pos(r->tr, frame_time);
    
    if(frame % r->spacing_major)
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

      sprintf(str, "%"PRId64, frame);
      pango_layout_set_text(pl, str, -1);
      cairo_move_to(c, pos+2.0, 0.0);
      pango_cairo_show_layout(c, pl);
      }
    frame += r->spacing_minor;
    }
  }

static void redraw(bg_nle_time_ruler_t * r)
  {
  double pos;
  float start_pos;
  float end_pos;
  
  PangoLayout * pl;
  cairo_t * c = gdk_cairo_create(r->wid->window);

  r->time_unit = r->ti->mode;
  
  switch(r->ti->mode)
    {
    case BG_GTK_DISPLAY_MODE_HMSMS:
      break;
    case BG_GTK_DISPLAY_MODE_TIMECODE:
      if(!r->ti->fmt.int_framerate ||
         !r->ti->tab)
        r->time_unit = BG_GTK_DISPLAY_MODE_HMSMS;
      break;
    case BG_GTK_DISPLAY_MODE_FRAMECOUNT:
      if(!r->ti->tab)
        r->time_unit = BG_GTK_DISPLAY_MODE_HMSMS;
      break;
    }
  
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

  cairo_set_line_width(c, 1.0);
  pango_layout_set_font_description(pl, r->font_desc);

  switch(r->time_unit)
    {
    case BG_GTK_DISPLAY_MODE_HMSMS:
      draw_tics_hmsms(r, pl, c);
      break;
    case BG_GTK_DISPLAY_MODE_TIMECODE:
      draw_tics_timecode(r, pl, c);
      break;
    case BG_GTK_DISPLAY_MODE_FRAMECOUNT:
      draw_tics_framecount(r, pl, c);
      break;
    }
  
  /* Draw selection */

  if(bg_nle_time_range_intersect(&r->tr->selection,
                                 &r->tr->visible))
    {
    start_pos = bg_nle_time_2_pos(r->tr,
                                            r->tr->selection.start);
    end_pos = bg_nle_time_2_pos(r->tr,
                                          r->tr->selection.end);
  
    if(r->tr->selection.start >= 0)
      {
      cairo_move_to(c, start_pos, RULER_HEIGHT/2);
      cairo_line_to(c, start_pos, RULER_HEIGHT);
      cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
      cairo_stroke(c);
    
      if(r->tr->selection.end >= 0)
        {
        GdkRectangle r;
        r.x = start_pos;
        r.width = end_pos - start_pos;
        r.y = RULER_HEIGHT/2;
        r.height = RULER_HEIGHT/2;
        gdk_cairo_rectangle(c, &r);

        cairo_set_source_rgba(c, 1.0, 0.0, 0.0, 0.2);
        cairo_fill(c);

        cairo_move_to(c, end_pos, RULER_HEIGHT/2);
        cairo_line_to(c, end_pos, RULER_HEIGHT);
        cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
        cairo_stroke(c);
        }
    
      }

    }

  if(bg_nle_time_range_intersect(&r->tr->in_out,
                                 &r->tr->visible))
    {
    start_pos = bg_nle_time_2_pos(r->tr, r->tr->in_out.start);
    end_pos = bg_nle_time_2_pos(r->tr,   r->tr->in_out.end);
    
    if(r->tr->in_out.start >= 0)
      {
      cairo_move_to(c, start_pos+4.0, RULER_HEIGHT/2);
      cairo_line_to(c, start_pos, RULER_HEIGHT/2);
      cairo_line_to(c, start_pos, RULER_HEIGHT-1);
      cairo_line_to(c, start_pos+4.0, RULER_HEIGHT-1);
      cairo_set_source_rgb(c, 0.0, 0.0, 1.0);
      cairo_stroke(c);
      }
    if(r->tr->in_out.end >= 0)
      {
      cairo_move_to(c, end_pos-4.0, RULER_HEIGHT/2);
      cairo_line_to(c, end_pos, RULER_HEIGHT/2);
      cairo_line_to(c, end_pos, RULER_HEIGHT-1);
      cairo_line_to(c, end_pos-4.0, RULER_HEIGHT-1);
      cairo_set_source_rgb(c, 0.0, 0.0, 1.0);
      cairo_stroke(c);
      }
    
    if((r->tr->in_out.start >= 0) && (r->tr->in_out.end >= 0))
      {
      GdkRectangle r;
      r.x = start_pos;
      r.width = end_pos - start_pos;
      r.y = RULER_HEIGHT/2;
      r.height = RULER_HEIGHT/2;
      gdk_cairo_rectangle(c, &r);
      
      cairo_set_source_rgba(c, 0.0, 0.0, 1.0, 0.2);
      cairo_fill(c);
      }
    }
  
  // draw cursor

  pos = bg_nle_time_2_pos(r->tr, r->tr->cursor_pos);

  if((pos > -16.0) && (pos < r->tr->width + 16.0))
    {
    cairo_set_source_rgba(c, 0.0, 0.6, 0.0, 5.0);
    cairo_move_to(c, pos - RULER_HEIGHT/4, RULER_HEIGHT/4);
    cairo_line_to(c, pos + RULER_HEIGHT/4, RULER_HEIGHT/4);
    cairo_line_to(c, pos, RULER_HEIGHT/2);
    cairo_fill(c);
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

static gboolean scroll_callback(GtkWidget *widget,
                                GdkEventScroll * evt,
                                gpointer user_data)
  {
  bg_nle_time_ruler_t * r = user_data;
  if(evt->direction == GDK_SCROLL_UP)
    bg_nle_timerange_widget_handle_scroll(r->tr,
                                          -3 * r->spacing_minor);
  else
    bg_nle_timerange_widget_handle_scroll(r->tr,
                                          3 * r->spacing_minor);
    
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
  
  bg_nle_timerange_widget_handle_motion(r->tr, widget, evt);
  return FALSE;
  }

static void realize_callback(GtkWidget *widget,
                             gpointer       user_data)
  {
  bg_nle_time_ruler_t * r = user_data;
  gdk_window_set_cursor(r->wid->window, bg_nle_cursor_xterm);
  }

bg_nle_time_ruler_t * bg_nle_time_ruler_create(bg_nle_timerange_widget_t * tr,
                                               bg_nle_time_info_t * ti)
  {
  bg_nle_time_ruler_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->tr = tr;

  ret->ti = ti;
  
  //  ret->visible.start = 0;
  //  ret->visible.end = 10 * GAVL_TIME_SCALE;
  
  //  ret->spacing_major = GAVL_TIME_SCALE;
  //  ret->spacing_minor = GAVL_TIME_SCALE / 10;
    
  ret->wid = gtk_drawing_area_new();

  ret->font_desc = pango_font_description_from_string(FONT);
  
  gtk_widget_set_size_request(ret->wid, 100, RULER_HEIGHT);

  gtk_widget_set_events(ret->wid,
                        GDK_EXPOSURE_MASK |
                        GDK_POINTER_MOTION_MASK |
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  
  g_signal_connect(ret->wid, "size-allocate", G_CALLBACK(size_allocate_callback),
                   ret);
  g_signal_connect(ret->wid, "expose-event", G_CALLBACK(expose_callback),
                   ret);

  g_signal_connect(ret->wid, "button-press-event",
                   G_CALLBACK(button_press_callback),
                   ret);
  g_signal_connect(ret->wid, "scroll-event",
                   G_CALLBACK(scroll_callback),
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

void bg_nle_time_ruler_update_in_out(bg_nle_time_ruler_t * t)
  {
  if((t->tr->width > 0) && GTK_WIDGET_REALIZED(t->wid)) 
    {
    redraw(t);
    }
  }

void bg_nle_time_ruler_update_cursor_pos(bg_nle_time_ruler_t * t)
  {
  if((t->tr->width > 0) && GTK_WIDGET_REALIZED(t->wid)) 
    {
    redraw(t);
    }
  }



bg_nle_timerange_widget_t * bg_nle_time_ruler_get_tr(bg_nle_time_ruler_t * t)
  {
  return t->tr;
  }

void bg_nle_time_ruler_update_mode(bg_nle_time_ruler_t * t)
  {
  fprintf(stderr, "bg_nle_time_ruler_update_mode\n");
  
  t->spacing_major = 0;
  t->spacing_minor = 0;
  redraw(t);
  }
