/*****************************************************************
 
  cfg_decoder.c
 
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
#include <utils.h>
#include <gui_gtk/codecinfo.h>

enum
  {
    COLUMN_NAME,
    NUM_COLUMNS
  };

typedef struct
  {
  GtkWidget * treeview;
  GtkWidget * config_button;
  GtkWidget * info_button;
  GtkWidget * up_button;
  GtkWidget * down_button;
  GtkWidget * scrolled;

  
  bg_cfg_section_t * cfg_section;
  bg_parameter_func  set_param;
  void * data;
  int selected;
  } decoder_t;

static void set_value(bg_gtk_widget_t * w)
  {
  int num;
  GtkTreeModel * model;
  GtkTreeIter iter;
  char * name;
  
  decoder_t * priv = (decoder_t*)(w->priv);

  if(w->value.val_str)
    {
    free(w->value.val_str);
    w->value.val_str = (char*)0;
    }
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
  if(!gtk_tree_model_get_iter_first(model, &iter))
    {
    return;
    }

  while(1)
    {
    gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
    if(w->info->codec_long_names)
      {
      num = 0;
      while(strcmp(w->info->codec_long_names[num], name))
        num++;
      w->value.val_str = bg_strcat(w->value.val_str,
                                   w->info->codec_names[num]);
      }
    else
      w->value.val_str = bg_strcat(w->value.val_str,
                                   name);

    g_free(name);
    if(!gtk_tree_model_iter_next(model, &iter))
      break;
    w->value.val_str = bg_strcat(w->value.val_str, ",");
    }
  fprintf(stderr, "Set Value: %s\n", w->value.val_str);
  }



static void get_value(bg_gtk_widget_t * w)
  {
  GtkTreeIter iter;
  GtkTreeModel * model;
  char * tmp_string;
  char ** names;
  int init;
  int i, j, do_add;
  decoder_t * priv = (decoder_t*)(w->priv);
  /* Fill the list */
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
  gtk_list_store_clear(GTK_LIST_STORE(model));

  tmp_string = bg_strdup(NULL, w->value.val_str);
  names = bg_strbreak(tmp_string, ',');
  if(tmp_string)
    free(tmp_string);
  
  if(!names)
    {
    names = w->info->codec_names;
    init = 1;
    }
  else
    init = 0;

  /* Count and append the codecs */


  /* Append the codec names from the string, if they are available */
  
  i = 0;
  if(!init)
    {
    while(names[i])
      {
      j = 0;
      while(w->info->codec_names[j])
        {
        if(!strcmp(names[i], w->info->codec_names[j]))
          {
          gtk_list_store_append(GTK_LIST_STORE(model), &iter);
          if(w->info->codec_long_names)
            gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                               COLUMN_NAME,
                               w->info->codec_long_names[j],
                               -1);
          else
            gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                               COLUMN_NAME,
                               w->info->codec_names[j],
                               -1);
          break;
          }
        j++;
        }
      i++;
      }
    }

  /* Append the which are new */
  
  i = 0;
  while(w->info->codec_names[i])
    {
    /* Test, if we didn't alreary add this */
    
    if(init)
      do_add = 1;
    else
      {
      do_add = 1;
      j = 0;
      while(names[j])
        {
        if(!strcmp(names[j], w->info->codec_names[i]))
          {
          do_add = 0;
          break;
          }
        j++;
        }
      }
    if(do_add)
      {
      gtk_list_store_append(GTK_LIST_STORE(model), &iter);
      if(w->info->codec_long_names)
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_NAME,
                           w->info->codec_long_names[i],
                           -1);
      else
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_NAME,
                           w->info->codec_names[i],
                           -1);
      }
    i++;
    }
  if(!init)
    {
    bg_strbreak_free(names);
    }
  }



static void attach(void * p, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  decoder_t * e = (decoder_t*)(p);

  if(*num_columns < 3)
    *num_columns = 3;

  gtk_table_resize(GTK_TABLE(table), *row+4, *num_columns);

  gtk_table_attach_defaults(GTK_TABLE(table), e->scrolled,
                            0, 2, *row, *row+4);

  gtk_table_attach(GTK_TABLE(table), e->config_button,
                   2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), e->info_button,
                   2, 3, *row+1, *row+2, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), e->up_button,
                   2, 3, *row+2, *row+3, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), e->down_button,
                   2, 3, *row+3, *row+4, GTK_FILL, GTK_SHRINK, 0, 0);

  *row += 4;
  }

static void destroy(bg_gtk_widget_t * w)
  {
  decoder_t * priv = (decoder_t*)(w->priv);
  if(w->value.val_str)
    free(w->value.val_str);
  free(priv);
  }

static gtk_widget_funcs_t funcs =
  {
    get_value: get_value,
    set_value: set_value,
    destroy:   destroy,
    attach:    attach
  };

