/*****************************************************************
 
  treewidget.c
 
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

#include <gtk/gtk.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <tree.h>
#include <utils.h>

//#include <parameter.h>
#include <cfg_dialog.h>
#include <gui_gtk/tree.h>

static GdkPixbuf * root_pixbuf = (GdkPixbuf *)0;
static GdkPixbuf * folder_closed_pixbuf = (GdkPixbuf *)0;
static GdkPixbuf * folder_open_pixbuf   = (GdkPixbuf *)0;

static GdkPixbuf * removable_closed_pixbuf = (GdkPixbuf *)0;
static GdkPixbuf * removable_open_pixbuf = (GdkPixbuf *)0;
static GdkPixbuf * removable_error_pixbuf = (GdkPixbuf *)0;

static GdkPixbuf * hardware_pixbuf = (GdkPixbuf *)0;

/* Atoms */

static int atoms_created = 0;

GdkAtom bg_gtk_atom_entries = (GdkAtom)0;
GdkAtom bg_gtk_atom_album   = (GdkAtom)0;

void bg_gtk_tree_create_atoms()
  {
  if(atoms_created)
    return;

  atoms_created = 1;

  bg_gtk_atom_entries = gdk_atom_intern(bg_gtk_atom_entries_name, FALSE);
  bg_gtk_atom_album   = gdk_atom_intern(bg_gtk_atom_album_name,   FALSE);
  }

#define DND_GMERLIN_TRACKS 0
#define DND_GMERLIN_ALBUM  1
#define DND_TEXT_URI_LIST  2
#define DND_TEXT_PLAIN     3

static GtkTargetEntry dnd_src_entries[] = 
  {
    {bg_gtk_atom_album_name,   GTK_TARGET_SAME_WIDGET, DND_GMERLIN_ALBUM },
  };

static GtkTargetEntry dnd_dst_entries[] = 
  {
    {bg_gtk_atom_entries_name, GTK_TARGET_SAME_APP,    DND_GMERLIN_TRACKS },
    {bg_gtk_atom_album_name,   GTK_TARGET_SAME_WIDGET, DND_GMERLIN_ALBUM  },
    {"text/uri-list",          0,                      DND_TEXT_URI_LIST  },
    {"text/plain",             0,                      DND_TEXT_PLAIN     }
  };

/* Open the current album if it isn't already open */

static void open_album(bg_gtk_tree_widget_t * widget,
                       bg_album_t * album);

