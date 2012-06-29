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

enum
  {
    COLUMN_LABEL,
    COLUMN_NAME,
    NUM_COLUMNS
  };

typedef struct list_priv_s list_priv_t;


struct list_priv_s
  {
  GtkWidget * treeview;
  GtkWidget * config_button;
  GtkWidget * info_button;

  GtkWidget * top_button;
  GtkWidget * bottom_button;
  GtkWidget * up_button;
  GtkWidget * down_button;
  
  GtkWidget * add_button;
  GtkWidget * remove_button;
  

  GtkWidget * scrolled;

  const char * translation_domain;
  bg_set_parameter_func_t  set_param;
  bg_get_parameter_func_t  get_param;
  void * data;
  int selected;
  int param_selected;
  int is_chain;

  int num;
  
  char ** multi_labels;
  
  };

static void translate_labels(bg_gtk_widget_t * w)
  {
  list_priv_t * priv;
  int i;
  priv = (list_priv_t*)(w->priv);
  i = 0;
  while(w->info->multi_labels[i])
    i++;
  priv->multi_labels = calloc(i+1, sizeof(*priv->multi_labels));
  i = 0;
  while(w->info->multi_labels[i])
    {
    priv->multi_labels[i] =
      bg_strdup(NULL,
                TRD(w->info->multi_labels[i], priv->translation_domain));
    i++;
    }
  
  }

static void set_sub_param(void * priv, const char * name,
                          const bg_parameter_value_t * val)
  {
  char * tmp_string;
  list_priv_t * list;
  bg_gtk_widget_t * w;

  w = (bg_gtk_widget_t*)priv;
  list = (list_priv_t*)(w->priv);

  if(!list->set_param)
    return;
  
  if(!name)
    tmp_string = NULL;
  else if(list->is_chain)
    tmp_string = bg_sprintf("%s.%d.%s", w->info->name, list->selected,
                            name);
  else if(list->param_selected < 0)
    return;
  else
    {
    tmp_string = bg_sprintf("%s.%s.%s", w->info->name,
                            w->info->multi_names[list->param_selected],
                            name);
    }
  list->set_param(list->data, tmp_string, val);
  if(tmp_string)
    free(tmp_string);
  
  }

static int get_sub_param(void * priv, const char * name,
                         bg_parameter_value_t * val)
  {
  char * tmp_string;
  list_priv_t * list;
  bg_gtk_widget_t * w;
  int ret;
  w = (bg_gtk_widget_t*)priv;
  list = (list_priv_t*)(w->priv);

  if(!list->get_param)
    return 0;
  
  if(!name)
    tmp_string = NULL;
  else if(list->is_chain)
    tmp_string = bg_sprintf("%s.%d.%s", w->info->name, list->selected,
                            name);
  else
    {
    tmp_string = bg_sprintf("%s.%s.%s", w->info->name,
                            w->info->multi_names[list->param_selected],
                            name);
    }
  ret = list->get_param(list->data, tmp_string, val);
  if(tmp_string)
    free(tmp_string);
  return ret;
  }


static void apply_sub_params(bg_gtk_widget_t * w)
  {
  list_priv_t * list = (list_priv_t*)(w->priv);
  int index;
  bg_cfg_section_t * subsection;
  bg_cfg_section_t * subsubsection;
  int selected_save;
  char ** names;
  
  if(!list->set_param || !w->value.val_str)
    return;
  
  subsection =
    bg_cfg_section_find_subsection(w->cfg_section, w->info->name);
  
  names = bg_strbreak(w->value.val_str, ',');
  
  selected_save = list->selected;
      
  for(list->selected = 0; list->selected < list->num; list->selected++)
    {
    index = 0;
    while(w->info->multi_names[index])
      {
      if(!strcmp(w->info->multi_names[index], names[list->selected]))
        break;
      index++;
      }
    
    if(w->info->multi_names[index] && w->info->multi_parameters[index])
      {
      if(list->is_chain)
        subsubsection =
          bg_cfg_section_find_subsection_by_index(subsection, list->selected);
      else
        subsubsection =
          bg_cfg_section_find_subsection(subsection, names[list->selected]);
      
      bg_cfg_section_apply_noterminate(subsubsection,
                                       w->info->multi_parameters[index],
                                       set_sub_param, w);
      }
    }
  list->selected = selected_save;
  if(names)
    bg_strbreak_free(names);
  }

