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

#include <gui_gtk/message.h>

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

void bg_gtk_message(const char * message, int type)
  {
  GtkWidget * table;
  GtkWidget * buttonbox;
  int ret;
  message_t * q;

  q = calloc(1, sizeof(*q));
    
  /* Create objects */
  
  q->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_position(GTK_WINDOW(q->window), GTK_WIN_POS_CENTER);

  
  q->ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
  q->label = gtk_label_new(message);
  
  /* Set attributes */

  gtk_window_set_modal(GTK_WINDOW(q->window), 1);
  gtk_window_set_title(GTK_WINDOW(q->window), "Message");
  gtk_window_set_position(GTK_WINDOW(q->window), GTK_WIN_POS_CENTER);
  GTK_WIDGET_SET_FLAGS (q->ok_button, GTK_CAN_DEFAULT);

  gtk_label_set_justify(GTK_LABEL(q->label), GTK_JUSTIFY_CENTER);
  
  /* Set callbacks */

  g_signal_connect(G_OBJECT(q->ok_button), "clicked",
                   G_CALLBACK(button_callback),
                   (gpointer)q);


  g_signal_connect(G_OBJECT(q->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   (gpointer)q);

  /* Show everything */

  gtk_widget_show(q->label);
  gtk_widget_show(q->ok_button);
  
  /* Pack the objects */

  table = gtk_table_new(2, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  
  gtk_table_attach_defaults(GTK_TABLE(table), q->label, 0, 1, 0, 1);

  buttonbox = gtk_hbutton_box_new();

  gtk_box_set_spacing(GTK_BOX(buttonbox), 10);
  
  gtk_container_add(GTK_CONTAINER(buttonbox), q->ok_button);

  gtk_widget_show(buttonbox);
  gtk_table_attach_defaults(GTK_TABLE(table), buttonbox, 0, 1, 1, 2);
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(q->window), table);
  
  gtk_widget_show(q->window);
  gtk_widget_grab_default(q->ok_button);
  
  gtk_main();
  
  /* Destroy everything */
  gtk_widget_hide(q->window);
  g_object_unref(q->window);
  free(q);
  }
