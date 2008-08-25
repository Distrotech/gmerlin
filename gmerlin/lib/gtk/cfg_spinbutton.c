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
  GtkWidget * label;
  GtkWidget * spinbutton;
  GtkObject * adj;
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
  spinbutton_t * s = (spinbutton_t*)(w->priv);
  free(s);
  }

static void
get_value_int(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = (spinbutton_t*)(w->priv);


  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton),
                            (gdouble)(w->value.val_i));
  
  }

static void
set_value_int(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = (spinbutton_t*)(w->priv);

  w->value.val_i =
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->spinbutton));

  }

static void
get_value_float(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = (spinbutton_t*)(w->priv);
  
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton),
                            w->value.val_f);
  
  }

static void
set_value_float(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = (spinbutton_t*)(w->priv);

  w->value.val_f =
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(s->spinbutton));
  }


static void
attach(void * priv, GtkWidget * table, int * row, int * num_columns)
  {
  spinbutton_t * s = (spinbutton_t*)priv;

  if(*num_columns < 2)
    *num_columns = 2;

  gtk_table_resize(GTK_TABLE(table), *row + 1, *num_columns);

  gtk_table_attach(GTK_TABLE(table), s->label,
                   0, 1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
  
  gtk_table_attach(GTK_TABLE(table), s->spinbutton,
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  *row += 1;
  }

static const gtk_widget_funcs_t int_funcs =
  {
    .get_value = get_value_int,
    .set_value = set_value_int,
    .destroy = destroy,
    .attach =  attach
  };

static const gtk_widget_funcs_t float_funcs =
  {
    .get_value = get_value_float,
    .set_value = set_value_float,
    .destroy = destroy,
    .attach =  attach
  };

static void create_common(bg_gtk_widget_t * w,
                          const bg_parameter_info_t * info,
                          float min_value,
                          float max_value,
                          const char * translation_domain)
  {
  spinbutton_t * s = calloc(1, sizeof(*s));
  s->label = gtk_label_new(TR_DOM(info->long_name));
  
  gtk_widget_show(s->label);
  gtk_misc_set_alignment(GTK_MISC(s->label), 0.0, 0.5);
  s->adj =  gtk_adjustment_new(min_value, min_value, max_value,
                               1.0, 0.0, 0.0);
  s->spinbutton =
    gtk_spin_button_new(GTK_ADJUSTMENT(s->adj), 0.1, 0);
  if(info->flags & BG_PARAMETER_SYNC)
    {
    w->callback_id =
      g_signal_connect(G_OBJECT(s->spinbutton), "value-changed",
                       G_CALLBACK(bg_gtk_change_callback), (gpointer)w);
    w->callback_widget = s->spinbutton;
    }
  if(info->help_string)
    {
    bg_gtk_tooltips_set_tip(s->spinbutton,
                            info->help_string, translation_domain);
    }
  
  gtk_widget_show(s->spinbutton);
  gtk_widget_show(s->label);
  w->priv = s;
  }

void 
bg_gtk_create_int(bg_gtk_widget_t * w,
                  const bg_parameter_info_t * info,
                  const char * translation_domain)
  {
  float min_value;
  float max_value;
  
  spinbutton_t * s;

  min_value = (float)(info->val_min.val_i);
  max_value = (float)(info->val_max.val_i);

  if(min_value >= max_value)
    {
    min_value = -1.0;
    max_value = 1.0e9;
    }
  
  w->funcs = &int_funcs;
  create_common(w, info, min_value, max_value, translation_domain );
  s = (spinbutton_t *)(w->priv);

  
  }

void 
bg_gtk_create_float(bg_gtk_widget_t * w,
                    const bg_parameter_info_t * info,
                    const char * translation_domain)
  {
  float min_value;
  float max_value;
  
  spinbutton_t * s;

  min_value = (float)(info->val_min.val_f);
  max_value = (float)(info->val_max.val_f);

  if(min_value >= max_value)
    {
    min_value = 0.0;
    max_value = 100000.0;
    }
  w->funcs = &float_funcs;
  create_common(w, info, min_value, max_value, translation_domain );
  s = (spinbutton_t *)(w->priv);

  bg_gtk_change_callback_block(w, 1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton),
                             info->num_digits);
  bg_gtk_change_callback_block(w, 0);
  
  }
