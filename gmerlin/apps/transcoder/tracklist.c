
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <pluginregistry.h>
#include <utils.h>

#include <tree.h>

#include <gui_gtk/fileselect.h>
#include <gui_gtk/tree.h>
#include <gui_gtk/display.h>

#include <transcoder_track.h>
#include "tracklist.h"
#include "trackdialog.h"

static char ** file_plugins = (char **)0;
static char **  url_plugins = (char **)0;

static GdkPixbuf * has_audio_pixbuf = (GdkPixbuf *)0;
static GdkPixbuf * has_video_pixbuf = (GdkPixbuf *)0;

/* 0 means unset */

#define DND_GMERLIN_TRACKS   1
#define DND_GMERLIN_TRACKS_R 2
#define DND_TEXT_URI_LIST    3
#define DND_TEXT_PLAIN       4

static GtkTargetEntry dnd_dst_entries[] =
  {
    {"text/uri-list",            0, DND_TEXT_URI_LIST    },
    {"text/plain",               0, DND_TEXT_PLAIN       },
    {bg_gtk_atom_entries_name,   0, DND_GMERLIN_TRACKS   },
    {bg_gtk_atom_entries_name_r, 0, DND_GMERLIN_TRACKS_R },
  };

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

  GtkWidget * add_button;
  GtkWidget * delete_button;
  GtkWidget * config_button;
  GtkWidget * up_button;
  GtkWidget * down_button;
  
  bg_transcoder_track_t * tracks;

  bg_plugin_registry_t * plugin_reg;

  GtkTreeViewColumn * col_name;

  bg_transcoder_track_t * selected_track;
  int num_selected;

  gulong select_handler_id;

  bg_cfg_section_t * track_defaults_section;
  bg_gtk_time_display_t * time_total;
  
  };

static GtkWidget * create_pixmap_button(const char * filename)
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
  return button;
  }

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
    w->selected_track = (bg_transcoder_track_t*)0;
    gtk_widget_set_sensitive(w->up_button, 0);
    gtk_widget_set_sensitive(w->down_button, 0);
    gtk_widget_set_sensitive(w->delete_button, 0);
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
    gtk_widget_set_sensitive(w->up_button, 1);
    gtk_widget_set_sensitive(w->down_button, 1);
    gtk_widget_set_sensitive(w->delete_button, 1);
    }
  else if(w->num_selected == 0)
    {
    gtk_widget_set_sensitive(w->config_button, 0);
    w->selected_track = (bg_transcoder_track_t*)0;
    gtk_widget_set_sensitive(w->up_button, 0);
    gtk_widget_set_sensitive(w->down_button, 0);
    gtk_widget_set_sensitive(w->delete_button, 0);
    }
  else
    {
    gtk_widget_set_sensitive(w->config_button, 0);
    w->selected_track = (bg_transcoder_track_t*)0;
    gtk_widget_set_sensitive(w->up_button, 0);
    gtk_widget_set_sensitive(w->down_button, 0);
    gtk_widget_set_sensitive(w->delete_button, 1);
    }
  }

/* Update the entire list */

static void track_list_update(track_list_t * w)
  {
  int i;
  GtkTreeModel * model;
  GtkTreeIter iter;
  bg_transcoder_track_t * track;
  gavl_time_t duration;
  gavl_time_t duration_total;

  char string_buffer[GAVL_TIME_STRING_LEN + 32];
  GtkTreeSelection * selection;
   
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w->treeview));

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
   
  gtk_list_store_clear(GTK_LIST_STORE(model));

  track = w->tracks;
  i = 0;

  g_signal_handler_block(G_OBJECT(selection), w->select_handler_id);

  duration_total = 0;
    
  while(track)
    {
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
                       bg_transcoder_track_get_name(track), -1);
    
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
    
    duration = bg_transcoder_track_get_duration(track);
    
    if(duration_total != GAVL_TIME_UNDEFINED)
      {
      if(duration == GAVL_TIME_UNDEFINED)
        duration_total = GAVL_TIME_UNDEFINED;
      else
        duration_total += duration;
      }
    gavl_time_prettyprint(duration, string_buffer);
    gtk_list_store_set(GTK_LIST_STORE(model),
                       &iter,
                       COLUMN_DURATION,
                       string_buffer, -1);
    
    i++;
    track = track->next;
    }
  g_signal_handler_unblock(G_OBJECT(selection), w->select_handler_id);

  bg_gtk_time_display_update(w->time_total, duration_total);

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
  bg_transcoder_track_t * track, *tmp_track;
  bg_transcoder_track_t * new_tracks =
    (bg_transcoder_track_t*)0;
  bg_transcoder_track_t * end_track =
    (bg_transcoder_track_t*)0;

  track = l->tracks;


  while(track)
    {
    if(track->selected)
      {
      /* Copy non selected tracks */
      tmp_track = track->next;
      bg_transcoder_track_destroy(track);
      track = tmp_track;
      }
    else
      {
      /* Insert into new list */
      if(!new_tracks)
        {
        new_tracks = track;
        end_track = track;
        }
      else
        {
        end_track->next = track;
        end_track = end_track->next;
        }
      track = track->next;
      end_track->next = (bg_transcoder_track_t*)0;
      }
    }
  l->tracks = new_tracks;
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
    plugin_info = bg_plugin_find_by_long_name(l->plugin_reg, plugin);
  else
    plugin_info = (const bg_plugin_info_t*)0;
  while(files[i])
    {
    fprintf(stderr, "Add file %s with %s\n", files[i], plugin);
    
    new_track = bg_transcoder_track_create(files[i], plugin_info,
                                           -1, l->plugin_reg, l->track_defaults_section);

    add_track(l, new_track);
    track_list_update(l);
    i++;
    }
  
  }

