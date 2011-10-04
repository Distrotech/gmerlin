/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>

#include <gmerlin/translation.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>
#include <gmerlin/cfg_dialog.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>

enum
{
  COLUMN_PLUGIN,
  NUM_COLUMNS
};

struct bg_gtk_plugin_widget_multi_s
  {
  GtkWidget * info_button;
  GtkWidget * config_button;
  
  GtkWidget * treeview;
  GtkWidget * widget;
  GtkWidget * protocols;
  GtkWidget * extensions;
  GtkWidget * priority;
  
  bg_plugin_registry_t * reg;
  const bg_plugin_info_t * info;

  bg_parameter_info_t * parameters;

  bg_cfg_section_t * section;

  gulong extensions_changed_id;
  gulong protocols_changed_id;
  gulong priority_changed_id;

  uint32_t flag_mask;
  uint32_t type_mask;
  
  };

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_plugin_widget_multi_t * win;
  bg_dialog_t * dialog;
  
  win = (bg_gtk_plugin_widget_multi_t *)data;

  if(w == win->info_button)
    {
    bg_gtk_plugin_info_show(win->info, win->info_button);
    }
  else if(w == win->config_button)
    {
    dialog = bg_dialog_create(win->section,
                              NULL, NULL, NULL,
                              win->info->parameters,
                              TRD(win->info->long_name, win->info->gettext_domain));
    bg_dialog_show(dialog, win->config_button);
    bg_dialog_destroy(dialog);
    }
  }

static void select_row_callback(GtkTreeSelection * s, gpointer data)
  {
  GtkTreeModel * model;
  GtkTreeIter iter;

  int selected, i;
  
  bg_gtk_plugin_widget_multi_t * win = (bg_gtk_plugin_widget_multi_t*)data;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->treeview));

  if(!gtk_tree_model_get_iter_first(model, &iter))
    return;
  selected = -1;
  i = 0;
  while(1)
    {
    if(gtk_tree_selection_iter_is_selected(s, &iter))
      {
      selected = i;
      break;
      }
    gtk_tree_model_iter_next(model, &iter);
    i++;
    }
  if(selected == -1)
    return;
  
  win->info = bg_plugin_find_by_index(win->reg, selected, win->type_mask,
                                      win->flag_mask);
  
  if(win->info->parameters)
    win->parameters = win->info->parameters;
  else
    win->parameters = NULL;
  
  win->section = bg_plugin_registry_get_section(win->reg, win->info->name);
  
  if(win->parameters)
    gtk_widget_set_sensitive(win->config_button, 1);
  else
    gtk_widget_set_sensitive(win->config_button, 0);

  if(win->extensions)
    {
    g_signal_handler_block(G_OBJECT(win->extensions),
                           win->extensions_changed_id);
    if((win->info->flags & BG_PLUGIN_FILE) && win->info->extensions)
      {
      gtk_entry_set_text(GTK_ENTRY(win->extensions),
                         win->info->extensions);
      gtk_widget_set_sensitive(win->extensions, 1);
      }
    else
      {
      gtk_entry_set_text(GTK_ENTRY(win->extensions), "");
      gtk_widget_set_sensitive(win->extensions, 0);
      }
    g_signal_handler_unblock(G_OBJECT(win->extensions),
                             win->extensions_changed_id);

    }
  if(win->protocols)
    {
    g_signal_handler_block(G_OBJECT(win->protocols),
                           win->protocols_changed_id);
    if(win->info->flags & BG_PLUGIN_URL)
      {
      gtk_entry_set_text(GTK_ENTRY(win->protocols),
                         win->info->protocols);
      gtk_widget_set_sensitive(win->protocols, 1);
      }
    else if(win->info->flags & (BG_PLUGIN_REMOVABLE | BG_PLUGIN_TUNER))
      {
      gtk_entry_set_text(GTK_ENTRY(win->protocols),
                         win->info->protocols);
      gtk_widget_set_sensitive(win->protocols, 0);
      }
    else
      {
      gtk_entry_set_text(GTK_ENTRY(win->protocols), "");
      gtk_widget_set_sensitive(win->protocols, 0);
      }
    g_signal_handler_unblock(G_OBJECT(win->protocols),
                             win->protocols_changed_id);

    }

  
  gtk_widget_set_sensitive(win->info_button, 1);
  
  if(win->priority)
    {
    g_signal_handler_block(G_OBJECT(win->priority), win->priority_changed_id);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->priority), win->info->priority);
    g_signal_handler_unblock(G_OBJECT(win->priority), win->priority_changed_id);
    
    if(win->info->flags & (BG_PLUGIN_URL | BG_PLUGIN_FILE))
      gtk_widget_set_sensitive(win->priority, 1);
    else
      gtk_widget_set_sensitive(win->priority, 0);
    }
  }

static void change_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_plugin_widget_multi_t * win = (bg_gtk_plugin_widget_multi_t*)data;
  if(w == win->extensions)
    {
    bg_plugin_registry_set_extensions(win->reg, win->info->name,
                                      gtk_entry_get_text(GTK_ENTRY(win->extensions)));
    }
  else if(w == win->protocols)
    {
    bg_plugin_registry_set_protocols(win->reg, win->info->name,
                                     gtk_entry_get_text(GTK_ENTRY(win->protocols)));
    
    }
  else if(w == win->priority)
    {
    bg_plugin_registry_set_priority(win->reg, win->info->name,
                                    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->priority)));
    }
  }

static GtkWidget * create_pixmap_button(const char * filename,
                                        const char * tooltip)
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

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);

  return button;
  }