/* Return the string corresponding to the current list */
static char * get_list_string(bg_gtk_widget_t * w)
  {
  char * ret = NULL;
  GtkTreeModel * model;
  GtkTreeIter iter;
  char * name;
  
  list_priv_t * priv = (list_priv_t*)(w->priv);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
  if(!gtk_tree_model_get_iter_first(model, &iter))
    {
    return NULL;
    }

  while(1)
    {
    gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
    ret = bg_strcat(ret, name);
    
    g_free(name);
    if(!gtk_tree_model_iter_next(model, &iter))
      break;
    ret = bg_strcat(ret, ",");
    }
  return ret;
  }

/* Get the index in the multi_* arrays of the selected item */
static int get_selected_index(bg_gtk_widget_t * w)
  {
  GtkTreeModel * model;
  GtkTreeIter iter;
  char * name;
  int ret;
  list_priv_t * priv = (list_priv_t*)(w->priv);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
  if(!gtk_tree_model_iter_nth_child(model, &iter,
                                    NULL,
                                    priv->selected))
    {
    return 0;
    }

  gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);

  ret = 0;
  while(strcmp(w->info->multi_names[ret], name))
    ret++;
  
  g_free(name);
  
  return ret;
  }


static void set_value(bg_gtk_widget_t * w)
  {
  if(w->value.val_str)
    free(w->value.val_str);
  w->value.val_str = get_list_string(w);
  }

static void get_value(bg_gtk_widget_t * w)
  {
  GtkTreeIter iter;
  GtkTreeModel * model;
  char const * const * names_c;
  char ** names;
  int init;
  int i, j, do_add;
  list_priv_t * priv = (list_priv_t*)(w->priv);
  char * val_current;
  
  /* Return early if nothing changed */
  val_current = get_list_string(w);
  if((!val_current && !w->value.val_str)
     || ((val_current && w->value.val_str) &&
         !strcmp(val_current, w->value.val_str)))
    {
    if(val_current)
      free(val_current);
    return;
    }
  free(val_current);
  
  /* Fill the list */
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->treeview));
  gtk_list_store_clear(GTK_LIST_STORE(model));
  
  priv->num = 0;
  
  names = bg_strbreak(w->value.val_str, ',');
  
  if(!names)
    {
    names_c = w->info->multi_names;
    init = 1;
    }
  else
    {
    names_c = (char const **)names;
    init = 0;
    }
  /* create translated labels */

  if(!priv->multi_labels && w->info->multi_labels)
    translate_labels(w);
  
  /* Append the codec names from the string, if they are available */
  
  i = 0;
  if(!init)
    {
    while(names_c[i])
      {
      j = 0;
      while(w->info->multi_names[j])
        {
        if(!strcmp(names_c[i], w->info->multi_names[j]))
          {
          gtk_list_store_append(GTK_LIST_STORE(model), &iter);
          if(priv->multi_labels)
            gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                               COLUMN_LABEL,
                               priv->multi_labels[j],
                               -1);
          else
            gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                               COLUMN_LABEL,
                               w->info->multi_names[j],
                               -1);
          
          gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                             COLUMN_NAME,
                             w->info->multi_names[j],
                             -1);
          break;
          }
        j++;
        }
      i++;
      }
    }

  priv->num = i;
  
  /* Append the the new */

  if(!priv->is_chain)
    {
    i = 0;
    while(w->info->multi_names[i])
      {
      /* Test, if we didn't already add this */
      
      if(init)
        do_add = 1;
      else
        {
        do_add = 1;
        j = 0;
        while(names_c[j])
          {
          if(!strcmp(names_c[j], w->info->multi_names[i]))
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
        if(w->info->multi_labels)
          gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                             COLUMN_LABEL,
                             priv->multi_labels[i],
                             -1);
        else
          gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                             COLUMN_LABEL,
                             w->info->multi_names[i],
                             -1);
        
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_NAME,
                           w->info->multi_names[i],
                           -1);
        }
      i++;
      }

    }
  if(!init)
    bg_strbreak_free(names);
  
  //  if(w->info->flags & BG_PARAMETER_SYNC)
  //    bg_gtk_change_callback(NULL, w);
  }

