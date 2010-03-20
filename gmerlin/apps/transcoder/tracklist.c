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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <config.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>

#include <gmerlin/tree.h>

#include <gmerlin/cfg_dialog.h>


#include <gui_gtk/fileselect.h>
#include <gui_gtk/urlselect.h>
#include <gui_gtk/driveselect.h>
#include <gui_gtk/tree.h>
#include <gui_gtk/display.h>
#include <gui_gtk/plugin.h>
#include <gui_gtk/chapterdialog.h>
#include <gui_gtk/gtkutils.h>

#include <gmerlin/transcoder_track.h>
#include "tracklist.h"
#include "trackdialog.h"
#include "ppwidget.h"

static void track_list_update(track_list_t * w);

static GdkPixbuf * has_audio_pixbuf = (GdkPixbuf *)0;
static GdkPixbuf * has_video_pixbuf = (GdkPixbuf *)0;

#define cp_tracks_name "gmerlin_transcoder_tracks"

/* 0 means unset */

#define DND_GMERLIN_TRACKS    1
#define DND_GMERLIN_TRACKS_R  2
#define DND_TEXT_URI_LIST     3
#define DND_TEXT_PLAIN        4
#define DND_TRANSCODER_TRACKS 5

static GtkTargetEntry dnd_dst_entries[] =
  {
    {bg_gtk_atom_entries_name,   0, DND_GMERLIN_TRACKS   },
    {bg_gtk_atom_entries_name_r, 0, DND_GMERLIN_TRACKS_R },
    {"text/uri-list",            0, DND_TEXT_URI_LIST    },
    {"text/plain",               0, DND_TEXT_PLAIN       },
  };

static GtkTargetEntry copy_paste_entries[] =
  {
    { cp_tracks_name , 0, DND_TRANSCODER_TRACKS },
  };

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
#if 0
static int is_tracks(GtkSelectionData * data)
  {
  int ret;
  char * target_name;
  target_name = gdk_atom_name(data->target);
  if(!target_name)
    return 0;

  if(!strcmp(target_name, cp_tracks_name))
    ret = 1;
  else
    ret = 0;
  g_free(target_name);
  return ret;
  }
#endif
static int is_albumentries(GtkSelectionData * data)
  {
  int ret;
  char * target_name;
  target_name = gdk_atom_name(data->target);
  if(!target_name)
    return 0;

  if(!strcmp(target_name, bg_gtk_atom_entries_name) ||
     !strcmp(target_name, bg_gtk_atom_entries_name_r))
    ret = 1;
  else
    ret = 0;
  
  g_free(target_name);
  return ret;
  }


static void load_pixmaps()
  {
  char * filename;
  if(has_audio_pixbuf)
    return;
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
  }

typedef struct
  {
  GtkWidget * add_files_item;
  GtkWidget * add_urls_item;
  GtkWidget * add_drives_item;
  GtkWidget * menu;
  } add_menu_t;

typedef struct
  {
  GtkWidget * move_up_item;
  GtkWidget * move_down_item;
  GtkWidget * remove_item;
  GtkWidget * configure_item;
  GtkWidget * encoder_item;
  GtkWidget * chapter_item;
  GtkWidget * menu;
  } selected_menu_t;

typedef struct
  {
  GtkWidget * cut_item;
  GtkWidget * copy_item;
  GtkWidget * paste_item;
  GtkWidget * menu;
  } edit_menu_t;

typedef struct
  {
  GtkWidget *      add_item;
  add_menu_t       add_menu;
  GtkWidget *      selected_item;
  selected_menu_t  selected_menu;

  GtkWidget *      edit_item;
  edit_menu_t      edit_menu;
  
  GtkWidget      * pp_item;
  
  GtkWidget      * menu;
  } menu_t;

enum
  {
    COLUMN_INDEX,
    COLUMN_NAME,
    COLUMN_AUDIO,
    COLUMN_VIDEO,
    COLUMN_DURATION,
    NUM_COLUMNS
  };

struct track_list_s
  {
  GtkWidget * treeview;
  GtkWidget * widget;

  /* Buttons */

  GtkWidget * add_file_button;
  GtkWidget * add_url_button;
  GtkWidget * add_removable_button;

  GtkWidget * delete_button;
  GtkWidget * config_button;
  //  GtkWidget * up_button;
  //  GtkWidget * down_button;
  GtkWidget * encoder_button;
  GtkWidget * chapter_button;

  GtkWidget * cut_button;
  GtkWidget * copy_button;
  GtkWidget * paste_button;
  
  bg_transcoder_track_t * tracks;
  bg_transcoder_track_global_t track_global;

  bg_plugin_registry_t * plugin_reg;

  GtkTreeViewColumn * col_name;

  bg_transcoder_track_t * selected_track;
  int num_selected;

  gulong select_handler_id;

  bg_cfg_section_t * track_defaults_section;
  bg_cfg_section_t * encoder_section;
  const bg_parameter_info_t * encoder_parameters;
  
  bg_gtk_time_display_t * time_total;


  int show_tooltips;

  menu_t menu;

  encoder_pp_window_t * encoder_pp_window;

  char * open_path;
  bg_gtk_filesel_t * filesel;
  
  char * clipboard;
  
  GtkAccelGroup * accel_group;
  };


/* Called when the selecection changed */

