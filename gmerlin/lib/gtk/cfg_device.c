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

#include <config.h>
#include <stdio.h>
#include <string.h>

#include <gmerlin/utils.h>


#include "gtk_dialog.h"
#include <gui_gtk/gtkutils.h>

enum
{
  COLUMN_DEVICE,
  NUM_COLUMNS
};

typedef struct
  {
  GtkWidget    * treeview;
  
  GtkWidget * add_button;
  GtkWidget * delete_button;
  GtkWidget * scrolled;
  GtkWidget * fileselect;
  int selected;
  } device_t;

static void get_value(bg_gtk_widget_t * w)
  {
  int i;
  device_t * priv;
  GtkTreeSelection * selection;
  GtkTreeIter iter;
  GtkTreeModel * model;
  char ** tmp_items;
  
  priv = (device_t*)(w->priv);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));

  gtk_list_store_clear(GTK_LIST_STORE(model));
  
  if(!w->value.val_str || (*w->value.val_str == '\0'))
    return;
  
  tmp_items = bg_strbreak(w->value.val_str, ' ');
  

  i = 0;
  
  while(tmp_items[i])
    {
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       COLUMN_DEVICE,
                       tmp_items[i],
                       -1);
    if(!i)
      gtk_tree_selection_select_iter(selection, &iter);
    
    i++;
    }
  bg_strbreak_free(tmp_items);
  }

static void set_value(bg_gtk_widget_t * w)
  {
  device_t * priv;
  GtkTreeSelection * selection;
  GtkTreeIter iter;
  GtkTreeModel * model;

  int num_entries;
  int selected;
  char ** strings;
  int len;
  int index;
  int i, imax;
  
  priv = (device_t*)(w->priv);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
  
  if(!gtk_tree_model_get_iter_first(model, &iter))
    {
    if(w->value.val_str)
      {
      free(w->value.val_str);
      w->value.val_str = NULL;
      }
    return;
    }
  num_entries = 1;
  while(gtk_tree_model_iter_next(model, &iter))
    num_entries++;

  /* Copy the strings */
    
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));

  strings = malloc(num_entries * sizeof(char*));

  len = 0;
  selected = -1;

  gtk_tree_model_get_iter_first(model, &iter);
  for(i = 0; i < num_entries; i++)
    {
    gtk_tree_model_get(model, &iter,
                       COLUMN_DEVICE,
                       &strings[i],
                       -1);
    if(gtk_tree_selection_iter_is_selected(selection,
                                           &iter))
      {
      selected = i;
      }
    gtk_tree_model_iter_next(model, &iter);
    len += strlen(strings[i])+1;
    }

  /* Copy to the value */
  
  if(w->value.val_str)
    free(w->value.val_str);
  w->value.val_str = malloc(len);
  w->value.val_str[0] = '\0';

  
  if(selected >= 0)
    {
    strcat(w->value.val_str, strings[selected]);
    if(num_entries > 1)
      strcat(w->value.val_str, " ");
    imax = num_entries-1;
    }
  else
    imax = num_entries;
  index = 0;
  
  for(i = 0; i < imax; i++)
    {
    if(index == selected)
      index++;
    strcat(w->value.val_str, strings[index]);
    if(i < imax - 1)
      strcat(w->value.val_str, ":");
    index++;
    }
  for(i = 0; i < num_entries; i++)
    {
    g_free(strings[i]);
    }
  free(strings);
  }

static void destroy(bg_gtk_widget_t * w)
  {
  device_t * priv = (device_t*)w->priv;
  if(priv->fileselect)
    gtk_widget_destroy(priv->fileselect);
  free(priv);
  }

static void attach(void * priv, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  device_t * f = (device_t*)priv;
  if(*num_columns < 3)
    *num_columns = 3;

  gtk_table_resize(GTK_TABLE(table), *row+3, *num_columns);

  gtk_table_attach_defaults(GTK_TABLE(table), f->scrolled,
                            0, 2, *row, *row+3);

  gtk_table_attach(GTK_TABLE(table), f->add_button,
                   2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), f->delete_button,
                   2, 3, *row+1, *row+2, GTK_FILL, GTK_SHRINK, 0, 0);
  *row += 3;
  }

