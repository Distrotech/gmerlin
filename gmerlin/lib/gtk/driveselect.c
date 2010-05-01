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


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include <config.h>

#include <gmerlin/pluginregistry.h>
#include <gui_gtk/plugin.h>

#include <gui_gtk/driveselect.h>
#include <gui_gtk/gtkutils.h>

#include <gmerlin/utils.h>

struct bg_gtk_drivesel_s
  {
  GtkWidget * window;
  GtkWidget * add_button;
  GtkWidget * close_button;
  GtkWidget * drive_menu;
  
  const bg_plugin_info_t * plugin_info;
  bg_gtk_plugin_menu_t * plugin_menu;
  
  void (*add_files)(char ** files, const char * plugin,
                    void * data);

  void (*close_notify)(bg_gtk_drivesel_t * f, void * data);
  
  void * callback_data;

  int is_modal;
  int num_drives;

  bg_plugin_registry_t * plugin_reg;
  };

static void plugin_change_callback(bg_gtk_plugin_menu_t * m, void * data)
  {
  int i;
  bg_gtk_drivesel_t * ds;
  bg_device_info_t * devices;
  
  ds = (bg_gtk_drivesel_t*)data;
  
  for(i = 0; i < ds->num_drives; i++)
    gtk_combo_box_remove_text(GTK_COMBO_BOX(ds->drive_menu), 0);

  
  
  ds->plugin_info = bg_plugin_find_by_name(ds->plugin_reg,
                                           bg_gtk_plugin_menu_get_plugin(ds->plugin_menu));

  devices = ds->plugin_info->devices;
  
  ds->num_drives = 0;
  while(devices[ds->num_drives].device)
    {
    if(devices[ds->num_drives].name)
      gtk_combo_box_append_text(GTK_COMBO_BOX(ds->drive_menu),
                                devices[ds->num_drives].name);
    else
      gtk_combo_box_append_text(GTK_COMBO_BOX(ds->drive_menu),
                                devices[ds->num_drives].device);
    ds->num_drives++;
    }
  /* Select first entry */
  gtk_combo_box_set_active(GTK_COMBO_BOX(ds->drive_menu), 0);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_drivesel_t * f;
  const char * plugin = (const char*)0;

  char * drives[2];
  
  f = (bg_gtk_drivesel_t *)data;

  if(w == f->add_button)
    {
    //    plugin = menu_get_current(&f->plugins);
    
    drives[0] = f->plugin_info->devices[gtk_combo_box_get_active(GTK_COMBO_BOX(f->drive_menu))].device;
    drives[1] = NULL;

    plugin = f->plugin_info->name;
    
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

bg_gtk_drivesel_t *
bg_gtk_drivesel_create(const char * title,
                       void (*add_files)(char ** files, const char * plugin,
                                         void * data),
                       void (*close_notify)(bg_gtk_drivesel_t *,
                                            void * data),
                       void * user_data,
                       GtkWidget * parent_window,
                       bg_plugin_registry_t * plugin_reg,
                       int type_mask, int flag_mask)
  {
  bg_gtk_drivesel_t * ret;
  GtkWidget * box;
  GtkWidget * table;
  GtkWidget * mainbox;
  GtkWidget * label;
  
  ret = calloc(1, sizeof(*ret));
  
  /* Create window */

  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), title);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_container_set_border_width(GTK_CONTAINER(ret->window), 5);
    
  if(parent_window)
    {
    gtk_window_set_transient_for(GTK_WINDOW(ret->window),
                                 GTK_WINDOW(parent_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(ret->window), TRUE);
    g_signal_connect(G_OBJECT(ret->window), "destroy-event",
                     G_CALLBACK(destroy_callback), ret);
    }

  /* Create device menu */

  ret->drive_menu = gtk_combo_box_new_text();
  gtk_widget_show(ret->drive_menu);
  
  /* Create plugin menu */

  ret->plugin_reg = plugin_reg;
  ret->plugin_menu = bg_gtk_plugin_menu_create(0, plugin_reg, 
                                               type_mask, flag_mask);

  bg_gtk_plugin_menu_set_change_callback(ret->plugin_menu, plugin_change_callback,
                                         ret);
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
  
  bg_gtk_plugin_menu_attach(ret->plugin_menu, table,
                            0, 0);
  
  label = gtk_label_new(TR("Drive:"));
  gtk_widget_show(label);

  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach_defaults(GTK_TABLE(table), ret->drive_menu, 1, 2, 1, 2);
  
  gtk_widget_show(table);
  bg_gtk_box_pack_start_defaults(GTK_BOX(mainbox), table);
  
  box = gtk_hbutton_box_new();

  gtk_container_add(GTK_CONTAINER(box), ret->close_button);
  gtk_container_add(GTK_CONTAINER(box), ret->add_button);
  gtk_widget_show(box);
  bg_gtk_box_pack_start_defaults(GTK_BOX(mainbox), box);
  
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(ret->window), mainbox);
  
  /* Set pointers */
  
  ret->add_files = add_files;
  ret->close_notify = close_notify;
  ret->callback_data = user_data;

  plugin_change_callback(ret->plugin_menu, ret);
  
  return ret;
  }

/* Destroy driveselector */

void bg_gtk_drivesel_destroy(bg_gtk_drivesel_t * drivesel)
  {
  free(drivesel);
  }

/* Show the window */

void bg_gtk_drivesel_run(bg_gtk_drivesel_t * drivesel, int modal,
                         GtkWidget * parent)
  {
  if(modal)
    {
    parent = bg_gtk_get_toplevel(parent);
    if(parent)
      gtk_window_set_transient_for(GTK_WINDOW(drivesel->window),
                                   GTK_WINDOW(parent));
    
    }
  
  gtk_window_set_modal(GTK_WINDOW(drivesel->window), modal);
  gtk_widget_show(drivesel->window);

  gtk_widget_grab_focus(drivesel->close_button);
  gtk_widget_grab_default(drivesel->close_button);
  
  drivesel->is_modal = modal;
  if(modal)
    gtk_main();
  
  }