static void select_row_callback(GtkTreeSelection * sel,
                                gpointer data)
  {
  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  
  bg_transcoder_track_t * track;
  
  track_list_t * w = (track_list_t *)data;

  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w->treeview));
  
  w->num_selected = 0;

  track = w->tracks;

  if(!gtk_tree_model_get_iter_first(model, &iter) || !track)
    {
    gtk_widget_set_sensitive(w->config_button, 0);
    gtk_widget_set_sensitive(w->encoder_button, 0);
    w->selected_track = (bg_transcoder_track_t*)0;
    //    gtk_widget_set_sensitive(w->up_button, 0);
    //    gtk_widget_set_sensitive(w->down_button, 0);
    gtk_widget_set_sensitive(w->delete_button, 0);
    gtk_widget_set_sensitive(w->cut_button, 0);
    gtk_widget_set_sensitive(w->copy_button, 0);
    
    gtk_widget_set_sensitive(w->menu.selected_menu.move_up_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.move_down_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.configure_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.remove_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.encoder_item, 0);
    
    gtk_widget_set_sensitive(w->menu.edit_menu.cut_item, 0);
    gtk_widget_set_sensitive(w->menu.edit_menu.copy_item, 0);
    return;
    }
  
  while(1)
    {
    if(gtk_tree_selection_iter_is_selected(selection, &iter))
      {
      track->selected = 1;
      w->selected_track = track;
      w->num_selected++;
      }
    else
      track->selected = 0;
    
    if(!gtk_tree_model_iter_next(model, &iter))
      break;
    track = track->next;
    if(!track)
      break;
    }
  
  if(w->num_selected == 1)
    {
    gtk_widget_set_sensitive(w->config_button, 1);
    gtk_widget_set_sensitive(w->encoder_button, 1);
    //    gtk_widget_set_sensitive(w->up_button, 1);
    //    gtk_widget_set_sensitive(w->down_button, 1);
    gtk_widget_set_sensitive(w->delete_button, 1);
    gtk_widget_set_sensitive(w->chapter_button, 1);
    gtk_widget_set_sensitive(w->cut_button, 1);
    gtk_widget_set_sensitive(w->copy_button, 1);
    
    gtk_widget_set_sensitive(w->menu.selected_menu.move_up_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.move_down_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.configure_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.remove_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.encoder_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.chapter_item, 1);

    gtk_widget_set_sensitive(w->menu.edit_menu.cut_item, 1);
    gtk_widget_set_sensitive(w->menu.edit_menu.copy_item, 1);
    
    
    }
  else if(w->num_selected == 0)
    {
    gtk_widget_set_sensitive(w->config_button, 0);
    gtk_widget_set_sensitive(w->encoder_button, 0);
    w->selected_track = (bg_transcoder_track_t*)0;
    //    gtk_widget_set_sensitive(w->up_button, 0);
    //    gtk_widget_set_sensitive(w->down_button, 0);
    gtk_widget_set_sensitive(w->delete_button, 0);
    gtk_widget_set_sensitive(w->chapter_button, 0);
    gtk_widget_set_sensitive(w->cut_button, 0);
    gtk_widget_set_sensitive(w->copy_button, 0);

    gtk_widget_set_sensitive(w->menu.selected_menu.move_up_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.move_down_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.configure_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.remove_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.encoder_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.chapter_item, 0);

    gtk_widget_set_sensitive(w->menu.edit_menu.cut_item, 0);
    gtk_widget_set_sensitive(w->menu.edit_menu.copy_item, 0);
    }
  else
    {
    gtk_widget_set_sensitive(w->config_button, 0);
    w->selected_track = (bg_transcoder_track_t*)0;
    gtk_widget_set_sensitive(w->encoder_button, 1);
    //    gtk_widget_set_sensitive(w->up_button, 1);
    //    gtk_widget_set_sensitive(w->down_button, 1);
    gtk_widget_set_sensitive(w->delete_button, 1);
    gtk_widget_set_sensitive(w->chapter_button, 0);
    gtk_widget_set_sensitive(w->cut_button, 1);
    gtk_widget_set_sensitive(w->copy_button, 1);

    gtk_widget_set_sensitive(w->menu.selected_menu.move_up_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.move_down_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.configure_item, 0);
    gtk_widget_set_sensitive(w->menu.selected_menu.remove_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.encoder_item, 1);
    gtk_widget_set_sensitive(w->menu.selected_menu.chapter_item, 0);
    
    gtk_widget_set_sensitive(w->menu.edit_menu.cut_item, 1);
    gtk_widget_set_sensitive(w->menu.edit_menu.copy_item, 1);
    }
  }

/* Update the entire list */

