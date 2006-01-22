/*****************************************************************
 
  albumwidget.c
 
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
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <tree.h>
#include <utils.h>
#include <gui_gtk/tree.h>
#include <cfg_dialog.h>
#include <gui_gtk/fileselect.h>
#include <gui_gtk/urlselect.h>
#include <gui_gtk/albumentry.h>
#include <gui_gtk/display.h>

/* Since the gtk part is single threaded,
   we can load the pixbufs globally */

static GdkPixbuf * has_audio_pixbuf = (GdkPixbuf *)0;
static GdkPixbuf * has_video_pixbuf = (GdkPixbuf *)0;
static GdkPixbuf * has_still_pixbuf = (GdkPixbuf *)0;
static GdkPixbuf * dnd_pixbuf       = (GdkPixbuf *)0;

int num_album_widgets = 0;

/* Static stuff for deleting drag data */

// static bg_gtk_album_widget_t * drag_source = (bg_gtk_album_widget_t*)0;
// static int drag_do_delete = 0;

static char ** file_plugins = (char **)0;
static char **  url_plugins = (char **)0;

static GtkTargetList * target_list = (GtkTargetList *)0;
static GtkTargetList * target_list_r = (GtkTargetList *)0;

/* Drag & Drop struff */

/* 0 means unset */

#define DND_GMERLIN_TRACKS   1
#define DND_GMERLIN_TRACKS_R 2
#define DND_TEXT_URI_LIST    3
#define DND_TEXT_PLAIN       4

static GtkTargetEntry dnd_src_entries[] = 
  {
    { bg_gtk_atom_entries_name, 0, DND_GMERLIN_TRACKS },
  };

static GtkTargetEntry dnd_src_entries_r[] = 
  {
    { bg_gtk_atom_entries_name_r, 0, DND_GMERLIN_TRACKS_R },
  };

static GtkTargetEntry dnd_dst_entries[] = 
  {
    {"text/uri-list",          0, DND_TEXT_URI_LIST  },
    {"text/plain",             0, DND_TEXT_PLAIN     },
    {bg_gtk_atom_entries_name, 0, DND_GMERLIN_TRACKS },
  };

static GtkTargetEntry dnd_dst_entries_r[] = 
  {
    { bg_gtk_atom_entries_name_r, GTK_TARGET_SAME_WIDGET, DND_GMERLIN_TRACKS_R },
  };

static GtkTargetEntry dnd_copy_paste_entries[] =
  {
    { bg_gtk_atom_entries_name, 0, DND_GMERLIN_TRACKS },
    /* We also support text/plain */
    {"STRING",             0, DND_TEXT_PLAIN     },
  };

static void load_pixmaps()
  {
  char * filename;
  
  if(num_album_widgets)
    {
    num_album_widgets++;
    return;
    }

  num_album_widgets++;
  
  filename = bg_search_file_read("icons", "audio_16.png");
  if(filename)
    {
    has_audio_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  filename = bg_search_file_read("icons", "video_16.png");
  if(filename)
    {
    has_video_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  filename = bg_search_file_read("icons", "image_16.png");
  if(filename)
    {
    has_still_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  filename = bg_search_file_read("icons", "tracks_dnd_32.png");
  if(filename)
    {
    dnd_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    free(filename);
    }
  }

static void unload_pixmaps()
  {
  num_album_widgets--;
  if(!num_album_widgets)
    {
    //    fprintf(stderr, "albumwidget.c: unload_pixmaps\n");
    
    g_object_unref(has_audio_pixbuf);
    g_object_unref(has_video_pixbuf);
    g_object_unref(has_still_pixbuf);
    g_object_unref(dnd_pixbuf);
    
    has_audio_pixbuf = (GdkPixbuf *)0;
    has_video_pixbuf = (GdkPixbuf *)0;
    has_still_pixbuf = (GdkPixbuf *)0;
    dnd_pixbuf       = (GdkPixbuf *)0;
    }
  }

enum
  {
    COLUMN_INDEX,
    COLUMN_NAME,
    COLUMN_AUDIO,
    COLUMN_VIDEO,
    COLUMN_DURATION,
    COLUMN_WEIGHT,
    COLUMN_FG_COLOR,
    NUM_COLUMNS
  };

typedef struct
  {
  GtkWidget * files_item;
  GtkWidget * albums_item;
  GtkWidget * urls_item;
  GtkWidget * menu;
  } add_menu_t;

typedef struct
  {
  GtkWidget * cut_item;
  GtkWidget * copy_item;
  GtkWidget * paste_item;
  GtkWidget * move_up_item;
  GtkWidget * move_down_item;
  GtkWidget * copy_to_favourites_item;
  GtkWidget * remove_item;

  GtkWidget * menu;
  } edit_menu_t;

typedef struct
  {
  GtkWidget * transcode_item;
  GtkWidget * rename_item;
  GtkWidget * refresh_item;
  GtkWidget * info_item;

  
  GtkWidget * menu;
  } selected_menu_t;

typedef struct
  {
  GtkWidget * save_item;
  GtkWidget * sort_item;
  GtkWidget * menu;
  } album_menu_t;

typedef struct
  {
  GtkWidget *      add_item;
  add_menu_t       add_menu;

  GtkWidget *      edit_item;
  edit_menu_t      edit_menu;

  GtkWidget *      selected_item;
  selected_menu_t  selected_menu;
  
  GtkWidget *      album_item;
  album_menu_t  album_menu;

  GtkWidget * select_error_item;
  GtkWidget * show_toolbar_item;
  
  GtkWidget * menu;
  } menu_t;

struct bg_gtk_album_widget_s
  {
  GtkWidget * treeview;
  GtkWidget * widget;
  bg_album_t * album;
  const bg_album_entry_t * selected_entry;
  GtkTreeViewColumn * col_duration;
  GtkTreeViewColumn * col_name;
  gulong select_handler_id;
      
  menu_t menu;

  guint drop_time;

  /* File selectors */

  bg_gtk_filesel_t * add_files_filesel;
  bg_gtk_filesel_t * add_albums_filesel;
  bg_gtk_urlsel_t  * add_urls_urlsel;
  
  GtkWidget * parent;

  guint modifier_mask;

  GdkDragAction drag_action;

  int num_selected;
  int num_entries;
  
  int last_clicked_row;
  
  int mouse_x, mouse_y;

  /* Buttons */

  GtkWidget * add_files_button;
  GtkWidget * add_urls_button;
  GtkWidget * remove_selected_button;
  GtkWidget * rename_selected_button;
  GtkWidget * info_button;
  GtkWidget * move_selected_up_button;
  GtkWidget * move_selected_down_button;
  GtkWidget * copy_to_favourites_button;
  GtkWidget * copy_button;
  GtkWidget * cut_button;
  GtkWidget * paste_button;
    
  /* Display */

  bg_gtk_time_display_t * total_time;

  /* Toolbar (can be hidden) */

  GtkWidget * toolbar;
  GtkWidget * drag_dest;
  GtkWidget * drag_dest_current;
  
  /* Open path */

  char * open_path;

  /* Tooltips */

  GtkTooltips * tooltips;

  int release_updates_selection;
  
  /* For inserting */
  int cursor_pos;

  /* xml string as clipboard */
  char * clipboard;
  int clipboard_len;

  GtkAccelGroup * accel_group;
  };

/* Utilities */


static bg_album_entry_t * path_2_entry(bg_gtk_album_widget_t * w,
                                GtkTreePath * path)
  {
  int * indices;
  int index;

  indices = gtk_tree_path_get_indices(path);
  index = indices[0];
  return (bg_album_get_entry(w->album, index));
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:       "open_path",
      long_name:  "Open path",
      type:       BG_PARAMETER_DIRECTORY,
      flags:      BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_str: "." },
    },
    {
      name:       "show_toolbar",
      long_name:  "Show toolbar",
      type:       BG_PARAMETER_CHECKBUTTON,
      flags:      BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 1 },
    },
    { /* End of parameters */ }
  };

static void set_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  bg_gtk_album_widget_t * w;
  w = (bg_gtk_album_widget_t *)data;

  if(!name)
    return;

  if(!strcmp(name, "open_path"))
    {
    w->open_path = bg_strdup(w->open_path, val->val_str);
    }
  else if(!strcmp(name, "show_toolbar"))
    {
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w->menu.show_toolbar_item), val->val_i);
    }
  }

static int get_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  bg_gtk_album_widget_t * w;
  w = (bg_gtk_album_widget_t *)data;

  if(!name)
    return 1;

  if(!strcmp(name, "open_path"))
    {
    val->val_str = bg_strdup(val->val_str, w->open_path);
    return 1;
    }
  else if(!strcmp(name, "show_toolbar"))
    {
    val->val_i = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w->menu.show_toolbar_item));
    }

  return 0;
  }