static void load_pixmaps()
  {
  char * filename;
  
  if(folder_closed_pixbuf)
    return;
  
  filename = bg_search_file_read("icons", "folder_closed_16.png");
  if(filename)
    {
    folder_closed_pixbuf =
      gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  
  filename = bg_search_file_read("icons", "folder_open_16.png");
  if(filename)
    {
    folder_open_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }

  filename = bg_search_file_read("icons", "drive_16.png");
  if(filename)
    {
    removable_closed_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  filename = bg_search_file_read("icons", "drive_running_16.png");
  if(filename)
    {
    removable_open_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  filename = bg_search_file_read("icons", "drive_error_16.png");
  if(filename)
    {
    removable_error_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  filename = bg_search_file_read("icons", "hardware_16.png");
  if(filename)
    {
    hardware_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  filename = bg_search_file_read("icons", "tree_root_16.png");
  if(filename)
    {
    root_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  }

enum
{
  COLUMN_NAME,
  COLUMN_PIXMAP,
  COLUMN_WEIGHT,
  COLUMN_COLOR,
  NUM_COLUMNS
};


typedef struct
  {
  GtkWidget * menu;
  GtkWidget * expand_item;
  GtkWidget * collapse_item;
  } tree_menu_t;

typedef struct
  {
  GtkWidget * new_item;
  GtkWidget * rename_item;
  GtkWidget * open_item;
  GtkWidget * close_item;
  GtkWidget * remove_item;
  GtkWidget * menu;
  } album_menu_t;

typedef struct
  {
  GtkWidget  * tree_item;
  tree_menu_t tree_menu;
  GtkWidget  * album_item;
  album_menu_t album_menu;
  GtkWidget  * menu;
  } root_menu_t;


struct bg_gtk_tree_widget_s
  {
  GtkWidget * widget;
  GtkWidget * treeview;
  bg_media_tree_t * tree;
  bg_album_t * current_album;
  root_menu_t menu;
  GList * album_windows;

  gulong select_handler_id;
  guint drop_time;
  };

/* Utility funcions */

/* Return 1 if album and album_window match */

static gint is_window_of(gconstpointer a, gconstpointer b)
  {
  bg_gtk_album_window_t * album_win;
  bg_album_t * album;
  
  album_win = (bg_gtk_album_window_t*)a;
  album =     (bg_album_t *)b;
  
  return (album == bg_gtk_album_window_get_album(album_win)) ? 0 : 1;
  }

/*
 *  Return the corresponding album window for a widget,
 *  or NULL if there is none
 */

static bg_gtk_album_window_t * album_is_open(bg_gtk_tree_widget_t * widget,
                                             bg_album_t * album)
  {
  GList * tmp_list;
  tmp_list = g_list_find_custom(widget->album_windows,
                                (gconstpointer*)(album),
                                is_window_of);
  if(tmp_list)
    return tmp_list->data;
  return (bg_gtk_album_window_t*)0;
  }

/* Update the menu */

static void update_menu(bg_gtk_tree_widget_t * w)
  {
  /* En- or disable menu items */

  if(!w->current_album)
    {
    gtk_widget_hide(w->menu.album_menu.open_item);
    gtk_widget_hide(w->menu.album_menu.rename_item);
    gtk_widget_hide(w->menu.album_menu.close_item);
    gtk_widget_hide(w->menu.album_menu.remove_item);
    gtk_widget_show(w->menu.album_menu.new_item);
    }
  else
    {
    if(bg_album_is_removable(w->current_album))
      {
      gtk_widget_hide(w->menu.album_menu.remove_item);
      gtk_widget_hide(w->menu.album_menu.new_item);
      gtk_widget_hide(w->menu.album_menu.rename_item);
      }
    else
      {
      gtk_widget_show(w->menu.album_menu.remove_item);
      gtk_widget_show(w->menu.album_menu.new_item);
      gtk_widget_show(w->menu.album_menu.rename_item);
      }

    if(album_is_open(w, w->current_album))
      {
      gtk_widget_hide(w->menu.album_menu.open_item);
      gtk_widget_show(w->menu.album_menu.close_item);
      }
    else if(bg_album_is_removable(w->current_album) &&
            bg_album_get_num_children(w->current_album))
      {
      gtk_widget_hide(w->menu.album_menu.open_item);
      gtk_widget_hide(w->menu.album_menu.close_item);
      }
    else
      {
      gtk_widget_show(w->menu.album_menu.open_item);
      gtk_widget_hide(w->menu.album_menu.close_item);
      }
    }
  
  }


/* Utility funcitons: Convert between GtkTreeIter and bg_album_t */

static void album_2_iter(bg_gtk_tree_widget_t * widget,
                         bg_album_t * album,
                         GtkTreeIter * iter)
  {
  int * indices;
  int i;
  GtkTreeModel *model;
  GtkTreePath  *path;
    
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget->treeview));
  indices = bg_media_tree_get_path(widget->tree, album);

  path = gtk_tree_path_new_first();
    
  i = 0;

  while(indices[i] != -1)
    {
    gtk_tree_path_append_index(path, indices[i]);
    i++;
    }
  free(indices);

  gtk_tree_model_get_iter(model, iter, path);
  gtk_tree_path_free(path);
  }

static bg_album_t * path_2_album(bg_gtk_tree_widget_t * w,
                                 GtkTreePath * path)
  {
  int i;
  bg_album_t * ret;
  gint * indices;
  gint depth;
  depth = gtk_tree_path_get_depth(path);
  
  if(depth < 2)
    return (bg_album_t*)0;

  indices = gtk_tree_path_get_indices(path);
  
  ret = bg_media_tree_get_album(w->tree, indices[1]);

  for(i = 2; i < depth; i++)
    {
    ret = bg_album_get_child(ret, indices[i]);
    }
  return ret;
  }

static bg_album_t * iter_2_album(bg_gtk_tree_widget_t * w,
                                 GtkTreeIter * iter)
  {
  GtkTreePath* path;
  GtkTreeModel * model;
  bg_album_t * ret;
    
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  path = gtk_tree_model_get_path(model, iter);

  ret = path_2_album(w, path);
  gtk_tree_path_free(path);
  return ret;
  }

/* Set one album */

static void set_album(bg_gtk_tree_widget_t * widget,
                      bg_album_t   * album,
                      GtkTreeIter  * iter,
                      int set_children)
  {
  int num_children = 0;
  bg_album_t * child;
  int i;
  GtkTreeIter child_iter;
  GtkTreeModel * model;
  bg_album_t * current_album;
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget->treeview));

  /* Open window if necessary */
  
  //  if(bg_album_is_open(album))
  //    open_album(widget, album);
  
  /* Set values */

  /* Get the appropriate pixmap */

  current_album = bg_media_tree_get_current_album(widget->tree);
  
  if(bg_album_is_removable(album))
    {
    if(bg_album_get_num_children(album))
      {
      gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_NAME,
                         bg_album_get_name(album), -1);
      gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_PIXMAP,
                         hardware_pixbuf, -1);
      }
    else
      {
      gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_NAME,
                         bg_album_get_location(album), -1);

      if(album_is_open(widget, album))
        gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_PIXMAP,
                           removable_open_pixbuf, -1);
      else if(bg_album_get_error(album))
        gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_PIXMAP,
                           removable_error_pixbuf, -1);
      else
        gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_PIXMAP,
                           removable_closed_pixbuf, -1);
      }
    }
  else
    {
    gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_NAME,
                       bg_album_get_name(album), -1);

    if(album_is_open(widget, album))
      gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_PIXMAP,
                         folder_open_pixbuf, -1);
    else
      gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_PIXMAP,
                         folder_closed_pixbuf, -1);
    }

  if(album == current_album)
    gtk_tree_store_set(GTK_TREE_STORE(model),
                       iter,
                       COLUMN_WEIGHT,
                       PANGO_WEIGHT_BOLD, -1);
  else
    gtk_tree_store_set(GTK_TREE_STORE(model),
                       iter,
                       COLUMN_WEIGHT,
                       PANGO_WEIGHT_NORMAL, -1);


  if(bg_album_get_error(album))
    gtk_tree_store_set(GTK_TREE_STORE(model),
                       iter,
                       COLUMN_COLOR,
                       "#FF0000", -1);
  else
    gtk_tree_store_set(GTK_TREE_STORE(model),
                       iter,
                       COLUMN_COLOR,
                       "#000000", -1);

  
  
  /* Append all subalbums of one album */

  num_children = bg_album_get_num_children(album);

  if(set_children)
    {
    for(i = 0; i < num_children; i++)
      {
      gtk_tree_store_append(GTK_TREE_STORE(model),
                            &child_iter,
                            iter);
      child = bg_album_get_child(album, i);
      set_album(widget, child, &child_iter, set_children);
      }
    }
  }

