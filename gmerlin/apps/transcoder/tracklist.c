
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <pluginregistry.h>
#include <utils.h>

#include "track.h"
#include "tracklist.h"

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
  
  track_t * tracks;
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

static void button_callback(GtkWidget * w, gpointer data)
  {
  track_list_t * t;
  t = (track_list_t*)data;

  if(w == t->add_button)
    {
    fprintf(stderr, "Add button\n");
    }
  if(w == t->delete_button)
    {
    fprintf(stderr, "Delete button\n");
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
    }
  }

track_list_t * track_list_create(bg_plugin_registry_t * plugin_reg)
  {
  GtkWidget * scrolled;
  GtkWidget * box;
  track_list_t * ret;

  GtkTreeViewColumn * col;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeSelection * selection;

  ret = calloc(1, sizeof(*ret));

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

  /* Create list view */
  
  store = gtk_list_store_new(NUM_COLUMNS,
                             G_TYPE_STRING,   // Index
                             G_TYPE_STRING,   // Name
                             G_TYPE_STRING,   // Has audio
                             G_TYPE_STRING,   // Has video
                             G_TYPE_STRING);  // Duration
  ret->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_size_request(ret->treeview, 200, 100);
  
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
                                                                                
                                                                                
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
                                                                                
  gtk_tree_view_column_add_attribute(col, renderer,
                                     "text", COLUMN_NAME);
                                                                                
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_FIXED);
                                                                                
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);
  
  /* Audio */
                                                                                
  renderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
                                                                                
  col = gtk_tree_view_column_new ();
  
  gtk_tree_view_column_set_title(col, "A");
                                                                                
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
                                                                                
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "text", COLUMN_AUDIO);
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);

  /* Video */
                                                                                
  renderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
                                                                                
  col = gtk_tree_view_column_new ();
  
  gtk_tree_view_column_set_title(col, "V");
                                                                                
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
                                                                                
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "text", COLUMN_VIDEO);
  gtk_tree_view_column_set_sizing(col,
                                  GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview),
                               col);

  
  /* Duration */
                                                                                
  renderer = gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
                                                                                
  col = gtk_tree_view_column_new ();
                                                                                  
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
    
  ret->widget = gtk_table_new(1, 2, 0);
  gtk_table_attach_defaults(GTK_TABLE(ret->widget), scrolled, 0, 1, 0, 1);

  box = gtk_vbox_new(0, 0);
  gtk_box_pack_start_defaults(GTK_BOX(box), ret->add_button);
  gtk_box_pack_start_defaults(GTK_BOX(box), ret->delete_button);
  gtk_box_pack_start_defaults(GTK_BOX(box), ret->config_button);
  gtk_box_pack_start_defaults(GTK_BOX(box), ret->up_button);
  gtk_box_pack_start_defaults(GTK_BOX(box), ret->down_button);
  gtk_widget_show(box);

  gtk_table_attach_defaults(GTK_TABLE(ret->widget), box, 1, 2, 0, 1);
  
  gtk_widget_show(ret->widget);
  
  return ret;
  }

void track_list_destroy(track_list_t * t)
  {
  free(t);
  }

GtkWidget * track_list_get_widget(track_list_t * t)
  {
  return t->widget;
  }