bg_gtk_plugin_widget_multi_t *
bg_gtk_plugin_widget_multi_create(bg_plugin_registry_t * reg,
                                  uint32_t type_mask,
                                  uint32_t flag_mask)
  {
  bg_gtk_plugin_widget_multi_t * ret;
  GtkListStore *store;
  GtkTreeViewColumn *column;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkWidget * label;
  GtkWidget * scrolled;
  GtkWidget * table;
  GtkTreeSelection * selection;
  GtkWidget * hbox;
  
  const bg_plugin_info_t * info;
  
  int num_plugins, i;
  
  ret = calloc(1,sizeof(*ret));

  ret->reg = reg;
  
  ret->type_mask = type_mask;
  ret->flag_mask = flag_mask;
  
  /* Create buttons */

  ret->info_button = create_pixmap_button("info_16.png", TRS("Plugin info"));
  
  ret->config_button = create_pixmap_button("config_16.png",
                                            TRS("Plugin options"));
  
  g_signal_connect(G_OBJECT(ret->info_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->config_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)ret);
  
  gtk_widget_show(ret->info_button);
  gtk_widget_show(ret->config_button);
    
  /* Create list */

  store = gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING);
  ret->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->treeview));
  
  g_signal_connect(G_OBJECT(selection),
                   "changed", G_CALLBACK(select_row_callback),
                   (gpointer)ret);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Installed Plugins",
                                                     renderer,
                                                     "text",
                                                     COLUMN_PLUGIN,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_PLUGIN);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview), column);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ret->treeview), FALSE);
  
  gtk_widget_show(ret->treeview);

  scrolled =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(ret->treeview)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ret->treeview)));
  
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(scrolled), ret->treeview);
  gtk_widget_show(scrolled);

  
  /* Add Plugins */

  num_plugins = bg_plugin_registry_get_num_plugins(reg,
                                                   type_mask,
                                                   flag_mask);
  
  for(i = 0; i < num_plugins; i++)
    {
    info = bg_plugin_find_by_index(reg, i, type_mask, flag_mask);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       COLUMN_PLUGIN,
                       TRD(info->long_name, info->gettext_domain),
                       -1);
    }

  /* Create entries */

  if(type_mask & (BG_PLUGIN_INPUT | BG_PLUGIN_IMAGE_READER))
    {
    ret->extensions = gtk_entry_new();
    ret->protocols = gtk_entry_new();
    ret->priority = gtk_spin_button_new_with_range(BG_PLUGIN_PRIORITY_MIN,
                                                   BG_PLUGIN_PRIORITY_MAX, 1.0);
  
    ret->extensions_changed_id =
      g_signal_connect(G_OBJECT(ret->extensions),
                       "changed", G_CALLBACK(change_callback),
                       (gpointer)ret);
  
    ret->protocols_changed_id =
      g_signal_connect(G_OBJECT(ret->protocols),
                       "changed", G_CALLBACK(change_callback),
                       (gpointer)ret);

    ret->priority_changed_id =
      g_signal_connect(G_OBJECT(ret->priority),
                       "value-changed", G_CALLBACK(change_callback),
                       (gpointer)ret);
  

    gtk_widget_show(ret->protocols);
    gtk_widget_show(ret->extensions);
    gtk_widget_show(ret->priority);
    }
  
    
  /* Pack stuff */
  
  table = gtk_table_new(5, 4, 0);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);

  
  
  //   gtk_table_attach_defaults(GTK_TABLE(table), scrolled, 0, 1, 0, 5);

  gtk_table_attach(GTK_TABLE(table),
                   ret->config_button, 0, 1, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);

  if(ret->priority)
    {
    gtk_table_attach(GTK_TABLE(table),
                     ret->info_button, 1, 2, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
    
    label = gtk_label_new(TR("Priority"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL,
                     GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(table), ret->priority, 3, 4, 0, 1, GTK_FILL|GTK_EXPAND,
                     GTK_SHRINK, 0, 0);
    
    
    
    label = gtk_label_new(TR("Protocols"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 0, 4, 1, 2, GTK_FILL|GTK_EXPAND,
                     GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(table), ret->protocols, 0, 4, 2, 3, GTK_FILL|GTK_EXPAND,
                     GTK_SHRINK, 0, 0);
    
    label = gtk_label_new(TR("Extensions"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_widget_show(label);
    
    gtk_table_attach(GTK_TABLE(table), label, 0, 4, 3, 4, GTK_FILL|GTK_EXPAND, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   ret->extensions, 0, 4, 4, 5, GTK_FILL|GTK_EXPAND, GTK_SHRINK, 0, 0);
    }
  else
    {
    gtk_table_attach(GTK_TABLE(table),
                     ret->info_button, 0, 1, 1, 2,
                     GTK_FILL, GTK_SHRINK, 0, 0);

    }
  
  
  gtk_widget_show(table);

  hbox = gtk_hpaned_new();
  
  gtk_paned_add1(GTK_PANED(hbox), scrolled);
  gtk_paned_add2(GTK_PANED(hbox), table);
  gtk_widget_show(hbox);
  ret->widget = hbox;
  /* Make things insensitive, because nothing is selected so far */

  if(ret->priority)
    gtk_widget_set_sensitive(ret->priority, 0);
  if(ret->protocols) 
    gtk_widget_set_sensitive(ret->protocols, 0);

  if(ret->extensions)
    gtk_widget_set_sensitive(ret->extensions, 0);

  gtk_widget_set_sensitive(ret->config_button, 0);
  gtk_widget_set_sensitive(ret->info_button, 0);
  
  return ret;
  }

GtkWidget *
bg_gtk_plugin_widget_multi_get_widget(bg_gtk_plugin_widget_multi_t * w)
  {
  return w->widget;
  }

void bg_gtk_plugin_widget_multi_destroy(bg_gtk_plugin_widget_multi_t * w)
  {
  free(w);
  }
