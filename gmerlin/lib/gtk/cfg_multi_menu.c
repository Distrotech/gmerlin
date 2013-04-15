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
#include <string.h>

#include "gtk_dialog.h"
#include <gmerlin/utils.h>
#include <gui_gtk/multiinfo.h>
#include <gui_gtk/gtkutils.h>

typedef struct
  {
  GtkWidget * label;
  GtkWidget * combo;
  GtkWidget * config_button;
  GtkWidget * info_button;
  
  bg_set_parameter_func_t  set_param;
  bg_get_parameter_func_t  get_param;
  void * data;
  int selected;
  const char * translation_domain;
  } multi_menu_t;


static void get_value(bg_gtk_widget_t * w)
  {
  int i;
  multi_menu_t * priv;
  priv = (multi_menu_t*)(w->priv);

  if(w->info->multi_names)
    {
    i = bg_parameter_get_selected(w->info, 
                                  w->value.val_str);
    gtk_combo_box_set_active(GTK_COMBO_BOX(priv->combo), i);
    }
  
  }

static void set_value(bg_gtk_widget_t * w)
  {
  multi_menu_t * priv;
  
  priv = (multi_menu_t*)(w->priv);
  
  if(w->info->multi_names)
    {
    w->value.val_str = gavl_strrep(w->value.val_str,
                                 w->info->multi_names[priv->selected]);
    }
  else
    w->value.val_str = gavl_strrep(w->value.val_str,
                                   NULL);
    
  }

static void destroy(bg_gtk_widget_t * w)
  {
  multi_menu_t * priv = (multi_menu_t*)(w->priv);
  free(priv);
  }

static void attach(void * priv, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  GtkWidget * box;
  multi_menu_t * s = (multi_menu_t*)priv;
  
  if(*num_columns < 3)
    *num_columns = 3;

  box = gtk_hbox_new(0, 5);
    
  gtk_table_resize(GTK_TABLE(table), *row+1, *num_columns);
  //  gtk_table_attach_defaults(GTK_TABLE(table), b->button,
  //                            0, 1, *row, *row+1);
  gtk_table_attach(GTK_TABLE(table), s->label,
                    0, 1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
  
  gtk_table_attach(GTK_TABLE(table), s->combo,
                   1, 2, *row, *row+1, GTK_FILL|GTK_EXPAND, GTK_SHRINK, 0, 0);

  bg_gtk_box_pack_start_defaults(GTK_BOX(box), s->config_button);
  bg_gtk_box_pack_start_defaults(GTK_BOX(box), s->info_button);
  gtk_widget_show(box);
    
  gtk_table_attach(GTK_TABLE(table), box,
                   2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
  
  
  (*row)++;
  }

static const gtk_widget_funcs_t funcs =
  {
    .get_value = get_value,
    .set_value = set_value,
    .destroy =   destroy,
    .attach =    attach
  };

static GtkWidget * create_pixmap_button(const char * filename)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();

  gtk_widget_show(image);
  button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);
  return button;
  }

static void combo_box_change_callback(GtkWidget * wid, gpointer data)
  {
  bg_gtk_widget_t * w;
  multi_menu_t * priv;
  
  w = (bg_gtk_widget_t *)data;
  priv = (multi_menu_t *)(w->priv);
  priv->selected = gtk_combo_box_get_active(GTK_COMBO_BOX(priv->combo));
  if(w->info->multi_parameters && w->info->multi_parameters[priv->selected])
    gtk_widget_set_sensitive(priv->config_button, 1);
  else
    gtk_widget_set_sensitive(priv->config_button, 0);
  
  if(w->info->flags & BG_PARAMETER_SYNC)
    bg_gtk_change_callback(NULL, w);
  
  }

static void button_callback(GtkWidget * wid, gpointer data)
  {
  bg_gtk_widget_t * w;
  multi_menu_t * priv;
  bg_dialog_t * dialog;
  const char * label;

  bg_cfg_section_t * subsection;
    
  w = (bg_gtk_widget_t *)data;
  priv = (multi_menu_t *)(w->priv);

  if(wid == priv->info_button)
    {
    bg_gtk_multi_info_show(w->info, priv->selected,
                           priv->translation_domain, priv->info_button);
    }
  else if(wid == priv->config_button)
    {
    subsection = bg_cfg_section_find_subsection(w->cfg_section, w->info->name);
    subsection = bg_cfg_section_find_subsection(subsection,
                                                w->info->multi_names[priv->selected]);
    
    if(w->info->multi_labels && w->info->multi_labels[priv->selected])
      label = TRD(w->info->multi_labels[priv->selected], priv->translation_domain);
    else
      label = w->info->multi_names[priv->selected];

    if(priv->get_param)
      bg_cfg_section_get(subsection, w->info->multi_parameters[priv->selected],
                         priv->get_param, priv->data);
    
    dialog = bg_dialog_create(subsection, priv->set_param, priv->get_param,
                              priv->data,
                              w->info->multi_parameters[priv->selected],
                              label);
    bg_dialog_show(dialog, priv->config_button);
    }
  }

void bg_gtk_create_multi_menu(bg_gtk_widget_t * w,
                              bg_set_parameter_func_t set_param,
                              bg_get_parameter_func_t get_param,
                              void * data, const char * translation_domain)
  {
  int i;
  multi_menu_t * priv = calloc(1, sizeof(*priv));

  priv->set_param   = set_param;
  priv->get_param   = get_param;
  priv->data        = data;
  
  priv->translation_domain = translation_domain;
  
  w->funcs = &funcs;
  w->priv = priv;
  
  priv->config_button = create_pixmap_button("config_16.png");
  priv->info_button   = create_pixmap_button("info_16.png");

  g_signal_connect(G_OBJECT(priv->config_button), "clicked",
                   G_CALLBACK(button_callback), w);
  g_signal_connect(G_OBJECT(priv->info_button), "clicked",
                   G_CALLBACK(button_callback), w);
  
  gtk_widget_show(priv->config_button);
  gtk_widget_show(priv->info_button);

  priv->combo = bg_gtk_combo_box_new_text();

  if(w->info->help_string)
    {
    bg_gtk_tooltips_set_tip(priv->combo,
                            w->info->help_string, translation_domain);
    }
  
  i = 0;

  if(w->info->multi_names)
    {
    while(w->info->multi_names[i])
      {
      if(w->info->multi_labels && w->info->multi_labels[i])
        bg_gtk_combo_box_append_text(priv->combo,
                                  TR_DOM(w->info->multi_labels[i]));
      else
        bg_gtk_combo_box_append_text(priv->combo,
                                  w->info->multi_names[i]);
      i++;
      }
    g_signal_connect(G_OBJECT(priv->combo),
                     "changed", G_CALLBACK(combo_box_change_callback),
                     (gpointer)w);
    }
  else
    {
    gtk_widget_set_sensitive(priv->config_button, 0);
    gtk_widget_set_sensitive(priv->info_button, 0);
    }
  
  gtk_widget_show(priv->combo);

  
  
  priv->label = gtk_label_new(TR_DOM(w->info->long_name));
  gtk_misc_set_alignment(GTK_MISC(priv->label), 0.0, 0.5);

  gtk_widget_show(priv->label);
  }
