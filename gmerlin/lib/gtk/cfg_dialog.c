/*****************************************************************
 
  cfg_dialog.c
 
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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include "gtk_dialog.h"
#include <gui_gtk/gtkutils.h>

#include <log.h>
#define LOG_DOMAIN "cfg_dialog"

enum
{
  COLUMN_NAME,
  NUM_COLUMNS
};


typedef struct dialog_section_s
  {
  bg_set_parameter_func_t set_param;

  void * callback_data;

  bg_gtk_widget_t * widgets;
  int num_widgets;

  bg_parameter_info_t * infos;
  bg_cfg_section_t * cfg_section;

  /* Dialog sections can be nested */
  struct dialog_section_s * children;
  int num_children;
    
  struct dialog_section_s * parent;
  
  /* Index in global notebook */
  int notebook_index;
  } dialog_section_t;

struct bg_dialog_s
  {
  GtkWidget * ok_button;
  GtkWidget * apply_button;
  GtkWidget * close_button;
  GtkWidget * window;
  GtkWidget * mainbox;
  dialog_section_t root_section;
  GtkTooltips * tooltips;
  int visible;

  GtkWidget * notebook;
  GtkWidget * treeview;
  GtkWidget * scrolledwindow;
  guint select_handler_id;
  };

static int parent_index(dialog_section_t * s)
  {
  int i;

  if(!s->parent)
    return -1;

  for(i = 0; i < s->parent->num_children; i++)
    {
    if(&(s->parent->children[i]) == s)
      return i;
    }
  return -1;
  }

static int * section_to_path(bg_dialog_t * d, dialog_section_t * s)
  {
  int * ret;
  int index;
  dialog_section_t * tmp;
  int depth;

  /* Get depth */

  tmp = s;
  depth = 0;
  while(tmp->parent)
    {
    tmp = tmp->parent;
    depth++;
    }

  /* Allocate return value */
  ret = malloc((depth+1)*sizeof(*ret));
  index = depth;
  ret[index--] = -1;

  tmp = s;

  while(index >= 0)
    {
    ret[index--] = parent_index(tmp);
    tmp = tmp->parent;
    }
  return ret;
  }

static void
section_to_iter(bg_dialog_t * d, dialog_section_t * s, GtkTreeIter * iter)
  {
  int * indices;
  int i;
  GtkTreeModel *model;
  GtkTreePath  *path;
    
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->treeview));
  indices = section_to_path(d, s);
  
  path = gtk_tree_path_new();
    
  i = 0;

  while(indices[i] != -1)
    {
    gtk_tree_path_append_index(path, indices[i]);
    i++;
    }
  free(indices);

  gtk_tree_model_get_iter(model, iter, path);
  gtk_tree_path_free(path);
  }

static dialog_section_t * iter_2_section(bg_dialog_t * d,
                                         GtkTreeIter * iter)
  {
  dialog_section_t * ret;
  int i, depth;
  GtkTreeModel *model;
  GtkTreePath  *path;
  gint * indices;
    
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->treeview));
  path = gtk_tree_model_get_path(model, iter);

  indices = gtk_tree_path_get_indices(path);

  ret = &d->root_section.children[indices[0]];

  depth = gtk_tree_path_get_depth(path);
  
  for(i = 1; i < depth; i++)
    {
    ret = &(ret->children[indices[i]]);
    }
  
  gtk_tree_path_free(path);
  return ret;
  }