static void expand_album(bg_gtk_tree_widget_t * w, bg_album_t * album)
  {
  GtkTreeIter iter;
  GtkTreePath * path;
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  
  int i;
  bg_album_t * child_album;
  int num_children;
  int expanded;
  int selected;
  
  expanded = bg_album_get_expanded(album);
  selected = (w->current_album == album) ? 1 : 0;


  if(!selected && !expanded)
    return;
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  
  album_2_iter(w, album, &iter);
  path = gtk_tree_model_get_path(model, &iter);

  if(selected)
    {
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w->treeview));
    gtk_tree_selection_select_path(selection, path);
    }

  if(expanded)
    gtk_tree_view_expand_row(GTK_TREE_VIEW(w->treeview), path, 0);

  gtk_tree_path_free(path);
  
  num_children = bg_album_get_num_children(album);
  for(i = 0; i < num_children; i++)
    {
    child_album = bg_album_get_child(album, i);
    expand_album(w, child_album);
    }
  }


/* Update the entire tree */

static void bg_gtk_tree_widget_update(bg_gtk_tree_widget_t * w)
  {
  GtkTreeModel * model;
  GtkTreeIter iter;
  GtkTreeIter root_iter;
  GtkTreePath * path;
  int i;
  int num_children;
  bg_album_t * album;
  GtkTreeSelection * selection;
    
  /* We must block the selection callback */

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w->treeview));
  
  g_signal_handler_block(G_OBJECT(selection), w->select_handler_id);
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  
  gtk_tree_store_clear(GTK_TREE_STORE(model));

  gtk_tree_store_append(GTK_TREE_STORE(model),
                        &root_iter,
                        NULL);

  gtk_tree_store_set(GTK_TREE_STORE(model), &root_iter, COLUMN_NAME,
                     "Mediatree", -1);

  gtk_tree_store_set(GTK_TREE_STORE(model), &root_iter, COLUMN_PIXMAP,
                     root_pixbuf, -1);
  
  num_children = bg_media_tree_get_num_albums(w->tree);
  
  for(i = 0; i < num_children; i++)
    {
    gtk_tree_store_append(GTK_TREE_STORE(model),
                          &iter,
                          &root_iter);
    album = bg_media_tree_get_album(w->tree, i);
    set_album(w, album, &iter, 1);
    }

  path = gtk_tree_model_get_path(model, &root_iter);
  gtk_tree_view_expand_row(GTK_TREE_VIEW(w->treeview), path, FALSE);
  gtk_tree_path_free(path);

  for(i = 0; i < num_children; i++)
    {
    album = bg_media_tree_get_album(w->tree, i);
    expand_album(w, album);
    }
  g_signal_handler_unblock(G_OBJECT(selection), w->select_handler_id);
  }

