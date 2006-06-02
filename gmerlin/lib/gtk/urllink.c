/*****************************************************************
 
  urllink.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

#include <utils.h>
#include <gui_gtk/urllink.h>

struct bg_gtk_urllink_s
  {
  GtkWidget * widget;
  GdkCursor * cursor;
  char * url;
  char * browser_command;
  };

static void realize_callback(GtkWidget * w,
                             gpointer data)
  {
  bg_gtk_urllink_t * u;
  u = (bg_gtk_urllink_t *)data;
  gdk_window_set_cursor(u->widget->window, u->cursor);
  }

static void button_callback(GtkWidget * w, GdkEventButton * evt,
                            gpointer data)
  {
  char * command;
  bg_gtk_urllink_t * u;
  u = (bg_gtk_urllink_t *)data;
  command = bg_sprintf(u->browser_command, u->url);
  command = bg_strcat(command, " &");
  system(command);
  }

bg_gtk_urllink_t * bg_gtk_urllink_create(const char * text, const char * url)
  {
  bg_gtk_urllink_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  GtkWidget * label;
  char * tmp_string;
  ret->browser_command = bg_find_url_launcher();
  if(!ret->browser_command)
    {
    tmp_string = bg_sprintf("%s [%s]", text, url);
    ret->widget = gtk_label_new(tmp_string);
    gtk_widget_show(ret->widget);
    free(tmp_string);
    }
  else
    {
    ret->url = bg_strdup(ret->url, url);
    label = gtk_label_new("");
    tmp_string =
      bg_sprintf("<span foreground=\"blue\" underline=\"single\">%s</span>",
                 text);
    gtk_label_set_markup(GTK_LABEL(label), tmp_string);
    gtk_widget_show(label);
    
    ret->widget = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(ret->widget), label);
    ret->cursor = gdk_cursor_new(GDK_HAND2);

    gtk_widget_set_events(ret->widget, GDK_BUTTON_PRESS_MASK);
    
    g_signal_connect(ret->widget, "realize",
                     G_CALLBACK(realize_callback), ret);
    g_signal_connect(ret->widget, "button-press-event",
                     G_CALLBACK(button_callback), ret);
    
    gtk_widget_show(ret->widget);
    }
  if(tmp_string) free(tmp_string);
  return ret;
  }

GtkWidget * bg_gtk_urllink_get_widget(bg_gtk_urllink_t * u)
  {
  return u->widget;
  }

void bg_gtk_urllink_destroy(bg_gtk_urllink_t * u)
  {
  gdk_cursor_unref(u->cursor);
  if(u->url) free(u->url);
  if(u->browser_command) free(u->browser_command);
  free(u);
  }