static void set_sensitive(bg_gtk_album_widget_t * w)
  {
  if(!w->num_selected)
    {
    gtk_widget_set_sensitive(w->menu.selected_menu.rename_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.info_item, 0);

    gtk_widget_set_sensitive(w->menu.edit_menu.cut_item, 0);
    gtk_widget_set_sensitive(w->menu.edit_menu.copy_item, 0);

    gtk_widget_set_sensitive(w->info_button, 0);

    gtk_widget_set_sensitive(w->menu.edit_menu.remove_item, 0);
    gtk_widget_set_sensitive(w->remove_selected_button, 0);
    gtk_widget_set_sensitive(w->rename_selected_button, 0);

    if(w->menu.selected_menu.refresh_item)
      gtk_widget_set_sensitive(w->menu.selected_menu.refresh_item, 0);

    if(w->menu.selected_menu.transcode_item)
      gtk_widget_set_sensitive(w->menu.selected_menu.transcode_item, 0);
        
    if(w->menu.edit_menu.copy_to_favourites_item)
      gtk_widget_set_sensitive(w->menu.edit_menu.copy_to_favourites_item, 0);

    if(w->copy_to_favourites_button)
      gtk_widget_set_sensitive(w->copy_to_favourites_button, 0);

    if(w->cut_button)
      gtk_widget_set_sensitive(w->cut_button, 0);
    if(w->copy_button)
      gtk_widget_set_sensitive(w->copy_button, 0);
    
    gtk_widget_set_sensitive(w->menu.edit_menu.move_up_item, 0);
    gtk_widget_set_sensitive(w->menu.edit_menu.move_down_item, 0);
    gtk_widget_set_sensitive(w->move_selected_up_button, 0);
    gtk_widget_set_sensitive(w->move_selected_down_button, 0);

    w->selected_entry = (bg_album_entry_t*)0;
    }
  else if(w->num_selected == 1)
    {
    gtk_widget_set_sensitive(w->menu.edit_menu.cut_item, 1);
    gtk_widget_set_sensitive(w->menu.edit_menu.copy_item, 1);

    gtk_widget_set_sensitive(w->menu.selected_menu.rename_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.info_item, 1);
    gtk_widget_set_sensitive(w->info_button, 1);

    gtk_widget_set_sensitive(w->menu.edit_menu.remove_item, 1);
    gtk_widget_set_sensitive(w->remove_selected_button, 1);
    gtk_widget_set_sensitive(w->rename_selected_button, 1);

    if(w->menu.selected_menu.refresh_item)
      gtk_widget_set_sensitive(w->menu.selected_menu.refresh_item, 1);

    if(w->menu.edit_menu.copy_to_favourites_item)
      gtk_widget_set_sensitive(w->menu.edit_menu.copy_to_favourites_item, 1);

    if(w->menu.selected_menu.transcode_item)
      gtk_widget_set_sensitive(w->menu.selected_menu.transcode_item, 1);
        
    if(w->copy_to_favourites_button)
      gtk_widget_set_sensitive(w->copy_to_favourites_button, 1);

    if(w->cut_button)
      gtk_widget_set_sensitive(w->cut_button, 1);
    if(w->copy_button)
      gtk_widget_set_sensitive(w->copy_button, 1);
    
    gtk_widget_set_sensitive(w->menu.edit_menu.move_up_item, 1);
    gtk_widget_set_sensitive(w->menu.edit_menu.move_down_item, 1);
    gtk_widget_set_sensitive(w->move_selected_up_button, 1);
    gtk_widget_set_sensitive(w->move_selected_down_button, 1);
    }
  else
    {
    gtk_widget_set_sensitive(w->menu.edit_menu.cut_item, 1);
    gtk_widget_set_sensitive(w->menu.edit_menu.copy_item, 1);

    gtk_widget_set_sensitive(w->menu.selected_menu.rename_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.info_item, 0);
    gtk_widget_set_sensitive(w->info_button, 0);

    gtk_widget_set_sensitive(w->menu.edit_menu.remove_item, 1);
    gtk_widget_set_sensitive(w->remove_selected_button, 1);
    gtk_widget_set_sensitive(w->rename_selected_button, 0);

    if(w->menu.selected_menu.refresh_item)
      gtk_widget_set_sensitive(w->menu.selected_menu.refresh_item, 1);

    if(w->menu.edit_menu.copy_to_favourites_item)
      gtk_widget_set_sensitive(w->menu.edit_menu.copy_to_favourites_item, 1);
    
    if(w->copy_to_favourites_button)
      gtk_widget_set_sensitive(w->copy_to_favourites_button, 1);

    if(w->cut_button)
      gtk_widget_set_sensitive(w->cut_button, 1);
    if(w->copy_button)
      gtk_widget_set_sensitive(w->copy_button, 1);
    
    if(w->menu.selected_menu.transcode_item)
      gtk_widget_set_sensitive(w->menu.selected_menu.transcode_item, 1);

    gtk_widget_set_sensitive(w->menu.edit_menu.move_up_item, 1);
    gtk_widget_set_sensitive(w->menu.edit_menu.move_down_item, 1);
    gtk_widget_set_sensitive(w->move_selected_up_button, 1);
    gtk_widget_set_sensitive(w->move_selected_down_button, 1);
    w->selected_entry = (bg_album_entry_t*)0;
    }

  }

static void update_cursor_pos(bg_gtk_album_widget_t * w)
  {
  GtkTreePath *path;

  if(!w->num_entries)
    {
    w->cursor_pos = 0;
    gtk_widget_queue_draw(w->treeview);
    return;
    }
  else if(w->cursor_pos < w->num_entries)
    {
    path = gtk_tree_path_new_from_indices(w->cursor_pos, -1);
    gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(w->treeview),
                                    path, GTK_TREE_VIEW_DROP_BEFORE);
    gtk_tree_path_free(path);
    }
  else
    {
    w->cursor_pos = w->num_entries;
    path = gtk_tree_path_new_from_indices(w->cursor_pos-1, -1);
    gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(w->treeview),
                                    path, GTK_TREE_VIEW_DROP_AFTER);
    gtk_tree_path_free(path);
    }
  
  gtk_widget_queue_draw(w->treeview);

  //  fprintf(stderr, "Cursor pos: %d\n", w->cursor_pos);
  
  while(gdk_events_pending() || gtk_events_pending())
    gtk_main_iteration();
  }

static int get_visible_range(bg_gtk_album_widget_t * w, int * start_index,
                         int * end_index)
  {
  int y1, y2;
  int x_dummy;
  GdkRectangle visible_rect;
  GtkTreePath * start_path, *end_path;
  int * start_indices, * end_indices;

  gtk_tree_view_get_visible_rect(GTK_TREE_VIEW(w->treeview),
                                 &visible_rect);

  gtk_tree_view_tree_to_widget_coords(GTK_TREE_VIEW(w->treeview),
                                      0,
                                      visible_rect.y,
                                      &x_dummy,
                                      &y1);
  
  gtk_tree_view_tree_to_widget_coords(GTK_TREE_VIEW(w->treeview),
                                      0,
                                      visible_rect.y + visible_rect.height,
                                      &x_dummy,
                                      &y2);

  //  fprintf(stderr, "y1: %d, y2: %d\n", y1, y2);
  
  if(!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(w->treeview),
                                    0, y1, &start_path,
                                    (GtkTreeViewColumn **)0,
                                    (gint *)0,
                                    (gint*)0))
    return 0;

  if(!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(w->treeview),
                                    0, y2, &end_path,
                                    (GtkTreeViewColumn **)0,
                                    (gint *)0,
                                    (gint*)0))
    {
    end_path = gtk_tree_path_new_from_indices(w->num_entries - 1, -1);
    }
  
  start_indices = gtk_tree_path_get_indices(start_path);
  end_indices   = gtk_tree_path_get_indices(end_path);

  *start_index = *start_indices;
  *end_index = *end_indices;

  gtk_tree_path_free(start_path);
  gtk_tree_path_free(end_path);
  return 1;
  }

static void scroll_to_cursor(bg_gtk_album_widget_t * w)
  {
  GtkTreePath * dst_path;
  int start_index, end_index;

  if(!get_visible_range(w, &start_index, &end_index))
    return;
  
  if((w->cursor_pos <= start_index) || (w->cursor_pos >= end_index))
    {
    if((w->cursor_pos && (w->cursor_pos <= start_index)) ||
       (w->cursor_pos == w->num_entries))
      dst_path = gtk_tree_path_new_from_indices(w->cursor_pos-1, -1);
    else
      dst_path = gtk_tree_path_new_from_indices(w->cursor_pos, -1);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(w->treeview),
                                 dst_path,
                                 (GtkTreeViewColumn*)0,
                                 0, 0.0, 0.0);
    gtk_tree_path_free(dst_path);
    }
  }

static gboolean setup_drag_dest(gpointer data)
  {
  bg_gtk_album_widget_t * w;
  GtkTargetEntry * dst_targets;
  int num_dst_targets;

  w = (bg_gtk_album_widget_t*)data;

  if(bg_album_get_type(w->album) == BG_ALBUM_TYPE_REMOVABLE)
    {
    dst_targets = dnd_dst_entries_r;
    num_dst_targets = sizeof(dnd_dst_entries_r)/sizeof(dnd_dst_entries_r[0]);
    }
  else
    {
    dst_targets = dnd_dst_entries;
    num_dst_targets = sizeof(dnd_dst_entries)/sizeof(dnd_dst_entries[0]);
    }
  
  if(!w->num_entries)
    {
    if(w->drag_dest_current == w->drag_dest)
      return FALSE;
    
    gtk_drag_dest_unset(w->treeview);
    gtk_drag_dest_set(w->drag_dest,
                      GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP |
                      GTK_DEST_DEFAULT_MOTION,
                      dst_targets,
                      num_dst_targets,
                      GDK_ACTION_COPY | GDK_ACTION_MOVE);
    w->drag_dest_current = w->drag_dest;
    }
  else
    {
    if(w->drag_dest_current == w->treeview)
      return FALSE;
    
    gtk_drag_dest_unset(w->drag_dest);
    gtk_drag_dest_set(w->treeview,
                      GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP |
                      GTK_DEST_DEFAULT_MOTION,
                      dst_targets,
                      num_dst_targets,
                      GDK_ACTION_COPY | GDK_ACTION_MOVE);
    w->drag_dest_current = w->treeview;
    }
  
  return FALSE;
  }

