/*****************************************************************
 
  cfg_device.c
 
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

#include <stdio.h>
#include <string.h>

#include "gtk_dialog.h"

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
  int num_entries;
  char * tmp_string;

  char *pos, *end = (char*)0;
  
  priv = (device_t*)(w->priv);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));

  gtk_list_store_clear(GTK_LIST_STORE(model));
  
  if(!w->value.val_str || (*w->value.val_str == '\0'))
    return;

  tmp_string = g_strdup(w->value.val_str);

  /*  fprintf(stderr, "Tmp string: %s\n", tmp_string); */
  
  pos = tmp_string;
  
  num_entries = 1;
  while((pos = strchr(pos, ':')))
    {
    num_entries++;
    pos++;
    }

  pos = tmp_string;
  for(i = 0; i < num_entries; i++)
    {
    if(i < num_entries-1)
      {
      end = strchr(pos, ':');
      *end = '\0';
      }

    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       COLUMN_DEVICE,
                       pos,
                       -1);
    if(!i)
      gtk_tree_selection_select_iter(selection, &iter);
    
    end++;
    pos = end;
    
    }
  g_free(tmp_string);
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
      w->value.val_str = (char*)0;
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
                       &(strings[i]),
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

  /*  fprintf(stderr, "Selected: %d\n", selected); */
  
  if(selected >= 0)
    {
    strcat(w->value.val_str, strings[selected]);
    if(num_entries > 1)
      strcat(w->value.val_str, ":");
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
  if(w->value.val_str)
    free(w->value.val_str);
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

static gtk_widget_funcs_t funcs =
  {
    get_value: get_value,
    set_value: set_value,
    destroy:   destroy,
    attach:    attach
  };

static void button_callback(GtkWidget * w, gpointer data)
  {
  device_t * priv;
  GtkTreeSelection * selection;
  GtkTreeIter iter;
  GtkTreeModel * model;
  
  priv = (device_t*)data;

  if(w == priv->add_button)
    {
    if(!priv->fileselect)
      {
      priv->fileselect = gtk_file_selection_new("Select a device");
      gtk_window_set_modal(GTK_WINDOW(priv->fileselect), TRUE);

      g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(priv->fileselect)->ok_button),
                       "clicked", G_CALLBACK(button_callback), (gpointer)priv);
      g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(priv->fileselect)->cancel_button),
                       "clicked", G_CALLBACK(button_callback), (gpointer)priv);
      
      
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
  else if(priv->fileselect)
    {
    if(w == GTK_FILE_SELECTION(priv->fileselect)->ok_button)
      {
      gtk_widget_hide(priv->fileselect);
      gtk_main_quit();

      model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
      gtk_list_store_append(GTK_LIST_STORE(model), &iter);
      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                         COLUMN_DEVICE,
                         gtk_file_selection_get_filename(GTK_FILE_SELECTION(priv->fileselect)),
                         -1);
      
      }
    if(w == GTK_FILE_SELECTION(priv->fileselect)->cancel_button)
      {
      gtk_widget_hide(priv->fileselect);
      gtk_main_quit();
      }
    }
  }

void bg_gtk_create_device(bg_gtk_widget_t * w, bg_parameter_info_t * info)
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
  
  priv->add_button = gtk_button_new_with_label("Add...");
  priv->delete_button = gtk_button_new_with_label("Delete");
  
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
