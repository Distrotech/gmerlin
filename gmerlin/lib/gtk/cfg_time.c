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
#include <config.h>

#include "gtk_dialog.h"
#include <utils.h>
#include <gui_gtk/gtkutils.h>

typedef struct
  {
  GtkWidget * label;
  GtkWidget * spinbutton_h;  /* Hours        */
  GtkWidget * spinbutton_m;  /* Minutes      */
  GtkWidget * spinbutton_s;  /* Seconds      */
  GtkWidget * spinbutton_ms; /* Milleseconds */
  GtkWidget * box;
  int no_change_callback;
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

static void set_time(spinbutton_t * s, gavl_time_t t)
  {
  int i;
  /* Fraction of a second */
  
  i = t % GAVL_TIME_SCALE;
  i /= (GAVL_TIME_SCALE / 1000);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_ms),
                            (gdouble)(i));

  /* Seconds */
  
  t /= GAVL_TIME_SCALE;
  
  i = t % 60;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_s),
                            (gdouble)(i));

  /* Minutes */

  t /= 60;
  
  i = t % 60;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_m),
                            (gdouble)(i));

  /* Hours */

  t /= 60;
  
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_h),
                            (gdouble)(t));

  }

static void
get_value(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = (spinbutton_t*)(w->priv);
  set_time(s, w->value.val_time);
  }

static gavl_time_t get_time(spinbutton_t * s)
  {
  gavl_time_t ret;
  int hours, minutes, seconds, milliseconds;
  
  milliseconds = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->spinbutton_ms));
  seconds      = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->spinbutton_s));
  minutes      = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->spinbutton_m));
  hours        = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->spinbutton_h));

  ret = hours;
  ret *= 60;
  ret += minutes;
  ret *= 60;
  ret += seconds;
  ret *= GAVL_TIME_SCALE;

  ret += milliseconds * (GAVL_TIME_SCALE / 1000);
  return ret;
  }

static void
set_value(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = (spinbutton_t*)(w->priv);
  w->value.val_time = get_time(s);
  }

static void change_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_widget_t * wid = (bg_gtk_widget_t *)data;
  spinbutton_t * s = (spinbutton_t*)(wid->priv);
  gavl_time_t t;
  int do_change = 0;

  
  if(s->no_change_callback)
    return;
  
  t = get_time(s);

  if(wid->info->val_max.val_time > wid->info->val_min.val_time)
    {
    
    if(t > wid->info->val_max.val_time)
      {
      do_change = 1;
      t = wid->info->val_max.val_time;
      }
    if(t < wid->info->val_min.val_time)
      {
      do_change = 1;
      t = wid->info->val_min.val_time;
      }
    }

  if(do_change)
    {
    s->no_change_callback = 1;
    set_time(s, t);
    s->no_change_callback = 0;
    }
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
  
  gtk_table_attach(GTK_TABLE(table), s->box,
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  *row += 1;
  }

static gtk_widget_funcs_t funcs =
  {
    get_value: get_value,
    set_value: set_value,
    destroy: destroy,
    attach:  attach
  };

void 
bg_gtk_create_time(bg_gtk_widget_t * w,
                   bg_parameter_info_t * info, const char * translation_domain)
  {
  char * tooltip;
  GtkWidget * label;
  spinbutton_t * s = calloc(1, sizeof(*s));
  s->label = gtk_label_new(TR_DOM(info->long_name));

  gtk_widget_show(s->label);
  gtk_misc_set_alignment(GTK_MISC(s->label), 0.0, 0.5);

  s->spinbutton_ms =
    gtk_spin_button_new_with_range(0.0, 999.0, 1.0);
  s->spinbutton_s =
    gtk_spin_button_new_with_range(0.0, 59.0, 1.0);
  s->spinbutton_m =
    gtk_spin_button_new_with_range(0.0, 59.0, 1.0);
  s->spinbutton_h =
    gtk_spin_button_new_with_range(0.0, 1000000.0, 1.0);
  
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_ms), 0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_s), 0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_m), 0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_h), 0);
  
  if(info->help_string)
    {
    tooltip = bg_sprintf(TR("%s (Hours)"), TR_DOM(info->help_string));
    bg_gtk_tooltips_set_tip(s->spinbutton_h, tooltip, PACKAGE);
    free(tooltip);

    tooltip = bg_sprintf(TR("%s (Minutes)"), TR_DOM(info->help_string));
    bg_gtk_tooltips_set_tip(s->spinbutton_m, tooltip, PACKAGE);
    free(tooltip);

    tooltip = bg_sprintf(TR("%s (Seconds)"), TR_DOM(info->help_string));
    bg_gtk_tooltips_set_tip(s->spinbutton_s, tooltip, PACKAGE);
    free(tooltip);

    tooltip = bg_sprintf(TR("%s (Milliseconds)"), TR_DOM(info->help_string));
    bg_gtk_tooltips_set_tip(s->spinbutton_ms, tooltip, PACKAGE);
    free(tooltip);
    }
  g_signal_connect(G_OBJECT(s->spinbutton_ms), "value-changed",
                   G_CALLBACK(change_callback), w);
  
  g_signal_connect(G_OBJECT(s->spinbutton_s), "value-changed",
                   G_CALLBACK(change_callback), w);
  
  g_signal_connect(G_OBJECT(s->spinbutton_m), "value-changed",
                   G_CALLBACK(change_callback), w);
  
  g_signal_connect(G_OBJECT(s->spinbutton_h), "value-changed",
                   G_CALLBACK(change_callback), w);
  
  
  gtk_widget_show(s->spinbutton_ms);
  gtk_widget_show(s->spinbutton_s);
  gtk_widget_show(s->spinbutton_m);
  gtk_widget_show(s->spinbutton_h);

  s->box = gtk_hbox_new(0, 2);

  label = gtk_label_new(TR("h:"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(s->box), label, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(s->box), s->spinbutton_h, FALSE, FALSE, 0);
  label = gtk_label_new(TR("m:"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(s->box), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(s->box), s->spinbutton_m, FALSE, FALSE, 0);
  label = gtk_label_new(TR("s:"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(s->box), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(s->box), s->spinbutton_s, FALSE, FALSE, 0);
  label = gtk_label_new(TR("ms:"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(s->box), label, FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(s->box), s->spinbutton_ms);
  
  gtk_widget_show(s->box);
  gtk_widget_show(s->label);
  w->priv = s;
  w->funcs = &funcs;
  }