void bg_gtk_album_widget_update(bg_gtk_album_widget_t * w)
  {
  GtkTreeModel * model;
  int i;
  const bg_album_entry_t * current_entry;
  const bg_album_entry_t * entry;
  GtkTreeIter iter;
  char string_buffer[GAVL_TIME_STRING_LEN + 32];
  GtkTreeSelection * selection;
  gavl_time_t total_time = 0;
  
  //  fprintf(stderr, "bg_gtk_album_widget_update\n");
  
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w->treeview));
  g_signal_handler_block(G_OBJECT(selection), w->select_handler_id);
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  
  gtk_list_store_clear(GTK_LIST_STORE(model));
  
  w->num_entries = bg_album_get_num_entries(w->album);
  w->num_selected = 0;

  current_entry = bg_album_get_current_entry(w->album);
  
  for(i = 0; i < w->num_entries; i++)
    {
    entry = bg_album_get_entry(w->album, i);
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    
    /* Set index */
    sprintf(string_buffer, "%d.", i+1);
    
    gtk_list_store_set(GTK_LIST_STORE(model),
                       &iter,
                       COLUMN_INDEX,
                       string_buffer, -1);
    /* Set name */
    gtk_list_store_set(GTK_LIST_STORE(model),
                       &iter,
                       COLUMN_NAME,
                       entry->name, -1);
    
    /* Audio */
    if(entry->num_audio_streams)
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_AUDIO,
                         has_audio_pixbuf, -1);
    else
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_AUDIO,
                         NULL, -1);
    
    /* Video */
    if(entry->num_video_streams)
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_VIDEO,
                         has_video_pixbuf, -1);
    else if(entry->num_still_streams)
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_VIDEO,
                         has_still_pixbuf, -1);
    else
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_VIDEO,
                         NULL, -1);
    
    /* Set time */
    
    gavl_time_prettyprint(entry->duration, string_buffer);
    gtk_list_store_set(GTK_LIST_STORE(model),
                       &iter,
                       COLUMN_DURATION,
                       string_buffer, -1);

    if(total_time != GAVL_TIME_UNDEFINED)
      {
      if(entry->duration != GAVL_TIME_UNDEFINED)
        total_time += entry->duration;
      else
        total_time = GAVL_TIME_UNDEFINED;
      }
    
    /* Color */

    if(entry->flags & BG_ALBUM_ENTRY_ERROR)
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_FG_COLOR,
                         "#FF0000", -1);
    else
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_FG_COLOR,
                         "#000000", -1);
      
    
    /* Set current track */

    if(entry == current_entry)
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_WEIGHT,
                         PANGO_WEIGHT_BOLD, -1);
    else
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_WEIGHT,
                         PANGO_WEIGHT_NORMAL, -1);
        
    /* Select entry */
    
    if(entry->flags & BG_ALBUM_ENTRY_SELECTED)
      {
      w->num_selected++;
      gtk_tree_selection_select_iter(selection, &iter);
      w->selected_entry = entry;
      }
    }

  bg_gtk_time_display_update(w->total_time, total_time);

  set_sensitive(w);
    
  
  /* Set up drag destination */
  /* Ok, since this function is called from within drag-data-received,
     we cannot touch the drag behaviour here. Therefore we'll set up an
     idle callback, to let things done when back in gtk_main() */

  g_idle_add(setup_drag_dest, (gpointer)w);
  
  g_signal_handler_unblock(G_OBJECT(selection), w->select_handler_id);
  w->last_clicked_row = -1;
  update_cursor_pos(w);
  }


/* Callback functions for the clipboard */

static void clipboard_get_func(GtkClipboard *clipboard,
                               GtkSelectionData *selection_data,
                               guint info,
                               gpointer data)
  {
  GdkAtom type_atom;
  bg_gtk_album_widget_t * w = (bg_gtk_album_widget_t*)data;
  fprintf(stderr, "clipboard_get_func %d\n", w->clipboard_len);

#if 0
  fprintf(stderr, "Have clipboard:");
  fwrite(w->clipboard, 1, w->clipboard_len, stderr);
#endif
  
  type_atom = gdk_atom_intern("STRING", FALSE);
  //  type_atom = gdk_atom_intern(bg_gtk_atom_entries_name, FALSE);
  if(!type_atom)
    return;
  
  gtk_selection_data_set(selection_data, type_atom, 8, (uint8_t*)w->clipboard,
                         w->clipboard_len);
  }

static void clipboard_clear_func(GtkClipboard *clipboard,
                                 gpointer data)
  {
  bg_gtk_album_widget_t * w = (bg_gtk_album_widget_t*)data;
  fprintf(stderr, "clipboard_clear_func\n");
  if(w->clipboard)
    {
    free(w->clipboard);
    w->clipboard_len = 0;
    w->clipboard = (char*)0;
    }
  }

static void clipboard_received_func(GtkClipboard *clipboard,
                                    GtkSelectionData *selection_data,
                                    gpointer data)
  {
  bg_album_entry_t * entry;
  bg_gtk_album_widget_t * w = (bg_gtk_album_widget_t*)data;
  
  fprintf(stderr, "clipboard_received_func %d\n", selection_data->length);

#if 1
  if(selection_data->length <= 0)
    {
    fprintf(stderr, "Retrieving clipboard failed\n");
    return;
    }
  entry = bg_album_get_entry(w->album, w->cursor_pos);
  
  bg_album_insert_xml_before(w->album, (char*)(selection_data->data),
                             selection_data->length,
                             entry);
  bg_gtk_album_widget_update(w);

#endif
  }

static void do_copy(bg_gtk_album_widget_t * w)
  {
  GtkClipboard *clipboard;
  GdkAtom clipboard_atom;
  
  // clipboard_atom = gdk_atom_intern ("PRIMARY", FALSE);
  clipboard_atom = gdk_atom_intern ("CLIPBOARD", FALSE);   
  clipboard = gtk_clipboard_get(clipboard_atom);
  
  if(gtk_clipboard_set_with_data(clipboard,
                                 dnd_copy_paste_entries,
                                 sizeof(dnd_copy_paste_entries)/
                                 sizeof(dnd_copy_paste_entries[0]),
                                 clipboard_get_func,
                                 clipboard_clear_func,
                                 (gpointer)w))
      {
      fprintf(stderr, "Setting clipboard succeeded\n");
      }
  else
    fprintf(stderr, "Setting clipboard failed\n");
  
  if(w->clipboard)
    free(w->clipboard);
  w->clipboard = bg_album_save_selected_to_memory(w->album, &w->clipboard_len,
                                                  0);
  }

static void do_cut(bg_gtk_album_widget_t * w)
  {
  do_copy(w);
  bg_album_delete_selected(w->album);
  bg_gtk_album_widget_update(w);
  }

static void do_paste(bg_gtk_album_widget_t * w)
  {

  GtkClipboard *clipboard;
  GdkAtom clipboard_atom;
  GdkAtom target;

  //    clipboard_atom = gdk_atom_intern ("PRIMARY", FALSE);
  clipboard_atom = gdk_atom_intern ("CLIPBOARD", FALSE);   
  clipboard = gtk_clipboard_get(clipboard_atom);
  
  target = gdk_atom_intern(bg_gtk_atom_entries_name, FALSE);
    
  gtk_clipboard_request_contents(clipboard,
                                 target,
                                 clipboard_received_func,
                                 w);
  }

static void add_file_callback(char ** files, const char * plugin,
                                 void * data)
  {
  bg_album_entry_t * entry;
  bg_gtk_album_widget_t * widget = (bg_gtk_album_widget_t*)data;
  //  fprintf(stderr, "append_file_callback: %s\n", (plugin ? plugin : "NULL"));

  gtk_widget_set_sensitive(widget->treeview, 0);

  entry = bg_album_get_entry(widget->album, widget->cursor_pos);
  bg_album_insert_urls_before(widget->album, files, plugin, entry);

  gtk_widget_set_sensitive(widget->treeview, 1);
  
  //  bg_gtk_album_widget_update(widget);

  widget->open_path = bg_strdup(widget->open_path,
                                bg_gtk_filesel_get_directory(widget->add_files_filesel));
  }

static void add_urls_callback(char ** urls, const char * plugin,
                                 void * data)
  {
  bg_album_entry_t * entry;
  bg_gtk_album_widget_t * widget = (bg_gtk_album_widget_t*)data;
  //  fprintf(stderr, "append_urls_callback: %s\n", (urls[0] ? urls[0] : "NULL"));
  gtk_widget_set_sensitive(widget->treeview, 0);
  entry = bg_album_get_entry(widget->album, widget->cursor_pos);
  bg_album_insert_urls_before(widget->album, urls, plugin, entry);
  gtk_widget_set_sensitive(widget->treeview, 1);
  
  //  bg_gtk_album_widget_update(widget);
  }

static void add_albums_callback(char ** files, const char * plugin,
                                   void * data)
  {
  bg_album_entry_t * entry;
  bg_gtk_album_widget_t * widget = (bg_gtk_album_widget_t*)data;
  entry = bg_album_get_entry(widget->album, widget->cursor_pos);
  bg_album_insert_albums_before(widget->album, files, entry);
  bg_gtk_album_widget_update(widget);
  }

static void filesel_close_callback(bg_gtk_filesel_t * f, void * data)
  {
  bg_gtk_album_widget_t * widget = (bg_gtk_album_widget_t*)data;

  if(f == widget->add_albums_filesel)
    {
    widget->add_albums_filesel = (bg_gtk_filesel_t*)0;
    }
  else if(f == widget->add_files_filesel)
    {
    widget->add_files_filesel = (bg_gtk_filesel_t*)0;
    }
  gtk_widget_set_sensitive(widget->add_files_button, 1);
  gtk_widget_set_sensitive(widget->menu.add_menu.files_item, 1);
  }

static void urlsel_close_callback(bg_gtk_urlsel_t * f, void * data)
  {
  bg_gtk_album_widget_t * widget = (bg_gtk_album_widget_t*)data;
  widget->add_urls_urlsel = (bg_gtk_urlsel_t*)0;

  gtk_widget_set_sensitive(widget->add_urls_button, 1);
  gtk_widget_set_sensitive(widget->menu.add_menu.urls_item, 1);

  }