static void track_list_update(track_list_t * w)
  {
  int i;
  GtkTreeModel * model;
  GtkTreeIter iter;
  bg_transcoder_track_t * track;
  gavl_time_t track_duration;
  gavl_time_t track_duration_total;

  gavl_time_t duration_total;
  char * name;
  
  char string_buffer[GAVL_TIME_STRING_LEN + 32];
  char string_buffer1[GAVL_TIME_STRING_LEN + 32];
  char * buf;
  
  GtkTreeSelection * selection;
   
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w->treeview));

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));

  g_signal_handler_block(G_OBJECT(selection), w->select_handler_id);
   
  gtk_list_store_clear(GTK_LIST_STORE(model));

  track = w->tracks;
  i = 0;


  duration_total = 0;
    
  while(track)
    {
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);

    name = bg_transcoder_track_get_name(track);
    
    
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
                       name, -1);
    
    /* Audio */
    if(track->num_audio_streams)
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
    if(track->num_video_streams)
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_VIDEO,
                         has_video_pixbuf, -1);
    else
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_VIDEO,
                         NULL, -1);

    /* Set time */
    
    bg_transcoder_track_get_duration(track, &track_duration, &track_duration_total);
    
    if(duration_total != GAVL_TIME_UNDEFINED)
      {
      if(track_duration == GAVL_TIME_UNDEFINED)
        duration_total = GAVL_TIME_UNDEFINED;
      else
        duration_total += track_duration;
      }

    if(track_duration == track_duration_total)
      {
      gavl_time_prettyprint(track_duration, string_buffer);
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_DURATION,
                         string_buffer, -1);
      }
    else
      {
      gavl_time_prettyprint(track_duration, string_buffer);
      gavl_time_prettyprint(track_duration_total, string_buffer1);
      buf = bg_sprintf("%s/%s", string_buffer, string_buffer1);
      gtk_list_store_set(GTK_LIST_STORE(model),
                         &iter,
                         COLUMN_DURATION,
                         buf, -1);
      free(buf);
      }
    
    /* Select track */

    if(track->selected)
      {
      gtk_tree_selection_select_iter(selection, &iter);
      }
    
    free(name);
        
    i++;
    track = track->next;
    }
  g_signal_handler_unblock(G_OBJECT(selection), w->select_handler_id);

  bg_gtk_time_display_update(w->time_total, duration_total,
                             BG_GTK_DISPLAY_MODE_HMS);

  /* Flush events */

  //  while(gdk_events_pending() || gtk_events_pending())
  //    gtk_main_iteration();
  
  select_row_callback(NULL, w);
  
  }

/* Append a track to the list */

static void add_track(track_list_t * l,
                      bg_transcoder_track_t * new_track)
  {
  bg_transcoder_track_t * track;

  if(!l->tracks)
    {
    l->tracks = new_track;
    return;
    }
  track = l->tracks;
  
  while(track->next)
    {
    track = track->next;
    }
  track->next = new_track;
  }

static void delete_selected(track_list_t * l)
  {
  l->tracks = 
    bg_transcoder_tracks_delete_selected(l->tracks);
  
  track_list_update(l);
  }

/* Callback functions for the clipboard */

static void clipboard_get_func(GtkClipboard *clipboard,
                               GtkSelectionData *selection_data,
                               guint info,
                               gpointer data)
  {
  GdkAtom type_atom;
  track_list_t * w = (track_list_t*)data;
  
  type_atom = gdk_atom_intern("STRING", FALSE);
  if(!type_atom)
    return;
  
  gtk_selection_data_set(selection_data, type_atom, 8, (uint8_t*)w->clipboard,
                         strlen(w->clipboard)+1);
  }

static void clipboard_clear_func(GtkClipboard *clipboard,
                                 gpointer data)
  {
  track_list_t * w = (track_list_t*)data;
  if(w->clipboard)
    {
    free(w->clipboard);
    w->clipboard = (char*)0;
    }
  }

static void
clipboard_received_func_tracks(GtkClipboard *clipboard,
                               GtkSelectionData *selection_data,
                               gpointer data)
  {
  bg_transcoder_track_t * new_tracks;
  track_list_t * w = (track_list_t*)data;
  
  if(selection_data->length <= 0)
    return;

  new_tracks = bg_transcoder_tracks_from_xml((char*)selection_data->data,
                                             w->plugin_reg);
  
  w->tracks = bg_transcoder_tracks_append(w->tracks, new_tracks);
  track_list_update(w);  
  }

static void
clipboard_received_func_albumentries(GtkClipboard *clipboard,
                                     GtkSelectionData *selection_data,
                                     gpointer data)
  {
  track_list_t * w = (track_list_t*)data;
  
  if(selection_data->length <= 0)
    return;
  track_list_add_albumentries_xml(w, (char*)(selection_data->data));
  }


static void do_copy(track_list_t * w)
  {
  GtkClipboard *clipboard;
  GdkAtom clipboard_atom;
  
  // clipboard_atom = gdk_atom_intern ("PRIMARY", FALSE);
  clipboard_atom = gdk_atom_intern ("CLIPBOARD", FALSE);   
  clipboard = gtk_clipboard_get(clipboard_atom);
  
  gtk_clipboard_set_with_data(clipboard,
                              copy_paste_entries,
                              sizeof(copy_paste_entries)/
                              sizeof(copy_paste_entries[0]),
                              clipboard_get_func,
                              clipboard_clear_func,
                              (gpointer)w);
  
  if(w->clipboard)
    free(w->clipboard);
  w->clipboard = bg_transcoder_tracks_selected_to_xml(w->tracks);

  }

static void do_cut(track_list_t * l)
  {
  do_copy(l);
  delete_selected(l);
  }

static void target_received_func(GtkClipboard *clipboard,
                                 GdkAtom *atoms,
                                 gint n_atoms,
                                 gpointer data)
  {
  track_list_t * l;
  int i = 0;
  char * atom_name;
  l = (track_list_t *)(data);
  
  for(i = 0; i < n_atoms; i++)
    {
    atom_name = gdk_atom_name(atoms[i]);
    if(!atom_name)
      return;
    else if(!strcmp(atom_name, cp_tracks_name))
      {
      gtk_clipboard_request_contents(clipboard,
                                     atoms[i],
                                     clipboard_received_func_tracks,
                                     l);
      g_free(atom_name);
      return;
      }
    else if(!strcmp(atom_name, bg_gtk_atom_entries_name) ||
            !strcmp(atom_name, bg_gtk_atom_entries_name_r))
      {
      fprintf(stderr, "Paste albumentries\n");
      gtk_clipboard_request_contents(clipboard,
                                     atoms[i],
                                     clipboard_received_func_albumentries,
                                     l);
      return;
      }
    }
  }