static void add_func(void * priv, const char * name,
                     const bg_parameter_value_t * val)
  {
  list_priv_t * list;
  bg_gtk_widget_t * w;
  int index;
  GtkTreeModel * model;
  GtkTreeIter iter;
  bg_cfg_section_t * subsection;
  bg_cfg_section_t * subsubsection;
  //  bg_cfg_section_t * subsection_default;
  w = priv;
  list = w->priv;
  
  if(!name)
    return;
  if(strcmp(name, w->info->name))
    return;
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(list->treeview));
  
  index = 0;
  while(strcmp(w->info->multi_names[index], val->val_str))
    index++;
    
  gtk_list_store_append(GTK_LIST_STORE(model), &iter);

  if(!list->multi_labels && w->info->multi_labels)
    translate_labels(w);
    
  if(list->multi_labels)
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       COLUMN_LABEL,
                       list->multi_labels[index],
                       -1);
  else
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       COLUMN_LABEL,
                       w->info->multi_names[index],
                       -1);

  gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                     COLUMN_NAME,
                     w->info->multi_names[index],
                     -1);
    
  subsection =
    bg_cfg_section_find_subsection(w->cfg_section, w->info->name);

#if 0 
  subsection_default =
    bg_cfg_section_find_subsection(subsection, val->val_str);
  
  if(w->info->multi_parameters[index])
    bg_cfg_section_create_items(subsection_default, 
                                w->info->multi_parameters[index]);
#endif
  
  subsubsection =
    bg_cfg_section_create_subsection_at_pos(subsection, list->num);
  if(w->info->multi_parameters[index])
    bg_cfg_section_create_items(subsubsection, 
                                w->info->multi_parameters[index]);
  
  //  bg_cfg_section_transfer(subsection_default, subsubsection);
    
  list->num++;
    
  if(w->info->flags & BG_PARAMETER_SYNC)
    {
    bg_gtk_change_callback(NULL, w);
    }
  }

