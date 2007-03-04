/*****************************************************************
 
  driveselect.c
 
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


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include <config.h>

#include <pluginregistry.h>
#include <gui_gtk/plugin.h>

#include <gui_gtk/driveselect.h>
#include <gui_gtk/gtkutils.h>

#include <utils.h>

#if GTK_MINOR_VERSION >= 4
#define GTK_2_4
#endif

typedef struct menu_s
  {
  GtkWidget * widget;
  int selected;
#ifdef GTK_2_4
  int num_options;
#else
  GList * options;
#endif
  void (*change_callback)(struct menu_s * menu, void * data);
  void * change_callback_data;
  } menu_t;

#ifdef GTK_2_4

static void combo_box_change_callback(GtkWidget * wid, gpointer data)
  {
  menu_t * m;
  m = (menu_t*)data;
  m->selected = gtk_combo_box_get_active(GTK_COMBO_BOX(m->widget));

  if(m->change_callback)
    m->change_callback(m, m->change_callback_data);
  }

#else

static void entry_change_callback(GtkWidget * wid, gpointer data)
  {
  const char * entry_str;
  const char * list_str;
  
  int i;
  menu_t * m;
  m = (menu_t*)data;

  i = 0;

  entry_str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(m->widget)->entry));

  m->selected = 0;
    
  if(entry_str)
    {
    while(1)
      {
      list_str = g_list_nth_data(m->options, m->selected);
      
      if(!list_str)
        {
        m->selected = 0;
        break;
        }
      else if(!strcmp(list_str, entry_str))
        {
        break;
        }
      else
        {
        m->selected++;
        }
      }
    }

  if(m->change_callback)
    m->change_callback(m, m->change_callback_data);
  }

#endif

static void menu_init(menu_t * m, void (*change_callback)(struct menu_s * menu, void * data),
                      void * change_callback_data)
  {
#ifdef GTK_2_4
  m->widget = gtk_combo_box_new_text();
  g_signal_connect(G_OBJECT(m->widget),
                   "changed", G_CALLBACK(combo_box_change_callback),
                   (gpointer)m);
#else
  m->widget = gtk_combo_new();
  gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(m->widget)->entry), FALSE);
  g_signal_connect(G_OBJECT(GTK_EDITABLE(GTK_COMBO(m->widget)->entry)),
                   "changed", G_CALLBACK(entry_change_callback),
                   (gpointer)m);

#endif
  m->change_callback      = change_callback;
  m->change_callback_data = change_callback_data;
  
  gtk_widget_show(m->widget);
  }

static void menu_set_options(menu_t * m, char ** options)
  {
#ifdef GTK_2_4
  int i;
  for(i = 0; i < m->num_options; i++)
    {
    gtk_combo_box_remove_text(GTK_COMBO_BOX(m->widget), 0);
    }
  m->num_options = 0;
  while(options[m->num_options])
    {
    gtk_combo_box_append_text(GTK_COMBO_BOX(m->widget), options[m->num_options]);
    m->num_options++;
    }
  /* Select first entry */

  gtk_combo_box_set_active(GTK_COMBO_BOX(m->widget), 0);
  
#else

  int i;
  if(m->options)
    g_list_free(m->options);
  
  m->options = (GList*)0;
  i = 0;

  while(options[i])
    {
    m->options = g_list_append(m->options, options[i]);
    i++;
    }

  gtk_combo_set_popdown_strings(GTK_COMBO(m->widget), m->options);
#endif
  }

static void menu_cleanup(menu_t * m)
  {
#ifdef GTK_2_4
#else
  if(m->options)
    g_list_free(m->options);
#endif
  }

struct bg_gtk_drivesel_s
  {
  char             ** drive_labels;
  char             ** plugin_labels;
  bg_device_info_t ** drive_infos;
  char             ** plugin_names;
  
  GtkWidget * window;
  GtkWidget * add_button;
  GtkWidget * close_button;
  GtkWidget * entry;
    
  menu_t plugins;
  menu_t drives;

  void (*add_files)(char ** files, const char * plugin,
                    void * data);

  void (*close_notify)(bg_gtk_drivesel_t * f, void * data);
  
  void * callback_data;

  int is_modal;
  };

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_drivesel_t * f;
  const char * plugin = (const char*)0;

  char * drives[2];
  
  f = (bg_gtk_drivesel_t *)data;

  if(w == f->add_button)
    {
    //    plugin = menu_get_current(&(f->plugins));
    
    drives[0] = f->drive_infos[f->plugins.selected][f->drives.selected].device;
    drives[1] = NULL;

    plugin = f->plugin_labels[f->plugins.selected];

    f->add_files(drives, plugin, f->callback_data);
    }
  
  else if((w == f->window) || (w == f->close_button))
    {
    if(f->close_notify)
      f->close_notify(f, f->callback_data);
    
    gtk_widget_hide(f->window);
    if(f->is_modal)
      gtk_main_quit();
    bg_gtk_drivesel_destroy(f);
    }

  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