static void do_paste(track_list_t * l)
  {
  GtkClipboard *clipboard;
  GdkAtom clipboard_atom;
  GdkAtom target;

  //    clipboard_atom = gdk_atom_intern ("PRIMARY", FALSE);
  clipboard_atom = gdk_atom_intern ("CLIPBOARD", FALSE);   
  clipboard = gtk_clipboard_get(clipboard_atom);
  
  target = gdk_atom_intern(cp_tracks_name, FALSE);
  
  gtk_clipboard_request_targets(clipboard,
                                target_received_func, l);  
  }



static void move_up(track_list_t * l)
  {
  l->tracks = bg_transcoder_tracks_move_selected_up(l->tracks);
  track_list_update(l);
  }

static void move_down(track_list_t * l)
  {
  l->tracks = bg_transcoder_tracks_move_selected_down(l->tracks);
  track_list_update(l);
  }

static void add_file_callback(char ** files, const char * plugin,
                              void * data)
  {
  const bg_plugin_info_t * plugin_info;
  bg_transcoder_track_t * new_track;
  track_list_t * l;
  int i = 0;
  
  l = (track_list_t *)(data);

  if(plugin)
    plugin_info = bg_plugin_find_by_name(l->plugin_reg, plugin);
  else
    plugin_info = (const bg_plugin_info_t*)0;
  while(files[i])
    {
    
    new_track =
      bg_transcoder_track_create(files[i], plugin_info,
                                 -1, l->plugin_reg,
                                 l->track_defaults_section, l->encoder_section, (char*)0);
    add_track(l, new_track);
    track_list_update(l);
    i++;
    }

  /* Remember open path */
  if(l->filesel)
    l->open_path = bg_strdup(l->open_path, bg_gtk_filesel_get_directory(l->filesel));
  }

static void filesel_close_callback(bg_gtk_filesel_t * f , void * data)
  {
  track_list_t * t;
  t = (track_list_t*)data;
  gtk_widget_set_sensitive(t->add_file_button, 1);
  t->filesel = (bg_gtk_filesel_t *)0;
  }

static void urlsel_close_callback(bg_gtk_urlsel_t * f , void * data)
  {
  track_list_t * t;
  t = (track_list_t*)data;
  gtk_widget_set_sensitive(t->add_url_button, 1);
  gtk_widget_set_sensitive(t->menu.add_menu.add_urls_item, 1);
  }

static void drivesel_close_callback(bg_gtk_drivesel_t * f , void * data)
  {
  track_list_t * t;
  t = (track_list_t*)data;
  gtk_widget_set_sensitive(t->add_removable_button, 1);
  }


/* Set track name */

static void update_track(void * data)
  {
  track_list_t * l = (track_list_t *)data;

  track_list_update(l);
  }

typedef struct
  {
  int changed;
  } encoder_parameter_data;

static void set_encoder_parameter(void * data, const char * name,
                                 const bg_parameter_value_t * val)
  {
  encoder_parameter_data * d = data;
  d->changed = 1;
  }