static void apply_section(dialog_section_t * s)
  {
  int i, parameter_index;
  bg_parameter_value_t val;
  char * pos;
  
  parameter_index = 0;

  for(i = 0; i < s->num_widgets; i++)
    {
    while(s->infos[parameter_index].flags & BG_PARAMETER_HIDE_DIALOG)
      parameter_index++;
    
    s->widgets[i].funcs->set_value(&(s->widgets[i]));
    if(s->cfg_section)
      {
      bg_cfg_section_set_parameter(s->cfg_section,
                                   &(s->infos[parameter_index]),
                                   &(s->widgets[i].value));
      }

    if(s->set_param)
      {
      if((s->infos[parameter_index].type == BG_PARAMETER_DEVICE) &&
         (s->widgets[i].value.val_str) &&
         strchr(s->widgets[i].value.val_str, ':'))
        {
        val.val_str = malloc(strlen(s->widgets[i].value.val_str)+1);
        strcpy(val.val_str, s->widgets[i].value.val_str);
        pos = strchr(val.val_str, ':');
        *pos = '\0';
        s->set_param(s->callback_data, s->infos[parameter_index].name,
                     &val);
        free(val.val_str);
        }
      else
        s->set_param(s->callback_data, s->infos[parameter_index].name,
                     &(s->widgets[i].value));
      }
    parameter_index++;
    }

  
  if(s->set_param)
    s->set_param(s->callback_data, NULL, NULL);

  for(i = 0; i < s->num_children; i++)
    apply_section(&(s->children[i]));
  
  }

static void apply_values(bg_dialog_t * d)
  {
  apply_section(&(d->root_section));
  }

static void button_callback(GtkWidget * w, gpointer * data)
  {
  bg_dialog_t * d = (bg_dialog_t *)data;
  if((w == d->close_button) ||
     (w == d->window))
    {
    d->visible = 0;
    gtk_widget_hide(d->window);
    gtk_main_quit();
    }
  else if(w == d->apply_button)
    {
    apply_values(d);
    }
  else if(w == d->ok_button)
    {
    d->visible = 0;
    gtk_widget_hide(d->window);
    gtk_main_quit();
    apply_values(d);
    }
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

static void select_row_callback(GtkTreeSelection * sel,
                                gpointer data)
  {
  int index = 0;
  dialog_section_t * selected_section;
  GtkTreeModel * model;
  GtkTreeIter iter;
  bg_dialog_t * d = (bg_dialog_t *)data;
  

  if(!gtk_tree_selection_get_selected(sel,
                                      &model,
                                      &iter))
    index = 0;
  else
    {
    selected_section = iter_2_section(d, &iter);
    index = selected_section->notebook_index;
    }
  gtk_notebook_set_current_page(GTK_NOTEBOOK(d->notebook), index);
  
  }

static bg_dialog_t * create_dialog(const char * title)
  {
  bg_dialog_t * ret;
  GtkWidget * buttonbox;
  GtkWidget * label, *tab_label;
  GtkWidget * hbox;
  GtkTreeStore *store;
  GtkCellRenderer   *text_renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection  *selection;
  
  
  ret = calloc(1, sizeof(*ret));

  ret->tooltips = gtk_tooltips_new();
  g_object_ref (G_OBJECT (ret->tooltips));

#if GTK_MINOR_VERSION < 10
  gtk_object_sink (GTK_OBJECT (ret->tooltips));
#else
  g_object_ref_sink(G_OBJECT(ret->tooltips));
#endif
  
  ret->window       = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(ret->window), title);
    
  ret->apply_button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  ret->ok_button    = gtk_button_new_from_stock(GTK_STOCK_OK);

  GTK_WIDGET_SET_FLAGS(ret->apply_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(ret->close_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(ret->ok_button, GTK_CAN_DEFAULT);
  
  gtk_window_set_modal(GTK_WINDOW(ret->window), TRUE);
  g_signal_connect(G_OBJECT(ret->ok_button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                     G_CALLBACK(button_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->apply_button), "clicked",
                     G_CALLBACK(button_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                     G_CALLBACK(delete_callback), (gpointer)ret);

  GTK_WIDGET_SET_FLAGS (ret->close_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (ret->apply_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (ret->ok_button, GTK_CAN_DEFAULT);
  
  gtk_widget_show(ret->apply_button);
  gtk_widget_show(ret->close_button);
  gtk_widget_show(ret->ok_button);

  /* Create notebook */

  ret->notebook = gtk_notebook_new();
  
  label = gtk_label_new(TR("No options here, choose subcategory"));
  tab_label = gtk_label_new("");
  gtk_widget_show(label);
  gtk_widget_show(tab_label);

  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(ret->notebook), 0);
  
  
  gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook), label, tab_label);
  
  gtk_widget_show(ret->notebook);
  
  /* Create treeview */

  store = gtk_tree_store_new (NUM_COLUMNS,
                              G_TYPE_STRING);

  ret->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ret->treeview), 0);
  //  gtk_widget_set_size_request(ret->treeview, 200, 300);
  text_renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
  gtk_tree_view_column_add_attribute(column,
                                     text_renderer,
                                     "text", COLUMN_NAME);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview), column);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->treeview));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  ret->select_handler_id =
    g_signal_connect(G_OBJECT(selection), "changed",
                     G_CALLBACK(select_row_callback), (gpointer)ret);
  gtk_widget_show(ret->treeview);
  ret->scrolledwindow =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(ret->treeview)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ret->treeview)));

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->scrolledwindow),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(ret->scrolledwindow), ret->treeview);
  gtk_widget_show(ret->scrolledwindow);

  
  /* Create the rest */

  hbox = gtk_hpaned_new();
  gtk_paned_add1(GTK_PANED(hbox), ret->scrolledwindow);
  gtk_paned_add2(GTK_PANED(hbox), ret->notebook);
  gtk_widget_show(hbox);
  
  ret->mainbox = gtk_vbox_new(0, 5);
  gtk_box_pack_start(GTK_BOX(ret->mainbox), hbox, FALSE, FALSE, 0);
  
  buttonbox = gtk_hbutton_box_new();
  gtk_box_set_spacing(GTK_BOX(buttonbox), 10);
  gtk_container_set_border_width(GTK_CONTAINER(buttonbox), 10);
  
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->ok_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->apply_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->close_button);
  gtk_widget_show(buttonbox);
  gtk_box_pack_end(GTK_BOX(ret->mainbox), buttonbox, FALSE, FALSE, 0);
  gtk_widget_show(ret->mainbox);
  gtk_container_add(GTK_CONTAINER(ret->window), ret->mainbox);  
  
  return ret;
  }