/*
 *  Remove an album window from the list
 *  This is called by the destructor of album windows
 */

void
bg_gtk_tree_widget_close_album(bg_gtk_tree_widget_t * widget,
                                bg_gtk_album_window_t * win)
  {
  bg_album_t * album;
  GtkTreeIter iter;
    
  widget->album_windows = g_list_remove(widget->album_windows, win);

  album = bg_gtk_album_window_get_album(win);

  bg_album_close(album);
  
  album_2_iter(widget, album, &iter);
  set_album(widget, album, &iter, 0);
  update_menu(widget);
  }

/* Open the current album if it isn't already open */

static void open_album(bg_gtk_tree_widget_t * widget,
                       bg_album_t * album)
  {
  int result;
  bg_gtk_album_window_t * album_window;
  GtkTreeIter iter;
  
  if(!album)
    return;

  if(bg_album_is_removable(album) && bg_album_get_num_children(album))
    return;
  
  /* Check, if the album is already open */
  
  if(!album_is_open(widget, album))
    {
    if(!bg_album_is_open(album))
      result = bg_album_open(album);

    if(result)
      {
      /* Open Album and create window */
      album_window = bg_gtk_album_window_create(album,
                                              widget);
      
      widget->album_windows = g_list_append(widget->album_windows,
                                            album_window);
      bg_album_set_error(album, 0);
      }
    else
      {
      bg_album_set_error(album, 1);
      }
    album_2_iter(widget, album, &iter);
    set_album(widget, album, &iter, 0);
    }
  update_menu(widget);
  }

static void set_parameter(void * data, char * name,
                          bg_parameter_value_t * val)
  {
  GtkTreeIter iter;
  bg_gtk_tree_widget_t * w = (bg_gtk_tree_widget_t*)data;

  if(!name)
    return;
  
  if(!strcmp(name, "album_name"))
    {
    if(w->current_album)
      {
      bg_album_set_name(w->current_album, val->val_str);
      album_2_iter(w, w->current_album, &iter);
      set_album(w, w->current_album, &iter, 0);
      }
        
    }
  }

static void rename_current_album(bg_gtk_tree_widget_t * w)
  {
  bg_dialog_t * dialog;
  GtkTreeIter iter;
  
  bg_parameter_info_t info[2];

  memset(info, 0, sizeof(info));

  info[0].name                = "album_name";
  info[0].long_name           = "Album name";
  info[0].type                = BG_PARAMETER_STRING;
  info[0].val_default.val_str = bg_album_get_name(w->current_album);

  dialog = bg_dialog_create((bg_cfg_section_t*)0,
                            set_parameter,
                            w,
                            info, "Rename album");
  
  bg_dialog_show(dialog);
  
  bg_dialog_destroy(dialog);

  album_2_iter(w, w->current_album, &iter);
  set_album(w, w->current_album, &iter, 0);
  }

static void create_new_album(bg_gtk_tree_widget_t * w)
  {
  bg_album_t * new_album =
    bg_media_tree_append_album(w->tree,
                               w->current_album);
  if(w->current_album)
    bg_album_set_expanded(w->current_album, 1);
  
  //  fprintf(stderr, "New album: %p\n", new_album);
    
  w->current_album = new_album;
  
  bg_gtk_tree_widget_update(w);
    
  rename_current_album(w);

  if(!bg_album_get_name(w->current_album))
    {
    bg_media_tree_remove_album(w->tree, w->current_album);
    w->current_album = (bg_album_t*)0;
    bg_gtk_tree_widget_update(w);
    }
  }

static void remove_album(bg_gtk_tree_widget_t * w)
  {
  bg_gtk_album_window_t * album_window;
  
  if(!w->current_album)
    return;

  album_window = album_is_open(w, w->current_album);
  if(album_window)
    {
    bg_gtk_album_window_destroy(album_window, 1);
    }

  bg_media_tree_remove_album(w->tree, w->current_album);
  bg_gtk_tree_widget_update(w);
  }