static void configure_encoders(track_list_t * l)
  {
  
  bg_cfg_section_t * s;
  bg_dialog_t * dlg;
  
  bg_transcoder_track_t * first_selected;

  encoder_parameter_data d;
  d.changed = 0;
  
  first_selected = l->tracks;

  while(first_selected && !first_selected->selected)
    first_selected = first_selected->next;

  if(!first_selected)
    return;

  s = bg_cfg_section_create("Encoders");

  /* All settings, which are not in the track, are taken from the
     current encoder settings */
  bg_cfg_section_transfer(l->encoder_section, s);
  
  bg_transcoder_track_get_encoders(first_selected, l->plugin_reg, s);
  
  dlg = bg_dialog_create(s,
                         set_encoder_parameter,
                         NULL,
                         &d,
                         l->encoder_parameters,
                         TR("Encoder configuration"));

  bg_dialog_show(dlg, l->widget);

  if(d.changed)
    {
    while(first_selected)
      {
      if(first_selected->selected)
        {
        bg_transcoder_track_set_encoders(first_selected,
                                         l->plugin_reg,
                                         s);
        }
      first_selected = first_selected->next;
      }
    }

  bg_dialog_destroy(dlg);
  bg_cfg_section_destroy(s);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  track_list_t * t;
  bg_gtk_urlsel_t * urlsel;
  bg_gtk_drivesel_t * drivesel;
  
  track_dialog_t * track_dialog;

  gavl_time_t track_duration;
  gavl_time_t track_duration_total;

  
  t = (track_list_t*)data;

  if((w == t->add_file_button) || (w == t->menu.add_menu.add_files_item))
    {
    t->filesel = bg_gtk_filesel_create("Add URLs",
                                       add_file_callback,
                                       filesel_close_callback,
                                       t, NULL /* parent */,
                                       t->plugin_reg, BG_PLUGIN_INPUT,
                                       BG_PLUGIN_FILE);

    bg_gtk_filesel_set_directory(t->filesel, t->open_path);
    gtk_widget_set_sensitive(t->add_file_button, 0);
    bg_gtk_filesel_run(t->filesel, 0);     
    
    //    bg_gtk_filesel_destroy(filesel);
    
    }
  else if((w == t->add_url_button) || (w == t->menu.add_menu.add_urls_item))
    {
    urlsel = bg_gtk_urlsel_create(TR("Add files"),
                                  add_file_callback,
                                  urlsel_close_callback,
                                  t, NULL  /* parent */,
                                  t->plugin_reg,
                                  BG_PLUGIN_INPUT,
                                  BG_PLUGIN_URL);
    gtk_widget_set_sensitive(t->add_url_button, 0);
    gtk_widget_set_sensitive(t->menu.add_menu.add_urls_item, 0);
    bg_gtk_urlsel_run(urlsel, 0, t->add_url_button);
    //    bg_gtk_urlsel_destroy(urlsel);
    }
  else if((w == t->add_removable_button) || (w == t->menu.add_menu.add_drives_item))
    {
    drivesel = bg_gtk_drivesel_create("Add drive",
                                      add_file_callback,
                                      drivesel_close_callback,
                                      t, NULL  /* parent */,
                                      t->plugin_reg, BG_PLUGIN_INPUT, BG_PLUGIN_REMOVABLE);
    gtk_widget_set_sensitive(t->add_removable_button, 0);
    bg_gtk_drivesel_run(drivesel, 0, t->add_removable_button);
    
    
    }
  else if((w == t->delete_button) || (w == t->menu.selected_menu.remove_item))
    {
    delete_selected(t);
    }
  else if(w == t->menu.selected_menu.move_up_item)
    {
    move_up(t);
    }
  else if(w == t->menu.selected_menu.move_down_item)
    {
    move_down(t);
    }
  else if((w == t->config_button) || (w == t->menu.selected_menu.configure_item))
    {
    track_dialog = track_dialog_create(t->selected_track, update_track,
                                       t, t->show_tooltips, t->plugin_reg);
    track_dialog_run(track_dialog, t->treeview);
    track_dialog_destroy(track_dialog);

    }
  else if((w == t->menu.edit_menu.cut_item) || (w == t->cut_button))
    {
    do_cut(t);
    }
  else if((w == t->menu.edit_menu.copy_item) || (w == t->copy_button))
    {
    do_copy(t);
    }
  else if((w == t->menu.edit_menu.paste_item) || (w == t->paste_button))
    {
    do_paste(t);
    }
  else if((w == t->chapter_button) || (w == t->menu.selected_menu.chapter_item))
    {
    bg_transcoder_track_get_duration(t->selected_track,
                                     &track_duration, &track_duration_total);

    
    bg_gtk_chapter_dialog_show(&(t->selected_track->chapter_list),
                               track_duration, t->treeview);
    }
  else if((w == t->encoder_button) ||
          (w == t->menu.selected_menu.encoder_item))
    {
    configure_encoders(t);
    }
  else if(w == t->menu.pp_item)
    {
    encoder_pp_window_run(t->encoder_pp_window, &t->track_global);
    }
  }

/* Menu stuff */

static GtkWidget *
create_item(track_list_t * w, GtkWidget * parent,
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
  
  g_signal_connect(G_OBJECT(ret), "activate", G_CALLBACK(button_callback),
                   (gpointer)w);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }

static void init_menu(track_list_t * t)
  {
  /* Add */

  t->menu.add_menu.menu = gtk_menu_new();

  t->menu.add_menu.add_files_item =
    create_item(t, t->menu.add_menu.menu, TR("Files..."), "folder_open_16.png");
  t->menu.add_menu.add_urls_item =
    create_item(t, t->menu.add_menu.menu, TR("Urls..."), "earth_16.png");
  t->menu.add_menu.add_drives_item =
    create_item(t, t->menu.add_menu.menu, TR("Drives..."), "drive_running_16.png");
  gtk_widget_show(t->menu.add_menu.menu);
  
  /* Selected */

  t->menu.selected_menu.menu = gtk_menu_new();

  t->menu.selected_menu.move_up_item =
    create_item(t, t->menu.selected_menu.menu, TR("Move Up"), "top_16.png");
  t->menu.selected_menu.move_down_item =
    create_item(t, t->menu.selected_menu.menu, TR("Move Down"), "bottom_16.png");
  t->menu.selected_menu.remove_item =
    create_item(t, t->menu.selected_menu.menu, TR("Remove"), "trash_16.png");
  t->menu.selected_menu.configure_item =
    create_item(t, t->menu.selected_menu.menu, TR("Configure..."), "config_16.png");
  t->menu.selected_menu.chapter_item =
    create_item(t, t->menu.selected_menu.menu, TR("Edit chapters..."), "chapter_16.png");
  t->menu.selected_menu.encoder_item =
    create_item(t, t->menu.selected_menu.menu, TR("Change encoders..."), "plugin_16.png");

  /* Edit */

  t->menu.edit_menu.menu = gtk_menu_new();

  t->menu.edit_menu.cut_item =
    create_item(t, t->menu.edit_menu.menu, TR("Cut"), "cut_16.png");
  gtk_widget_add_accelerator(t->menu.edit_menu.cut_item, "activate", t->accel_group,
                             GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  t->menu.edit_menu.copy_item =
    create_item(t, t->menu.edit_menu.menu, TR("Copy"), "copy_16.png");
  gtk_widget_add_accelerator(t->menu.edit_menu.copy_item, "activate", t->accel_group,
                             GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  t->menu.edit_menu.paste_item =
    create_item(t, t->menu.edit_menu.menu, TR("Paste"), "paste_16.png");
  gtk_widget_add_accelerator(t->menu.edit_menu.paste_item, "activate", t->accel_group,
                             GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  gtk_widget_show(t->menu.add_menu.menu);
  
  /* Root menu */

  t->menu.menu = gtk_menu_new();
  
  t->menu.add_item =
    create_item(t, t->menu.menu, TR("Add..."), (char*)0);
  
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(t->menu.add_item),
                            t->menu.add_menu.menu);

  t->menu.selected_item =
    create_item(t, t->menu.menu, TR("Selected..."), (char*)0);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(t->menu.selected_item),
                            t->menu.selected_menu.menu);

  t->menu.edit_item =
    create_item(t, t->menu.menu, TR("Edit..."), (char*)0);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(t->menu.edit_item),
                            t->menu.edit_menu.menu);
  
  t->menu.pp_item =
    create_item(t, t->menu.menu, TR("Postprocess..."), (char*)0);

  gtk_widget_set_sensitive(t->menu.selected_menu.move_up_item, 0);
  gtk_widget_set_sensitive(t->menu.selected_menu.move_down_item, 0);
  gtk_widget_set_sensitive(t->menu.selected_menu.configure_item, 0);
  gtk_widget_set_sensitive(t->menu.selected_menu.remove_item, 0);
  gtk_widget_set_sensitive(t->menu.selected_menu.encoder_item, 0);
  gtk_widget_set_sensitive(t->menu.selected_menu.chapter_item, 0);
  
  gtk_widget_set_sensitive(t->menu.edit_menu.cut_item, 0);
  gtk_widget_set_sensitive(t->menu.edit_menu.copy_item, 0);
  }

