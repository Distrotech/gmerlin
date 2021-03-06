/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/


#include <inttypes.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <ctype.h> 

#include <gavl/gavl.h>

#include <gtk/gtk.h>
#include <gui_gtk/display.h>
#include <gui_gtk/gtkutils.h>
#include <gmerlin/utils.h>

#define MAX_CHARS GAVL_TIME_STRING_LEN_MS // 15

#define DIGIT_0     0
#define DIGIT_1     1
#define DIGIT_2     2
#define DIGIT_3     3
#define DIGIT_4     4
#define DIGIT_5     5
#define DIGIT_6     6
#define DIGIT_7     7
#define DIGIT_8     8
#define DIGIT_9     9
#define DIGIT_COLON 10
#define DIGIT_MINUS 11
#define DIGIT_DOT   12

#define NUM_PIXBUFS 13

static int num_time_displays = 0;

static GdkPixbuf * digit_pixbufs[NUM_PIXBUFS];

static void load_pixbufs()
  {
  char * c_tmp1 = NULL;
  char * c_tmp2 = NULL;
  int i;

  if(num_time_displays)
    {
    num_time_displays++;
    return;
    }
  num_time_displays++;
  
  for(i = 0; i < 10; i++)
    {
    c_tmp1 = bg_sprintf("digit_%d.png", i);
    c_tmp2 = bg_search_file_read("icons", c_tmp1);
    digit_pixbufs[i] = gdk_pixbuf_new_from_file(c_tmp2, NULL);    
    free(c_tmp1);
    free(c_tmp2);
    }
  
  c_tmp2 = bg_search_file_read("icons", "digit_colon.png");
  digit_pixbufs[DIGIT_COLON] = gdk_pixbuf_new_from_file(c_tmp2, NULL);    
  free(c_tmp2);
  
  c_tmp2 = bg_search_file_read("icons", "digit_minus.png");
  digit_pixbufs[DIGIT_MINUS] = gdk_pixbuf_new_from_file(c_tmp2, NULL);    
  free(c_tmp2);

  c_tmp2 = bg_search_file_read("icons", "digit_dot.png");
  digit_pixbufs[DIGIT_DOT] = gdk_pixbuf_new_from_file(c_tmp2, NULL);    
  free(c_tmp2);
  }

static void unload_pixbufs()
  {
  int i;

  num_time_displays--;
  if(!num_time_displays)
    {
    for(i = 0; i < NUM_PIXBUFS; i++)
      {
      g_object_unref(digit_pixbufs[i]);
      digit_pixbufs[i] = NULL;
      }
    }
  }

struct bg_gtk_time_display_s
  {
  GdkPixbuf * pixbufs[NUM_PIXBUFS];
  float foreground_color[3];
  float background_color[3];
  int height;
  int digit_width;
  int colon_width;
  GtkWidget * widget;
  int indices[MAX_CHARS];
  
  GdkGC * gc;

  int type_mask;
  int max_width;
  int border_width;
  };

static void set_bg_color(bg_gtk_time_display_t * d)
  {
  GdkColor bg;
  if(!d->gc)
    return;
  
  bg.red   = (guint16)(d->background_color[0]*65535.0);
  bg.green = (guint16)(d->background_color[1]*65535.0);
  bg.blue  = (guint16)(d->background_color[2]*65535.0);
  bg.pixel =
    (bg.red >> 8)   << 16 |
    (bg.green >> 8) << 8  |
    (bg.blue >> 8);
  gdk_color_alloc(gdk_window_get_colormap(d->widget->window),
                  &bg);

  gtk_widget_modify_bg(d->widget, GTK_STATE_NORMAL, &bg);
  
  gdk_gc_set_foreground(d->gc, &bg);
  }