static void menu_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_tree_widget_t * widget = (bg_gtk_tree_widget_t*)data;
  bg_gtk_album_window_t * album_window;
    
  /* Album menu */
  if(widget->current_album)
    {
    if(w == widget->menu.album_menu.open_item)
      {
      open_album(widget, widget->current_album);
      }
    if(w == widget->menu.album_menu.rename_item)
      {
      rename_current_album(widget);
      }
    if(w == widget->menu.album_menu.remove_item)
      {
      remove_album(widget);
      }
    else if(w == widget->menu.album_menu.close_item)
      {
      /* Check, if album has already a window */
      album_window = album_is_open(widget, widget->current_album);
      if(album_window)
        {
        /* Open Album and create window */
        bg_gtk_album_window_destroy(album_window, 1);
        }
      }
    }

  if(w == widget->menu.album_menu.new_item)
    {
    create_new_album(widget);
    }
  if(w == widget->menu.tree_menu.expand_item)
    {
    gtk_tree_view_expand_all(GTK_TREE_VIEW(widget->treeview));
    }
  else if(w == widget->menu.tree_menu.collapse_item)
    {
    gtk_tree_view_collapse_all(GTK_TREE_VIEW(widget->treeview));
    }
  
  }

static GtkWidget * create_item(bg_gtk_tree_widget_t * w,
                               const char * label)
  {
  GtkWidget * ret;
  ret = gtk_menu_item_new_with_label(label);
  g_signal_connect(G_OBJECT(ret), "activate", G_CALLBACK(menu_callback),
                   (gpointer)w);
  gtk_widget_show(ret);
  return ret;
  }

static void init_menu(bg_gtk_tree_widget_t * w)
  {
  /* Tree Menu */

  w->menu.tree_menu.menu = gtk_menu_new();
  w->menu.tree_menu.expand_item = create_item(w, "Expand all");
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.tree_menu.menu),
                        w->menu.tree_menu.expand_item);
  
  w->menu.tree_menu.collapse_item = create_item(w, "Collapse all");
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.tree_menu.menu),
                        w->menu.tree_menu.collapse_item);
  gtk_widget_show(w->menu.tree_menu.menu);

  /* Album menu */
  
  w->menu.album_menu.menu = gtk_menu_new();
  
  w->menu.album_menu.open_item = create_item(w, "Open");
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.album_menu.menu),
                        w->menu.album_menu.open_item);
  
  w->menu.album_menu.close_item = create_item(w, "Close");
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.album_menu.menu),
                        w->menu.album_menu.close_item);

  w->menu.album_menu.new_item = create_item(w, "New...");
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.album_menu.menu),
                        w->menu.album_menu.new_item);
  
  w->menu.album_menu.rename_item = create_item(w, "Rename...");
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.album_menu.menu),
                        w->menu.album_menu.rename_item);
  
  w->menu.album_menu.remove_item = create_item(w, "Remove");
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.album_menu.menu),
                        w->menu.album_menu.remove_item);
  gtk_widget_show(w->menu.album_menu.menu);
  
  /* Root menu */

  w->menu.menu = gtk_menu_new();
  w->menu.album_item = create_item(w, "Album...");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(w->menu.album_item),
                            w->menu.album_menu.menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu),
                        w->menu.album_item);

  w->menu.tree_item = create_item(w, "Tree...");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(w->menu.tree_item),
                            w->menu.tree_menu.menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu),
                        w->menu.tree_item);
  } 

GtkWidget * bg_gtk_tree_widget_get_widget(bg_gtk_tree_widget_t * w)
  {
  return w->widget;
  }


/* Callbacks */

static gboolean button_press_callback(GtkWidget * w, GdkEventButton * evt,
                                      gpointer data)
  {
  bg_gtk_tree_widget_t * tw = (bg_gtk_tree_widget_t *)data;

  if(evt->button == 3)
    { 
    gtk_menu_popup(GTK_MENU(tw->menu.menu),
                   (GtkWidget *)0,
                   (GtkWidget *)0,
                   (GtkMenuPositionFunc)0,
                   (gpointer)0,
                   3, evt->time);
    return TRUE;
    }
  else if((evt->button == 1) && (evt->type == GDK_2BUTTON_PRESS))
    {
    open_album(tw, tw->current_album);
    return TRUE;
    }
  
  return FALSE;
  }