GtkWidget * track_list_get_menu(track_list_t * t)
  {
  return t->menu.menu;
  }

static gboolean button_press_callback(GtkWidget * w, GdkEventButton * evt,
                                      gpointer data)
  {
  track_list_t * t = (track_list_t*)data;

  if(evt->button == 3)
    {
    gtk_menu_popup(GTK_MENU(t->menu.menu),
                   (GtkWidget *)0,
                   (GtkWidget *)0,
                   (GtkMenuPositionFunc)0,
                   (gpointer)0,
                   3, evt->time);
    return TRUE;
    }
  return FALSE;
  }

/* */

static void column_resize_callback(GtkTreeViewColumn * col,
                                   gint * width_val,
                                   gpointer data)
  {
  int width_needed;
  int name_width;
  int width;
  
  track_list_t * w = (track_list_t *)data;
  
  width = col->width;
  
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


void track_list_add_albumentries_xml(track_list_t * l, char * xml_string)
  {
  bg_transcoder_track_t * new_tracks;
  new_tracks =
    bg_transcoder_track_create_from_albumentries(xml_string,
                                                 l->plugin_reg,
                                                 l->track_defaults_section, l->encoder_section);
  add_track(l, new_tracks);
  track_list_update(l);  
  }

void track_list_add_url(track_list_t * l, char * url)
  {
  bg_transcoder_track_t * new_tracks;
  new_tracks =
    bg_transcoder_track_create(url,
                               (const bg_plugin_info_t *)0,
                               -1, l->plugin_reg,
                               l->track_defaults_section, l->encoder_section, (char*)0);
  add_track(l, new_tracks);
  track_list_update(l);  
  }
#if 0
static gboolean drag_drop_callback(GtkWidget      *widget,
                               GdkDragContext *drag_context,
                               gint            x,
                               gint            y,
                               guint           time,
                               gpointer        user_data)
  {
  GdkAtom target;
  track_list_t * l = (track_list_t *)user_data;
  
  target = gtk_drag_dest_find_target(widget, drag_context,
                                     gtk_drag_dest_get_target_list(widget));

  //  fprintf(stderr, "Drag drop %d (%s)\n", target, gtk_atom_get);
  if(target != GDK_NONE)
    gtk_drag_get_data(widget,
                      drag_context,
                      target,
                      time);
  return TRUE;
  }
#endif
static void drag_received_callback(GtkWidget *widget,
                                   GdkDragContext *drag_context,
                                   gint x,
                                   gint y,
                                   GtkSelectionData *data,
                                   guint info,
                                   guint time,
                                   gpointer d)
  {
  bg_transcoder_track_t * new_tracks;
  track_list_t * l = (track_list_t *)d;
  
  if(is_urilist(data))
    {
    new_tracks =
      bg_transcoder_track_create_from_urilist((char*)(data->data),
                                              data->length,
                                              l->plugin_reg,
                                              l->track_defaults_section, l->encoder_section);
    add_track(l, new_tracks);
    track_list_update(l);  
    }
  else if(is_albumentries(data))
    {
    track_list_add_albumentries_xml(l, (char*)(data->data));
    }

  gtk_drag_finish(drag_context,
                  TRUE, /* Success */
                  0, /* Delete */
                  time);
  }

/* Buttons */

static GtkWidget * create_pixmap_button(track_list_t * l, const char * filename,
                                        const char * tooltip)
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

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(button_callback), l);
  gtk_widget_show(button);
  return button;
  }