static void set_name(void * data, char * name,
                     bg_parameter_value_t * val)
  {
  bg_gtk_album_widget_t * w;
  if(!name)
    return;
  w = (bg_gtk_album_widget_t*)data;
  if(!strcmp(name, "track_name") && w->selected_entry)
    {
    bg_album_rename_track(w->album,
                          w->selected_entry,
                          val->val_str);
    bg_gtk_album_widget_update(w);
    }
  }

static void rename_current_entry(bg_gtk_album_widget_t * w)
  {
  bg_dialog_t * dialog;
  
  bg_parameter_info_t info[2];

  if(!w->selected_entry)
    return;
  
  /* Set up parameter info */
  
  memset(info, 0, sizeof(info));

  info[0].name                = "track_name";
  info[0].long_name           = "Track name";
  info[0].type                = BG_PARAMETER_STRING;
  info[0].val_default.val_str = w->selected_entry->name;

  /* Create and run dialog */
  
  dialog = bg_dialog_create((bg_cfg_section_t*)0,
                            set_name,
                            w,
                            info,
                            "Rename entry");
  
  bg_dialog_show(dialog);
  
  bg_dialog_destroy(dialog);
  
  }

static void add_files(bg_gtk_album_widget_t * widget)
  {
  char * tmp_string;

  tmp_string = bg_sprintf("Add files to album %s",
                          bg_album_get_name(widget->album));
  if(!file_plugins)
    file_plugins = bg_plugin_registry_get_plugins(bg_album_get_plugin_registry(widget->album),
                                                  BG_PLUGIN_INPUT,
                                                  BG_PLUGIN_FILE);
    
  widget->add_files_filesel =
    bg_gtk_filesel_create(tmp_string,
                          add_file_callback,
                          filesel_close_callback,
                          file_plugins,
                          widget, widget->parent);
  free(tmp_string);

  bg_gtk_filesel_set_directory(widget->add_files_filesel,
                               widget->open_path);

  gtk_widget_set_sensitive(widget->add_files_button, 0);
  gtk_widget_set_sensitive(widget->menu.add_menu.files_item, 0);
  
  bg_gtk_filesel_run(widget->add_files_filesel, 0);
  }

static void add_urls(bg_gtk_album_widget_t * widget)
  {
  char * tmp_string;

  tmp_string = bg_sprintf("Add URLS to album %s",
                          bg_album_get_name(widget->album));
  if(!url_plugins)
    url_plugins =
      bg_plugin_registry_get_plugins(bg_album_get_plugin_registry(widget->album),
                                     BG_PLUGIN_INPUT,
                                     BG_PLUGIN_URL);
  
  widget->add_urls_urlsel =
    bg_gtk_urlsel_create(tmp_string,
                         add_urls_callback,
                         urlsel_close_callback,
                         url_plugins,
                         widget, widget->parent);
  free(tmp_string);

  gtk_widget_set_sensitive(widget->add_urls_button, 0);
  gtk_widget_set_sensitive(widget->menu.add_menu.urls_item, 0);
  
  bg_gtk_urlsel_run(widget->add_urls_urlsel, 0);

  }

static void remove_selected(bg_gtk_album_widget_t * widget)
  {
  bg_album_delete_selected(widget->album);
  bg_gtk_album_widget_update(widget);
  }

static void move_selected_up(bg_gtk_album_widget_t * widget)
  {
  bg_album_move_selected_up(widget->album);
  bg_gtk_album_widget_update(widget);
  }

static void move_selected_down(bg_gtk_album_widget_t * widget)
  {
  bg_album_move_selected_down(widget->album);
  bg_gtk_album_widget_update(widget);
  }

static void transcode_selected(bg_gtk_album_widget_t * w)
  {
  FILE * file;
  char * filename;
  char * str;
  int len;
  char * command;
    
  str = bg_album_save_selected_to_memory(w->album, &len, 0);
  filename = bg_create_unique_filename("/tmp/gmerlin-%08x.xml");
  file = fopen(filename, "w");
  if(!file)
    {
    fprintf(stderr, "Cannot open temporary file %s\n", filename);
    free(filename);
    return;
    }

  fwrite(str, 1, len, file);
  fclose(file);
  
  command = bg_sprintf("gmerlin_transcoder_remote -launch -addalbum %s",
                       filename);
  system(command);
  remove(filename);
  free(filename);
  free(str);
  free(command);
  }

static void menu_callback(GtkWidget * w, gpointer data)
  {
  char * tmp_string;
  bg_gtk_album_widget_t * widget = (bg_gtk_album_widget_t*)data;

  /* Add files */
  
  if(w == widget->menu.add_menu.files_item)
    {
    add_files(widget);
    }

  /* Add URLS */

  else if(w == widget->menu.add_menu.urls_item)
    {
    add_urls(widget);
    }
  
  /* Add Albums */
  
  else if(w == widget->menu.add_menu.albums_item)
    {
    tmp_string = bg_sprintf("Add albums to %s",
                            bg_album_get_name(widget->album));
    widget->add_files_filesel =
      bg_gtk_filesel_create(tmp_string,
                            add_albums_callback,
                            filesel_close_callback,
                            (char **)0,
                            widget, widget->parent);
    free(tmp_string);
    bg_gtk_filesel_run(widget->add_files_filesel, 0);
    }
  
  /* Remove selected */

  else if(w == widget->menu.edit_menu.remove_item)
    remove_selected(widget);

  /* Transcode selected */
  
  else if(w == widget->menu.selected_menu.transcode_item)
    transcode_selected(widget);
  
  /* Select error tracks */
  
  else if(w == widget->menu.select_error_item)
    {
    bg_album_select_error_tracks(widget->album);
    bg_gtk_album_widget_update(widget);
    }

  /* Copy to favourites */

  else if(w == widget->menu.edit_menu.copy_to_favourites_item)
    bg_album_copy_selected_to_favourites(widget->album);
  
  /* Move up/down */
  
  else if(w == widget->menu.edit_menu.move_up_item)
    move_selected_up(widget);
  
  else if(w == widget->menu.edit_menu.move_down_item)
    move_selected_down(widget);
  
  /* Rename */
    
  else if(w == widget->menu.selected_menu.rename_item)
    {
    rename_current_entry(widget);
    bg_gtk_album_widget_update(widget);
    }

  /* Info */
  
  else if(w == widget->menu.selected_menu.info_item)
    {
    bg_gtk_album_enrty_show(widget->selected_entry);
    }
  /* Cut */
  else if(w == widget->menu.edit_menu.cut_item)
    {
    do_cut(widget);
    }
  /* Copy */
  else if(w == widget->menu.edit_menu.copy_item)
    {
    do_copy(widget);
    }
  /* Paste */
  else if(w == widget->menu.edit_menu.paste_item)
    {
    do_paste(widget);
    }

  
  /* Refresh */
  else if(w == widget->menu.selected_menu.refresh_item)
    {
    bg_album_refresh_selected(widget->album);
    bg_gtk_album_widget_update(widget);
    }
  /* Sort */
  else if(w == widget->menu.album_menu.sort_item)
    {
    bg_album_sort_entries(widget->album);
    bg_gtk_album_widget_update(widget);
    }
  /* Save */
  else if(w == widget->menu.album_menu.save_item)
    {
    tmp_string = bg_gtk_get_filename_write("Save album as",
                                           (char**)0,
                                           0);
    if(tmp_string)
      {
      bg_album_save(widget->album, tmp_string);
      free(tmp_string);
      }
    }
  
  /* Show toolbar */

  else if(w == widget->menu.show_toolbar_item)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget->menu.show_toolbar_item)))
      gtk_widget_show(widget->toolbar);
    else
      gtk_widget_hide(widget->toolbar);
    }
  
  }


static GtkWidget *
create_item(bg_gtk_album_widget_t * w, GtkWidget * parent,
            const char * label, const char * pixmap)
  {
  GtkWidget * ret, *image;
  char * path;
  
  
  if(pixmap)
    {
    path = bg_search_file_read("icons", pixmap);
    if(path)
      {
      image = gtk_image_new_from_file(path);
      free(path);
      }
    else
      image = gtk_image_new();
    gtk_widget_show(image);
    ret = gtk_image_menu_item_new_with_label(label);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(ret), image);
    }
  else
    {
    ret = gtk_menu_item_new_with_label(label);
    }
  
  g_signal_connect(G_OBJECT(ret), "activate", G_CALLBACK(menu_callback),
                   (gpointer)w);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }

static GtkWidget *
create_toggle_item(bg_gtk_album_widget_t * w, GtkWidget * parent,
                   const char * label)
  {
  GtkWidget * ret;
  ret = gtk_check_menu_item_new_with_label(label);
  g_signal_connect(G_OBJECT(ret), "toggled", G_CALLBACK(menu_callback),
                   (gpointer)w);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }


static void init_menu(bg_gtk_album_widget_t * w)
  {
  bg_album_type_t type;
  type = bg_album_get_type(w->album);
  
  /* Add... */

  if(type == BG_ALBUM_TYPE_REGULAR)
    {
    w->menu.add_menu.menu = gtk_menu_new();
    w->menu.add_menu.files_item =
      create_item(w, w->menu.add_menu.menu, "Add Files", "folder_open_16.png");
    w->menu.add_menu.urls_item =
      create_item(w, w->menu.add_menu.menu, "Add URLs", "earth_16.png");
    w->menu.add_menu.albums_item =
      create_item(w, w->menu.add_menu.menu, "Add Albums", (char*)0);
    gtk_widget_show(w->menu.add_menu.menu);
    }

  /* Edit */

  w->menu.edit_menu.menu = gtk_menu_new();
    
  if((type == BG_ALBUM_TYPE_REGULAR) ||
     (type == BG_ALBUM_TYPE_INCOMING))
    {
    w->menu.edit_menu.copy_to_favourites_item =
      create_item(w, w->menu.edit_menu.menu, "Copy to favourites", "favourites_16.png");
    gtk_widget_add_accelerator(w->menu.edit_menu.copy_to_favourites_item, "activate",
                               w->accel_group,
                               GDK_F10, 0, GTK_ACCEL_VISIBLE);
    
    }
  
  w->menu.edit_menu.move_up_item =
    create_item(w, w->menu.edit_menu.menu, "Move to top", "top_16.png");

  w->menu.edit_menu.move_down_item =
    create_item(w, w->menu.edit_menu.menu, "Move to bottom", "bottom_16.png");
  
  w->menu.edit_menu.remove_item =
    create_item(w, w->menu.edit_menu.menu, "Remove", "trash_16.png");

  w->menu.edit_menu.cut_item =
    create_item(w, w->menu.edit_menu.menu, "Cut", "cut_16.png");

  gtk_widget_add_accelerator(w->menu.edit_menu.cut_item, "activate", w->accel_group,
                             GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  w->menu.edit_menu.copy_item =
    create_item(w, w->menu.edit_menu.menu, "Copy", "copy_16.png");

  gtk_widget_add_accelerator(w->menu.edit_menu.copy_item, "activate", w->accel_group,
                             GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  w->menu.edit_menu.paste_item =
    create_item(w, w->menu.edit_menu.menu, "Paste", "paste_16.png");

  gtk_widget_add_accelerator(w->menu.edit_menu.paste_item, "activate", w->accel_group,
                             GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  
  /* Selected */

  w->menu.selected_menu.menu = gtk_menu_new();
    
  w->menu.selected_menu.rename_item =
    create_item(w, w->menu.selected_menu.menu, "Rename...", "rename_16.png");
  w->menu.selected_menu.info_item =
    create_item(w, w->menu.selected_menu.menu, "Info...", "info_16.png");

  if(bg_search_file_exec("gmerlin_transcoder_remote", (char**)0))  
    w->menu.selected_menu.transcode_item =
      create_item(w, w->menu.selected_menu.menu, "Tanscode", (char*)0);
  
  if((type == BG_ALBUM_TYPE_REGULAR) ||
     (type == BG_ALBUM_TYPE_FAVOURITES) ||
     (type == BG_ALBUM_TYPE_INCOMING))
    w->menu.selected_menu.refresh_item =
      create_item(w, w->menu.selected_menu.menu, "Refresh", "refresh_16.png");
  
  gtk_widget_set_sensitive(w->menu.selected_menu.rename_item, 0);
  
 
  /* Album */

  w->menu.album_menu.menu = gtk_menu_new();

  if(type == BG_ALBUM_TYPE_REGULAR)
    {
    w->menu.album_menu.save_item =
      create_item(w, w->menu.album_menu.menu, "Save as...", "save_16.png");
    }
  w->menu.album_menu.sort_item =
    create_item(w, w->menu.album_menu.menu, "Sort", "sort_16.png");
    
  /* Root menu */
  
  w->menu.menu = gtk_menu_new();

  if(type == BG_ALBUM_TYPE_REGULAR)
    {
    w->menu.add_item =
      create_item(w, w->menu.menu, "Add...", (char*)0);
    
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(w->menu.add_item),
                              w->menu.add_menu.menu);
    }

  w->menu.edit_item =
    create_item(w, w->menu.menu, "Edit...", (char*)0);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(w->menu.edit_item),
                            w->menu.edit_menu.menu);

  
  w->menu.selected_item =
    create_item(w, w->menu.menu, "Selected...", (char*)0);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(w->menu.selected_item),
                            w->menu.selected_menu.menu);

  w->menu.album_item =
    create_item(w, w->menu.menu, "Album...", (char*)0);

  
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(w->menu.album_item),
                            w->menu.album_menu.menu);
  w->menu.select_error_item =
    create_item(w, w->menu.menu, "Select error tracks", (char*)0);

  w->menu.show_toolbar_item =
    create_toggle_item(w, w->menu.menu, "Show toolbar");
  }

static void update_selected(bg_gtk_album_widget_t * aw, GtkTreePath * path,
                            guint state, int force)
  {
  int i;
  GtkTreeSelection * selection;
  GtkTreeModel * model;

  GtkTreeIter clicked_iter;
  GtkTreeIter last_clicked_iter;
  
  gint * indices;
  int clicked_row = -1;
  
  indices = gtk_tree_path_get_indices(path);
  clicked_row = indices[0];
  
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(aw->treeview));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(aw->treeview));

  /* Get the clicked iter */
  
  gtk_tree_model_get_iter(model, &clicked_iter, path);

  if(!force) /* Called by button press callback */
    {
    if(gtk_tree_selection_iter_is_selected(selection, &clicked_iter) &&
       (aw->num_selected > 1))
      {
      aw->release_updates_selection = 1;
      return;
      }
    }
  
  if(state & GDK_CONTROL_MASK)
    {
    if(gtk_tree_selection_iter_is_selected(selection, &clicked_iter))
      gtk_tree_selection_unselect_iter(selection, &clicked_iter);
    else
      gtk_tree_selection_select_iter(selection, &clicked_iter);
    }
  else if(state & GDK_SHIFT_MASK)
    {
    /* Select anything between the last clicked row and here */

    if(aw->last_clicked_row >= 0)
      {
      if(!gtk_tree_model_get_iter_first(model, &last_clicked_iter))
        {
        gtk_tree_selection_unselect_all(selection);
        gtk_tree_selection_select_iter(selection, &clicked_iter);
        aw->last_clicked_row = clicked_row;
        return;
        }
      for(i = 0; i < aw->last_clicked_row; i++)
        {
        if(!gtk_tree_model_iter_next(model, &last_clicked_iter))
          {
          gtk_tree_selection_unselect_all(selection);
          gtk_tree_selection_select_iter(selection, &clicked_iter);
          aw->last_clicked_row = clicked_row;
          return;
          }
        }

      if(clicked_row > aw->last_clicked_row)
        {
        for(i = aw->last_clicked_row; i <= clicked_row; i++)
          {
          gtk_tree_selection_select_iter(selection, &last_clicked_iter);
          gtk_tree_model_iter_next(model, &last_clicked_iter);
          }
        }
      else
        {
        for(i = clicked_row; i <= aw->last_clicked_row; i++)
          {
          gtk_tree_selection_select_iter(selection, &clicked_iter);
          gtk_tree_model_iter_next(model, &clicked_iter);
          }
        }
      }
    else
      {
      gtk_tree_selection_unselect_all(selection);
      gtk_tree_selection_select_iter(selection, &clicked_iter);
      }
    }
  else
    {
    gtk_tree_selection_unselect_all(selection);
    gtk_tree_selection_select_iter(selection, &clicked_iter);
    }

  if((aw->last_clicked_row < 0) ||
     (aw->last_clicked_row > clicked_row))
    {
    aw->cursor_pos = clicked_row;
    }
  else
    {
    aw->cursor_pos = clicked_row+1;
    }
  update_cursor_pos(aw);
  
  aw->last_clicked_row = clicked_row;
  
  }

static gboolean button_release_callback(GtkWidget * w, GdkEventButton * evt,
                                        gpointer data)
  {
  GtkTreePath * path;
  bg_gtk_album_widget_t * aw = (bg_gtk_album_widget_t *)data;

  if(!aw->release_updates_selection)
    return TRUE;
  
  if(!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(aw->treeview),
                                   evt->x, evt->y, &path,
                                   (GtkTreeViewColumn **)0,
                                   (gint *)0,
                                   (gint*)0))
    {
    /* Didn't click any entry, return here */
    return TRUE;
    }
  update_selected(aw, path, evt->state, 1);
  aw->release_updates_selection = 0;
  gtk_tree_path_free(path);
  return TRUE;
  }

static gboolean button_press_callback(GtkWidget * w, GdkEventButton * evt,
                                      gpointer data)
  {
  gint * indices;
  GtkTreePath * path;
  
  bg_gtk_album_widget_t * aw = (bg_gtk_album_widget_t *)data;
    
  if(!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(aw->treeview),
                                   evt->x, evt->y, &path,
                                   (GtkTreeViewColumn **)0,
                                   (gint *)0,
                                   (gint*)0))
    {
    path = (GtkTreePath *)0;
    /* Didn't click any entry, return here */
    //    return TRUE;
    }
    
  /* No matter which button was clicked, we must update the selection if the
     current track isn't already selected */
  
  if((evt->type == GDK_BUTTON_PRESS) && path)
    {
    update_selected(aw, path, evt->state, 0);

    indices = gtk_tree_path_get_indices(path);
    aw->cursor_pos = *indices;

    gtk_widget_grab_focus(aw->treeview);
    }
  
  if(evt->button == 3)
    {
    gtk_menu_popup(GTK_MENU(aw->menu.menu),
                   (GtkWidget *)0,
                   (GtkWidget *)0,
                   (GtkMenuPositionFunc)0,
                   (gpointer)0,
                   3, evt->time);
    }
  else if((evt->button == 1) || (evt->button == 2))
    {
    if((evt->type == GDK_2BUTTON_PRESS) && path)
      {
      if(aw->selected_entry)
        {
        /* Play that track */
        bg_album_set_current(aw->album, aw->selected_entry);
        bg_album_play(aw->album);
        }
      }
    }
  if(evt->button == 1)
    {
    aw->mouse_x = (int)evt->x;
    aw->mouse_y = (int)evt->y;
    if(path)
      gtk_tree_view_row_activated(GTK_TREE_VIEW(aw->treeview),
                                  path, (GtkTreeViewColumn *)0);
    }
  if(path)
    gtk_tree_path_free(path);
  return TRUE;
  }

