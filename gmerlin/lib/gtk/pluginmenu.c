/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/pluginregistry.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>


static char const * const auto_string = TRS("Auto Select");

struct bg_gtk_plugin_menu_s
  {
  int auto_supported;
  GtkWidget * combo;
  GtkWidget * label;
  bg_plugin_registry_t * plugin_reg;
  int type_mask;
  int flag_mask;

  void (*callback)(bg_gtk_plugin_menu_t*, void*);
  void * callback_data;
  };

static void change_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_plugin_menu_t * m;
  m = (bg_gtk_plugin_menu_t*)data;
  if(m->callback)
    m->callback(m, m->callback_data);
  }

bg_gtk_plugin_menu_t *
bg_gtk_plugin_menu_create(int auto_supported,
                          bg_plugin_registry_t * plugin_reg,
                          int type_mask, int flag_mask)
  {
  int num, i;
  bg_gtk_plugin_menu_t * ret;

  const bg_plugin_info_t * plugin_info;
  
  ret = calloc(1, sizeof(*ret));
  ret->auto_supported = auto_supported;

  ret->combo = gtk_combo_box_new_text();

  g_signal_connect(G_OBJECT(ret->combo),
                   "changed", G_CALLBACK(change_callback),
                   (gpointer)ret);
  
  if(auto_supported)
    gtk_combo_box_append_text(GTK_COMBO_BOX(ret->combo), TR(auto_string));

  ret->plugin_reg = plugin_reg;
  ret->type_mask = type_mask;
  ret->flag_mask = flag_mask;
  
  num = bg_plugin_registry_get_num_plugins(ret->plugin_reg,
                                           ret->type_mask,
                                           ret->flag_mask);
  
  for(i = 0; i < num; i++)
    {
    plugin_info = bg_plugin_find_by_index(ret->plugin_reg, i,
                                          ret->type_mask,
                                          ret->flag_mask);
    
    bg_bindtextdomain(plugin_info->gettext_domain,
                      plugin_info->gettext_directory);
    gtk_combo_box_append_text(GTK_COMBO_BOX(ret->combo),
                              TRD(plugin_info->long_name,
                                  plugin_info->gettext_domain));
    }
  
  /* We always take the 0th option */
  gtk_combo_box_set_active(GTK_COMBO_BOX(ret->combo), 0);
    
  gtk_widget_show(ret->combo);
    
  ret->label = gtk_label_new(TR("Plugin: "));
  gtk_widget_show(ret->label);
  gtk_widget_show(ret->combo);
  
  return ret;
  }

const char * bg_gtk_plugin_menu_get_plugin(bg_gtk_plugin_menu_t * m)
  {
  int selected;
  const bg_plugin_info_t * plugin_info;
  
  selected = gtk_combo_box_get_active(GTK_COMBO_BOX(m->combo));
  
  if(m->auto_supported)
    {
    if(!selected)
      return (const char*)0;
    else
      {
      plugin_info = bg_plugin_find_by_index(m->plugin_reg, selected-1,
                                            m->type_mask,
                                            m->flag_mask);
      return plugin_info->name;
      }
    }
  else
    {
    plugin_info = bg_plugin_find_by_index(m->plugin_reg, selected,
                                          m->type_mask,
                                          m->flag_mask);
    return plugin_info->name;
    }
  }

GtkWidget * bg_gtk_plugin_menu_get_widget(bg_gtk_plugin_menu_t * m)
  {
  GtkWidget * ret;

  ret = gtk_hbox_new(0, 5);
  
  gtk_box_pack_start(GTK_BOX(ret), m->label, FALSE, FALSE, 0);
  bg_gtk_box_pack_start_defaults(GTK_BOX(ret), m->combo);
  gtk_widget_show(ret);
  return ret;
  }

void bg_gtk_plugin_menu_attach(bg_gtk_plugin_menu_t * m, GtkWidget * table,
                               int row,
                               int column)
  {
  gtk_table_attach(GTK_TABLE(table), m->label,
                   column, column+1, row, row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), m->combo,
                   column+1, column+2, row, row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  
  }


void bg_gtk_plugin_menu_destroy(bg_gtk_plugin_menu_t * m)
  {
  }

void bg_gtk_plugin_menu_set_change_callback(bg_gtk_plugin_menu_t * m,
                                            void (*callback)(bg_gtk_plugin_menu_t*, void*),
                                            void * callback_data)
  {
  m->callback = callback;
  m->callback_data = callback_data;
  }

