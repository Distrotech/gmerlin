/*****************************************************************
 
  plugin_single.c
 
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

struct bg_gtk_plugin_widget_single_s
  {
  GtkWidget * table;
  GtkWidget * combo;
  GtkWidget * config_button;
  GtkWidget * info_button;

  bg_plugin_registry_t * reg;
  const bg_plugin_info_t * info;
  bg_plugin_handle_t * handle;

  bg_parameter_info_t * parameters;
  bg_cfg_section_t * section;

  int32_t type_mask;
  void (*set_plugin)(bg_plugin_handle_t *, void*);
  void * set_plugin_data;
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

static void set_parameter(void * data, char * name,
                          bg_parameter_value_t * v)
  {
  bg_gtk_plugin_widget_single_t * widget;
  widget = (bg_gtk_plugin_widget_single_t *)data;

  if(widget->handle->plugin->set_parameter)
    {
    bg_plugin_lock(widget->handle);
    widget->handle->plugin->set_parameter(widget->handle->priv, name, v);
    bg_plugin_unlock(widget->handle);
    }
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_plugin_widget_single_t * widget;
  //  bg_gtk_plugin_info_t * info_window;
  
  bg_dialog_t * dialog;
  
  widget = (bg_gtk_plugin_widget_single_t *)data;
  
  if(w == widget->info_button)
    {
    bg_gtk_plugin_info_show(widget->info);
    }
  else if(w == widget->config_button)
    {
    dialog = bg_dialog_create(widget->section,
                              set_parameter,
                              (void*)widget,
                              widget->parameters);
    bg_dialog_show(dialog);
    bg_dialog_destroy(dialog);
    }
  }

static void change_callback(GtkWidget * w, gpointer data)
  {
  const char * long_name;
  
  bg_gtk_plugin_widget_single_t * widget;
  widget = (bg_gtk_plugin_widget_single_t *)data;
    
  long_name = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget->combo)->entry));

  if(*long_name == '\0')
    return;
  
  if(widget->handle)
    bg_plugin_unref(widget->handle);
  
  widget->info = bg_plugin_find_by_long_name(widget->reg, long_name);
    
  widget->handle = bg_plugin_load(widget->reg, widget->info);

  if(widget->handle->plugin->get_parameters)
    widget->parameters =
      widget->handle->plugin->get_parameters(widget->handle->priv);

  widget->section = bg_plugin_registry_get_section(widget->reg,
                                                   widget->info->name);
  
  if(widget->set_plugin)
    widget->set_plugin(widget->handle, widget->set_plugin_data);
    
  }

bg_gtk_plugin_widget_single_t *
bg_gtk_plugin_widget_single_create(bg_plugin_registry_t * reg,
                                   uint32_t type_mask,
                                   uint32_t flag_mask,
                                   void (*set_plugin)(bg_plugin_handle_t * plugin,
                                                     void * data),
                                   void * set_plugin_data)
  {
  GList * strings;

  int num_plugins, i;
  char * tmp_string;
  bg_gtk_plugin_widget_single_t * ret;
  const bg_plugin_info_t * info;
  
  ret = calloc(1, sizeof(*ret));

  ret->set_plugin = set_plugin;
  ret->set_plugin_data = set_plugin_data;
  
  ret->reg = reg;
  ret->type_mask = type_mask;
  /* Make combo */
    
  ret->combo = gtk_combo_new();
  gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(ret->combo)->entry),
                            FALSE);
  g_signal_connect(G_OBJECT(GTK_EDITABLE(GTK_COMBO(ret->combo)->entry)),
                   "changed", G_CALLBACK(change_callback),
                   (gpointer)ret);
  
  strings = (GList*)0;
  
  num_plugins = bg_plugin_registry_get_num_plugins(reg,
                                                   type_mask,
                                                   flag_mask);
  
  for(i = 0; i < num_plugins; i++)
    {
    info = bg_plugin_find_by_index(reg, i, type_mask, flag_mask);
    tmp_string = g_strdup(info->long_name);
    strings = g_list_append(strings, tmp_string);
    }

  gtk_combo_set_popdown_strings(GTK_COMBO(ret->combo), strings);

  /* Get default plugin */

  info = bg_plugin_registry_get_default(reg, type_mask);
  if(info)
    {
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(ret->combo)->entry),
                       info->long_name);
    }

  /* Show */
  
  gtk_widget_show(ret->combo);
  
  /* Make buttons */
    
  ret->config_button = create_pixmap_button("config_16.png");
  ret->info_button = create_pixmap_button("info_16.png");

  g_signal_connect(G_OBJECT(ret->config_button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->info_button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)ret);

  gtk_widget_show(ret->config_button);
  gtk_widget_show(ret->info_button);

  /* Pack the objects */

  ret->table = gtk_table_new(1, 3, 0);
  gtk_table_set_col_spacings(GTK_TABLE(ret->table), 5);
  gtk_table_attach_defaults(GTK_TABLE(ret->table), ret->combo,
                           0, 1, 0, 1);
  gtk_table_attach(GTK_TABLE(ret->table), ret->config_button,
                   1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(ret->table), ret->info_button,
                   2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(ret->table);

  return ret;
  }

void bg_gtk_plugin_widget_single_destroy(bg_gtk_plugin_widget_single_t * w)
  {
  if(w->info)
    bg_plugin_registry_set_default(w->reg, w->type_mask, w->info->name);
  free(w);
  }

GtkWidget *
bg_gtk_plugin_widget_single_get_widget(bg_gtk_plugin_widget_single_t * w)
  {
  return w->table;
  }

const bg_plugin_info_t *
bg_gtk_plugin_widget_single_get_plugin(bg_gtk_plugin_widget_single_t * w)
  {
  return w->info;
  }