static gboolean destroy_callback(GtkWidget * w, GdkEvent * event,
                                  gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

static void change_callback(menu_t * m, void * data)
  {
  int i, num;

  bg_gtk_drivesel_t * f;
  f = (bg_gtk_drivesel_t *)data;

  /* Free drive labels */

  i = 0;
  if(f->drive_labels)
    {
    free(f->drive_labels);
    }
  
  /* Count the devices */
  
  num = 0;
  while(f->drive_infos[f->plugins.selected][num].device)
    num++;

  f->drive_labels = calloc(num+1, sizeof(*(f->drive_labels)));

  for(i = 0; i < num; i++)
    {
    if(f->drive_infos[f->plugins.selected][i].name)
      f->drive_labels[i] = f->drive_infos[f->plugins.selected][i].name;
    else
      f->drive_labels[i] = f->drive_infos[f->plugins.selected][i].device;
    }

  menu_set_options(&(f->drives), f->drive_labels);    
  }

bg_gtk_drivesel_t *
bg_gtk_drivesel_create(const char * title,
                       void (*add_files)(char ** files, const char * plugin,
                                         void * data),
                       void (*close_notify)(bg_gtk_drivesel_t *,
                                            void * data),
                       char ** plugins,
                       bg_device_info_t ** drive_infos,
                       void * user_data,
                       GtkWidget * parent_window,
                       bg_plugin_registry_t * plugin_reg)
  {
  bg_gtk_drivesel_t * ret;
  GtkWidget * box;
  GtkWidget * table;
  GtkWidget * mainbox;
  GtkWidget * label;
  const bg_plugin_info_t * info;
  int index;
  
  ret = calloc(1, sizeof(*ret));
  ret->drive_infos = drive_infos;  
  ret->plugin_names = plugins;
  
  /* Create window */

  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), title);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(ret->window), 5);
    
  if(parent_window)
    {
    gtk_window_set_transient_for(GTK_WINDOW(ret->window),
                                 GTK_WINDOW(parent_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(ret->window), TRUE);
    g_signal_connect(G_OBJECT(ret->window), "destroy-event",
                     G_CALLBACK(destroy_callback), ret);
    }
  
  /* Create plugin menu */

  index = 0;
  while(plugins[index])
    index++;

  ret->plugin_labels = calloc(index+1, sizeof(*ret->plugin_labels));

  index = 0;
  while(plugins[index])
    {
    info = bg_plugin_find_by_name(plugin_reg, plugins[index]);

    bg_bindtextdomain(info->gettext_domain, info->gettext_directory);
    ret->plugin_labels[index] = bg_strdup(ret->plugin_labels[index],
                                          TRD(info->long_name,
                                              info->gettext_domain));
    index++;
    }
  
  menu_init(&(ret->plugins), change_callback, ret);
  menu_init(&(ret->drives), NULL, NULL);

  menu_set_options(&(ret->plugins), ret->plugin_labels);
  
  /* Create Buttons */

  ret->add_button = gtk_button_new_from_stock(GTK_STOCK_ADD);
  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);

  GTK_WIDGET_SET_FLAGS(ret->close_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(ret->add_button, GTK_CAN_DEFAULT);
  
  /* Set callbacks */

  g_signal_connect(G_OBJECT(ret->window), "delete-event",
                   G_CALLBACK(delete_callback), ret);
  g_signal_connect(G_OBJECT(ret->add_button),
                   "clicked", G_CALLBACK(button_callback), ret);
  g_signal_connect(G_OBJECT(ret->close_button),
                   "clicked", G_CALLBACK(button_callback), ret);

  /* Show Buttons */

  gtk_widget_show(ret->add_button);
  gtk_widget_show(ret->close_button);
  
  /* Pack everything */

  mainbox = gtk_vbox_new(0, 5);

  table = gtk_table_new(2, 2, 0);

  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  
  
  label = gtk_label_new(TR("Plugin:"));
  gtk_widget_show(label);

  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach_defaults(GTK_TABLE(table), ret->plugins.widget, 1, 2, 0, 1);

  label = gtk_label_new(TR("Drive:"));
  gtk_widget_show(label);

  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach_defaults(GTK_TABLE(table), ret->drives.widget, 1, 2, 1, 2);
  
  gtk_widget_show(table);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), table);
  
  box = gtk_hbutton_box_new();

  gtk_container_add(GTK_CONTAINER(box), ret->close_button);
  gtk_container_add(GTK_CONTAINER(box), ret->add_button);
  gtk_widget_show(box);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), box);
  
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(ret->window), mainbox);
  
  /* Set pointers */
  
  ret->add_files = add_files;
  ret->close_notify = close_notify;
  ret->callback_data = user_data;
  
  return ret;
  }

/* Destroy driveselector */

void bg_gtk_drivesel_destroy(bg_gtk_drivesel_t * drivesel)
  {
  int index = 0;

  if(drivesel->plugin_labels)
    {
    while(drivesel->plugin_labels[index])
      free(drivesel->plugin_labels[index++]);
    }
  
  menu_cleanup(&(drivesel->plugins));
  menu_cleanup(&(drivesel->drives));
  if(drivesel->drive_labels)
    free(drivesel->drive_labels);
  //  g_object_unref(G_OBJECT(drivesel));
  free(drivesel);
  }

/* Show the window */

void bg_gtk_drivesel_run(bg_gtk_drivesel_t * drivesel, int modal)
  {
  gtk_window_set_modal(GTK_WINDOW(drivesel->window), modal);
  gtk_widget_show(drivesel->window);

  gtk_widget_grab_focus(drivesel->close_button);
  gtk_widget_grab_default(drivesel->close_button);
  
  drivesel->is_modal = modal;
  if(modal)
    gtk_main();
  
  }