track_list_t * track_list_create(bg_plugin_registry_t * plugin_reg,
                                 bg_cfg_section_t * track_defaults_section,
                                 const bg_parameter_info_t * encoder_parameters,
                                 bg_cfg_section_t * encoder_section)
  {
  GtkWidget * scrolled;
  GtkWidget * box;
  track_list_t * ret;

  GtkTreeViewColumn * col;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeSelection * selection;
  char * tmp_path;
  
  
  load_pixmaps();
  
  ret = calloc(1, sizeof(*ret));
  
  ret->track_defaults_section = track_defaults_section;
  ret->encoder_section = encoder_section;
  ret->encoder_parameters = encoder_parameters;
  
  ret->time_total =
    bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL, 4,
                               BG_GTK_DISPLAY_MODE_HMS);
  ret->encoder_pp_window = encoder_pp_window_create(plugin_reg);

  ret->accel_group = gtk_accel_group_new();
    
  bg_gtk_tooltips_set_tip(bg_gtk_time_display_get_widget(ret->time_total),
                          TRS("Total playback time"),
                          PACKAGE);

  ret->plugin_reg = plugin_reg;
  
  /* Create buttons */
  ret->add_file_button =
    create_pixmap_button(ret,
                         "folder_open_16.png",
                         TRS("Append files to the task list"));

  ret->add_url_button =
    create_pixmap_button(ret,
                         "earth_16.png",
                         TRS("Append URLs to the task list"));

  ret->add_removable_button =
    create_pixmap_button(ret,
                         "drive_running_16.png",
                         TRS("Append removable media to the task list"));
  
  ret->delete_button =
    create_pixmap_button(ret,
                         "trash_16.png",
                         TRS("Delete selected tracks"));

  ret->config_button =
    create_pixmap_button(ret,
                         "config_16.png",
                         TRS("Configure selected track"));

  ret->chapter_button =
    create_pixmap_button(ret,
                         "chapter_16.png",
                         TRS("Edit chapters"));
  
  ret->encoder_button =
    create_pixmap_button(ret,
                         "plugin_16.png",
                         TRS("Change encoder plugins for selected tracks"));

  ret->copy_button =
    create_pixmap_button(ret,
                         "copy_16.png",
                         TRS("Copy selected tracks to clipboard"));
  ret->cut_button =
    create_pixmap_button(ret,
                         "cut_16.png",
                         TRS("Cut selected tracks to clipboard"));

  ret->paste_button =
    create_pixmap_button(ret,
                         "paste_16.png",
                         TRS("Paste tracks from clipboard"));
 
  
  gtk_widget_set_sensitive(ret->delete_button, 0);
  gtk_widget_set_sensitive(ret->encoder_button, 0);
  gtk_widget_set_sensitive(ret->config_button, 0);
  gtk_widget_set_sensitive(ret->chapter_button, 0);
  //  gtk_widget_set_sensitive(ret->up_button, 0);
  //  gtk_widget_set_sensitive(ret->down_button, 0);
  gtk_widget_set_sensitive(ret->cut_button, 0);
  gtk_widget_set_sensitive(ret->copy_button, 0);
  
  /* Create list view */
  
  store = gtk_list_store_new(NUM_COLUMNS,
                             G_TYPE_STRING,   // Index
                             G_TYPE_STRING,   // Name
                             GDK_TYPE_PIXBUF, // Audio
                             GDK_TYPE_PIXBUF, // Video
                             G_TYPE_STRING);  // Duration
  ret->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_size_request(ret->treeview, 200, 100);

  selection =
    gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->treeview));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

  ret->select_handler_id =
    g_signal_connect(G_OBJECT(selection), "changed",
                     G_CALLBACK(select_row_callback), (gpointer)ret);
  
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ret->treeview), 0);
  
  gtk_drag_dest_set(ret->treeview,
                    GTK_DEST_DEFAULT_ALL,
                    /*
                      GTK_DEST_DEFAULT_HIGHLIGHT |
                      GTK_DEST_DEFAULT_DROP |
                      GTK_DEST_DEFAULT_MOTION,
                    */
                    dnd_dst_entries,
                    sizeof(dnd_dst_entries)/sizeof(dnd_dst_entries[0]),
                    GDK_ACTION_COPY | GDK_ACTION_MOVE);

  g_signal_connect(G_OBJECT(ret->treeview), "drag-data-received",
                   G_CALLBACK(drag_received_callback),
                   (gpointer)ret);
#if 0
  g_signal_connect(G_OBJECT(ret->treeview), "drag-drop",
                   G_CALLBACK(drag_drop_callback),
                   (gpointer)ret);
