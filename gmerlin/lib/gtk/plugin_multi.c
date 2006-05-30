/*****************************************************************
 
  plugin_mutli.c
 
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>

#include <pluginregistry.h>
#include <utils.h>
#include <cfg_dialog.h>

#include <gui_gtk/plugin.h>

enum
{
  COLUMN_PLUGIN,
  NUM_COLUMNS
};

struct bg_gtk_plugin_widget_multi_s
  {
  GtkWidget * info_button;
  GtkWidget * config_button;
  GtkWidget * up_button;
  GtkWidget * down_button;
  
  GtkWidget * treeview;
  GtkWidget * table;
  GtkWidget * mimetypes;
  GtkWidget * extensions;
  
  bg_plugin_registry_t * reg;
  const bg_plugin_info_t * info;

  bg_parameter_info_t * parameters;
  bg_cfg_section_t * section;

  gulong extensions_changed_id;
  gulong mimetypes_changed_id;
  
  };

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_plugin_widget_multi_t * win;
  GtkTreeSelection * selection;
  bg_dialog_t * dialog;
  GtkTreeModel * model;
  GtkTreeIter iter;
  GtkTreePath * path;
  char * name, * sort_string;
  const bg_plugin_info_t * info;
  
  win = (bg_gtk_plugin_widget_multi_t *)data;

  if(w == win->info_button)
    {
    bg_gtk_plugin_info_show(win->info);
    }
  else if(w == win->config_button)
    {
    dialog = bg_dialog_create(win->section,
                              NULL, NULL,
                              win->parameters,
                              win->info->long_name);
    bg_dialog_show(dialog);
    bg_dialog_destroy(dialog);
    }
  else /* Up / Down */
    {
    selection =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(win->treeview));

    if(!gtk_tree_selection_get_selected(selection, &model, &iter))
      return;
    
    if(w == win->up_button)
      {
      gtk_list_store_move_after(GTK_LIST_STORE(model),
                                &iter,
                                (GtkTreeIter*)0);
      
      }
    if(w == win->down_button)
      {
      gtk_list_store_move_before(GTK_LIST_STORE(model),
                                 &iter,
                                 (GtkTreeIter*)0);
      }

    /* Scroll to the cell */

    path = gtk_tree_model_get_path(model, &iter);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(win->treeview),
                                 path,
                                 (GtkTreeViewColumn *)0,
                                 0, 0.0, 0.0);
    gtk_tree_path_free(path);

    /* Sort the plugins */
    sort_string = (char*)0;
    
    if(!gtk_tree_model_get_iter_first(model, &iter))
      {
      return;
      }
    while(1)
      {
      gtk_tree_model_get(model, &iter, COLUMN_PLUGIN, &name, -1);

      info = bg_plugin_find_by_long_name(win->reg, name);

      g_free(name);

      sort_string = bg_strcat(sort_string, info->name);
      if(!gtk_tree_model_iter_next(model, &iter))
        break;
      sort_string = bg_strcat(sort_string, ",");

      }
    fprintf(stderr, "Sort string: %s\n", sort_string);
    bg_plugin_registry_sort(win->reg, sort_string);
    }
  }

static void select_row_callback(GtkTreeSelection * s, gpointer data)
  {
  GtkTreeModel * model;
  GtkTreeIter iter;
  
  char * long_name;

  bg_gtk_plugin_widget_multi_t * win = (bg_gtk_plugin_widget_multi_t*)data;

  if(!gtk_tree_selection_get_selected(s, &model, &iter))
    return;
  
  gtk_tree_model_get(model, &iter, COLUMN_PLUGIN,
                     &long_name, -1);
#if 0
  if(win->info)
    {
    if(win->info->flags & BG_PLUGIN_FILE)
      {
      bg_plugin_registry_set_extensions(win->reg, win->info->name,
                                        gtk_entry_get_text(GTK_ENTRY(win->extensions)));
      
      }
    if(win->info->flags & BG_PLUGIN_URL)
      {
      bg_plugin_registry_set_mimetypes(win->reg, win->info->name,
                                       gtk_entry_get_text(GTK_ENTRY(win->mimetypes)));
      }
    }
#endif
  
  win->info = bg_plugin_find_by_long_name(win->reg, long_name);

  if(win->info->parameters)
    win->parameters = win->info->parameters;
  else
    win->parameters = (bg_parameter_info_t*)0;
  
  win->section = bg_plugin_registry_get_section(win->reg, win->info->name);
  
  if(win->parameters)
    gtk_widget_set_sensitive(win->config_button, 1);
  else
    gtk_widget_set_sensitive(win->config_button, 0);

  g_signal_handler_block(G_OBJECT(win->extensions), win->extensions_changed_id);
  if(win->info->flags & BG_PLUGIN_FILE)
    {
    gtk_entry_set_text(GTK_ENTRY(win->extensions), win->info->extensions);
    gtk_widget_set_sensitive(win->extensions, 1);
    }
  else
    {
    gtk_entry_set_text(GTK_ENTRY(win->extensions), "");
    gtk_widget_set_sensitive(win->extensions, 0);
    }
  g_signal_handler_unblock(G_OBJECT(win->extensions), win->extensions_changed_id);

  
  g_signal_handler_block(G_OBJECT(win->mimetypes), win->mimetypes_changed_id);
  if(win->info->flags & BG_PLUGIN_URL)
    {
    gtk_entry_set_text(GTK_ENTRY(win->mimetypes), win->info->mimetypes);
    gtk_widget_set_sensitive(win->mimetypes, 1);
    }
  else
    {
    gtk_entry_set_text(GTK_ENTRY(win->mimetypes), "");
    gtk_widget_set_sensitive(win->mimetypes, 0);
    }
  g_signal_handler_unblock(G_OBJECT(win->mimetypes), win->mimetypes_changed_id);
  gtk_widget_set_sensitive(win->info_button, 1);
  

  g_free(long_name);
  }