static void select_row_callback(GtkTreeSelection * sel,
                                gpointer data)
  {
  GtkTreeIter iter;
  GtkTreeModel * model;
  bg_gtk_album_window_t * album_window;
  
  bg_gtk_tree_widget_t * w = (bg_gtk_tree_widget_t *)data;
  
  if(!gtk_tree_selection_get_selected(sel,
                                      &model,
                                      &iter))
    {
    w->current_album = (bg_album_t*)0;
    }
  else
    {
    w->current_album = iter_2_album(w, &iter);
    }
  update_menu(w);
  if(w->current_album)
    {
    album_window = album_is_open(w, w->current_album);
    if(album_window)
      bg_gtk_album_window_raise(album_window);
    if(!bg_album_is_removable(w->current_album))
      {
      gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(w->treeview),
                                             GDK_BUTTON1_MASK,
                                             dnd_src_entries,
                                             sizeof(dnd_src_entries)/sizeof(dnd_src_entries[0]),
                                             GDK_ACTION_COPY);

      }
    else
      {
      gtk_tree_view_unset_rows_drag_source(GTK_TREE_VIEW(w->treeview));
      }
    }
  else
    gtk_tree_view_unset_rows_drag_source(GTK_TREE_VIEW(w->treeview));
  }

static void row_expanded_callback(GtkTreeView *treeview,
                                  GtkTreeIter *arg1,
                                  GtkTreePath *arg2,
                                  gpointer user_data)
  {
  bg_gtk_tree_widget_t * w;
  bg_album_t * album;
  
  w = (bg_gtk_tree_widget_t*)user_data;

  album = iter_2_album(w, arg1);
  if(!album)
    return;
  
  bg_album_set_expanded(album, 1);
  }

static void row_collapsed_callback(GtkTreeView *treeview,
                                   GtkTreeIter *arg1,
                                   GtkTreePath *arg2,
                                   gpointer user_data)
  {
  bg_gtk_tree_widget_t * w;
  bg_album_t * album;

  w = (bg_gtk_tree_widget_t*)user_data;

  album = iter_2_album(w, arg1);
  if(!album)
    return;
  
  bg_album_set_expanded(album, 0);
  }

static void drag_get_callback(GtkWidget *widget,
                              GdkDragContext *drag_context,
                              GtkSelectionData *data,
                              guint info,
                              guint time,
                              gpointer user_data)
  {
  GdkAtom type_atom;
  
  bg_gtk_tree_widget_t * w;
  w = (bg_gtk_tree_widget_t *)user_data;

  fprintf(stderr, "drag_get_callback\n");
  
#if 0
  fprintf(stderr, "selection: %s\n", gdk_atom_name(data->selection));
  fprintf(stderr, "target:    %s\n", gdk_atom_name(data->target));
  fprintf(stderr, "type:      %s\n", gdk_atom_name(data->type));
  fprintf(stderr, "format:    %d\n", data->format);
#endif
  
  type_atom = gdk_atom_intern("INTEGER", FALSE);
  if(!type_atom)
    return;
  
  gtk_selection_data_set(data, type_atom, 8, (void*)(w->current_album),
                         sizeof(w->current_album));
  }


static void drag_received_callback(GtkWidget *widget,
                                   GdkDragContext *drag_context,
                                   gint x,
                                   gint y,
                                   GtkSelectionData *data,
                                   guint info,
                                   guint time,
                                   gpointer d)
  {
  
  //  fprintf(stderr, "drag_received_callback\n");
  bg_gtk_album_window_t * album_window;
  bg_album_t * dest_album;
  gchar * atom_name;
  GtkTreePath * path;
  GtkTreeViewDropPosition pos;
  int was_open;
  int do_delete = 0;
  
  bg_gtk_tree_widget_t * w = (bg_gtk_tree_widget_t *)d;
  
  if(!gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(w->treeview),
                                        x, y, &path,
                                        &pos))
    return;


  if(!path)
    return;
  
  dest_album = path_2_album(w, path);
  gtk_tree_path_free(path);
  
  if(!dest_album)
    return;

  atom_name = gdk_atom_name(data->target);

  //  fprintf(stderr, "tree drop %d\n", data->length);
  //  fwrite(data->data, 1, data->length, stderr);

  if(!strcmp(atom_name, bg_gtk_atom_album_name))
    {
    switch(pos)
      {
      case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
      case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
        fprintf(stderr, "Gmerlin album dropped %p\n", dest_album);
        bg_media_tree_move_album(w->tree,
                                 w->current_album,
                                 dest_album);
        break;
      case GTK_TREE_VIEW_DROP_AFTER:
        fprintf(stderr, "Gmerlin album dropped after %p\n", dest_album);
        bg_media_tree_move_album_after(w->tree,
                                       w->current_album,
                                       dest_album);
        break;
      case GTK_TREE_VIEW_DROP_BEFORE:
        fprintf(stderr, "Gmerlin album dropped before %p\n", dest_album);
        bg_media_tree_move_album_before(w->tree,
                                        w->current_album,
                                        dest_album);
        break;
      }
    }
  else
    {
    if(bg_album_is_removable(dest_album))
      return;

    /* Open album if necessary */

    if(!bg_album_is_open(dest_album))
      {
      bg_album_open(dest_album);
      was_open = 0;
      }
    else
      was_open = 1;
        
    if(!strcmp(atom_name, "text/uri-list") ||
       !strcmp(atom_name, "text/plain"))
      {
      bg_album_insert_urilist_before(dest_album, data->data, data->length,
                                     (bg_album_entry_t*)0);
      
      
      fprintf(stderr, "Files dropped\n");
      }
    else if(!strcmp(atom_name, bg_gtk_atom_entries_name))
      {
      bg_album_insert_xml_before(dest_album, data->data, data->length,
                                 (bg_album_entry_t*)0);
      fprintf(stderr, "Gmerlin entries dropped\n");
      if(drag_context->action == GDK_ACTION_MOVE)
        do_delete = 1;
      }
    album_window = album_is_open(w, dest_album);
    if(album_window)
      bg_gtk_album_window_update(album_window);
    if(!was_open)
      bg_album_close(dest_album);
    }
  
  g_free(atom_name);
  
  gtk_drag_finish(drag_context,
                  TRUE, /* Success */
                  do_delete, /* Delete */
                  w->drop_time);
  bg_gtk_tree_widget_update(w);
  }