static void attach(void * p, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  list_priv_t * e = (list_priv_t*)(p);

  int num_rows = 2;
  if(e->is_chain)
    num_rows += 2;
  if(e->top_button)
    num_rows+=4;
  
  if(*num_columns < 3)
    *num_columns = 3;

  gtk_table_resize(GTK_TABLE(table), *row+num_rows, *num_columns);

  gtk_table_attach_defaults(GTK_TABLE(table), e->scrolled,
                            0, 2, *row, *row+num_rows);

  if(e->is_chain)
    {
    gtk_table_attach(GTK_TABLE(table), e->add_button,
                     2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
    *row += 1;
    gtk_table_attach(GTK_TABLE(table), e->remove_button,
                     2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
    *row += 1;
    
    }
  
  gtk_table_attach(GTK_TABLE(table), e->config_button,
                   2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
  *row += 1;
  
  
  gtk_table_attach(GTK_TABLE(table), e->info_button,
                   2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
  *row += 1;

  if(e->top_button)
    {
    gtk_table_attach(GTK_TABLE(table), e->top_button,
                     2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
    *row += 1;
    }

  if(e->up_button)
    {
    gtk_table_attach(GTK_TABLE(table), e->up_button,
                     2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
    *row += 1;
    }

  if(e->down_button)
    {
    gtk_table_attach(GTK_TABLE(table), e->down_button,
                     2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
    *row += 1;
    }

  if(e->bottom_button)
    {
    gtk_table_attach(GTK_TABLE(table), e->bottom_button,
                     2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
    *row += 1;
    }
  }

static void destroy(bg_gtk_widget_t * w)
  {
  list_priv_t * priv = (list_priv_t*)(w->priv);
  if(priv->multi_labels)
    {
    int i = 0;
    while(priv->multi_labels[i])
      free(priv->multi_labels[i++]);
    free(priv->multi_labels);
    }
  free(priv);
  }

static const gtk_widget_funcs_t funcs =
  {
    .get_value = get_value,
    .set_value = set_value,
    .destroy =   destroy,
    .attach =    attach,
    .apply_sub_params = apply_sub_params,
  };

static void select_row_callback(GtkTreeSelection * s, gpointer data)
  {
  bg_gtk_widget_t * w;
  list_priv_t * priv;
  char * name;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  w = (bg_gtk_widget_t *)data;
  priv = (list_priv_t *)(w->priv);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));

  if(!gtk_tree_selection_get_selected(selection, &model, &iter))
    priv->selected = -1;
  else
    {
    priv->selected = 0;
    gtk_tree_model_get_iter_first(model, &iter);
    while(1)
      {
      if(gtk_tree_selection_iter_is_selected(selection, &iter))
        break;
      priv->selected++;
      gtk_tree_model_iter_next(model, &iter);
      }
    }
  
  if(priv->selected < 0)
    {
    gtk_widget_set_sensitive(priv->info_button, 0);
    gtk_widget_set_sensitive(priv->config_button, 0);

    if(priv->top_button)
      gtk_widget_set_sensitive(priv->top_button, 0);
    if(priv->bottom_button)
      gtk_widget_set_sensitive(priv->bottom_button, 0);
    if(priv->up_button)
      gtk_widget_set_sensitive(priv->up_button, 0);
    if(priv->down_button)
      gtk_widget_set_sensitive(priv->down_button, 0);
    if(priv->remove_button)
      gtk_widget_set_sensitive(priv->remove_button, 0);
    priv->param_selected = priv->selected;
    }
  else
    {
    gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
    priv->param_selected = 0;
    while(strcmp(w->info->multi_names[priv->param_selected], name))
      priv->param_selected++;
    g_free(name);
    
    if(w->info->multi_descriptions &&
       w->info->multi_descriptions[priv->param_selected])
      gtk_widget_set_sensitive(priv->info_button, 1);
    else
      gtk_widget_set_sensitive(priv->info_button, 0);
    
    if(w->info->multi_parameters &&
       w->info->multi_parameters[priv->param_selected])
      gtk_widget_set_sensitive(priv->config_button, 1);
    else
      gtk_widget_set_sensitive(priv->config_button, 0);

    if(priv->selected > 0)
      {
      if(priv->top_button)
        gtk_widget_set_sensitive(priv->top_button, 1);
      if(priv->up_button)
        gtk_widget_set_sensitive(priv->up_button, 1);
      }
    else
      {
      if(priv->top_button)
        gtk_widget_set_sensitive(priv->top_button, 0);
      if(priv->up_button)
        gtk_widget_set_sensitive(priv->up_button, 0);
      }

    if(priv->selected < priv->num-1)
      {
      if(priv->bottom_button)
        gtk_widget_set_sensitive(priv->bottom_button, 1);
      if(priv->down_button)
        gtk_widget_set_sensitive(priv->down_button, 1);
      }
    else
      {
      if(priv->bottom_button)
        gtk_widget_set_sensitive(priv->bottom_button, 0);
      if(priv->down_button)
        gtk_widget_set_sensitive(priv->down_button, 0);
      }
    
    if(priv->remove_button)
      gtk_widget_set_sensitive(priv->remove_button, 1);
    }
  }

static void move_selected(bg_gtk_widget_t * w, int new_pos)
  {
  int i;
  list_priv_t * priv;

  bg_cfg_section_t * subsection;
  bg_cfg_section_t * subsubsection;

  GtkTreeSelection * selection;
  GtkTreeModel * model;
  GtkTreePath      * path;

  GtkTreeIter iter;
  GtkTreeIter iter_before;

  priv = (list_priv_t *)(w->priv);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
  gtk_tree_selection_get_selected(selection, &model, &iter);

  if(!new_pos)
    {
    gtk_list_store_move_after(GTK_LIST_STORE(model),
                              &iter,
                              NULL);

    }
  else
    {
    gtk_tree_model_get_iter_first(model, &iter_before);

    for(i = 0; i < new_pos-1; i++)
      gtk_tree_model_iter_next(model, &iter_before);

    if(new_pos > priv->selected)
      gtk_tree_model_iter_next(model, &iter_before);
        
    gtk_list_store_move_after(GTK_LIST_STORE(model),
                              &iter,
                              &iter_before);
    }
  //  gtk_tree_selection_select_iter(selection, &iter);
  
  path = gtk_tree_model_get_path(model, &iter);
  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(priv->treeview),
                               path,
                               NULL,
                               0, 0.0, 0.0);
  gtk_tree_path_free(path);

  subsection = 
    bg_cfg_section_find_subsection(w->cfg_section, w->info->name);
  
  /* Move config section */
  
  if(priv->is_chain)
    {
    subsubsection =
      bg_cfg_section_find_subsection_by_index(subsection, priv->selected);
    bg_cfg_section_move_child(subsection, subsubsection, new_pos);
    }
  
  /* Apply parameter and subsections. It's easier to do it here. */

  if(w->info->flags & BG_PARAMETER_SYNC)
    bg_gtk_change_callback(NULL, w);
  
  priv->selected = new_pos;
  
  if(!priv->selected)
    {
    if(priv->top_button)
      gtk_widget_set_sensitive(priv->top_button, 0);
    if(priv->up_button)
      gtk_widget_set_sensitive(priv->up_button, 0);
    }
  else
    {
    if(priv->top_button)
      gtk_widget_set_sensitive(priv->top_button, 1);
    if(priv->up_button)
      gtk_widget_set_sensitive(priv->up_button, 1);
    }

  if(priv->selected >= priv->num - 1)
    {
    if(priv->down_button)
      gtk_widget_set_sensitive(priv->down_button, 0);
    if(priv->bottom_button)
      gtk_widget_set_sensitive(priv->bottom_button, 0);
    }
  else
    {
    if(priv->down_button)
      gtk_widget_set_sensitive(priv->down_button, 1);
    if(priv->bottom_button)
      gtk_widget_set_sensitive(priv->bottom_button, 1);
    }
  }

static void button_callback(GtkWidget * wid, gpointer data)
  {
  bg_gtk_widget_t * w;
  list_priv_t * priv;
  bg_dialog_t * dialog;
  GtkTreeSelection * selection;

  GtkTreeModel * model;
  const char * label;
  bg_cfg_section_t * subsection;
  bg_cfg_section_t * subsubsection;

  w = data;
  priv = w->priv;
  
  if(wid == priv->config_button)
    {
    if(w->cfg_section)
      {
      subsection = bg_cfg_section_find_subsection(w->cfg_section, w->info->name);
    
      if(priv->is_chain)
        subsection =
          bg_cfg_section_find_subsection_by_index(subsection,
                                                  priv->selected);
      else
        subsection =
          bg_cfg_section_find_subsection(subsection,
                                         w->info->multi_names[priv->param_selected]);
      }
    else
      subsection = NULL;

    
    if(w->info->multi_labels && w->info->multi_labels[priv->param_selected])
      label = TRD(w->info->multi_labels[priv->param_selected],
                  priv->translation_domain);
    else
      label = w->info->multi_names[priv->param_selected];
    
    dialog = bg_dialog_create(subsection, set_sub_param, get_sub_param, w,
                              w->info->multi_parameters[priv->param_selected],
                              label);
    bg_dialog_show(dialog, priv->treeview);
    }
  else if(wid == priv->info_button)
    {
    bg_gtk_multi_info_show(w->info, get_selected_index(w),
                           priv->translation_domain, priv->info_button);
    }
  else if(wid == priv->top_button)
    {
    if(priv->selected == 0)
      return;
    move_selected(w, 0);
    }
  else if(wid == priv->up_button)
    {
    if(priv->selected == 0)
      return;
    move_selected(w, priv->selected - 1);
    }
  else if(wid == priv->down_button)
    {
    if(priv->selected >= priv->num - 1)
      return;
    move_selected(w, priv->selected+1);
    }
  
  else if(wid == priv->bottom_button)
    {
    if(priv->selected >= priv->num-1)
      return;

    move_selected(w, priv->num-1);
    }
  else if(wid == priv->add_button)
    {
    bg_parameter_info_t params[2];
    char * tmp_string;
    memset(params, 0, sizeof(params));
    params[0].name               = w->info->name;
    params[0].long_name          = w->info->long_name;
    params[0].type               = BG_PARAMETER_MULTI_MENU;
    params[0].gettext_domain     = bg_strdup(params[0].gettext_domain,
                                             priv->translation_domain);
    params[0].multi_names        = w->info->multi_names;
    params[0].multi_labels       = w->info->multi_labels;
    params[0].multi_descriptions = w->info->multi_descriptions;
    params[0].help_string        = w->info->help_string;
    //    params[0].multi_parameters   = w->info->multi_parameters;
    
    tmp_string = bg_sprintf(TR("Add %s"),
                            TRD(w->info->long_name, priv->translation_domain));
    
    dialog = bg_dialog_create(w->cfg_section, add_func, NULL,
                              w, params, tmp_string);
    
    
    free(tmp_string);
    bg_dialog_show(dialog, priv->treeview);
    free(params[0].gettext_domain);
    
    }
  else if(wid == priv->remove_button)
    {
    GtkTreeIter iter;
    
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
    
    if(!gtk_tree_selection_get_selected(selection, &model, &iter))
      return;

    subsection = bg_cfg_section_find_subsection(w->cfg_section, w->info->name);
    subsubsection = bg_cfg_section_find_subsection_by_index(subsection,
                                                            priv->selected);
    bg_cfg_section_delete_subsection(subsection,
                                     subsubsection);
    
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);        
    priv->num--;
    if(w->info->flags & BG_PARAMETER_SYNC)
      {
      bg_gtk_change_callback(NULL, w);
      }
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

static void create_list_common(bg_gtk_widget_t * w, const bg_parameter_info_t * info,
                               bg_set_parameter_func_t set_param,
                               bg_get_parameter_func_t get_param,
                               void * data, const char * translation_domain,
                               int is_chain)
  {
  GtkListStore *store;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection * selection;

  list_priv_t * priv = calloc(1, sizeof(*priv));

  priv->set_param   = set_param;
  priv->get_param   = get_param;
  priv->data        = data;
  priv->translation_domain = translation_domain;

  priv->is_chain = is_chain;

  w->funcs = &funcs;
  w->priv = priv;
  
  /* Create objects */

  priv->info_button = create_pixmap_button("info_16.png");
  priv->config_button = create_pixmap_button("config_16.png");
  g_signal_connect(G_OBJECT(priv->info_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)w);
  g_signal_connect(G_OBJECT(priv->config_button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)w);
  gtk_widget_show(priv->info_button);
  gtk_widget_show(priv->config_button);

  gtk_widget_set_sensitive(priv->info_button, 0);
  gtk_widget_set_sensitive(priv->config_button, 0);

  if(!(info->flags & BG_PARAMETER_NO_SORT))
    {
    priv->top_button = create_pixmap_button("top_16.png");
    priv->bottom_button = create_pixmap_button("bottom_16.png");
    priv->up_button = create_pixmap_button("up_16.png");
    priv->down_button = create_pixmap_button("down_16.png");
    g_signal_connect(G_OBJECT(priv->top_button),
                     "clicked", G_CALLBACK(button_callback),
                     (gpointer)w);
    g_signal_connect(G_OBJECT(priv->bottom_button),
                     "clicked", G_CALLBACK(button_callback),
                     (gpointer)w);
    g_signal_connect(G_OBJECT(priv->up_button),
                     "clicked", G_CALLBACK(button_callback),
                     (gpointer)w);
    g_signal_connect(G_OBJECT(priv->down_button),
                     "clicked", G_CALLBACK(button_callback),
                     (gpointer)w);
    gtk_widget_show(priv->top_button);
    gtk_widget_show(priv->bottom_button);
    gtk_widget_show(priv->up_button);
    gtk_widget_show(priv->down_button);

    gtk_widget_set_sensitive(priv->top_button, 0);
    gtk_widget_set_sensitive(priv->bottom_button, 0);
    gtk_widget_set_sensitive(priv->up_button, 0);
    gtk_widget_set_sensitive(priv->down_button, 0);
    }
  
  if(priv->is_chain)
    {
    priv->add_button = create_pixmap_button("add_16.png");
    priv->remove_button = create_pixmap_button("trash_16.png");

    g_signal_connect(G_OBJECT(priv->add_button),
                     "clicked", G_CALLBACK(button_callback),
                     (gpointer)w);
    g_signal_connect(G_OBJECT(priv->remove_button),
                     "clicked", G_CALLBACK(button_callback),
                     (gpointer)w);
    gtk_widget_show(priv->add_button);
    gtk_widget_show(priv->remove_button);
    gtk_widget_set_sensitive(priv->remove_button, 0);
    }
  
  store = gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

  priv->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  if(info->help_string)
    {
    bg_gtk_tooltips_set_tip(priv->treeview,
                            info->help_string, translation_domain);
    }

  
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(priv->treeview), FALSE);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));

  
  
  g_signal_connect(G_OBJECT(selection),
                   "changed", G_CALLBACK(select_row_callback),
                   (gpointer)w);

  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes("",
                                             renderer,
                                             "text",
                                             COLUMN_LABEL,
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

void
bg_gtk_create_multi_list(bg_gtk_widget_t * w,
                         bg_set_parameter_func_t set_param,
                         bg_get_parameter_func_t get_param,
                         void * data, const char * translation_domain)
  {
  create_list_common(w, w->info, set_param, get_param, data, translation_domain, 0);
  }

void
bg_gtk_create_multi_chain(bg_gtk_widget_t * w,
                          bg_set_parameter_func_t set_param,
                          bg_get_parameter_func_t get_param,
                          void * data, const char * translation_domain)
  {
  create_list_common(w, w->info, set_param, get_param, data, translation_domain, 1);
  }