static GtkWidget * create_section(dialog_section_t * section,
                                  bg_parameter_info_t * info,
                                  bg_cfg_section_t * cfg_section,
                                  bg_set_parameter_func_t set_param,
                                  void * data, GtkTooltips * tooltips,
                                  const char * translation_domain)
  {
  int i, count;
  int row, column, num_columns;
  
  GtkWidget * table;
  GtkWidget * label;
  
  /* If info == NULL, we create a label, which
     tell the user, that there is nothing to do */

  if(!info)
    {
    table = gtk_table_new(1, 1, 0);
    label = gtk_label_new(TR("No options available"));
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL|GTK_EXPAND, 
                     GTK_FILL|GTK_EXPAND, 0, 0);
    gtk_widget_show(table);
    return table;
    }
  
  section->num_widgets = 0;
  i = 0;
  while(info[i].name &&
        (info[i].type != BG_PARAMETER_SECTION))
    {
    if(!(info[i].flags & BG_PARAMETER_HIDE_DIALOG))
      section->num_widgets++;
    i++;
    }
  section->infos = info;
  section->cfg_section = cfg_section;
  section->callback_data = data;
  section->set_param = set_param;
    
  /* Now, create and place all widgets */

  table = gtk_table_new(1, 1, 0);

  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    
  row = 0;
  column = 0;
  num_columns = 1;
    
  section->widgets = calloc(section->num_widgets, sizeof(bg_gtk_widget_t));

  i = 0;
  count = 0;

  while(count  < section->num_widgets)
    {
    if(info[i].gettext_domain)
      translation_domain = info[i].gettext_domain;
    if(info[i].gettext_directory)
      bg_bindtextdomain(translation_domain, info[i].gettext_directory);
    
    if(info[i].flags & BG_PARAMETER_HIDE_DIALOG)
      {
      i++;
      continue;
      }
    section->widgets[count].tooltips = tooltips;
      
    if(info[i].flags & BG_PARAMETER_SYNC)
      {
      section->widgets[count].change_callback = set_param;
      section->widgets[count].change_callback_data = data;
      }
    section->widgets[count].info = &(info[i]);
    switch(info[i].type)
      {
      case BG_PARAMETER_CHECKBUTTON:
        bg_gtk_create_checkbutton(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_INT:
        bg_gtk_create_int(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_FLOAT:
        bg_gtk_create_float(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_SLIDER_INT:
        bg_gtk_create_slider_int(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_TIME:
        bg_gtk_create_time(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_SLIDER_FLOAT:
        bg_gtk_create_slider_float(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_STRING:
      case BG_PARAMETER_STRING_HIDDEN:
        bg_gtk_create_string(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_STRINGLIST:
        bg_gtk_create_stringlist(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_COLOR_RGB:
        bg_gtk_create_color_rgb(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_COLOR_RGBA:
        bg_gtk_create_color_rgba(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_FILE:
        bg_gtk_create_file(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_DIRECTORY:
        bg_gtk_create_directory(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_FONT:
        bg_gtk_create_font(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_DEVICE:
        bg_gtk_create_device(&(section->widgets[count]), &(info[i]),
                                  translation_domain);
        break;
      case BG_PARAMETER_MULTI_MENU:
        bg_gtk_create_multi_menu(&(section->widgets[count]), &(info[i]),
                                 cfg_section, set_param, data,
                                  translation_domain);
        break;
      case BG_PARAMETER_MULTI_LIST:
        bg_gtk_create_multi_list(&(section->widgets[count]), &(info[i]),
                                 cfg_section, set_param, data,
                                 translation_domain);
        break;
      case BG_PARAMETER_MULTI_CHAIN:
        bg_gtk_create_multi_chain(&(section->widgets[count]), &(info[i]),
                                  cfg_section, set_param, data,
                                  translation_domain);
        break;
      case BG_PARAMETER_SECTION:
        break;
      }
    section->widgets[count].funcs->attach(section->widgets[count].priv, table,
                                 &row, &num_columns);

    /* Get the value from the config data... */
    if(cfg_section)
      bg_cfg_section_get_parameter(cfg_section, &(info[i]),
                                   &(section->widgets[count].value));
    /* ... or from the parameter default */
    else
      {
      bg_parameter_value_copy(&(section->widgets[count].value),
                              &(info[i].val_default),
                              &(info[i]));
      }
    if(section->widgets[count].info->flags & BG_PARAMETER_SYNC)
      bg_gtk_change_callback_block(&section->widgets[count], 1);
    section->widgets[count].funcs->get_value(&(section->widgets[count]));    
    if(section->widgets[count].info->flags & BG_PARAMETER_SYNC)
      bg_gtk_change_callback_block(&section->widgets[count], 0);
    i++;
    count++;
    }
  gtk_widget_show(table);
  return table;
  }

static int count_sections(bg_parameter_info_t * info)
  {
  int ret = 0;
  int i = 0;

  if(!info[0].name)
    return 0;

  if(info[0].type == BG_PARAMETER_SECTION)
    {
    while(info[i].name)
      {
      if(info[i].type == BG_PARAMETER_SECTION)
        ret++;
      i++;
      }
    return ret;
    }
  return 0;
  }

bg_dialog_t * bg_dialog_create(bg_cfg_section_t * section,
                               bg_set_parameter_func_t set_param,
                               void * callback_data,
                               bg_parameter_info_t * info,
                               const char * title)
  {
  int i, index;
  int num_sections;
  GtkWidget * label;
  GtkWidget * table;
  GtkTreeIter root_iter;
  GtkTreeModel * model;
  const char * translation_domain = (const char *)0;
  bg_dialog_t * ret = create_dialog(title);

  num_sections = count_sections(info);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(ret->treeview));
  
  if(num_sections)
    {
    ret->root_section.num_children = num_sections;
    ret->root_section.children =
      calloc(ret->root_section.num_children, sizeof(dialog_section_t));
    
    index = 0;

    for(i = 0; i < ret->root_section.num_children; i++)
      {
      if(info[index].gettext_domain)
        translation_domain = info[i].gettext_domain;
      if(info[index].gettext_directory)
        bg_bindtextdomain(translation_domain, info[i].gettext_directory);
      

      label = gtk_label_new(TR_DOM(info[index].long_name));
      gtk_widget_show(label);
      
      gtk_tree_store_append(GTK_TREE_STORE(model), &root_iter, NULL);
      gtk_tree_store_set(GTK_TREE_STORE(model), &root_iter, COLUMN_NAME,
                         info[index].long_name, -1);
            
      while(info[index].type == BG_PARAMETER_SECTION)
        index++;
      
      table = create_section(&(ret->root_section.children[i]), &(info[index]),
                             section, set_param, callback_data, ret->tooltips,
                             translation_domain);
      
      ret->root_section.children[i].notebook_index =
        gtk_notebook_get_n_pages(GTK_NOTEBOOK(ret->notebook));
      gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook), table, label);

      ret->root_section.children[i].parent = &(ret->root_section);
            
      while(info[index].name &&
            (info[index].type != BG_PARAMETER_SECTION))
        index++;
      }
    }
  else
    {
    label = gtk_label_new(title);
    gtk_widget_show(label);

    ret->root_section.num_children = 1;
    ret->root_section.children = calloc(1,
                           ret->root_section.num_children * sizeof(dialog_section_t));
    table =
      create_section(ret->root_section.children, info, section, set_param,
                     callback_data, ret->tooltips, (const char *)0);
    gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook), table, label);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(ret->notebook), 1);
    gtk_widget_hide(ret->scrolledwindow);
    }
  return ret;
  }

bg_dialog_t * bg_dialog_create_multi(const char * title)
  {
  bg_dialog_t * ret = create_dialog(title);
  return ret;
  }

void bg_dialog_add_child(bg_dialog_t *d, void * _parent,
                         const char * name,
                         bg_cfg_section_t * section,
                         bg_set_parameter_func_t set_param,
                         void * callback_data,
                         bg_parameter_info_t * info)
  {
  GtkTreeIter iter, parent_iter;
  int num_items;
  int num_sections;
  GtkWidget * table;
  GtkWidget * tab_label;
  int i, item_index, section_index;
  GtkTreeModel *model;
  const char * translation_domain = (const char *)0;
  dialog_section_t * parent = (dialog_section_t*)_parent;

  num_items = 0;
  num_sections = 0;
  
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->treeview));
  if(info)
    {
    while(info[num_items+num_sections].name)
      {
      if(info[num_items+num_sections].type == BG_PARAMETER_SECTION)
        num_sections++;
      else
        num_items++;
      }
    }
  
  if(!num_sections)
    {
    parent->children = realloc(parent->children,
                            (parent->num_children+1)*sizeof(dialog_section_t));
    memset(&(parent->children[parent->num_children]),0,
           sizeof(parent->children[parent->num_children]));
    
    table = create_section(&(parent->children[parent->num_children]),
                           info, section, set_param, callback_data, d->tooltips,
                           (const char*)0);
    tab_label = gtk_label_new(name);
    gtk_widget_show(tab_label);

    parent->children[parent->num_children].notebook_index =
      gtk_notebook_get_n_pages(GTK_NOTEBOOK(d->notebook));

    gtk_notebook_append_page(GTK_NOTEBOOK(d->notebook),
                             table, tab_label);

    if(parent == &(d->root_section))
      {
      gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
      }
    else
      {
      section_to_iter(d, parent, &parent_iter);
      gtk_tree_store_append(GTK_TREE_STORE(model), &iter, &parent_iter);
      }
    
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COLUMN_NAME,
                       name, -1);
    
    parent->children[parent->num_children].parent = parent;
    
    parent->num_children++;
    }
  else
    {
    parent->children = realloc(parent->children,
                               (parent->num_children+num_sections)*
                               sizeof(dialog_section_t));

    memset(&(parent->children[parent->num_children]),0,
           sizeof(parent->children[parent->num_children]) * num_sections);
    
    item_index = 0;
    section_index = parent->num_children;
    for(i = 0; i < num_sections; i++)
      {
      if(info[item_index].gettext_domain)
        translation_domain = info[item_index].gettext_domain;
      if(info[item_index].gettext_directory)
        bg_bindtextdomain(translation_domain, info[item_index].gettext_directory);
      
      tab_label = gtk_label_new(TR_DOM(info[item_index].long_name));
      gtk_widget_show(tab_label);
      
      if(parent == &(d->root_section))
        {
        parent = (dialog_section_t*)_parent;
        gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
        }
      else
        {
        section_to_iter(d, parent, &parent_iter);
        gtk_tree_store_append(GTK_TREE_STORE(model), &iter, &parent_iter);
        }
      
      gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COLUMN_NAME,
                         info[item_index].long_name, -1);

      item_index++;
      
      table = create_section(&(parent->children[section_index]),
                             &(info[item_index]),
                             section, set_param, callback_data, d->tooltips,
                             translation_domain);
      
      parent->children[section_index].parent = parent;

      parent->children[section_index].notebook_index =
        gtk_notebook_get_n_pages(GTK_NOTEBOOK(d->notebook));
      
      gtk_notebook_append_page(GTK_NOTEBOOK(d->notebook),
                               table, tab_label);
      
      while((info[item_index].name) &&
            (info[item_index].type != BG_PARAMETER_SECTION))
        item_index++;
      section_index++;
      }
    parent->num_children += num_sections;
    }

  }

void bg_dialog_add(bg_dialog_t *d,
                   const char * name,
                   bg_cfg_section_t * section,
                   bg_set_parameter_func_t set_param,
                   void * callback_data,
                   bg_parameter_info_t * info)
  {
  bg_dialog_add_child(d, &(d->root_section), name,
                      section, set_param, callback_data, info);
  }

void bg_dialog_show(bg_dialog_t * d)
  {
  
  if(d->visible)
    {
    gtk_window_present(GTK_WINDOW(d->window));
    return;
    }
  
  d->visible = 1;
  

  gtk_widget_show(d->window);
  gtk_widget_grab_default(d->ok_button);
  gtk_widget_grab_focus(d->ok_button);
  gtk_main();
  }


static void destroy_section(dialog_section_t * s)
  {
  int i;

  if(s->num_widgets)
    {
    for(i = 0; i < s->num_widgets; i++)
      {
      s->widgets[i].funcs->destroy(&(s->widgets[i]));
      }
    free(s->widgets);
    }
  
  if(s->num_children)
    {
    for(i = 0; i < s->num_children; i++)
      {
      destroy_section(&(s->children[i]));
      }
    free(s->children);
    }
  
  }

void bg_dialog_destroy(bg_dialog_t * d)
  {
  destroy_section(&(d->root_section));
  gtk_widget_destroy(d->window);

  g_object_unref(d->tooltips);
  
  free(d);
  }

void bg_gtk_change_callback(GtkWidget * gw, gpointer data)
  {
  bg_gtk_widget_t * w = (bg_gtk_widget_t*)data;
  
  w->funcs->set_value(w);
  if(w->change_callback)
    w->change_callback(w->change_callback_data,
                       w->info->name, &(w->value));
  }

void bg_gtk_change_callback_block(bg_gtk_widget_t * w, int block)
  {
  if(!w->callback_widget)
    {
    return;
    }
  if(block)
    g_signal_handler_block(w->callback_widget, w->callback_id);
  else
    g_signal_handler_unblock(w->callback_widget, w->callback_id);
  }

void * bg_dialog_add_parent(bg_dialog_t *d, void * _parent, const char * label)
  {
  GtkTreeIter iter, parent_iter;
  dialog_section_t * parent;
  GtkTreeModel *model;
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->treeview));
  if(_parent)
    {
    parent = (dialog_section_t*)_parent;

    section_to_iter(d, parent, &parent_iter);
    gtk_tree_store_append(GTK_TREE_STORE(model), &iter, &parent_iter);
    }
  else
    {
    parent = &(d->root_section);
    gtk_tree_store_append(GTK_TREE_STORE(model), &iter, NULL);
    }

  gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COLUMN_NAME,
                     label, -1);

  
  parent->children = realloc(parent->children,
                             sizeof(*(parent->children))*(parent->num_children+1));

  memset(&(parent->children[parent->num_children]),0,
         sizeof(parent->children[parent->num_children]));

  parent->children[parent->num_children].parent = parent;
  
  parent->num_children++;
  return &(parent->children[parent->num_children-1]);
  }

void bg_dialog_set_tooltips(bg_dialog_t *d, int enable)
  {
  if(enable)
    gtk_tooltips_enable(d->tooltips);
  else
    gtk_tooltips_disable(d->tooltips);
  }
