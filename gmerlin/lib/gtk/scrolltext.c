/*****************************************************************
 
  scrolltext.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gui_gtk/scrolltext.h>

#include <utils.h>

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
  guint timeout_tag;
    
  PangoFontDescription* font_desc;
    
  GtkWidget * drawingarea;

  GdkPixmap * pixmap_1; /* Pixmap for the whole string */
  GdkPixmap * pixmap_2; /* Pixmap for the drawingarea  */

  GdkGC * gc;
  
  };

static gboolean expose_callback(GtkWidget * w, GdkEventExpose * evt,
                                gpointer data)
  {
  bg_gtk_scrolltext_t * st = (bg_gtk_scrolltext_t *)data;
  gdk_draw_drawable(st->drawingarea->window, st->gc, st->pixmap_2,
                    0, 0, 0, 0, st->width, st->height);
  
  return TRUE;
  }

static gboolean configure_callback(GtkWidget * w, GdkEventConfigure * evt,
                                   gpointer data)
  {
  bg_gtk_scrolltext_t * st = (bg_gtk_scrolltext_t *)data;
  fprintf(stderr, "Configure: %d %d\n", evt->width, evt->height);
  return FALSE;
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
    gdk_draw_drawable(st->pixmap_2, st->gc, st->pixmap_1,
                      st->offset, 0, 0, 0,
                      st->text_width - st->offset,
                      st->height);
    gdk_draw_drawable(st->pixmap_2, st->gc, st->pixmap_1,
                      0, 0, st->text_width - st->offset, 0,
                      st->width - (st->text_width - st->offset), st->height);
    }
  else
    {
    gdk_draw_drawable(st->pixmap_2, st->gc, st->pixmap_1,
                      st->offset, 0, 0, 0, st->width, st->height);
    }
  expose_callback(st->drawingarea, (GdkEventExpose*)0,
                  st);
  return TRUE;
  }

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

static void create_text_pixmap(bg_gtk_scrolltext_t * st)
  {
  char * tmp_string;
  PangoLayout * layout;
  GdkColor fg;
  GdkColor bg;

  PangoRectangle ink_rect;
  PangoRectangle logical_rect;
  
  int height;

  if(!st->is_realized)
    return;
  
  //  fprintf(stderr, "create_text_pixmap %s.", st->text);
  
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
  
  if(st->pixmap_1)
    g_object_unref(G_OBJECT(st->pixmap_1));

  st->pixmap_1 = gdk_pixmap_new(st->drawingarea->window,
                                st->text_width, st->height, -1);

  gdk_gc_set_foreground(st->gc, &bg);
  gdk_draw_rectangle(st->pixmap_1,
                     st->gc, TRUE,
                     0, 0, st->text_width, st->height);
    
  gdk_gc_set_foreground(st->gc, &fg);
  //  fprintf(stderr, "y: %d\n", (st->height - height)/2);
  gdk_draw_layout(st->pixmap_1, st->gc, 0, (st->height - height)/2,
                  layout);

  if(!st->do_scroll)
    {
    gdk_gc_set_foreground(st->gc, &bg);
    gdk_draw_rectangle(st->pixmap_2,
                       st->gc, TRUE,
                       0, 0, st->width, st->height);
    gdk_draw_drawable(st->pixmap_2, st->gc, st->pixmap_1,
                      0, 0, (st->width - st->text_width)/2,
                      0, st->width, st->height);
    }
  else
    {
    st->timeout_tag = g_timeout_add(TIMEOUT_INTERVAL,
                                    timeout_func,
                                    st);
    }
  
  
  g_object_unref(layout);

  expose_callback(st->drawingarea, (GdkEventExpose*)0,
                  st);
  //  fprintf(stderr, "done\n");
  
  }

static void realize_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_scrolltext_t * st = (bg_gtk_scrolltext_t *)data;
  fprintf(stderr, "Realize!!\n");
  st->is_realized = 1;
  st->gc = gdk_gc_new(st->drawingarea->window);

  st->pixmap_2 = gdk_pixmap_new(st->drawingarea->window,
                                st->width, st->height, -1);
  
  if(st->text)
    create_text_pixmap(st);
  }

bg_gtk_scrolltext_t * bg_gtk_scrolltext_create(int width, int height)
  {
  bg_gtk_scrolltext_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->width = width;
  ret->height = height;

  ret->drawingarea = gtk_drawing_area_new();
  gtk_widget_set_size_request(ret->drawingarea,
                              ret->width, ret->height);

  g_signal_connect(G_OBJECT(ret->drawingarea),
                   "realize", G_CALLBACK(realize_callback),
                   ret);
  g_signal_connect(G_OBJECT(ret->drawingarea),
                   "expose-event", G_CALLBACK(expose_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->drawingarea),
                   "configure-event", G_CALLBACK(configure_callback),
                   ret);
  
  gtk_widget_show(ret->drawingarea);
  
  return ret;
  }

void bg_gtk_scrolltext_set_text(bg_gtk_scrolltext_t * d, const char * text,
                                float * foreground_color,
                                float * background_color)
  {
  d->text = bg_strdup(d->text, text);

  memcpy(d->foreground_color, foreground_color, 3 * sizeof(float));
  memcpy(d->background_color, background_color, 3 * sizeof(float));
  create_text_pixmap(d);
  }

void bg_gtk_scrolltext_set_colors(bg_gtk_scrolltext_t * d,
                                  float * fg_color, float * bg_color)
  {
  memcpy(d->foreground_color, fg_color, 3 * sizeof(float));
  memcpy(d->background_color, bg_color, 3 * sizeof(float));
  create_text_pixmap(d);
  }


void bg_gtk_scrolltext_set_font(bg_gtk_scrolltext_t * d, const char * font)
  {
  if(d->font_desc)
    pango_font_description_free(d->font_desc);
  d->font_desc = pango_font_description_from_string(font);
  }

void bg_gtk_scrolltext_destroy(bg_gtk_scrolltext_t * d)
  {
  if(d->font_desc)
    pango_font_description_free(d->font_desc);
  if(d->text)
    free(d->text);
  free(d);
  }

GtkWidget * bg_gtk_scrolltext_get_widget(bg_gtk_scrolltext_t * d)
  {
  return d->drawingarea;
  }