#endif
  g_signal_connect(G_OBJECT(ret->treeview), "button-press-event",
                   G_CALLBACK(button_press_callback), (gpointer)ret);
  
  
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

  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview), col);

  /* Name */
  
  renderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer), "xalign", 0.0, NULL);
  
  col = gtk_tree_view_column_new ();
  ret->col_name = col;
  
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  
  gtk_tree_view_column_add_attribute(col, renderer,
                                     "text", COLUMN_NAME);
  
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_FIXED);
  
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);
  
  /* Audio */
  
  renderer = gtk_cell_renderer_pixbuf_new();
  
  col = gtk_tree_view_column_new ();
  
  gtk_tree_view_column_set_title(col, "A");
  
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
  
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "pixbuf", COLUMN_AUDIO);
  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_GROW_ONLY);

  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);

  /* Video */
  
  renderer = gtk_cell_renderer_pixbuf_new();
  
  col = gtk_tree_view_column_new ();
  
  gtk_tree_view_column_set_title(col, "V");
  
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
  
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "pixbuf", COLUMN_VIDEO);
  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
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
  
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);

  /* Done with columns */
  
  gtk_widget_show(ret->treeview);

  /* Pack */
  
  scrolled =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(ret->treeview)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ret->treeview)));
  
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(scrolled), ret->treeview);
  gtk_widget_show(scrolled);
    
  ret->widget = gtk_table_new(2, 1, 0);
  gtk_table_attach_defaults(GTK_TABLE(ret->widget), scrolled, 0, 1, 0, 1);

  box = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->add_file_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->add_url_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->add_removable_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->delete_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->encoder_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->config_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->chapter_button, FALSE, FALSE, 0);
  //  gtk_box_pack_start(GTK_BOX(box), ret->up_button, FALSE, FALSE, 0);
  //  gtk_box_pack_start(GTK_BOX(box), ret->down_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->cut_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->copy_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->paste_button, FALSE, FALSE, 0);
  
  gtk_box_pack_end(GTK_BOX(box), bg_gtk_time_display_get_widget(ret->time_total),
                     FALSE, FALSE, 0);
  gtk_widget_show(box);

  gtk_table_attach(GTK_TABLE(ret->widget),
                   box, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(ret->widget);
  init_menu(ret);
  
  /* Load tracks */

  tmp_path = bg_search_file_read("transcoder", "tracks.xml");
  
  if(tmp_path)
    {
    ret->tracks = bg_transcoder_tracks_load(tmp_path, &ret->track_global, ret->plugin_reg);

    free(tmp_path);

    track_list_update(ret);
    }
  return ret;
  }

void track_list_destroy(track_list_t * t)
  {
  char * tmp_path;
  bg_transcoder_track_t * tmp_track;

  bg_transcoder_track_global_free(&t->track_global);
  
  tmp_path = bg_search_file_write("transcoder", "tracks.xml");

  if(tmp_path)
    {
    bg_transcoder_tracks_save(t->tracks, &t->track_global, tmp_path);
    free(tmp_path);
    }

  tmp_track = t->tracks;

  while(t->tracks)
    {
    tmp_track = t->tracks->next;
    bg_transcoder_track_destroy(t->tracks);
    t->tracks = tmp_track;
    }
  g_object_unref(t->accel_group);

  if(t->open_path)
    free(t->open_path);
  if(t->clipboard)
    free(t->clipboard);
  
  free(t);
  }

GtkWidget * track_list_get_widget(track_list_t * t)
  {
  return t->widget;
  }

GtkAccelGroup * track_list_get_accel_group(track_list_t * t)
  {
  return t->accel_group;
  }

bg_plugin_handle_t * track_list_get_pp_plugin(track_list_t * t)
  {
  const bg_plugin_info_t * info;
  info = encoder_pp_window_get_plugin(t->encoder_pp_window);
  
  return bg_plugin_load(t->plugin_reg, info);
  }


bg_transcoder_track_t * track_list_get_track(track_list_t * t)
  {
  bg_transcoder_track_t * ret;
  if(!t->tracks)
    return (bg_transcoder_track_t*)0;
  ret = t->tracks;
  t->tracks = t->tracks->next;
  track_list_update(t);
  return ret;
  }

void track_list_prepend_track(track_list_t * t, bg_transcoder_track_t * track)
  {
  track->next = t->tracks;
  t->tracks = track;
  track_list_update(t);
  }

void track_list_load(track_list_t * t, const char * filename)
  {
  bg_transcoder_track_t * end_track;
  bg_transcoder_track_t * new_tracks;

  new_tracks = bg_transcoder_tracks_load(filename, &t->track_global, t->plugin_reg);
  if(!t->tracks)
    {
    t->tracks = new_tracks;
    }
  else
    {
    end_track = t->tracks;
    while(end_track->next)
      end_track = end_track->next;
    end_track->next = new_tracks;
    }
  track_list_update(t);
  }

void track_list_save(track_list_t * t, const char * filename)
  {
  bg_transcoder_tracks_save(t->tracks, &t->track_global, filename);
  }

void track_list_set_display_colors(track_list_t * t, float * fg, float * bg)
  {
  bg_gtk_time_display_set_colors(t->time_total, fg, bg);
  }


bg_parameter_info_t parameters[] =
  {
    {
      .name =        "open_path",
      .long_name =   "Open Path",
      .type =        BG_PARAMETER_DIRECTORY,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_str = "." },
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t * track_list_get_parameters(track_list_t * t)
  {
  return parameters;
  }

void
track_list_set_parameter(void * data, const char * name,
                         const bg_parameter_value_t * val)
  {
  track_list_t * t;
  if(!name)
    return;

  t = (track_list_t *)data;
  
  if(!strcmp(name, "open_path"))
    t->open_path = bg_strdup(t->open_path, val->val_str);
  }

int track_list_get_parameter(void * data, const char * name, bg_parameter_value_t * val)
  {
  track_list_t * t;
  if(!name)
    return 0;

  t = (track_list_t *)data;
  
  if(!strcmp(name, "open_path"))
    {
    val->val_str = bg_strdup(val->val_str, t->open_path);
    return 1;
    }
  return 0;
  }
