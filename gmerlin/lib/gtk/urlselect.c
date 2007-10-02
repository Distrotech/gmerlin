/*****************************************************************
 
  urlselect.c
 
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
// #include <gui_gtk/question.h>

#include <config.h>

#include <pluginregistry.h>

#include <gui_gtk/plugin.h>

#include <gui_gtk/urlselect.h>
#include <gui_gtk/gtkutils.h>

#include <utils.h>

struct bg_gtk_urlsel_s
  {
  GtkWidget * window;
  GtkWidget * add_button;
  GtkWidget * close_button;
  GtkWidget * entry;
    
  bg_gtk_plugin_menu_t * plugins;
  void (*add_files)(char ** files, const char * plugin,
                    void * data);

  void (*close_notify)(bg_gtk_urlsel_t * f, void * data);
  
  void * callback_data;

  int is_modal;
  };

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_urlsel_t * f;
  const char * plugin = (const char*)0;

  char * urls[2];
  
  f = (bg_gtk_urlsel_t *)data;

  if(w == f->add_button)
    {
    if(f->plugins)
      plugin = bg_gtk_plugin_menu_get_plugin(f->plugins);

    urls[0] = bg_strdup(NULL, gtk_entry_get_text(GTK_ENTRY(f->entry)));
    urls[1] = NULL;
    
    f->add_files(urls, plugin,
                 f->callback_data);
    free(urls[0]);
    }
  
  else if((w == f->window) || (w == f->close_button))
    {
    if(f->close_notify)
      f->close_notify(f, f->callback_data);
    
    gtk_widget_hide(f->window);
    if(f->is_modal)
      gtk_main_quit();
    bg_gtk_urlsel_destroy(f);
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

bg_gtk_urlsel_t *
bg_gtk_urlsel_create(const char * title,
                     void (*add_files)(char ** files, const char * plugin,
                                       void * data),
                     void (*close_notify)(bg_gtk_urlsel_t *,
                                          void * data),
                     void * user_data,
                     GtkWidget * parent_window,
                     bg_plugin_registry_t * plugin_reg, int type_mask,
                     int flag_mask)
  {
  bg_gtk_urlsel_t * ret;
  GtkWidget * box;
  GtkWidget * mainbox;
  GtkWidget * label;
  
  ret = calloc(1, sizeof(*ret));

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

  /* Create entry */
  
  ret->entry = gtk_entry_new();
  gtk_widget_show(ret->entry);
  
  /* Create plugin menu */

  if(plugin_reg)
    ret->plugins = bg_gtk_plugin_menu_create(1, plugin_reg, type_mask, flag_mask);
  
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
  box = gtk_hbox_new(0, 5);

  label = gtk_label_new(TR("URL:"));
  gtk_widget_show(label);
  
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(box), ret->entry);
  gtk_widget_show(box);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), box);

  if(ret->plugins)
    gtk_box_pack_start_defaults(GTK_BOX(mainbox),
                                bg_gtk_plugin_menu_get_widget(ret->plugins));

  box = gtk_hbutton_box_new();

  gtk_box_set_spacing(GTK_BOX(box), 5);
  
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

/* Destroy urlselector */

void bg_gtk_urlsel_destroy(bg_gtk_urlsel_t * urlsel)
  {
  //  g_object_unref(G_OBJECT(urlsel));
  free(urlsel);
  }

/* Show the window */

void bg_gtk_urlsel_run(bg_gtk_urlsel_t * urlsel, int modal)
  {
  gtk_window_set_modal(GTK_WINDOW(urlsel->window), modal);
  gtk_widget_show(urlsel->window);

  gtk_widget_grab_default(urlsel->close_button);
  gtk_widget_grab_focus(urlsel->close_button);
  
  urlsel->is_modal = modal;
  if(modal)
    gtk_main();
  
  }
