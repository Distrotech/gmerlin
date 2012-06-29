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

#include <stdio.h>

#include "gtk_dialog.h"

#include <gui_gtk/gtkutils.h>

typedef struct
  {
  GtkWidget * label;
  GtkWidget * box;
  GtkWidget * spinbutton_x;
  GtkWidget * spinbutton_y;
  GtkObject * adj_x;
  GtkObject * adj_y;
  } spinbutton_t;

/*
  typedef enum
{
  GTK_EXPAND = 1 << 0,
  GTK_SHRINK = 1 << 1,
  GTK_FILL   = 1 << 2
} GtkAttachOptions;
*/

static void
destroy(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = w->priv;
  free(s);
  }

static void
get_value(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = w->priv;
  // Need to save this because the callback of the x coordinate
  // will change w->value
  float tmp = w->value.val_pos[1]; 
  
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_x),
                            w->value.val_pos[0]);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_y),
                            tmp);
          
  }

static void
set_value(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = w->priv;

  w->value.val_pos[0] =
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(s->spinbutton_x));
  w->value.val_pos[1] =
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(s->spinbutton_y));
  }

static void
attach(void * priv, GtkWidget * table, int * row, int * num_columns)
  {
  spinbutton_t * s = priv;

  if(*num_columns < 2)
    *num_columns = 2;

  gtk_table_resize(GTK_TABLE(table), *row + 1, *num_columns);

  gtk_table_attach(GTK_TABLE(table), s->label,
                   0, 1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
  
  gtk_table_attach(GTK_TABLE(table), s->box,
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  *row += 1;
  }

static const gtk_widget_funcs_t pos_funcs =
  {
    .get_value = get_value,
    .set_value = set_value,
    .destroy = destroy,
    .attach =  attach
  };

void 
bg_gtk_create_position(bg_gtk_widget_t * w,
                       const char * translation_domain)
  {
  GtkWidget * label;
  spinbutton_t * s = calloc(1, sizeof(*s));
  w->funcs = &pos_funcs;
  s->label = gtk_label_new(TR_DOM(w->info->long_name));
  
  gtk_widget_show(s->label);
  gtk_misc_set_alignment(GTK_MISC(s->label), 0.0, 0.5);
  s->adj_x =  gtk_adjustment_new(0.0, 0.0, 1.0,
                                 0.01, 0.0, 0.0);
  s->adj_y =  gtk_adjustment_new(0.0, 0.0, 1.0,
                                 0.01, 0.0, 0.0);
  s->spinbutton_x =
    gtk_spin_button_new(GTK_ADJUSTMENT(s->adj_x), 0.1, 0);
  s->spinbutton_y =
    gtk_spin_button_new(GTK_ADJUSTMENT(s->adj_y), 0.1, 0);
  if(w->info->flags & BG_PARAMETER_SYNC)
    {
    w->callback_id =
      g_signal_connect(G_OBJECT(s->spinbutton_x), "value-changed",
                       G_CALLBACK(bg_gtk_change_callback), (gpointer)w);
    w->callback_widget = s->spinbutton_x;

    w->callback_id_2 =
      g_signal_connect(G_OBJECT(s->spinbutton_y), "value-changed",
                       G_CALLBACK(bg_gtk_change_callback), (gpointer)w);
    w->callback_widget_2 = s->spinbutton_y;
    }
  if(w->info->help_string)
    {
    bg_gtk_tooltips_set_tip(s->spinbutton_x,
                            w->info->help_string, translation_domain);
    bg_gtk_tooltips_set_tip(s->spinbutton_y,
                            w->info->help_string, translation_domain);
    }
  
  gtk_widget_show(s->spinbutton_x);
  gtk_widget_show(s->spinbutton_y);
  gtk_widget_show(s->label);

  s->box = gtk_hbox_new(0, 5);
  label = gtk_label_new(TR("X"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(s->box), label, FALSE, FALSE, 0);
  bg_gtk_box_pack_start_defaults(GTK_BOX(s->box), s->spinbutton_x);
    
  label = gtk_label_new(TR("Y"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(s->box), label, FALSE, FALSE, 0);
  bg_gtk_box_pack_start_defaults(GTK_BOX(s->box), s->spinbutton_y);

  gtk_widget_show(s->box);
  
  w->priv = s;

  bg_gtk_change_callback_block(w, 1);
  
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_x),
                             w->info->num_digits);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_y),
                             w->info->num_digits);
  bg_gtk_change_callback_block(w, 0);
  
  }