static const gtk_widget_funcs_t funcs =
  {
    .get_value = get_value,
    .set_value = set_value,
    .destroy =   destroy,
    .attach =    attach
  };

static void
fileselect_callback(GtkWidget *chooser,
                    gint       response_id,
                    gpointer data)
  {
  device_t * priv;
  GtkTreeIter iter;
  gchar *uri;
  priv = (device_t*)data;
  
  if(response_id == GTK_RESPONSE_OK)
    {
    GtkTreeModel * model;
    uri = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (chooser));
    
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
    
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       COLUMN_DEVICE,
                       uri,
                       -1);
    g_free (uri);
    }
  //  gtk_widget_destroy (chooser);
  
  gtk_widget_hide(priv->fileselect);
  gtk_main_quit();
  }

static gboolean
fileselect_delete_callback(GtkWidget * w, GdkEventAny * event,
                           gpointer data)
  {
  fileselect_callback(w, GTK_RESPONSE_CANCEL, data);
  return TRUE;
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  device_t * priv;
  GtkTreeSelection * selection;
  GtkTreeModel * model;
  GtkWidget * toplevel;
  GtkTreeIter iter;
  
  priv = (device_t*)data;

  if(w == priv->add_button)
    {
    if(!priv->fileselect)
      {
      toplevel = bg_gtk_get_toplevel(w);
      
      priv->fileselect =
        gtk_file_chooser_dialog_new (TRD("Select a device", PACKAGE),
                                     GTK_WINDOW(toplevel),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     GTK_STOCK_CANCEL,
                                     GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OK, GTK_RESPONSE_OK,
                                     NULL);
      
      gtk_window_set_modal(GTK_WINDOW(priv->fileselect), TRUE);

      g_signal_connect(priv->fileselect, "response",
                       G_CALLBACK(fileselect_callback),
                       (gpointer)priv);
      g_signal_connect(G_OBJECT(priv->fileselect), "delete_event",
                       G_CALLBACK(fileselect_delete_callback),
                       (gpointer)priv);
      }
    gtk_widget_show(priv->fileselect);
    gtk_main();
    }
  else if(w == priv->delete_button)
    {
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
    if(!gtk_tree_selection_get_selected(selection, &model, &iter))
      return;
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    }
  }

void bg_gtk_create_device(bg_gtk_widget_t * w,
                          const char * translation_domain)
  {
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection * selection;
  device_t * priv = calloc(1, sizeof(*priv));
  
  priv->selected = -1;
  
  store = gtk_list_store_new (NUM_COLUMNS,
			      G_TYPE_STRING);

  priv->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  
  if(w->info->help_string)
    {
    bg_gtk_tooltips_set_tip(priv->treeview,
                            w->info->help_string, translation_domain);
    }

  
  /* Add the columns */

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Devices",
						     renderer,
						     "text",
						     COLUMN_DEVICE,
						     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_DEVICE);
  gtk_tree_view_append_column (GTK_TREE_VIEW(priv->treeview), column);

  /* Set Selection mode */
  
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  
  priv->add_button = gtk_button_new_with_label(TR("Add..."));
  priv->delete_button = gtk_button_new_with_label(TR("Delete"));
  
  g_signal_connect(G_OBJECT(priv->add_button), "clicked",
                   G_CALLBACK(button_callback),
                   (gpointer)priv);
  g_signal_connect(G_OBJECT(priv->delete_button), "clicked",
                   G_CALLBACK(button_callback),
                   (gpointer)priv);
  
  gtk_widget_show(priv->add_button);
  gtk_widget_show(priv->delete_button);
    
  gtk_widget_show(priv->treeview);

  priv->scrolled =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(priv->treeview)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(priv->treeview)));
  
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(priv->scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(priv->scrolled), priv->treeview);
  gtk_widget_show(priv->scrolled);
  w->funcs = &funcs;
  w->priv = priv;
  }