static gboolean drag_drop_callback(GtkWidget *widget,
                                   GdkDragContext *drag_context,
                                   gint x,
                                   gint y,
                                   guint time,
                                   gpointer d)
  {
  bg_gtk_tree_widget_t * w = (bg_gtk_tree_widget_t *)d;
  w->drop_time = time;

#if 0
  gtk_drag_finish(drag_context,
                  FALSE, /* Success */
                  FALSE, /* Delete */
                  w->drop_time);
  return TRUE;
#else
  return TRUE;
#endif
  }

static gboolean drag_motion_callback(GtkWidget *widget,
                                     GdkDragContext *drag_context,
                                     gint x,
                                     gint y,
                                     guint time,
                                     gpointer user_data)
  {
  GtkTreePath * path;
  bg_album_t * drop_album;
  GtkTreeViewDropPosition pos;
  int result = 0;
  
  bg_gtk_tree_widget_t * w = (bg_gtk_tree_widget_t *)user_data;
  //  fprintf(stderr, "Drag motion callback\n");

  if(!gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(w->treeview),
                                        x, y, &path,
                                        &pos))
    {
    return TRUE;
    }
  else
    {
    drop_album = path_2_album(w, path);
    
    if(drop_album && !bg_album_is_removable(drop_album))
      {
      if(widget != gtk_drag_get_source_widget(drag_context))
        {
        //        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w->treeview));
        //        gtk_tree_selection_select_path(selection, path);
        gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(w->treeview),
                                        path,
                                        GTK_TREE_VIEW_DROP_INTO_OR_BEFORE);      
        }
      else
        {
        /* Check if we can drop here */
        
        switch(pos)
          {
          case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
          case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
            result =
              bg_media_tree_check_move_album(w->tree,
                                             w->current_album,
                                             drop_album);
            break;
          case GTK_TREE_VIEW_DROP_AFTER:
            result =
              bg_media_tree_check_move_album_after(w->tree,
                                                   w->current_album,
                                                   drop_album);
            break;
          case GTK_TREE_VIEW_DROP_BEFORE:
            result =
              bg_media_tree_check_move_album_before(w->tree,
                                                    w->current_album,
                                                    drop_album);
            break;
            
          }

        if(result)
          {
          gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(w->treeview),
                                          path, pos);      
          }
        }
      }
    gtk_tree_path_free(path);
    }
  return TRUE;
  }

void tree_changed_callback(bg_media_tree_t * t, void * data)
  {
  bg_gtk_tree_widget_t * w = (bg_gtk_tree_widget_t *)data;
  bg_gtk_tree_widget_update(w);
  }

/* Constructor */

