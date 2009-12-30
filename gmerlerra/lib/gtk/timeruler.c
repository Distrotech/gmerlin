
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

#define MIN_TIC_SPACING   10.0
#define MIN_MAJOR_SPACING 80.0

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

static int check_time_spacing(bg_nle_time_ruler_t * r, gavl_time_t spacing)
  {
  if(bg_nle_time_2_pos(r->tr, r->tr->visible.start + spacing) > MIN_TIC_SPACING)
    return 1;
  return 0;
  }

static void calc_spacing_hmsms(bg_nle_time_ruler_t * r)
  {
  int64_t test_fac;
  int major_fac = 10;
  int done = 0;

  if(!r->spacing_minor)
    r->spacing_minor = 1000; /* Start with one millisecond */

  /* Minor spacing 1 ms .. 10 sec */
  while(1)
    {
    if(r->spacing_minor >= GAVL_TIME_SCALE)
      {
      break;
      }
    
    if(check_time_spacing(r, r->spacing_minor))
      {
      done = 1;
      break;
      }
    test_fac = 2;
    if(check_time_spacing(r, test_fac*r->spacing_minor))
      {
      r->spacing_minor *= test_fac;
      done = 1;
      break;
      }

    test_fac = 5;
    
    if(check_time_spacing(r, test_fac*r->spacing_minor))
      {
      r->spacing_minor *= test_fac;
      done = 1;
      break;
      }

    r->spacing_minor *= 10;
    }
  
  if(!done)
    {
    /* Minor spacing > 1 sec */
    while(1)
      {
      if(check_time_spacing(r, r->spacing_minor)) // 1 / 10 sec
        break;
    
      test_fac = 2; // 2 / 20 sec
      if(check_time_spacing(r, test_fac*r->spacing_minor))
        {
        r->spacing_minor *= test_fac;
        break;
        }

      test_fac = 3; // 3 / 30 sec
    
      if(check_time_spacing(r, test_fac*r->spacing_minor))
        {
        r->spacing_minor *= test_fac;
        break;
        }

      test_fac = 6; // 6 / 60 sec
      
      if(check_time_spacing(r, test_fac*r->spacing_minor))
        {
        r->spacing_minor *= test_fac;
        break;
        }
      
      test_fac = 12; // 12 / 120 sec
      if(check_time_spacing(r, test_fac*r->spacing_minor))
        {
        r->spacing_minor *= test_fac;
        break;
        }

      test_fac = 30; // 30 / 300 sec
      if(check_time_spacing(r, test_fac*r->spacing_minor))
        {
        r->spacing_minor *= test_fac;
        break;
        }
      r->spacing_minor *= 60;
      }
    }
  
  r->spacing_major = major_fac * r->spacing_minor;
  }

static double get_frame_spacing(bg_nle_time_ruler_t * r, int frames,
                                int64_t frame1, double pos1)
  {
  double pos2;
  int64_t frame2_time;
  frame2_time = gavl_frame_table_frame_to_time(r->ti->tab, frame1 + frames, NULL);
  pos2 = bg_nle_time_2_pos(r->tr, gavl_time_unscale(r->ti->scale, frame2_time));
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
    if(get_frame_spacing(r, r->spacing_minor, frame1, pos1) >= MIN_TIC_SPACING)
      {
      break;
      }
    else if(get_frame_spacing(r, r->spacing_minor * 2, frame1, pos1) >= MIN_TIC_SPACING)
      {
      r->spacing_minor *= 2;
      break;
      }
    else if(get_frame_spacing(r, r->spacing_minor * 5, frame1, pos1) >= MIN_TIC_SPACING)
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
  double pos1;
  int64_t frame1;
  int64_t frame1_time;
  double spacing_f;
  /* spacing_major and spacing_minor are frame counts */


  frame1 = gavl_frame_table_time_to_frame(r->ti->tab,
                                          gavl_time_scale(r->ti->scale, r->tr->visible.start),
                                          &frame1_time);
  
  if(frame1 < 0)
    {
    frame1 = 0;
    frame1_time = gavl_frame_table_frame_to_time(r->ti->tab, 0, NULL);
    }
  pos1 = bg_nle_time_2_pos(r->tr, gavl_time_unscale(r->ti->scale, frame1_time));
  
  /* Check if minor tics can be frames */
  spacing_f = get_frame_spacing(r, 1, frame1, pos1);

  if(spacing_f >= 4.0)
    {
    r->spacing_minor = 1;

    if(spacing_f >= MIN_MAJOR_SPACING)
      r->spacing_major = 1;
    else
      r->spacing_major = 0;
    return;
    }

  /* Make at least second steps as minor tics */
  
  r->spacing_minor = GAVL_TIME_SCALE;
  calc_spacing_hmsms(r);

  /* spacing_major is 10 * spacing_minor now, but should probably be smaller */

  spacing_f = bg_nle_time_2_pos(r->tr, r->tr->visible.start + r->spacing_major);
  
  if(spacing_f > 5 * MIN_MAJOR_SPACING)
    {
    r->spacing_major = r->spacing_minor * 2;
    }
  else if(spacing_f > 2 * MIN_MAJOR_SPACING)
    {
    r->spacing_major = r->spacing_minor * 5;
    }
  
  return;
  }

