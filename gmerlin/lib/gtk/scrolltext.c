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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gui_gtk/scrolltext.h>
#include <gui_gtk/gtkutils.h>

#include <gmerlin/utils.h>

#define TIMEOUT_INTERVAL 30
#define SCROLL_ADVANCE   1

struct bg_gtk_scrolltext_s
  {
  int width;
  int height;
  int offset;

  int text_width;
  
  int is_realized;
  char * text;
  float foreground_color[3];
  float background_color[3];

  int do_scroll;
  gulong timeout_tag;
    
  PangoFontDescription* font_desc;
    
  GtkWidget * drawingarea;

  GdkPixmap * pixmap_string; /* Pixmap for the whole string */
  GdkPixmap * pixmap_da; /* Pixmap for the drawingarea  */

  GdkGC * gc;

  int pixmap_width;
  int pixmap_height;
  
  };

static void set_color(bg_gtk_scrolltext_t * st,
                      float * color,
                      GdkColor * gdk_color)
  {
  if(!st->is_realized)
    return;
   
  gdk_color->red   = (guint16)(color[0] * 65535.0);
  gdk_color->green = (guint16)(color[1] * 65535.0);
  gdk_color->blue  = (guint16)(color[2] * 65535.0);
 
  gdk_color->pixel =
    ((gdk_color->red >> 8)   << 16) |
    ((gdk_color->green >> 8) << 8) |
    ((gdk_color->blue >> 8));
 
  gdk_color_alloc(gdk_window_get_colormap(st->drawingarea->window),
                  gdk_color);
   
  }


static void create_text_pixmap(bg_gtk_scrolltext_t * st);

static gboolean expose_callback(GtkWidget * w, GdkEventExpose * evt,
                                gpointer data)
  {
  bg_gtk_scrolltext_t * st = (bg_gtk_scrolltext_t *)data;
  if(st->pixmap_da)
    gdk_draw_drawable(st->drawingarea->window, st->gc, st->pixmap_da,
                      0, 0, 0, 0, st->width, st->height);
  
  return TRUE;
  }

static void size_allocate_callback(GtkWidget * w, GtkAllocation * evt,
                                   gpointer data)
     //static gboolean configure_callback(GtkWidget * w, GdkEventConfigure * evt,
     //                                   gpointer data)
  {
  GdkColor bg;
  bg_gtk_scrolltext_t * st = (bg_gtk_scrolltext_t *)data;

  if((st->width == evt->width) && (st->height == evt->height) &&
     (st->pixmap_da))
    return;
  
  st->width = evt->width;
  st->height = evt->height;

  if(!st->is_realized)
    return;

  if(st->pixmap_da)
    {
    if((st->pixmap_width < evt->width) || (st->pixmap_height < evt->height))
      {
      /* Enlarge pixmap */
      
      st->pixmap_width  = evt->width + 10;
      st->pixmap_height = evt->height + 10;

      g_object_unref(st->pixmap_da);
      st->pixmap_da = gdk_pixmap_new(st->drawingarea->window,
                                    st->pixmap_width, st->pixmap_height, -1);
      }
    /* Put pixmap_string onto pixmap_da if we won't scroll */
    if(st->width >= st->text_width)
      {
      set_color(st, st->background_color, &bg);
      
      gdk_gc_set_foreground(st->gc, &bg);
      gdk_draw_rectangle(st->pixmap_da,
                         st->gc, TRUE,
                         0, 0, st->width, st->height);
      if(st->pixmap_string)
        gdk_draw_drawable(st->pixmap_da, st->gc, st->pixmap_string,
                          0, 0, (st->width - st->text_width)/2,
                          0, st->text_width, st->height);
      }
    }
  else
    {
    st->pixmap_width  = evt->width + 10;
    st->pixmap_height = evt->height + 10;

    st->pixmap_da = gdk_pixmap_new(st->drawingarea->window,
                                  st->pixmap_width, st->pixmap_height, -1);
    
    if(st->text)
      create_text_pixmap(st);
    }
  
  if(((st->width < st->text_width) && !st->do_scroll) ||
     ((st->width >= st->text_width) && st->do_scroll) ||
     !st->pixmap_string)
    {
    create_text_pixmap(st);
    }
  
  //  expose_callback(w, NULL, data);
  return;
  }

