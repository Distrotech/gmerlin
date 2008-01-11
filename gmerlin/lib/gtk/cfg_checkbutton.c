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

#include "gtk_dialog.h"

#include <gui_gtk/gtkutils.h>

typedef struct
  {
  GtkWidget * button;
  } checkbutton_t;

static void get_value(bg_gtk_widget_t * w)
  {
  checkbutton_t * priv;
  priv = (checkbutton_t*)(w->priv);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->button), w->value.val_i);
  }

static void set_value(bg_gtk_widget_t * w)
  {
  checkbutton_t * priv;
  priv = (checkbutton_t*)(w->priv);
  w->value.val_i =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->button));
  }

static void destroy(bg_gtk_widget_t * w)
  {
  checkbutton_t * priv = (checkbutton_t *)w->priv;
  free(priv);
  }

static void attach(void * priv, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  checkbutton_t * b = (checkbutton_t*)priv;

  if(*num_columns < 2)
    *num_columns = 2;
  
  gtk_table_resize(GTK_TABLE(table), *row+1, *num_columns);
  gtk_table_attach(GTK_TABLE(table), b->button,
                    0, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  (*row)++;
  }

static gtk_widget_funcs_t funcs =
  {
    .get_value = get_value,
    .set_value = set_value,
    .destroy =   destroy,
    .attach =    attach
  };


void bg_gtk_create_checkbutton(bg_gtk_widget_t * w, bg_parameter_info_t * info,
                               const char * translation_domain)
  {
  checkbutton_t * priv = calloc(1, sizeof(*priv));
  priv->button = gtk_check_button_new_with_label(TR_DOM(info->long_name));

  if(info->flags & BG_PARAMETER_SYNC)
    {
    w->callback_id = 
      g_signal_connect(G_OBJECT(priv->button), "toggled",
                       G_CALLBACK(bg_gtk_change_callback), (gpointer)w);
    w->callback_widget = priv->button;
    }

  if(info->help_string)
    {
    bg_gtk_tooltips_set_tip(priv->button,
                            info->help_string, translation_domain);
    }
  
  gtk_widget_show(priv->button);
 
  w->funcs = &funcs;
  w->priv = priv;
  }
