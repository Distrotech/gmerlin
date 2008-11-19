/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <stdio.h>
#include <string.h>

#include "gtk_dialog.h"
#include <gui_gtk/gtkutils.h>

typedef struct
  {
  GtkWidget * button;
  GtkWidget * label;
  GtkWidget * drawingarea;
  GtkWidget * colorsel;
  GdkDrawable * background_pixmap;
  GdkColor gdk_color_1;
  GdkColor gdk_color_2;
  GdkGC * gc;
  int has_alpha;
  GdkColor color;
  GdkColor last_color;
  guint16 alpha;
  } color_t;

static void button_callback(GtkWidget * w, gpointer data);


static void destroy(bg_gtk_widget_t * w)
  {
  color_t * priv = (color_t *)w->priv;
  if(priv->colorsel)
    gtk_widget_destroy(priv->colorsel);
  if(priv->gc)
    gdk_gc_unref(priv->gc);
  if(priv->background_pixmap)
    gdk_drawable_unref(priv->background_pixmap);
  free(priv);
  }

static const guint16 background_color_1[3] = { 0xc0c0, 0xc0c0, 0xc0c0 };
static const guint16 background_color_2[3] = { 0x8080, 0x8080, 0x8080 };

static void set_button(color_t * c)
  {
  GdkRectangle rect;
  guint32 i_tmp;
  int depth;
  
  if(!c->drawingarea->window)
    return;
  
  if(c->has_alpha)
    {
    i_tmp = (c->color.red   * c->alpha + background_color_1[0] * (0xffff - c->alpha)) >> 16;
    c->gdk_color_1.red   = (guint16)(i_tmp);
    i_tmp = (c->color.green * c->alpha + background_color_1[1] * (0xffff - c->alpha)) >> 16;
    c->gdk_color_1.green = (guint16)(i_tmp);
    i_tmp = (c->color.blue  * c->alpha + background_color_1[2] * (0xffff - c->alpha)) >> 16;
    c->gdk_color_1.blue  = (guint16)(i_tmp);

    c->gdk_color_1.pixel = (c->gdk_color_1.red >> 8) << 16 |
      (c->gdk_color_1.green >> 8) << 8 | (c->gdk_color_1.blue >> 8);

    i_tmp = (c->color.red   * c->alpha + background_color_2[0] * (0xffff - c->alpha)) >> 16;
    c->gdk_color_2.red   = (guint16)(i_tmp);
    i_tmp = (c->color.green * c->alpha + background_color_2[1] * (0xffff - c->alpha)) >> 16;
    c->gdk_color_2.green = (guint16)(i_tmp);
    i_tmp = (c->color.blue  * c->alpha + background_color_2[2] * (0xffff - c->alpha)) >> 16;
    c->gdk_color_2.blue  = (guint16)(i_tmp);

    c->gdk_color_2.pixel = (c->gdk_color_2.red >> 8) << 16 |
      (c->gdk_color_2.green >> 8) << 8 | (c->gdk_color_2.blue >> 8);

    gdk_color_alloc(gdk_window_get_colormap(c->drawingarea->window),
                    &(c->gdk_color_1));
    gdk_color_alloc(gdk_window_get_colormap(c->drawingarea->window),
                    &(c->gdk_color_2));

    gdk_gc_set_foreground(c->gc, &(c->gdk_color_1));
    gdk_draw_rectangle(c->background_pixmap, c->gc, 1, 0, 0, 16, 16);
    gdk_draw_rectangle(c->background_pixmap, c->gc, 1, 16, 16, 16, 16);

    gdk_gc_set_foreground(c->gc, &(c->gdk_color_2));
    gdk_draw_rectangle(c->background_pixmap, c->gc, 1, 16, 0, 16, 16);
    gdk_draw_rectangle(c->background_pixmap, c->gc, 1, 0, 16, 16, 16);
    }
  else
    {
    gdk_color_alloc(gdk_window_get_colormap(c->drawingarea->window),
                    &(c->color));

    gdk_gc_set_foreground(c->gc, &(c->color));
    gdk_draw_rectangle(c->background_pixmap, c->gc, 1, 0, 0, 32, 32);

    gdk_window_set_background(c->drawingarea->window, &(c->color));
    }

  gdk_window_get_geometry(c->drawingarea->window,
                          &rect.x, &rect.y, &rect.width, &rect.height, &depth);
  rect.x = 0;
  rect.y = 0;
  
  gdk_window_invalidate_rect(c->drawingarea->window, &rect,
                             TRUE);
  }