bg_gtk_tree_widget_t *
bg_gtk_tree_widget_create(bg_media_tree_t * tree)
  {
  bg_gtk_tree_widget_t * ret;

  GtkTreeStore      *store;
  GtkCellRenderer   *text_renderer;
  GtkCellRenderer   *pixmap_renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection  *selection;
  
  load_pixmaps();
  bg_gtk_tree_create_atoms();
  
  ret = calloc(1, sizeof(*ret));
  ret->tree = tree;

  bg_media_tree_set_change_callback(ret->tree, tree_changed_callback, ret);
  
  store = gtk_tree_store_new (NUM_COLUMNS,
                              G_TYPE_STRING,
                              GDK_TYPE_PIXBUF,
                              G_TYPE_INT,
                              G_TYPE_STRING);
  
  ret->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  /* Set as drag destination */

  gtk_drag_dest_set(ret->treeview,
                    GTK_DEST_DEFAULT_ALL,
                    dnd_dst_entries,
                    sizeof(dnd_dst_entries)/sizeof(dnd_dst_entries[0]),
                    GDK_ACTION_COPY | GDK_ACTION_MOVE);
  /*
  gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(ret->treeview),
                                       dnd_dst_entries,
                                       sizeof(dnd_dst_entries)/sizeof(dnd_dst_entries[0]),
                                       GDK_ACTION_COPY);
  */
  //  iface = GTK_TREE_DRAG_DEST_GET_IFACE (GTK_TREE_DRAG_DEST(store));
  //  iface->row_drop_possible = row_drop_possible;
    
  /* Set callbacks */

  gtk_widget_set_events(ret->treeview, GDK_BUTTON_PRESS_MASK);

  g_signal_connect(G_OBJECT(ret->treeview), "button-press-event",
                   G_CALLBACK(button_press_callback), (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "row-collapsed",
                   G_CALLBACK(row_collapsed_callback), (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "row-expanded",
                   G_CALLBACK(row_expanded_callback), (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "drag-data-received",
                   G_CALLBACK(drag_received_callback),
                   (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "drag-drop",
                   G_CALLBACK(drag_drop_callback),
                   (gpointer)ret);
  
  g_signal_connect(G_OBJECT(ret->treeview), "drag-motion",
                   G_CALLBACK(drag_motion_callback),
                   (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "drag-data-get",
                   G_CALLBACK(drag_get_callback),
                   (gpointer)ret);


  
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ret->treeview), 0);
  
  gtk_widget_set_size_request(ret->treeview, 200, 300);

  /* Add the columns */
 
  text_renderer = gtk_cell_renderer_text_new();
  pixmap_renderer = gtk_cell_renderer_pixbuf_new();

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title(column, "Albums");
  
  gtk_tree_view_column_pack_start(column, pixmap_renderer, FALSE);
  gtk_tree_view_column_pack_end(column, text_renderer, TRUE);

  gtk_tree_view_column_add_attribute(column,
                                     text_renderer,
                                     "text", COLUMN_NAME);
  gtk_tree_view_column_add_attribute(column,
                                     text_renderer,
                                     "weight", COLUMN_WEIGHT);
  gtk_tree_view_column_add_attribute(column,
                                     text_renderer,
                                     "foreground", COLUMN_COLOR);

  gtk_tree_view_column_add_attribute(column,
                                     pixmap_renderer,
                                     "pixbuf-expander-closed", COLUMN_PIXMAP);
  gtk_tree_view_column_add_attribute(column,
                                     pixmap_renderer,
                                     "pixbuf-expander-open", COLUMN_PIXMAP);
  gtk_tree_view_column_add_attribute(column,
                                     pixmap_renderer,
                                     "pixbuf", COLUMN_PIXMAP);
  
  //  gtk_tree_view_column_set_sort_column_id (column, COLUMN_DEVICE);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview), column);
 
  /* Set Selection mode */
   
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->treeview));

  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  ret->select_handler_id =
    g_signal_connect(G_OBJECT(selection), "changed",
                     G_CALLBACK(select_row_callback), (gpointer)ret);
  
  gtk_widget_show(ret->treeview);

  ret->widget =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(ret->treeview)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ret->treeview)));
  
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->widget),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(ret->widget), ret->treeview);
  gtk_widget_show(ret->widget);

  init_menu(ret);
  
  bg_gtk_tree_widget_update(ret);    
  
  return ret;
  }

void bg_gtk_tree_widget_destroy(bg_gtk_tree_widget_t * w)
  {
  bg_gtk_album_window_t * win;
  
  while(w->album_windows)
    {
    win = (bg_gtk_album_window_t*)w->album_windows->data;
    w->album_windows = g_list_remove(w->album_windows, (gpointer)win);
    bg_gtk_album_window_destroy(win, 0);
    }
  
  free(w);
  }


bg_media_tree_t * bg_gtk_tree_widget_get_tree(bg_gtk_tree_widget_t * w)
  {
  return w->tree;
  }