static int is_urilist(GtkSelectionData * data)
  {
  int ret;
  char * target_name;
  target_name = gdk_atom_name(data->target);
  if(!target_name)
    return 0;

  if(!strcmp(target_name, "text/uri-list") ||
     !strcmp(target_name, "text/plain"))
    ret = 1;
  else
    ret = 0;
  
  g_free(target_name);
  return ret;
  }

static int is_entries(GtkSelectionData * data)
  {
  int ret;
  char * target_name;
  target_name = gdk_atom_name(data->target);
  if(!target_name)
    return 0;

  if(!strcmp(target_name, bg_gtk_atom_entries_name))
    ret = 1;
  else
    ret = 0;
  
  g_free(target_name);
  return ret;
  }

static int is_entries_r(GtkSelectionData * data)
  {
  int ret;
  char * target_name;
  target_name = gdk_atom_name(data->target);
  if(!target_name)
    return 0;

  if(!strcmp(target_name, bg_gtk_atom_entries_name_r))
    ret = 1;
  else
    ret = 0;
  
  g_free(target_name);
  return ret;
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
  GtkTreePath * path;
  GtkTreeViewDropPosition pos;
  bg_album_entry_t * entry;
  GtkTreeModel * model;
  bg_gtk_album_widget_t * aw;
  int do_delete = 0;
  int source_type = 0;
  
  aw = (bg_gtk_album_widget_t *)d;

  //  fprintf(stderr, "album drop %d\n", data->length);
  //  fwrite(data->data, 1, data->length, stderr);
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(aw->treeview));
  
  gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(aw->treeview),
                                    x, y, &path,
                                    &pos);
  if(is_entries(data))
    {
    source_type = DND_GMERLIN_TRACKS;
    if(drag_context->action == GDK_ACTION_MOVE)
      do_delete = 1;
    }
  else if(is_entries_r(data))
    {
    source_type = DND_GMERLIN_TRACKS_R;
    if(drag_context->action == GDK_ACTION_MOVE)
      do_delete = 1;
    }
  else if(is_urilist(data))
    {
    //    fprintf(stderr, "text/urilist");
    source_type = DND_TEXT_URI_LIST;
    }
  else
    {
    //    fprintf(stderr, "text/plain");
    source_type = DND_TEXT_PLAIN;
    }
  
  if(path)
    {
    entry = path_2_entry(aw, path);
    gtk_tree_path_free(path);

    if(!entry)
      return;

    if(!source_type)
      return;
    
    switch(pos)
      {
      case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
      case GTK_TREE_VIEW_DROP_BEFORE:
        switch(source_type)
          {
          case DND_GMERLIN_TRACKS:
          case DND_GMERLIN_TRACKS_R:
            bg_album_insert_xml_before(aw->album, (char*)(data->data), data->length,
                                       entry);
            break;
          case DND_TEXT_URI_LIST:
            gtk_widget_set_sensitive(aw->treeview, 0);
            bg_album_insert_urilist_before(aw->album, (char*)(data->data), data->length, entry);
            gtk_widget_set_sensitive(aw->treeview, 1);
            break;
          }
        break;
      case GTK_TREE_VIEW_DROP_AFTER:
      case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
        switch(source_type)
          {
          case DND_GMERLIN_TRACKS:
          case DND_GMERLIN_TRACKS_R:
            bg_album_insert_xml_after(aw->album, (char*)(data->data), data->length,
                                      entry);
            break;
          case DND_TEXT_URI_LIST:
            gtk_widget_set_sensitive(aw->treeview, 0);
            bg_album_insert_urilist_after(aw->album, (char*)(data->data), data->length,
                                          entry);
            gtk_widget_set_sensitive(aw->treeview, 1);
            break;
          }
        break;
      }
    }
  else
    {
    switch(source_type)
      {
      case DND_GMERLIN_TRACKS:
        bg_album_insert_xml_before(aw->album, (char*)(data->data), data->length,
                                   (bg_album_entry_t*)0);
        break;
      case DND_TEXT_URI_LIST:
        bg_album_insert_urilist_before(aw->album, (char*)(data->data), data->length,
                                       (bg_album_entry_t*)0);
        break;
      }
    }
  
  /* Tell source we are ready */
  
  gtk_drag_finish(drag_context,
                  TRUE, /* Success */
                  do_delete, /* Delete */
                  aw->drop_time);
  bg_gtk_album_widget_update(aw);
  
  }

static gboolean drag_drop_callback(GtkWidget *widget,
                                   GdkDragContext *drag_context,
                                   gint x,
                                   gint y,
                                   guint time,
                                   gpointer d)
  {
  bg_gtk_album_widget_t * aw = (bg_gtk_album_widget_t *)d;
  aw->drop_time = time;

#if 0
  gtk_drag_finish(drag_context,
                  FALSE, /* Success */
                  FALSE, /* Delete */
                  aw->drop_time);
  return TRUE;
#else
  return TRUE;
#endif
  }


static void drag_get_callback(GtkWidget *widget,
                              GdkDragContext *drag_context,
                              GtkSelectionData *data,
                              guint info,
                              guint time,
                              gpointer user_data)
  {
  char * str;
  int len;
  GdkAtom type_atom;
  
  bg_gtk_album_widget_t * w;
  w = (bg_gtk_album_widget_t *)user_data;
  //  fprintf(stderr, "Drag get callback\n");
  str = bg_album_save_selected_to_memory(w->album, &len, 1);
#if 0
  fprintf(stderr, "selection: %s\n", gdk_atom_name(data->selection));
  fprintf(stderr, "target:    %s\n", gdk_atom_name(data->target));
  fprintf(stderr, "type:      %s\n", gdk_atom_name(data->type));
  fprintf(stderr, "format:    %d\n", data->format);
#endif
  type_atom = gdk_atom_intern("STRING", FALSE);
  if(!type_atom)
    return;
    
  gtk_selection_data_set(data, type_atom, 8, (uint8_t*)str, len);
  free(str);
  }

static void select_row_callback(GtkTreeSelection * sel,
                                gpointer data)
  {
  int i;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  
  bg_album_entry_t * album_entry;
  bg_gtk_album_widget_t * w = (bg_gtk_album_widget_t *)data;


  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  if(!gtk_tree_model_get_iter_first(model, &iter))
    return;
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w->treeview));
  w->num_selected = 0;
  for(i = 0; i < w->num_entries; i++)
    {
    album_entry = bg_album_get_entry(w->album, i);
    if(gtk_tree_selection_iter_is_selected(selection, &iter))
      {
      album_entry->flags |= BG_ALBUM_ENTRY_SELECTED;
      w->selected_entry = album_entry;
      w->num_selected++;
      //      fprintf(stderr, "Entry %d is selected\n", i+1);
      }
    else
      album_entry->flags &= ~BG_ALBUM_ENTRY_SELECTED;

    if(!gtk_tree_model_iter_next(model, &iter))
      break;
    }
  set_sensitive(w);
  }

static gboolean drag_motion_callback(GtkWidget *widget,
                                     GdkDragContext *drag_context,
                                     gint x,
                                     gint y,
                                     guint time,
                                     gpointer user_data)
  {
  GtkTreePath * path;
  GtkTreeViewDropPosition pos;
  gint * indices;
  bg_gtk_album_widget_t * w = (bg_gtk_album_widget_t *)user_data;

  //  fprintf(stderr, "Drag motion callback, x: %d, y: %d\n", x, y);

#if 0
  print_action(drag_context->action);
  fprintf(stderr, "Suggtested Action: ");
  print_action(drag_context->suggested_action);
#endif
  gdk_drag_status(drag_context, drag_context->suggested_action, time);
  
  if(gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(w->treeview),
                                       x, y, &path,
                                       &pos))
    {
    if(pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
      pos = GTK_TREE_VIEW_DROP_BEFORE;
    else if(pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
      pos = GTK_TREE_VIEW_DROP_AFTER;

    indices = gtk_tree_path_get_indices(path);
    if(pos == GTK_TREE_VIEW_DROP_BEFORE)
      w->cursor_pos = *indices;
    else
      w->cursor_pos = *indices+1;
    gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(w->treeview),
                                    path, pos);      
    gtk_tree_path_free(path);
    }
  else if(path)
    gtk_tree_path_free(path);
  
  return TRUE;
  }

static void drag_delete_callback(GtkWidget *widget,
                         GdkDragContext *drag_context,
                         gpointer user_data)
  {
  bg_gtk_album_widget_t * w = (bg_gtk_album_widget_t *)user_data;

  //  fprintf(stderr, "Drag delete %s\n", bg_album_get_name(w->album));

  bg_album_delete_selected(w->album);
  bg_gtk_album_widget_update(w);
  }


static gboolean
motion_callback(GtkWidget * w, GdkEventMotion * evt, gpointer user_data)
  {
  GdkDragContext* ctx;
  bg_gtk_album_widget_t * wid = (bg_gtk_album_widget_t *)user_data;
  GtkTargetList * tl;

  //  fprintf(stderr, "wid: %p, wid->album: %p\n", wid, wid->album);
  
  if(bg_album_get_type(wid->album) == BG_ALBUM_TYPE_REMOVABLE)
    tl = target_list_r;
  else
    tl = target_list;
    
  if(evt->state & GDK_BUTTON1_MASK)
    {
    //    fprintf(stderr, "Motion callback, %d\n",
    //            abs((int)(evt->x) - wid->mouse_x) + abs((int)(evt->y) - wid->mouse_y));
    
    if((abs((int)(evt->x) - wid->mouse_x) + abs((int)(evt->y) - wid->mouse_y) < 10) ||
       (!wid->num_selected))
      return FALSE;
    if(evt->state & GDK_CONTROL_MASK)
      {
      //      fprintf(stderr, "Start drag copy\n");

      ctx = gtk_drag_begin(w, tl,
                           GDK_ACTION_COPY,
                           1, (GdkEvent*)(evt));
      gtk_drag_set_icon_pixbuf(ctx,
                               dnd_pixbuf,
                               0, 0);
      wid->release_updates_selection = 0;
      }
    else
      {
      //      fprintf(stderr, "Start drag move\n");
      ctx = gtk_drag_begin(w, tl,
                           GDK_ACTION_MOVE,
                           1, (GdkEvent*)(evt));
      gtk_drag_set_icon_pixbuf(ctx,
                               dnd_pixbuf,
                               0, 0);
      wid->release_updates_selection = 0;
      }
    }
  return TRUE;
  }