static void get_value(bg_gtk_widget_t * w)
  {
  color_t * priv;
  priv = (color_t*)(w->priv);

  priv->color.red   = (guint16)(w->value.val_color[0]*65535.0);
  priv->color.green = (guint16)(w->value.val_color[1]*65535.0);
  priv->color.blue  = (guint16)(w->value.val_color[2]*65535.0);
  if(priv->has_alpha)
    priv->alpha = (guint16)(w->value.val_color[3]*65535.0);
  else
    priv->alpha = 0xffff;
  set_button(priv);
  }

static void set_value(bg_gtk_widget_t * w)
  {
  color_t * priv;
  priv = (color_t*)(w->priv);

  w->value.val_color[0] = (float)priv->color.red/65535.0;
  w->value.val_color[1] = (float)priv->color.green/65535.0;
  w->value.val_color[2] = (float)priv->color.blue/65535.0;

  if(priv->has_alpha)
    w->value.val_color[3] = (float)priv->alpha/65535.0;
  else
    w->value.val_color[3] = 1.0;
  }

static void attach(void * priv, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  color_t * c = (color_t*)priv;

  if(*num_columns < 2)
    *num_columns = 2;
  
  gtk_table_resize(GTK_TABLE(table), *row+1, *num_columns);

  gtk_table_attach(GTK_TABLE(table), c->label,
                   0, 1, *row, *row+1, GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(table), c->button,
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  
  (*row)++;
  }

static const gtk_widget_funcs_t funcs =
  {
    .get_value = get_value,
    .set_value = set_value,
    .destroy =   destroy,
    .attach =    attach
  };

static void realize_callback(GtkWidget * _w, gpointer data)
  {
  int x, y, w, h, depth;
  color_t * priv = (color_t*)data;

  //  if(priv->has_alpha)
  //    {
    gdk_window_get_geometry(priv->drawingarea->window,
                            &x, &y, &w, &h, &depth);
    priv->background_pixmap = gdk_pixmap_new(priv->drawingarea->window,
                                             32, 32, depth);
    priv->gc = gdk_gc_new(priv->drawingarea->window);

    //    gdk_window_set_back_pixmap(priv->drawingarea->window,
    //                               priv->background_pixmap, 0);
    bg_gtk_set_widget_bg_pixmap(priv->drawingarea, priv->background_pixmap);
    //    }

  set_button(priv);
  }

static void changed_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_widget_t * wid = (bg_gtk_widget_t*)data;
  color_t * priv = (color_t*)wid->priv;
  
  gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->colorsel), &(priv->color));
  priv->alpha = gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->colorsel));
  
  bg_gtk_change_callback((GtkWidget*)0, wid);
  set_button(priv);
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }


static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_widget_t * wid = (bg_gtk_widget_t*)data;
  GtkWidget * toplevel;
  color_t * priv = (color_t*)wid->priv;
  if(w == priv->button)
    {
    /* Save last color */
    memcpy(&priv->last_color, &priv->color, sizeof(priv->color));
    
    if(!priv->colorsel)
      {
      priv->colorsel =
        gtk_color_selection_dialog_new("Select a color");

      if(wid->info->flags & BG_PARAMETER_SYNC)
        {
        g_signal_connect(G_OBJECT(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->colorsel),
                         "color-changed", G_CALLBACK(changed_callback), wid);
        }
      
      gtk_window_set_modal(GTK_WINDOW(priv->colorsel), TRUE);

      toplevel = bg_gtk_get_toplevel(priv->button);
      if(toplevel)
        gtk_window_set_transient_for(GTK_WINDOW(priv->colorsel),
                                     GTK_WINDOW(toplevel));
      
      g_signal_connect(G_OBJECT(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->ok_button),
                       "clicked", G_CALLBACK(button_callback),
                       (gpointer)wid);
      g_signal_connect(G_OBJECT(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->cancel_button),
                       "clicked", G_CALLBACK(button_callback),
                       (gpointer)wid);
      g_signal_connect(G_OBJECT(priv->colorsel),
                       "delete_event", G_CALLBACK(delete_callback),
                       (gpointer)wid);
      gtk_widget_hide(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->help_button);
      if(priv->has_alpha)
        gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->colorsel),
                                        TRUE);
      }

    gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->colorsel),
                                          &(priv->color));
    if(priv->has_alpha)
      gtk_color_selection_set_current_alpha(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->colorsel),
                                            priv->alpha);
    
    gtk_widget_show(priv->colorsel);
    gtk_main();
    }
  else if(priv->colorsel)
    {
    if(w == GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->ok_button)
      {
      gtk_main_quit();
      gtk_widget_hide(priv->colorsel);
      gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->colorsel), &(priv->color));
      priv->alpha = gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->colorsel));
      set_button(priv);
      }
    else if((w == GTK_COLOR_SELECTION_DIALOG(priv->colorsel)->cancel_button) ||
            (w == priv->colorsel))
      {
      gtk_main_quit();
      gtk_widget_hide(priv->colorsel);
      
      if(wid->info->flags & BG_PARAMETER_SYNC)
        {
        /* Restore last color */
        memcpy(&priv->color, &priv->last_color, sizeof(priv->color));

        wid->funcs->set_value(wid);
        if(wid->change_callback)
          wid->change_callback(wid->change_callback_data,
                               wid->info->name, &(wid->value));
        set_button(priv);
        }

      }
    }
  }

void bg_gtk_create_color_rgba(bg_gtk_widget_t * w,
                              const char * translation_domain)
  {
  color_t * priv;
  bg_gtk_create_color_rgb(w, translation_domain);
  
  priv = (color_t*)(w->priv);
  priv->has_alpha = 1;
  }

void bg_gtk_create_color_rgb(bg_gtk_widget_t * w,
                             const char * translation_domain)
  {
  color_t * priv = calloc(1, sizeof(*priv));

  w->funcs = &funcs;
  
  w->value.val_color[0] = 0.0;
  w->value.val_color[1] = 0.0;
  w->value.val_color[2] = 0.0;
  w->value.val_color[3] = 1.0;
  
  priv->button = gtk_button_new();
  priv->drawingarea = gtk_drawing_area_new();
  
  gtk_widget_set_size_request(priv->drawingarea,
                              priv->drawingarea->requisition.width,
                              16);
  
  gtk_widget_set_events(priv->drawingarea, GDK_EXPOSURE_MASK);
  g_signal_connect(G_OBJECT(priv->drawingarea),
                     "realize", G_CALLBACK(realize_callback),
                     (gpointer)priv);
  g_signal_connect(G_OBJECT(priv->button),
                     "clicked", G_CALLBACK(button_callback),
                     (gpointer)w);
  
  gtk_widget_show(priv->drawingarea);
  gtk_container_add(GTK_CONTAINER(priv->button), priv->drawingarea);

  if(w->info->help_string)
    {
    bg_gtk_tooltips_set_tip(priv->button, w->info->help_string,
                            translation_domain);
    }
  
  gtk_widget_show(priv->button);

  priv->label = gtk_label_new(TR_DOM(w->info->long_name));
  gtk_misc_set_alignment(GTK_MISC(priv->label), 0.0, 0.5);

  gtk_widget_show(priv->label);
  
  w->priv = priv;
  }