static void entry_change_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_plugin_widget_multi_t * win = (bg_gtk_plugin_widget_multi_t*)data;
  fprintf(stderr, "entry_change_callback\n");
  if(w == win->extensions)
    {
    bg_plugin_registry_set_extensions(win->reg, win->info->name,
                                      gtk_entry_get_text(GTK_ENTRY(win->extensions)));
    }
  else if(w == win->mimetypes)
    {
    bg_plugin_registry_set_mimetypes(win->reg, win->info->name,
                                     gtk_entry_get_text(GTK_ENTRY(win->mimetypes)));
    
    }
  }

static GtkWidget * create_pixmap_button(const char * filename, GtkTooltips * tooltips,
                                        const char * tooltip, const char * tooltip_private)
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

  gtk_tooltips_set_tip(tooltips, button, tooltip, tooltip_private);

  return button;
  }

bg_gtk_plugin_widget_multi_t *
bg_gtk_plugin_widget_multi_create(bg_plugin_registry_t * reg,
                                  uint32_t type_mask,
                                  uint32_t flag_mask,
                                  GtkTooltips * tooltips)
  {
  bg_gtk_plugin_widget_multi_t * ret;
  GtkListStore *store;
  GtkTreeViewColumn *column;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkWidget * label;
  GtkWidget * scrolled;
  GtkTreeSelection * selection;
  
  const bg_plugin_info_t * info;
  
  int num_plugins, i;
  
  ret = calloc(1,sizeof(*ret));

  ret->reg = reg;

  /* Create buttons */

  ret->info_button = create_pixmap_button("info_16.png", tooltips, "Plugin info", "Plugin info");
  
  ret->config_button = create_pixmap_button("config_16.png", tooltips,
                                            "Plugin options", "Plugin options");
                                                                                
  ret->up_button = create_pixmap_button("top_16.png", tooltips, "Move plugin to top",
                                        "Move plugin to top");
  
  ret->down_button = create_pixmap_button("bottom_16.png", tooltips, "Move plugin to bottom",
                                        "Move plugin to bottom");
                                                                                
  g_signal_connect(G_OBJECT(ret->info_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->config_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->up_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->down_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)ret);
  
  gtk_widget_show(ret->info_button);
  gtk_widget_show(ret->config_button);
  gtk_widget_show(ret->up_button);
  gtk_widget_show(ret->down_button);
    
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
    if(!info)
      fprintf(stderr, "No more plugins\n");
    else
      {
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter,
                         COLUMN_PLUGIN,
                         info->long_name,
                         -1);
      }
    }

  /* Create entries */

  ret->extensions = gtk_entry_new();
  ret->mimetypes = gtk_entry_new();

  ret->extensions_changed_id =
    g_signal_connect(G_OBJECT(ret->extensions),
                     "changed", G_CALLBACK(entry_change_callback),
                     (gpointer)ret);
  
  ret->mimetypes_changed_id =
    g_signal_connect(G_OBJECT(ret->mimetypes),
                     "changed", G_CALLBACK(entry_change_callback),
                     (gpointer)ret);
  

  gtk_widget_show(ret->mimetypes);
  gtk_widget_show(ret->extensions);
    
  /* Pack stuff */
  
  ret->table = gtk_table_new(4, 3, 0);
  gtk_container_set_border_width(GTK_CONTAINER(ret->table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(ret->table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(ret->table), 5);
 
  gtk_table_attach_defaults(GTK_TABLE(ret->table), scrolled, 0, 1, 0, 4);

  gtk_table_attach_defaults(GTK_TABLE(ret->table),
                            ret->config_button, 1, 2, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),
                            ret->info_button, 1, 2, 1, 2);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),
                            ret->up_button, 1, 2, 2, 3);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),
                            ret->down_button, 1, 2, 3, 4);
    
  label = gtk_label_new("Mimetypes");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(ret->table), label, 2, 3, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->mimetypes, 2, 3, 1, 2);
  
  label = gtk_label_new("Extensions");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  gtk_table_attach_defaults(GTK_TABLE(ret->table),
                            label, 2, 3, 2, 3);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),
                            ret->extensions, 2, 3, 3, 4);
  
  gtk_widget_show(ret->table);
    
  /* Make things insensitive, because nothing is selected so far */

  gtk_widget_set_sensitive(ret->mimetypes, 0);
  gtk_widget_set_sensitive(ret->extensions, 0);
  gtk_widget_set_sensitive(ret->config_button, 0);
  gtk_widget_set_sensitive(ret->info_button, 0);
  
  return ret;
  }

GtkWidget *
bg_gtk_plugin_widget_multi_get_widget(bg_gtk_plugin_widget_multi_t * w)
  {
  return w->table;
  }

void bg_gtk_plugin_widget_multi_destroy(bg_gtk_plugin_widget_multi_t * w)
  {
  free(w);
  }