static void create_pixmaps(bg_gtk_time_display_t * d)
  {
  int i;
  
  for(i = 0; i < NUM_PIXBUFS; i++)
    {
    if(d->pixbufs[i])
      {
      g_object_unref(G_OBJECT(d->pixbufs[i]));
      d->pixbufs[i] = NULL;
      }
    }
  /* Scale down the pixmaps */
  
  for(i = 0; i < 10; i++)
    {
    d->pixbufs[i] = bg_gtk_pixbuf_scale_alpha(digit_pixbufs[i],
                                              d->digit_width,
                                              d->height,
                                              d->foreground_color,
                                              d->background_color);
    }
  d->pixbufs[DIGIT_COLON] = bg_gtk_pixbuf_scale_alpha(digit_pixbufs[DIGIT_COLON],
                                                      d->colon_width,
                                                      d->height,
                                                      d->foreground_color,
                                                      d->background_color);

  d->pixbufs[DIGIT_MINUS] = bg_gtk_pixbuf_scale_alpha(digit_pixbufs[DIGIT_MINUS],
                                                      d->digit_width,
                                                      d->height,
                                                      d->foreground_color,
                                                      d->background_color);
  if(d->type_mask & BG_GTK_DISPLAY_MODE_HMSMS)
    {
    d->pixbufs[DIGIT_DOT] = bg_gtk_pixbuf_scale_alpha(digit_pixbufs[DIGIT_DOT],
                                                      d->colon_width,
                                                      d->height,
                                                      d->foreground_color,
                                                      d->background_color);
    }
  
  }

static gboolean expose_callback(GtkWidget * w, GdkEventExpose * evt,
                                gpointer data)
  {
  int pos_i;
  bg_gtk_time_display_t * d;
  int x;

  d = (bg_gtk_time_display_t *)data;

  if(!d->widget->window)
    return TRUE;
    
  pos_i = 0;
  x = d->max_width - d->border_width;
  
  while((d->indices[pos_i] >= 0) && (pos_i < MAX_CHARS))
    {
    if((d->indices[pos_i] == DIGIT_COLON) ||
       (d->indices[pos_i] == DIGIT_DOT))
      {
      x -= d->colon_width;
      gdk_draw_pixbuf(d->widget->window,
                      NULL,
                      d->pixbufs[d->indices[pos_i]],
                      0, // gint src_x,
                      0, // gint src_y,
                      x,
                      d->border_width,
                      d->colon_width,
                      d->height,
                      GDK_RGB_DITHER_NONE,
                      0, 0);
      }
    else
      {
      x -= d->digit_width;
      gdk_draw_pixbuf(d->widget->window,
                      NULL,
                      d->pixbufs[d->indices[pos_i]],
                      0, // gint src_x,
                      0, // gint src_y,
                      x,
                      d->border_width,
                      d->digit_width,
                      d->height,
                      GDK_RGB_DITHER_NONE,
                      0, 0);
      }
    pos_i++;
    }
  if(x)
    gdk_draw_rectangle(d->widget->window,
                       d->gc,
                       TRUE,
                       0, 0, x, d->height + 2 * d->border_width);
  return TRUE;
  }

static void realize_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_time_display_t * d;

  d = (bg_gtk_time_display_t *)data;

  d->gc = gdk_gc_new(d->widget->window);
  set_bg_color(d);
  }

void bg_gtk_time_display_set_colors(bg_gtk_time_display_t * d,
                                    float * foreground,
                                    float * background)
  {
  memcpy(d->foreground_color, foreground, 3 * sizeof(float));
  memcpy(d->background_color, background, 3 * sizeof(float));
  create_pixmaps(d);

  set_bg_color(d);
  
  expose_callback(d->widget, NULL, d);
  }


void bg_gtk_time_display_update(bg_gtk_time_display_t * d,
                                int64_t time, int mode)
  {
  char * pos;
  char buf[MAX_CHARS];
  int pos_i;

  switch(mode)
    {
    case BG_GTK_DISPLAY_MODE_HMS:
      gavl_time_prettyprint(time, buf);
      break;
    case BG_GTK_DISPLAY_MODE_HMSMS:
      gavl_time_prettyprint_ms(time, buf);
      break;
    case BG_GTK_DISPLAY_MODE_TIMECODE:
      gavl_timecode_prettyprint_short(time, buf);
      break;
    case BG_GTK_DISPLAY_MODE_FRAMECOUNT:
      sprintf(buf, "%"PRId64, time);
      break;
    }
  
  pos = &buf[strlen(buf)];
 
  pos_i = 0;
  
  do{
    pos--;
    if(*pos == ':')
      d->indices[pos_i] = DIGIT_COLON;
    else if(*pos == '-')
      d->indices[pos_i] = DIGIT_MINUS;
    else if(*pos == '.')
      d->indices[pos_i] = DIGIT_DOT;
    else if(isdigit(*pos))
      d->indices[pos_i] = (int)(*pos) - (int)('0');
    pos_i++;
  } while(pos != buf);
  
  while(pos_i < MAX_CHARS)
    {
    d->indices[pos_i] = -1;
    pos_i++;
    }
  expose_callback(d->widget, NULL, d);
  }