static gboolean timeout_func(gpointer data)
  {
  bg_gtk_scrolltext_t * st = (bg_gtk_scrolltext_t*)data;
  
  if(!st->do_scroll)
    return FALSE;
  
  st->offset += SCROLL_ADVANCE;

  if(st->offset > st->text_width)
    st->offset = 0;

  if(st->text_width - st->offset < st->width)
    {
    gdk_draw_drawable(st->pixmap_da, st->gc, st->pixmap_string,
                      st->offset, 0, 0, 0,
                      st->text_width - st->offset,
                      st->height);
    gdk_draw_drawable(st->pixmap_da, st->gc, st->pixmap_string,
                      0, 0, st->text_width - st->offset, 0,
                      st->width - (st->text_width - st->offset), st->height);
    }
  else
    {
    gdk_draw_drawable(st->pixmap_da, st->gc, st->pixmap_string,
                      st->offset, 0, 0, 0, st->width, st->height);
    }
  expose_callback(st->drawingarea, NULL,
                  st);
  return TRUE;
  }


static void create_text_pixmap(bg_gtk_scrolltext_t * st)
  {
  char * tmp_string;
  PangoLayout * layout;
  GdkColor fg;
  GdkColor bg;

  PangoRectangle ink_rect;
  PangoRectangle logical_rect;
  
  int height;

  if(!st->is_realized || !st->width || ! st->height)
    return;
  
  /* Create pango layout */
  
  layout = gtk_widget_create_pango_layout(st->drawingarea,
                                          st->text);

  if(st->font_desc)
    pango_layout_set_font_description(layout,
                                      st->font_desc);

  /* Set Colors */

  set_color(st, st->foreground_color, &fg);
  set_color(st, st->background_color, &bg);

  /* Remove previous timeout callbacks */

  if(st->do_scroll)
    {
    g_source_remove(st->timeout_tag);
    st->do_scroll = 0;
    st->timeout_tag = 0;
    }
  
  /* Set up pixmap */
  
  pango_layout_get_extents(layout,
                           &ink_rect,
                           &logical_rect);

  st->text_width =  logical_rect.width  / PANGO_SCALE;
  height = logical_rect.height / PANGO_SCALE;

  /* Change string if necessary */

  if(st->text_width > st->width)
    {
    st->do_scroll = 1;
    tmp_string = bg_sprintf("%s * * * ", st->text);

    pango_layout_set_text(layout, tmp_string, -1);
    
    pango_layout_get_extents(layout,
                             &ink_rect,
                             &logical_rect);
    
    st->text_width = logical_rect.width  / PANGO_SCALE;
    height         = logical_rect.height / PANGO_SCALE;
    free(tmp_string);
    }
  else
    st->do_scroll = 0;
  
  /* Set up Pixmap */
  
  if(st->pixmap_string)
    g_object_unref(G_OBJECT(st->pixmap_string));
  
  st->pixmap_string = gdk_pixmap_new(st->drawingarea->window,
                                st->text_width, st->height, -1);

  gdk_gc_set_foreground(st->gc, &bg);
  gdk_draw_rectangle(st->pixmap_string,
                     st->gc, TRUE,
                     0, 0, st->text_width, st->height);
    
  gdk_gc_set_foreground(st->gc, &fg);
  gdk_draw_layout(st->pixmap_string, st->gc, 0, (st->height - height)/2,
                  layout);

  if(!st->do_scroll)
    {
    gdk_gc_set_foreground(st->gc, &bg);
    gdk_draw_rectangle(st->pixmap_da,
                       st->gc, TRUE,
                       0, 0, st->width, st->height);
    gdk_draw_drawable(st->pixmap_da, st->gc, st->pixmap_string,
                      0, 0, (st->width - st->text_width)/2,
                      0, st->text_width, st->height);
    }
  else
    {
    st->timeout_tag = g_timeout_add(TIMEOUT_INTERVAL,
                                    timeout_func,
                                    st);
    }
  g_object_unref(layout);
  
  expose_callback(st->drawingarea, NULL,
                  st);
  
  }

