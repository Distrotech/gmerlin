/*****************************************************************
 
  codecinfo.c
 
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
#include <gtk/gtk.h>
#include <parameter.h>
#include <gui_gtk/codecinfo.h>

typedef struct bg_gtk_codec_info_s
  {
  GtkWidget * window;
  GtkWidget * close_button;
  } bg_gtk_codec_info_t;

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_codec_info_t * win;
  win = (bg_gtk_codec_info_t*)data;

  gtk_widget_hide(win->window);
  gtk_widget_destroy(win->window);
  free(win);
  } 

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  } 

static bg_gtk_codec_info_t *
bg_gtk_codec_info_create(const bg_parameter_info_t * info, int selected)
  {
  GtkWidget * label;
  GtkWidget * table;
  int num_rows;
  int row;
    
  bg_gtk_codec_info_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  if(info->codec_long_names)
    gtk_window_set_title(GTK_WINDOW(ret->window),
                         info->codec_long_names[selected]);
  else
    gtk_window_set_title(GTK_WINDOW(ret->window),
                         info->codec_names[selected]);
    
  /* Create close button */

  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);

  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)ret);
  gtk_widget_show(ret->close_button);

  num_rows = 1;
  if(info->codec_long_names)
    num_rows++;
  if(info->codec_descriptions)
    num_rows++;
  
  table = gtk_table_new(num_rows, 2, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  
  /* Create labels */

  row = 0;
  
  label = gtk_label_new("Name:");
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);

  label = gtk_label_new(info->codec_names[selected]);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, row, row+1);

  if(info->codec_long_names)
    {
    row++;

    label = gtk_label_new("Long Name:");
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_widget_show(label);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    
    label = gtk_label_new(info->codec_long_names[selected]);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_widget_show(label);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, row, row+1);
    
    }

  if(info->codec_descriptions)
    {
    row++;

    label = gtk_label_new("Description:");
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_widget_show(label);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, row, row+1);
    
    label = gtk_label_new(info->codec_descriptions[selected]);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_widget_show(label);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, row, row+1);
    }

  gtk_table_attach(GTK_TABLE(table), ret->close_button, 0, 2, 5, 6,
                   GTK_SHRINK, GTK_SHRINK, 0, 0);
  
  
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(ret->window), table);
    
  return ret;
  }

void bg_gtk_codec_info_show(const bg_parameter_info_t * i, int index)
  {
  bg_gtk_codec_info_t * info;
  info = bg_gtk_codec_info_create(i, index);
  
  gtk_widget_show(info->window);
  }


