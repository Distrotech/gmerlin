#include <stdlib.h>
#include <gtk/gtk.h>

#include <pluginregistry.h>

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

track_list_t * track_list_create(bg_plugin_registry_t * plugin_reg)
  {
  track_list_t * ret;

  GtkTreeViewColumn * col;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeSelection * selection;

  ret = calloc(1, sizeof(*ret));
  
  store = gtk_list_store_new(NUM_COLUMNS,
                             G_TYPE_STRING,   // Index
                             G_TYPE_STRING,   // Name
                             G_TYPE_STRING,   // Has audio
                             G_TYPE_STRING,   // Has video
                             G_TYPE_STRING);  // Duration
  ret->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_widget_set_size_request(ret->treeview, 200, 100);
  /* Create columns */

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
  
  ret->widget =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(ret->treeview)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ret->treeview)));
  
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->widget),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(ret->widget), ret->treeview);
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
