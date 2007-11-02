/*****************************************************************
 
  message.c
 
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

#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include <config.h>

#include <gui_gtk/message.h>
#include <gui_gtk/gtkutils.h>

typedef struct
  {
  GtkWidget * window;
  GtkWidget * ok_button;
  GtkWidget * label;
  } message_t;

static void button_callback(GtkWidget * w, gpointer * data)
  {

  gtk_main_quit();
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * evt,
                            gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

void bg_gtk_message(const char * message, int type, GtkWidget * parent)
  {
  GtkWidget * buttonbox;
  message_t * q;
  GtkWidget * label;

  GtkWidget * mainbox;
  GtkWidget * hbox;
  GtkWidget * image = (GtkWidget *)0;
    
  q = calloc(1, sizeof(*q));
    
  /* Create objects */
  
  q->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_position(GTK_WINDOW(q->window), GTK_WIN_POS_CENTER_ON_PARENT);

  parent = bg_gtk_get_toplevel(parent);
  if(parent)
    gtk_window_set_transient_for(GTK_WINDOW(q->window),
                                 GTK_WINDOW(parent));
  
  q->ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
  label = gtk_label_new(message);

  if(type == BG_GTK_MESSAGE_INFO)
    image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
                                     GTK_ICON_SIZE_DIALOG);
  else if(type == BG_GTK_MESSAGE_ERROR)
    image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR,
                                     GTK_ICON_SIZE_DIALOG);
  
  /* Set attributes */

  gtk_window_set_modal(GTK_WINDOW(q->window), 1);
  gtk_window_set_title(GTK_WINDOW(q->window), TR("Message"));
  gtk_window_set_position(GTK_WINDOW(q->window), GTK_WIN_POS_CENTER_ON_PARENT);
  GTK_WIDGET_SET_FLAGS (q->ok_button, GTK_CAN_DEFAULT);

  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
  
  /* Set callbacks */

  g_signal_connect(G_OBJECT(q->ok_button), "clicked",
                   G_CALLBACK(button_callback),
                   (gpointer)q);


  g_signal_connect(G_OBJECT(q->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   (gpointer)q);

  /* Show everything */

  gtk_widget_show(label);
  gtk_widget_show(image);
  gtk_widget_show(q->ok_button);
  
  /* Pack the objects */

  mainbox = gtk_vbox_new(0, 5);
  hbox    = gtk_hbox_new(0, 5);
    
  gtk_container_set_border_width(GTK_CONTAINER(mainbox), 5);
  
  gtk_box_pack_start_defaults(GTK_BOX(hbox), image);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
  gtk_widget_show(hbox);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), hbox);
  
  buttonbox = gtk_hbutton_box_new();

  gtk_box_set_spacing(GTK_BOX(buttonbox), 10);
  
  gtk_container_add(GTK_CONTAINER(buttonbox), q->ok_button);

  gtk_widget_show(buttonbox);

  gtk_box_pack_start_defaults(GTK_BOX(mainbox), buttonbox);
  
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(q->window), mainbox);
  
  gtk_widget_show(q->window);
  
  gtk_main();

  /* Window has gone, fetch the answer */


  /* Destroy everything */
  gtk_widget_hide(q->window);
  gtk_widget_destroy(q->window);
  free(q);
  }