static void filesel_close_callback(bg_gtk_filesel_t * f , void * data)
  {
  track_list_t * t;
  t = (track_list_t*)data;
  fprintf(stderr, "Filesel close\n");
  gtk_widget_set_sensitive(t->add_button, 1);
  }
     
static void button_callback(GtkWidget * w, gpointer data)
  {
  track_list_t * t;
  bg_gtk_filesel_t * filesel;
  track_dialog_t * track_dialog;
  
  t = (track_list_t*)data;

  if(w == t->add_button)
    {
    fprintf(stderr, "Add button\n");

    if(!file_plugins)
      {
      file_plugins =
        bg_plugin_registry_get_plugins(t->plugin_reg,
                                       BG_PLUGIN_INPUT,
                                       BG_PLUGIN_FILE);

      }

    filesel = bg_gtk_filesel_create("Add files...",
                                    add_file_callback,
                                    filesel_close_callback,
                                    file_plugins,
                                    t, NULL /* parent */ );
    gtk_widget_set_sensitive(t->add_button, 0);
    bg_gtk_filesel_run(filesel, 0);     
    
    }
  if(w == t->delete_button)
    {
    fprintf(stderr, "Delete button\n");
    delete_selected(t);
    }
  if(w == t->up_button)
    {
    fprintf(stderr, "Up button\n");
    }
  if(w == t->down_button)
    {
    fprintf(stderr, "Down button\n");
    }
  if(w == t->config_button)
    {
    fprintf(stderr, "Config button\n");
    track_dialog = track_dialog_create(t->selected_track);
    track_dialog_run(track_dialog);
    track_dialog_destroy(track_dialog);

    }
  }

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
  
  fprintf(stderr, "Drag received callback\n");

  if(is_urilist(data))
    {
    fprintf(stderr, "Urilist\n");
    new_tracks =
      bg_transcoder_track_create_from_urilist(data->data,
                                              data->length,
                                              l->plugin_reg,
                                              l->track_defaults_section);
    add_track(l, new_tracks);
    }
  else if(is_albumentries(data))
    {
    fprintf(stderr, "Album entries\n");
    
    new_tracks =
      bg_transcoder_track_create_from_albumentries(data->data,
                                                   data->length,
                                                   l->plugin_reg,
                                                   l->track_defaults_section);

    add_track(l, new_tracks);
    
    }

  track_list_update(l);  
  gtk_drag_finish(drag_context,
                  TRUE, /* Success */
                  0, /* Delete */
                  time);
  }

track_list_t * track_list_create(bg_plugin_registry_t * plugin_reg,
                                 bg_cfg_section_t * track_defaults_section)
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

  ret->time_total = bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL, 4);
  
  ret->plugin_reg = plugin_reg;
  
  /* Create buttons */
  ret->add_button = create_pixmap_button("add_16.png");
  ret->delete_button = create_pixmap_button("trash_16.png");
  ret->config_button = create_pixmap_button("config_16.png");
  ret->up_button = create_pixmap_button("up_16.png");
  ret->down_button = create_pixmap_button("down_16.png");

  g_signal_connect(G_OBJECT(ret->down_button), "clicked",
                   G_CALLBACK(button_callback), ret);
  g_signal_connect(G_OBJECT(ret->up_button), "clicked",
                   G_CALLBACK(button_callback), ret);
  g_signal_connect(G_OBJECT(ret->add_button), "clicked",
                   G_CALLBACK(button_callback), ret);
  g_signal_connect(G_OBJECT(ret->delete_button), "clicked",
                   G_CALLBACK(button_callback), ret);
  g_signal_connect(G_OBJECT(ret->config_button), "clicked",
                   G_CALLBACK(button_callback), ret);

  gtk_widget_show(ret->add_button);
  gtk_widget_show(ret->delete_button);
  gtk_widget_show(ret->config_button);
  gtk_widget_show(ret->up_button);
  gtk_widget_show(ret->down_button);

  gtk_widget_set_sensitive(ret->delete_button, 0);
  gtk_widget_set_sensitive(ret->config_button, 0);
  gtk_widget_set_sensitive(ret->up_button, 0);
  gtk_widget_set_sensitive(ret->down_button, 0);
    
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
  gtk_box_pack_start(GTK_BOX(box), ret->add_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->delete_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->config_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->up_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->down_button, FALSE, FALSE, 0);

  gtk_box_pack_end(GTK_BOX(box), bg_gtk_time_display_get_widget(ret->time_total),
                     FALSE, FALSE, 0);
  gtk_widget_show(box);

  gtk_table_attach(GTK_TABLE(ret->widget),
                   box, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(ret->widget);

  /* Load tracks */

  tmp_path = bg_search_file_read("transcoder", "tracks.xml");
  
  if(tmp_path)
    {
    ret->tracks = bg_transcoder_tracks_load(tmp_path, ret->plugin_reg);

    fprintf(stderr, "Loading tracks from %s\n", tmp_path);
    free(tmp_path);

    track_list_update(ret);
    }
  else
    {
    fprintf(stderr, "No tracks found\n");
    }
  
  return ret;
  }

void track_list_destroy(track_list_t * t)
  {
  char * tmp_path;
  tmp_path = bg_search_file_write("transcoder", "tracks.xml");

  if(tmp_path)
    {
    bg_transcoder_tracks_save(t->tracks, tmp_path);
    free(tmp_path);
    }
  free(t);
  }

GtkWidget * track_list_get_widget(track_list_t * t)
  {
  return t->widget;
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