static gboolean key_press_callback(GtkWidget * w, GdkEventKey * evt,
                                   gpointer user_data)
  {
  int i;
  GtkTreeIter iter;
  GtkTreeModel *tree_model;
  GtkTreeSelection * selection;
  gint start_index, end_index;
  bg_gtk_album_widget_t * wid = (bg_gtk_album_widget_t *)user_data;
  int old_cursor_pos;

  //  fprintf(stderr, "Key Press Callback, state: %08x %08x\n",
  //          evt->state, evt->keyval);

  switch(evt->keyval)
    {
    case GDK_Up:
      if(wid->cursor_pos)
        {
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(wid->treeview));
        tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(wid->treeview));
        
        wid->cursor_pos--;

        if(evt->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK))
          {
          selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(wid->treeview));
          tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(wid->treeview));
          gtk_tree_model_iter_nth_child(tree_model,
                                        &iter,
                                        (GtkTreeIter*)0,
                                        wid->cursor_pos);
          if(evt->state & (GDK_SHIFT_MASK))
            gtk_tree_selection_select_iter(selection, &iter);
          else
            gtk_tree_selection_unselect_iter(selection, &iter);
          }
        update_cursor_pos(wid);
        scroll_to_cursor(wid);
        }
      break;
    case GDK_Down:
      if(wid->cursor_pos < wid->num_entries)
        {
        if(evt->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK))
          {
          selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(wid->treeview));
          tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(wid->treeview));
          gtk_tree_model_iter_nth_child(tree_model,
                                        &iter,
                                        (GtkTreeIter*)0,
                                        wid->cursor_pos);
          if(evt->state & (GDK_SHIFT_MASK))
            gtk_tree_selection_select_iter(selection, &iter);
          else
            gtk_tree_selection_unselect_iter(selection, &iter);
          }
        wid->cursor_pos++;
        
        update_cursor_pos(wid);
        scroll_to_cursor(wid);
        }
      break;
    case GDK_Page_Up:
      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(wid->treeview));
      tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(wid->treeview));
      old_cursor_pos = wid->cursor_pos;
      
      get_visible_range(wid, &start_index, &end_index);
      wid->cursor_pos -= end_index - start_index;
      if(wid->cursor_pos < 0)
        wid->cursor_pos = 0;
      
      update_cursor_pos(wid);
      scroll_to_cursor(wid);
      
      if(wid->cursor_pos < old_cursor_pos)
        {
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(wid->treeview));
        tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(wid->treeview));
        gtk_tree_model_iter_nth_child(tree_model,
                                      &iter,
                                      (GtkTreeIter*)0,
                                      wid->cursor_pos);

        for(i = wid->cursor_pos; i < old_cursor_pos; i++)
          {
          if(evt->state & (GDK_SHIFT_MASK))
            gtk_tree_selection_select_iter(selection, &iter);
          else
            gtk_tree_selection_unselect_iter(selection, &iter);
          gtk_tree_model_iter_next(tree_model, &iter);
          }
        
        }
      
      break;
    case GDK_Page_Down:
      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(wid->treeview));
      tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(wid->treeview));

      get_visible_range(wid, &start_index, &end_index);

      wid->cursor_pos+=end_index - start_index;
      if(wid->cursor_pos > wid->num_entries)
        wid->cursor_pos = wid->num_entries;
      update_cursor_pos(wid);
      scroll_to_cursor(wid);

      if(wid->cursor_pos > old_cursor_pos)
        {
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(wid->treeview));
        tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(wid->treeview));
        gtk_tree_model_iter_nth_child(tree_model,
                                      &iter,
                                      (GtkTreeIter*)0,
                                      old_cursor_pos);

        for(i = old_cursor_pos; i < wid->cursor_pos; i++)
          {
          if(evt->state & (GDK_SHIFT_MASK))
            gtk_tree_selection_select_iter(selection, &iter);
          else
            gtk_tree_selection_unselect_iter(selection, &iter);
          gtk_tree_model_iter_next(tree_model, &iter);
          }
        }

      break;
    }
  return TRUE;
  }

static gboolean
album_changed_foreach(GtkTreeModel *model,
                          GtkTreePath  *path,
                          GtkTreeIter  *iter,
                          gpointer      data)
  {
  gtk_tree_model_row_changed(model, path, iter);
  return FALSE;
  }

static void column_resize_callback(GtkTreeViewColumn * col,
                                   gint * width_val,
                                   gpointer data)
  {
  GtkTreeModel * model;
  int width_needed;
  int name_width;
  int width;
  
  bg_gtk_album_widget_t * w = (bg_gtk_album_widget_t *)data;

  width = col->width;
  
  if(col == w->col_duration)
    {
    gtk_tree_view_column_cell_get_size(col,
                                       (GdkRectangle*)0,
                                       (gint *)0,
                                       (gint *)0,
                                       &width_needed,
                                       (gint *)0);
    name_width = gtk_tree_view_column_get_fixed_width (w->col_name);
    
    if(width > width_needed)
      {
      name_width += width - width_needed;
      gtk_tree_view_column_set_fixed_width (w->col_name, name_width);
      }
    else if(width < width_needed)
      {
      name_width -= width_needed - width;
      gtk_tree_view_column_set_fixed_width (w->col_name, name_width);
      }
    
    }
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  gtk_tree_model_foreach (GTK_TREE_MODEL(model),
                          album_changed_foreach, NULL);   
  }

static void album_changed_callback(bg_album_t * a, void * data)
  {
  bg_gtk_album_widget_t * w = (bg_gtk_album_widget_t*)data;
  bg_gtk_album_widget_update(w);
  
  /* This is also called during loading of huge amounts of
     urls, so we update the GUI a bit */

  while(gdk_events_pending() || gtk_events_pending())
    gtk_main_iteration();
  }

static void button_callback(GtkWidget * wid, gpointer data)
  {
  
  bg_gtk_album_widget_t * w = (bg_gtk_album_widget_t*)data;
  
  if(wid == w->add_files_button)
    {
    add_files(w);
    }
  else if(wid == w->add_urls_button)
    {
    add_urls(w);
    }
  else if(wid == w->remove_selected_button)
    {
    remove_selected(w);
    }
  else if(wid == w->rename_selected_button)
    {
    rename_current_entry(w);
    bg_gtk_album_widget_update(w);
    }
  
  else if(wid == w->info_button)
    {
    bg_gtk_album_enrty_show(w->selected_entry);
    }
  else if(wid == w->move_selected_up_button)
    {
    move_selected_up(w);
    }
  else if(wid == w->move_selected_down_button)
    {
    move_selected_down(w);
    }
  else if(wid == w->copy_to_favourites_button)
    {
    bg_album_copy_selected_to_favourites(w->album);
    }
  else if(wid == w->copy_button)
    {
    fprintf(stderr, "Copy button clicked\n");
    do_copy(w);
    }
  else if(wid == w->cut_button)
    {
    fprintf(stderr, "Cut button clicked\n");
    do_cut(w);
    }
  else if(wid == w->paste_button)
    {
    fprintf(stderr, "Paste button clicked\n");
    do_paste(w);
    }
  }

static GtkWidget * create_pixmap_button(bg_gtk_album_widget_t * w,
                                        const char * filename,
                                        const char * tooltip,
                                        const char * tooltip_private)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();

  gtk_widget_show(image);
  button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(button_callback), w);

  gtk_widget_show(button);

  gtk_tooltips_set_tip(w->tooltips, button, tooltip, tooltip_private);
  
  return button;
  }


