/*****************************************************************
 
  albumwindow.c
 
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
#include <string.h>
#include <gtk/gtk.h>

#include <tree.h>
#include <gui_gtk/tree.h>

/* This is missing in the gtk headers */

extern void
gtk_decorated_window_move_resize_window(GtkWindow*, int, int, int, int);


struct bg_gtk_album_window_s
  {
  bg_gtk_album_widget_t * widget;
  bg_gtk_tree_widget_t * tree_widget;
  GtkWidget * window;
  
  int x, y, width, height;

  bg_cfg_section_t * cfg_section;
  
  };

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name: "x",
      long_name: "X",
      type: BG_PARAMETER_INT,
      val_default: { val_i: 100 }
    },
    {
      name: "y",
      long_name: "Y",
      type: BG_PARAMETER_INT,
      val_default: { val_i: 100 }
    },
    {
      name: "width",
      long_name: "Width",
      type: BG_PARAMETER_INT,
      val_default: { val_i: 200 }
    },
    {
      name: "height",
      long_name: "Height",
      type: BG_PARAMETER_INT,
      val_default: { val_i: 300 }
    },
    { /* End of parameters */ }
  };

static void set_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  bg_gtk_album_window_t * win;
  win = (bg_gtk_album_window_t*)data;
  if(!name)
    return;
  else if(!strcmp(name, "x"))
    {
    win->x = val->val_i;
    }
  else if(!strcmp(name, "y"))
    {
    win->y = val->val_i;
    }
  else if(!strcmp(name, "width"))
    {
    win->width = val->val_i;
    }
  else if(!strcmp(name, "height"))
    {
    win->height = val->val_i;
    }
  }

static int get_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  bg_gtk_album_window_t * win;
  win = (bg_gtk_album_window_t*)data;
  if(!name)
    return 1;
  else if(!strcmp(name, "x"))
    {
    val->val_i = win->x;
    return 1;
    }
  else if(!strcmp(name, "y"))
    {
    val->val_i = win->y;
    return 1;
    }
  else if(!strcmp(name, "width"))
    {
    val->val_i = win->width;
    return 1;
    }
  else if(!strcmp(name, "height"))
    {
    val->val_i = win->height;
    return 1;
    }
  return 0;
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  bg_gtk_album_window_t * win;
  win = (bg_gtk_album_window_t*)data;
  bg_gtk_album_window_destroy(win, 1);
  return TRUE;
  }

bg_gtk_album_window_t *
bg_gtk_album_window_create(bg_album_t * album,
                           bg_gtk_tree_widget_t * tree_widget)
  {
  
  bg_gtk_album_window_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->tree_widget = tree_widget;

  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  ret->widget = bg_gtk_album_widget_create(album, ret->window);

  g_signal_connect(G_OBJECT(ret->window), "delete-event",
                   G_CALLBACK(delete_callback),
                   ret);
  
  gtk_container_add(GTK_CONTAINER(ret->window),
                    bg_gtk_album_widget_get_widget(ret->widget));

  gtk_window_set_title(GTK_WINDOW(ret->window), bg_album_get_name(album));
      
  gtk_widget_show(ret->window);

  /* Set config stuff */

  ret->cfg_section =
    bg_cfg_section_find_subsection(bg_album_get_cfg_section(album), "gtk_albumwindow");

  bg_cfg_section_apply(ret->cfg_section, parameters, set_parameter, ret);
  
  gtk_decorated_window_move_resize_window(GTK_WINDOW(ret->window),
                                          ret->x, ret->y, ret->width, ret->height);
  return ret;
  }

void bg_gtk_album_window_destroy(bg_gtk_album_window_t * w, int notify)
  {
  
  /* Get the window coordinates */
  
  if(w->window->window)
    {
    gdk_window_get_geometry(w->window->window,
                            (gint *)0, (gint *)0, &(w->width), &(w->height),
                            (gint *)0);

    gdk_window_get_root_origin(w->window->window, &(w->x), &(w->y));

    bg_cfg_section_get(w->cfg_section, parameters, get_parameter, w);
    }

  bg_gtk_album_widget_put_config(w->widget);
  
  if(w->tree_widget && notify)
    {
    bg_gtk_tree_widget_close_album(w->tree_widget, w);
    }

  if(w->widget)
    bg_gtk_album_widget_destroy(w->widget);
    
  gtk_widget_destroy(w->window);
  free(w);
  }

bg_album_t * bg_gtk_album_window_get_album(bg_gtk_album_window_t*w)
  {
  return bg_gtk_album_widget_get_album(w->widget);
  }

void bg_gtk_album_window_raise(bg_gtk_album_window_t* w)
  {
  if(w->window->window)
    gdk_window_raise(w->window->window);
  }

void bg_gtk_album_window_update(bg_gtk_album_window_t* w)
  {
  bg_gtk_album_widget_update(w->widget);
  }

void bg_gtk_album_window_set_tooltips(bg_gtk_album_window_t * w, int enable)
  {
  bg_gtk_album_widget_set_tooltips(w->widget, enable);
  }