static void select_row_callback(GtkTreeSelection * s, gpointer data)
  {
  bg_gtk_widget_t * w;
  decoder_t * priv;
  char * name;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  int i;
  
  w = (bg_gtk_widget_t *)data;
  priv = (decoder_t *)(w->priv);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
  gtk_tree_selection_get_selected(selection, &model, &iter);
  
  gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);

  /* Now, get the selected item */

  i = 0;
  while(w->info->codec_names[i])
    {
    if((w->info->codec_long_names) &&
       !strcmp(w->info->codec_long_names[i], name))
      {
      priv->selected = i;
      break;
      }
    else if(!strcmp(w->info->codec_names[i], name))
      {
      priv->selected = i;
      break;
      }
    i++;
    }

  if(w->info->codec_descriptions &&
     w->info->codec_descriptions[priv->selected])
    gtk_widget_set_sensitive(priv->info_button, 1);
  else
    gtk_widget_set_sensitive(priv->info_button, 0);

  if(w->info->codec_parameters &&
     w->info->codec_parameters[priv->selected])
    gtk_widget_set_sensitive(priv->config_button, 1);
  else
    gtk_widget_set_sensitive(priv->config_button, 0);

  gtk_widget_set_sensitive(priv->up_button, 1);
  gtk_widget_set_sensitive(priv->down_button, 1);
  
  g_free(name);
  }

static void button_callback(GtkWidget * wid, gpointer data)
  {
  bg_gtk_widget_t * w;
  decoder_t * priv;
  bg_dialog_t * dialog;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  GtkTreePath      * path;
  w = (bg_gtk_widget_t *)data;
  priv = (decoder_t *)(w->priv);

  if(wid == priv->config_button)
    {
    fprintf(stderr, "Config button clicked\n");
    dialog = bg_dialog_create(priv->cfg_section, priv->set_param,
                              priv->data,
                              w->info->codec_parameters[priv->selected]);
    bg_dialog_show(dialog);
    }
  else if(wid == priv->info_button)
    {
    bg_gtk_codec_info_show(w->info, priv->selected);
    }
  else if(wid == priv->up_button)
    {
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
    gtk_tree_selection_get_selected(selection, &model, &iter);
    gtk_list_store_move_after(GTK_LIST_STORE(model),
                              &iter,
                              (GtkTreeIter*)0);
    path = gtk_tree_model_get_path(model, &iter);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(priv->treeview),
                                 path,
                                 (GtkTreeViewColumn *)0,
                                 0, 0.0, 0.0);
    gtk_tree_path_free(path);
    }
  else if(wid == priv->down_button)
    {
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
    gtk_tree_selection_get_selected(selection, &model, &iter);
    gtk_list_store_move_before(GTK_LIST_STORE(model),
                               &iter,
                               (GtkTreeIter*)0);
    path = gtk_tree_model_get_path(model, &iter);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(priv->treeview),
                                 path,
                                 (GtkTreeViewColumn *)0,
                                 0, 0.0, 0.0);
    gtk_tree_path_free(path);
    }

  }

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


void
bg_gtk_create_decoder(bg_gtk_widget_t * w, bg_parameter_info_t * info,
                      bg_cfg_section_t * cfg_section,
                      bg_parameter_func set_param,
                      void * data)
  {
  GtkListStore *store;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection * selection;

  decoder_t * priv = calloc(1, sizeof(*priv));

  priv->cfg_section = cfg_section;
  priv->set_param   = set_param;
  priv->data        = data;

  w->funcs = &funcs;
  w->priv = priv;

  /* Create objects */

  priv->info_button = create_pixmap_button("info_16.png");

  priv->config_button = create_pixmap_button("config_16.png");

  priv->up_button = create_pixmap_button("up_16.png");

  priv->down_button = create_pixmap_button("down_16.png");

  g_signal_connect(G_OBJECT(priv->info_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)w);
  g_signal_connect(G_OBJECT(priv->config_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)w);
  g_signal_connect(G_OBJECT(priv->up_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)w);
  g_signal_connect(G_OBJECT(priv->down_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)w);

  gtk_widget_show(priv->info_button);
  gtk_widget_show(priv->config_button);
  gtk_widget_show(priv->up_button);
  gtk_widget_show(priv->down_button);

  gtk_widget_set_sensitive(priv->info_button, 0);
  gtk_widget_set_sensitive(priv->config_button, 0);
  gtk_widget_set_sensitive(priv->up_button, 0);
  gtk_widget_set_sensitive(priv->down_button, 0);
  
  store = gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

  priv->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(priv->treeview), FALSE);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));

  
  
  g_signal_connect(G_OBJECT(selection),
                   "changed", G_CALLBACK(select_row_callback),
                   (gpointer)w);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Installed Codecs",
                                                    renderer,
                                                    "text",
                                                    COLUMN_NAME,
                                                    NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(priv->treeview), column);
  gtk_widget_show(priv->treeview);

  priv->scrolled =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(priv->treeview)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(priv->treeview)));
  
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(priv->scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(priv->scrolled), priv->treeview);
  gtk_widget_show(priv->scrolled);

    
  }
