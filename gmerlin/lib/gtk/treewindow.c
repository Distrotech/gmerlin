/*****************************************************************
 
  treewindow.c
 
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

#include <tree.h>
#include <gui_gtk/tree.h>

extern void
gtk_decorated_window_move_resize_window(GtkWindow*,int, int, int, int);


struct bg_gtk_tree_window_s
  {
  bg_gtk_tree_widget_t * widget;
  GtkWidget * window;
  void (*close_callback)(bg_gtk_tree_window_t*,void*);
  void * close_callback_data;
  };

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  bg_gtk_tree_window_t * win;
  win = (bg_gtk_tree_window_t*)data;

  if(win->close_callback)
    win->close_callback(win, win->close_callback_data);
  
  bg_gtk_tree_window_hide(win);
  return TRUE;
  }

bg_gtk_tree_window_t *
bg_gtk_tree_window_create(bg_media_tree_t * tree,
                          void (*close_callback)(bg_gtk_tree_window_t*,void*),
                          void * close_callback_data)
  {
  bg_gtk_tree_window_t * ret = calloc(1, sizeof(*ret));
  ret->widget = bg_gtk_tree_widget_create(tree);
  
  ret->close_callback = close_callback;
  ret->close_callback_data = close_callback_data;
    
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_signal_connect(G_OBJECT(ret->window), "delete-event",
                   G_CALLBACK(delete_callback),
                   ret);
  
  gtk_container_add(GTK_CONTAINER(ret->window),
                    bg_gtk_tree_widget_get_widget(ret->widget));

  gtk_window_set_title(GTK_WINDOW(ret->window), "Gmerlin Media Tree");
  
  return ret;
  }

void bg_gtk_tree_window_destroy(bg_gtk_tree_window_t * w)
  {
  bg_gtk_tree_widget_destroy(w->widget);
  gtk_widget_hide(w->window);
  gtk_widget_destroy(w->window);
  free(w);
  }

void bg_gtk_tree_window_show(bg_gtk_tree_window_t* w)
  {
  int x, y, width, height;
  gtk_widget_show(w->window);

  bg_media_tree_get_coords(bg_gtk_tree_widget_get_tree(w->widget),
                           &x, &y, &width, &height);

  if((width > 0) && (height > 0))
    {
    gtk_decorated_window_move_resize_window(GTK_WINDOW(w->window),
                                            x, y, width, height);
    }
  }

void bg_gtk_tree_window_hide(bg_gtk_tree_window_t* w)
  {
  int x, y, width, height;
  if(w->window->window)
    {
    gdk_window_get_geometry(w->window->window,
                            (gint *)0, (gint *)0, &width, &height,
                            (gint *)0);

    gdk_window_get_root_origin(w->window->window, &x, &y);
    
    bg_media_tree_set_coords(bg_gtk_tree_widget_get_tree(w->widget),
                             x, y, width, height);
    }
  gtk_widget_hide(w->window);
  }

void bg_gtk_tree_window_set_tooltips(bg_gtk_tree_window_t* win, int enable)
  {
  bg_gtk_tree_widget_set_tooltips(win->widget, enable);
  }