bg_gtk_time_display_t *
bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE size, int border_width,
                           int type_mask)
  {
  bg_gtk_time_display_t * ret;
  
  load_pixbufs();
  
  ret = calloc(1, sizeof(*ret));
  ret->border_width = border_width;
  ret->type_mask = type_mask;

  switch(size)
    {
    case BG_GTK_DISPLAY_SIZE_HUGE:   /* 480 x 96, 1/1 */
      ret->height       = 96;
      ret->digit_width  = 60;
      ret->colon_width  = 30;
      break;
    case BG_GTK_DISPLAY_SIZE_LARGE:  /* 240 x 48, 1/2 */
      ret->height       = 48;
      ret->digit_width  = 30;
      ret->colon_width  = 15;
      break;
    case BG_GTK_DISPLAY_SIZE_NORMAL: /* 160 x 32  1/3 */
      ret->height       = 32;
      ret->digit_width  = 20;
      ret->colon_width  = 10;
      break;
    case BG_GTK_DISPLAY_SIZE_SMALL:  /*  80 x 16  1/6 */
      ret->height       = 16;
      ret->digit_width  = 10;
      ret->colon_width  = 5;
      break;

    }
  ret->foreground_color[0] = 0.0;
  ret->foreground_color[1] = 1.0;
  ret->foreground_color[2] = 0.0;

  ret->background_color[0] = 0.0;
  ret->background_color[1] = 0.0;
  ret->background_color[2] = 0.0;
  create_pixmaps(ret);

  ret->widget = gtk_drawing_area_new();

  g_signal_connect(G_OBJECT(ret->widget), "expose_event",
                     G_CALLBACK(expose_callback), (gpointer)ret);

  gtk_widget_set_events(ret->widget,
                        GDK_EXPOSURE_MASK |
                        GDK_ENTER_NOTIFY_MASK |
                        GDK_LEAVE_NOTIFY_MASK);
  
  g_signal_connect(G_OBJECT(ret->widget), "realize",
                   G_CALLBACK(realize_callback), (gpointer)ret);
  

  ret->max_width = 2 * ret->border_width;
  
  if(ret->type_mask & BG_GTK_DISPLAY_MODE_HMSMS)
    { // -000:00:00.000
    ret->max_width += 3 * ret->colon_width + 10 * ret->digit_width;
    }
  else if(ret->type_mask & BG_GTK_DISPLAY_MODE_TIMECODE)
    { // -00:00:00:00
    ret->max_width += 3 * ret->colon_width + 9 * ret->digit_width;
    }
  else
    { // -000:00:00
    ret->max_width += 2 * ret->colon_width + 7 * ret->digit_width;
    }
  
  gtk_widget_set_size_request(ret->widget,
                              ret->max_width,
                              2 * ret->border_width + ret->height);
  
  gtk_widget_show(ret->widget);
  return ret;
  }

GtkWidget * bg_gtk_time_display_get_widget(bg_gtk_time_display_t * d)
  {
  return d->widget;
  }

void bg_gtk_time_display_destroy(bg_gtk_time_display_t * d)
  {
  int i;

  if(d->gc)
    g_object_unref(d->gc);

  for(i = 0; i < NUM_PIXBUFS; i++)
    {
    if(d->pixbufs[i])
      g_object_unref(d->pixbufs[i]);
    }
  free(d);
  unload_pixbufs();

  }