static void calc_spacing(bg_nle_time_ruler_t * r)
  {
  r->spacing_minor = 0;
  r->spacing_major = 0;
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

  while((time < r->tr->visible.end) && (time < r->tr->end_time))
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

static gavl_time_t make_fake_time(gavl_timecode_t tc)
  {
  int hours, minutes, seconds, frames;
  gavl_time_t ret;
  
  gavl_timecode_to_hmsf(tc, &hours, &minutes, &seconds, &frames);
  
  ret = hours;
  ret *= 60;
  ret += minutes;
  ret *= 60;
  ret += seconds;
  ret *= GAVL_TIME_SCALE;
  
  if(frames)
    ret += GAVL_TIME_SCALE/2;
  return ret;
  }

static void draw_tics_timecode(bg_nle_time_ruler_t * r, PangoLayout * pl, cairo_t * c)
  {
  double pos;
  char str[128];
  int64_t frame, frame_time;
  int64_t frame_time_scaled;
  int64_t total_frames;
  gavl_timecode_t tc;
  int frames;
  
  total_frames = gavl_frame_table_num_frames(r->ti->tab);

  frame = gavl_frame_table_time_to_frame(r->ti->tab,
                                         gavl_time_scale(r->ti->scale, r->tr->visible.start),
                                         NULL);
  
  if(r->spacing_minor == 1) /* Draw minor tics for each frame */
    {
    /* Get the start frame */
    while(1)
      {
      tc = gavl_frame_table_frame_to_timecode(r->ti->tab, frame, &frame_time_scaled, &r->ti->fmt);
      frame_time = gavl_time_unscale(r->ti->scale, frame_time_scaled);
      
      gavl_timecode_to_hmsf(tc, NULL, NULL, NULL, &frames);
      if(!frames)
        break;
      frame--;
      if(frame < 0)
        break;
      }
    
    while(1)
      {
      if(frame >= total_frames)
        break;

      tc = gavl_frame_table_frame_to_timecode(r->ti->tab, frame, &frame_time_scaled, &r->ti->fmt);
      frame_time = gavl_time_unscale(r->ti->scale, frame_time_scaled);
      
      if(frame_time > r->tr->visible.end)
        break;
      
      if(!r->spacing_major)
        gavl_timecode_to_hmsf(tc, NULL, NULL, NULL, &frames);
      else
        frames = 0;
      
      pos = bg_nle_time_2_pos(r->tr, frame_time);
      
      if(frames)
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

        gavl_timecode_prettyprint_short(tc, str);
        pango_layout_set_text(pl, str, -1);
        cairo_move_to(c, pos+2.0, 0.0);
        pango_cairo_show_layout(c, pl);
        }
      frame++;
      }
    }
  else
    {
    gavl_time_t fake_time;
    int diff_min = ((r->spacing_minor / GAVL_TIME_SCALE) - 1) * (r->ti->fmt.int_framerate - 1);
    
    if(diff_min <= 0)
      diff_min = 1;
    
    /* Get start frame */
    
    while(1)
      {
      tc = gavl_frame_table_frame_to_timecode(r->ti->tab, frame, &frame_time_scaled, &r->ti->fmt);
      fake_time = make_fake_time(tc);
      
      if(!(fake_time % r->spacing_major))
        break;
      frame--;
      if(frame < 0)
        {
        frame = 0;
        break;
        }
      }

    frame_time = gavl_time_unscale(r->ti->scale, frame_time_scaled);
    
    /* Draw the tics */
    
    while(1)
      {
      pos = bg_nle_time_2_pos(r->tr, frame_time);
      
      if(!(fake_time % r->spacing_major))
        {
        cairo_move_to(c, pos, 0.0);
        cairo_line_to(c, pos, 32.0);
        cairo_stroke(c);

        gavl_timecode_prettyprint_short(tc, str);
        pango_layout_set_text(pl, str, -1);
        cairo_move_to(c, pos+2.0, 0.0);
        pango_cairo_show_layout(c, pl);
        }
      else
        {
        cairo_move_to(c, pos, 16.0);
        cairo_line_to(c, pos, 32.0);
        cairo_stroke(c);
        }

      /* Find next frame */

      frame += diff_min;
      
      if(frame >= total_frames)
        break;
      
      tc = gavl_frame_table_frame_to_timecode(r->ti->tab, frame, &frame_time_scaled, &r->ti->fmt);

      while(1)
        {
        fake_time = make_fake_time(tc);

        //        fprintf(stderr, "Fake time: %ld, spacing_minor: %ld\n", 
        //                fake_time, r->spacing_minor);
                
        if(!(fake_time % r->spacing_minor))
          break;

        frame++;
        if(frame >= total_frames)
          break;
        
        tc = gavl_frame_table_frame_to_timecode(r->ti->tab, frame, &frame_time_scaled, &r->ti->fmt);
        }
      if(frame >= total_frames)
        break;
      
      frame_time = gavl_time_unscale(r->ti->scale, frame_time_scaled);
      }
    
    }
  
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
  int pos_i;
  
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
        {
        r->time_unit = BG_GTK_DISPLAY_MODE_HMSMS;
        fprintf(stderr, "Disabling timecode display mode");
        }
      break;
    case BG_GTK_DISPLAY_MODE_FRAMECOUNT:
      if(!r->ti->tab)
        r->time_unit = BG_GTK_DISPLAY_MODE_HMSMS;
      break;
    }
  
  if(!r->spacing_major || !r->spacing_minor)
    calc_spacing(r);
  
  pl = pango_cairo_create_layout(c);

  if(r->tr->visible.end > r->tr->end_time)
    pos_i = bg_nle_time_2_pos(r->tr, r->tr->end_time);
  else
    pos_i = r->tr->width;
  
  gtk_paint_box(gtk_widget_get_style(r->wid),
                r->wid->window,
                GTK_STATE_ACTIVE,
                GTK_SHADOW_OUT,
                (const GdkRectangle *)0,
                r->wid,
                (const gchar *)0,
                0,
                0,
                pos_i,
                RULER_HEIGHT);

  if(pos_i < r->tr->width)
    {
    gtk_paint_box(gtk_widget_get_style(r->wid),
                  r->wid->window,
                  GTK_STATE_NORMAL,
                  GTK_SHADOW_OUT,
                  (const GdkRectangle *)0,
                  r->wid,
                  (const gchar *)0,
                  pos_i,
                  0,
                  r->tr->width - pos_i,
                  RULER_HEIGHT);
    }
  
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
  t->spacing_major = 0;
  t->spacing_minor = 0;
  redraw(t);
  }