static void realize_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_scrolltext_t * st = (bg_gtk_scrolltext_t *)data;
  st->is_realized = 1;
  st->gc = gdk_gc_new(st->drawingarea->window);

  if(!st->pixmap_da)
    {
    GtkAllocation a;
    a.width  = st->width;
    a.height = st->height;
    size_allocate_callback(w, &a, data);
    }
  
  }

bg_gtk_scrolltext_t * bg_gtk_scrolltext_create(int width, int height)
  {
  bg_gtk_scrolltext_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->drawingarea = gtk_drawing_area_new();

  if((width >= 0) && (height >= 0))
    {
    //    ret->width = width;
    //    ret->height = height;
    gtk_widget_set_size_request(ret->drawingarea,
                                width, height);
    }
  else
    {
    gtk_widget_set_size_request(ret->drawingarea, 16, 16);
    }
  
  g_signal_connect(G_OBJECT(ret->drawingarea),
                   "realize", G_CALLBACK(realize_callback),
                   ret);
  g_signal_connect(G_OBJECT(ret->drawingarea),
                   "expose-event", G_CALLBACK(expose_callback),
                   ret);

  //  g_signal_connect(G_OBJECT(ret->drawingarea),
  //                   "configure-event", G_CALLBACK(configure_callback),
  //                   ret);

  g_signal_connect(G_OBJECT(ret->drawingarea),
                   "size-allocate", G_CALLBACK(size_allocate_callback),
                   ret);
  
  gtk_widget_show(ret->drawingarea);
  
  return ret;
  }

void bg_gtk_scrolltext_set_text(bg_gtk_scrolltext_t * d, const char * text,
                                const float * foreground_color,
                                const float * background_color)
  {
  d->text = gavl_strrep(d->text, text);

  memcpy(d->foreground_color, foreground_color, 3 * sizeof(float));
  memcpy(d->background_color, background_color, 3 * sizeof(float));
  create_text_pixmap(d);
  }

void bg_gtk_scrolltext_set_colors(bg_gtk_scrolltext_t * d,
                                  const float * fg_color, const float * bg_color)
  {
  memcpy(d->foreground_color, fg_color, 3 * sizeof(float));
  memcpy(d->background_color, bg_color, 3 * sizeof(float));
  create_text_pixmap(d);
  }


void bg_gtk_scrolltext_set_font(bg_gtk_scrolltext_t * d, const char * font)
  {
  char * tmp;
  if(d->font_desc)
    pango_font_description_free(d->font_desc);
  tmp = bg_gtk_convert_font_name_to_pango(font);
  d->font_desc = pango_font_description_from_string(tmp);
  free(tmp);
  }

void bg_gtk_scrolltext_destroy(bg_gtk_scrolltext_t * d)
  {
  if(d->timeout_tag)
    g_source_remove(d->timeout_tag);
  if(d->font_desc)
    pango_font_description_free(d->font_desc);
  if(d->text)
    free(d->text);
  if(d->pixmap_string)
    g_object_unref(d->pixmap_string);
  if(d->pixmap_da)
    g_object_unref(d->pixmap_da);
  if(d->gc)
    g_object_unref(d->gc);

  free(d);
  }

GtkWidget * bg_gtk_scrolltext_get_widget(bg_gtk_scrolltext_t * d)
  {
  return d->drawingarea;
  }
