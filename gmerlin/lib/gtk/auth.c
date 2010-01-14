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

#include <config.h>

#include <stdlib.h>
#include <gtk/gtk.h>

#include <gui_gtk/auth.h>
#include <gui_gtk/gtkutils.h>

#include <gmerlin/utils.h>

typedef struct
  {
  GtkWidget * window;
  GtkWidget * user;
  GtkWidget * pass;
  GtkWidget * save_auth;

  GtkWidget * ok_button;
  GtkWidget * cancel_button;

  int ok_clicked;
  } userpass_win;

static void button_callback(GtkWidget * w, gpointer data)
  {
  userpass_win * win = (userpass_win *)data;
  if(w == win->ok_button)
    win->ok_clicked = 1;
  gtk_widget_hide(win->window);
  gtk_main_quit();
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

static userpass_win * userpass_win_create(const char * resource)
  {
  GtkWidget * w;
  GtkWidget * table;
  GtkWidget * box;
  GtkWidget * mainbox;
  
  userpass_win * ret;
  ret = calloc(1, sizeof(*ret));

  /* Create window */
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), TR("Authentication"));
  gtk_window_set_modal(GTK_WINDOW(ret->window), 1);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(ret->window), 5);

  /* Create OK/Cancel */

  ret->ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
  ret->cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

  GTK_WIDGET_SET_FLAGS(ret->ok_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(ret->cancel_button, GTK_CAN_DEFAULT);
  
  g_signal_connect(G_OBJECT(ret->ok_button),
                   "clicked", G_CALLBACK(button_callback), ret);
  g_signal_connect(G_OBJECT(ret->cancel_button),
                   "clicked", G_CALLBACK(button_callback), ret);
  g_signal_connect(G_OBJECT(ret->window), "delete-event",
                   G_CALLBACK(delete_callback), ret);

  gtk_widget_show(ret->ok_button);
  gtk_widget_show(ret->cancel_button);
  
  ret->user = gtk_entry_new();
  ret->pass = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(ret->pass), FALSE);

  gtk_widget_show(ret->user);
  gtk_widget_show(ret->pass);
  
  ret->save_auth = gtk_check_button_new_with_label(TR("Save user/password (can be dangerous!)"));
  gtk_widget_show(ret->save_auth);
    
  /* Pack everything */

  mainbox = gtk_vbox_new(0, 5);
  
  table = gtk_table_new(5, 3, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);

  w = gtk_label_new(TR("Enter username and password for"));
  gtk_widget_show(w);
  gtk_table_attach(GTK_TABLE(table), w, 0, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  w = gtk_label_new(resource);
  gtk_widget_show(w);
  gtk_table_attach(GTK_TABLE(table), w, 0, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
 
#ifdef GTK_STOCK_DIALOG_AUTHENTICATION 
  w = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION,
                               GTK_ICON_SIZE_DIALOG);
  gtk_widget_show(w);
  gtk_table_attach(GTK_TABLE(table), w, 0, 1, 2, 5, GTK_FILL, GTK_FILL, 0, 0);
#endif

  w = gtk_label_new(TR("Username"));
  gtk_widget_show(w);
  gtk_table_attach(GTK_TABLE(table), w, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach_defaults(GTK_TABLE(table), ret->user, 2, 3, 2, 3);

  w = gtk_label_new(TR("Password"));
  gtk_widget_show(w);
  gtk_table_attach(GTK_TABLE(table), w, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach_defaults(GTK_TABLE(table), ret->pass, 2, 3, 3, 4);

  gtk_table_attach_defaults(GTK_TABLE(table), ret->save_auth, 1, 3, 4, 5);
    
  gtk_widget_show(table);
  bg_gtk_box_pack_start_defaults(GTK_BOX(mainbox), table);
  
  
  box = gtk_hbutton_box_new();

  gtk_box_set_spacing(GTK_BOX(box), 5);
  
  gtk_container_add(GTK_CONTAINER(box), ret->cancel_button);
  gtk_container_add(GTK_CONTAINER(box), ret->ok_button);
  gtk_widget_show(box);
  gtk_box_pack_start(GTK_BOX(mainbox), box, FALSE, FALSE, 0);

  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(ret->window), mainbox);

  
  return ret;
  }

int bg_gtk_get_userpass(const char * resource,
                        char ** user, char ** pass,
                        int * save_password,
                        void * data)
  {
  int ret = 0;
  userpass_win * win = userpass_win_create(resource);
  gtk_widget_show(win->window);
  gtk_main();
  if(win->ok_clicked)
    {
    *user = bg_strdup(*user, gtk_entry_get_text(GTK_ENTRY(win->user)));
    *pass = bg_strdup(*pass, gtk_entry_get_text(GTK_ENTRY(win->pass)));
    *save_password =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->save_auth));
    ret = 1;
    }

  gtk_widget_destroy(win->window);
  free(win);
  return ret;
  }