void bg_nle_time_ruler_frame_forward(bg_nle_time_ruler_t * t)
  {
  int64_t time;
  int64_t frame;
  bg_nle_time_range_t selection;
  
  if(!t->ti->tab)
    return;

  frame = gavl_frame_table_time_to_frame(t->ti->tab,
                                         gavl_time_scale(t->ti->scale,
                                                         t->tr->cursor_pos+10), NULL);
  
  if(frame+1 >= gavl_frame_table_num_frames(t->ti->tab))
    return;

  frame++;
  
  time =
    gavl_time_unscale(t->ti->scale,
                      gavl_frame_table_frame_to_time(t->ti->tab, frame, NULL)) + 10;
  
  bg_nle_time_range_copy(&selection, &t->tr->selection);
  
  if(t->tr->set_selection)
    t->tr->set_selection(&selection, time, t->tr->callback_data);
  }

void bg_nle_time_ruler_frame_backward(bg_nle_time_ruler_t * t)
  {
  int64_t time;
  int64_t frame;
  bg_nle_time_range_t selection;
  
  if(!t->ti->tab)
    return;

  frame = gavl_frame_table_time_to_frame(t->ti->tab,
                                         gavl_time_scale(t->ti->scale,
                                                         t->tr->cursor_pos+10), NULL);
  
  if(frame <= 0)
    return;
  
  frame--;
  
  time =
    gavl_time_unscale(t->ti->scale,
                      gavl_frame_table_frame_to_time(t->ti->tab, frame, NULL)) + 10;
  
  bg_nle_time_range_copy(&selection, &t->tr->selection);
  
  if(t->tr->set_selection)
    t->tr->set_selection(&selection, time, t->tr->callback_data);
  
  }