bg_gtk_album_widget_t *
bg_gtk_album_widget_create(bg_album_t * album, GtkWidget * parent)
  {
  bg_gtk_album_widget_t * ret;
  GtkTreeViewColumn * col;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeSelection * selection;
  GtkWidget * scrolledwin;

  bg_cfg_section_t * section, * subsection;

  bg_album_type_t type;
  type = bg_album_get_type(album);
  
  load_pixmaps();
  bg_gtk_tree_create_atoms();

  if(!target_list)
    {
    target_list =
      gtk_target_list_new(dnd_src_entries,
                          sizeof(dnd_src_entries)/sizeof(dnd_src_entries[0]));
    target_list_r =
      gtk_target_list_new(dnd_src_entries_r,
                          sizeof(dnd_src_entries_r)/sizeof(dnd_src_entries_r[0]));
    }
  
  ret = calloc(1, sizeof(*ret));
  ret->album = album;
  ret->parent = parent;

  ret->accel_group = gtk_accel_group_new();
  
  ret->tooltips = gtk_tooltips_new();
  ret->last_clicked_row = -1;
  g_object_ref (G_OBJECT (ret->tooltips));
  gtk_object_sink (GTK_OBJECT (ret->tooltips));
  
  bg_album_set_change_callback(album, album_changed_callback, ret);

  /* Create list */
  
  store = gtk_list_store_new(NUM_COLUMNS,
                             G_TYPE_STRING,     // Index
                             G_TYPE_STRING,     // Name
                             GDK_TYPE_PIXBUF,   // Has audio
                             GDK_TYPE_PIXBUF,   // Has video
                             G_TYPE_STRING,     // Duration
                             G_TYPE_INT,        // Weight
                             G_TYPE_STRING);    // Foreground
  
  ret->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_size_request(ret->treeview, 200, 100);

  selection =
    gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->treeview));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

  ret->select_handler_id = 
    g_signal_connect(G_OBJECT(selection), "changed",
                     G_CALLBACK(select_row_callback), (gpointer)ret);
  
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ret->treeview), 0);

  /* Set list callbacks */
  gtk_widget_set_events(ret->treeview,
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_KEY_PRESS_MASK |
                        GDK_BUTTON1_MOTION_MASK );
  
  g_signal_connect(G_OBJECT(ret->treeview), "button-press-event",
                   G_CALLBACK(button_press_callback), (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "button-release-event",
                   G_CALLBACK(button_release_callback), (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "key-press-event",
                   G_CALLBACK(key_press_callback), (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "motion-notify-event",
                   G_CALLBACK(motion_callback), (gpointer)ret);

  

  g_signal_connect(G_OBJECT(ret->treeview), "drag-data-delete",
                   G_CALLBACK(drag_delete_callback),
                   (gpointer)ret);
  
  g_signal_connect(G_OBJECT(ret->treeview), "drag-data-received",
                   G_CALLBACK(drag_received_callback),
                   (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "drag-drop",
                   G_CALLBACK(drag_drop_callback),
                   (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "drag-data-get",
                   G_CALLBACK(drag_get_callback),
                   (gpointer)ret);
  
  g_signal_connect(G_OBJECT(ret->treeview), "drag-motion",
                   G_CALLBACK(drag_motion_callback),
                   (gpointer)ret);
  
  /* Create columns */
  /* Index */
  
  renderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);

  col = gtk_tree_view_column_new ();
  
  gtk_tree_view_column_set_title(col, "I");
  
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
  
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "text", COLUMN_INDEX);
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "weight", COLUMN_WEIGHT);
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "foreground", COLUMN_FG_COLOR);
  
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview), col);

  /* Name */
  
  renderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer), "xalign", 0.0, NULL);

  col = gtk_tree_view_column_new ();

  
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  
  gtk_tree_view_column_add_attribute(col, renderer,
                                     "text", COLUMN_NAME);
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "weight", COLUMN_WEIGHT);
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "foreground", COLUMN_FG_COLOR);
  
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_FIXED);
  
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);

  ret->col_name = col;
  /* Audio */

  renderer = gtk_cell_renderer_pixbuf_new();

  col = gtk_tree_view_column_new ();
    
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
  
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "pixbuf", COLUMN_AUDIO);
  
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);

  /* Video */
  
  renderer = gtk_cell_renderer_pixbuf_new();

  col = gtk_tree_view_column_new ();
  
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
  
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "pixbuf", COLUMN_VIDEO);
  
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);

  /* Duration */
  
  renderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);

  col = gtk_tree_view_column_new ();

  g_signal_connect(G_OBJECT(col),
                   "notify::width", G_CALLBACK(column_resize_callback),
                   (gpointer)ret);
  

  gtk_tree_view_column_set_title(col, "T");
  
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
  
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "text", COLUMN_DURATION);
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "weight", COLUMN_WEIGHT);
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "foreground", COLUMN_FG_COLOR);
  
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);

  ret->col_duration = col;

  /* Done with columns */
    
  gtk_widget_show(ret->treeview);
  
  scrolledwin =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(ret->treeview)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ret->treeview)));

  ret->drag_dest = scrolledwin;
  
  g_signal_connect(G_OBJECT(ret->drag_dest), "drag-data-received",
                   G_CALLBACK(drag_received_callback),
                   (gpointer)ret);
  
  
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(scrolledwin), ret->treeview);

  gtk_widget_show(scrolledwin);
    
  /* Create toolbar */

  if(type == BG_ALBUM_TYPE_REGULAR)
    {
    ret->add_files_button        = create_pixmap_button(ret, "folder_open_16.png",
                                                        "Add files", "Append files to the track list");
    ret->add_urls_button         = create_pixmap_button(ret, "earth_16.png",
                                                        "Add URLs", "Append URLs to the track list");
    }

  if((type == BG_ALBUM_TYPE_REGULAR) ||
     (type == BG_ALBUM_TYPE_INCOMING) ||
     (type == BG_ALBUM_TYPE_FAVOURITES))
    {
    ret->cut_button              = create_pixmap_button(ret, "cut_16.png",
                                                        "Cut", "Cut");
    ret->copy_button             = create_pixmap_button(ret, "copy_16.png",
                                                      "Copy", "Copy");
    ret->paste_button             = create_pixmap_button(ret, "paste_16.png",
                                                         "Paste", "Paste");
    }

  if((type == BG_ALBUM_TYPE_REGULAR) ||
     (type == BG_ALBUM_TYPE_INCOMING))
    ret->copy_to_favourites_button = create_pixmap_button(ret, "favourites_16.png",
                                                          "Copy to favourites",
                                                          "Copy selected tracks to favourites");
  
  ret->remove_selected_button    = create_pixmap_button(ret, "trash_16.png",
                                                        "Delete", "Delete selected tracks");

  ret->rename_selected_button    = create_pixmap_button(ret, "rename_16.png",
                                                        "Rename", "Rename selected track");

  ret->info_button               = create_pixmap_button(ret, "info_16.png",
                                                        "Show track info", "Show track info");
  ret->move_selected_up_button   = create_pixmap_button(ret, "top_16.png",
                                                        "Move to top", "Move to top");
  ret->move_selected_down_button = create_pixmap_button(ret, "bottom_16.png",
                                                        "Move to bottom", "Move to bottom");
  
  ret->total_time                = bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL, 4);

  gtk_tooltips_set_tip(ret->tooltips,
                       bg_gtk_time_display_get_widget(ret->total_time),
                       "Total playback time",
                       "Total playback time");


  ret->toolbar                   = gtk_hbox_new(0, 0);

  if(type == BG_ALBUM_TYPE_REGULAR)
    {
    gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->add_files_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->add_urls_button, FALSE, FALSE, 0);
    }
  gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->remove_selected_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->info_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->rename_selected_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->move_selected_up_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->move_selected_down_button, FALSE, FALSE, 0);
  
  if((type == BG_ALBUM_TYPE_REGULAR) ||
     (type == BG_ALBUM_TYPE_INCOMING) ||
     (type == BG_ALBUM_TYPE_FAVOURITES))
    {
    gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->cut_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->copy_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->paste_button, FALSE, FALSE, 0);
    }
  
  if((type == BG_ALBUM_TYPE_REGULAR) ||
     (type == BG_ALBUM_TYPE_INCOMING))
    {
    gtk_box_pack_start(GTK_BOX(ret->toolbar), ret->copy_to_favourites_button, FALSE, FALSE, 0);
    }
  
  gtk_box_pack_end(GTK_BOX(ret->toolbar),
                   bg_gtk_time_display_get_widget(ret->total_time), FALSE, FALSE, 0);

  //  gtk_widget_show(ret->toolbar);
  
  ret->widget = gtk_vbox_new(0, 0);

  gtk_box_pack_start_defaults(GTK_BOX(ret->widget), scrolledwin);
  gtk_box_pack_start(GTK_BOX(ret->widget), ret->toolbar, FALSE, FALSE, 0);
  
  init_menu(ret);
  
  /* Get config params from the album */

  section = bg_album_get_cfg_section(album);
  subsection = bg_cfg_section_find_subsection(section, "gtk_albumwidget");

  bg_cfg_section_apply(subsection, parameters, set_parameter, ret);
  
  bg_gtk_album_widget_update(ret);
  gtk_widget_show(ret->widget);
  
  return ret;
  }

void bg_gtk_album_widget_put_config(bg_gtk_album_widget_t * w)
  {
  bg_cfg_section_t * section, * subsection;

  section = bg_album_get_cfg_section(w->album);
  subsection = bg_cfg_section_find_subsection(section, "gtk_albumwidget");
  
  bg_cfg_section_get(subsection, parameters, get_parameter, w);
  }


void bg_gtk_album_widget_destroy(bg_gtk_album_widget_t * w)
  {
  if(w->open_path)
    free(w->open_path);
  if(w->clipboard)
    free(w->clipboard);
  bg_gtk_time_display_destroy(w->total_time);

  g_object_unref(w->tooltips);

  bg_album_set_change_callback(w->album, NULL, NULL);

  g_object_unref(w->accel_group);
  
  free(w);

  unload_pixmaps();
  }

GtkWidget * bg_gtk_album_widget_get_widget(bg_gtk_album_widget_t * w)
  {
  return w->widget;
  }

bg_album_t * bg_gtk_album_widget_get_album(bg_gtk_album_widget_t * w)
  {
  return w->album;
  }

void bg_gtk_album_widget_set_tooltips(bg_gtk_album_widget_t * w, int enable)
  {
  if(enable)
    gtk_tooltips_enable(w->tooltips);
  else
    gtk_tooltips_disable(w->tooltips);
  }

void bg_gtk_album_widget_goto_current(bg_gtk_album_widget_t * aw)
  {
  int i;
  const bg_album_entry_t * entry, *current_entry;
  
  GtkTreeSelection * selection;
  GtkTreePath * path;

  current_entry = bg_album_get_current_entry(aw->album);

  if(!current_entry)
    return;
    
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(aw->treeview));
  gtk_tree_selection_unselect_all(selection);
  
  for(i = 0; i < aw->num_entries; i++)
    {
    entry = bg_album_get_entry(aw->album, i);
    if(entry == current_entry)
      {
      path = gtk_tree_path_new_from_indices(i, -1);
      
      gtk_tree_selection_select_path(selection, path);
      gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(aw->treeview),
                                   path, (GtkTreeViewColumn*)0,
                                   1, 0.0, 0.5);
      gtk_tree_path_free(path);
      break;
      }
    }
  
  //  fprintf(stderr, "bg_gtk_album_widget_goto_current\n");
  }

GtkAccelGroup * bg_gtk_album_widget_get_accel_group(bg_gtk_album_widget_t * w)
  {
  return w->accel_group;
  }
