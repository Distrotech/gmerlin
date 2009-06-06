#include <gtk/gtk.h>
#include <stdlib.h>
#include <config.h>

#include <gmerlin/utils.h>
#include <gmerlin/gui_gtk/gtkutils.h>
#include <gmerlin/cfg_registry.h>

#include <track.h>
#include <project.h>

#include <gui_gtk/timeruler.h>
#include <gui_gtk/timeline.h>
#include <gui_gtk/trackwidget.h>

struct bg_nle_timeline_s
  {
  GtkWidget * table;
  bg_nle_time_ruler_t * ruler;
  
  GtkWidget * panel_window;
  GtkWidget * preview_window;

  GtkWidget * zoom_in;
  GtkWidget * zoom_out;
  GtkWidget * zoom_fit;
  GtkWidget * start_button;
  GtkWidget * end_button;
  
  int num_tracks;
  bg_nle_track_widget_t ** tracks;
  
  GtkWidget * panel_box;
  GtkWidget * preview_box;

  bg_nle_project_t * p;
  
  };

static void button_callback(GtkWidget * w, gpointer  data)
  {
  bg_nle_timeline_t * t = data;

  if(w == t->zoom_in)
    {
    fprintf(stderr, "zoom in\n");
    bg_nle_time_ruler_zoom_in(t->ruler);
    }
  else if(w == t->zoom_out)
    {
    fprintf(stderr, "zoom out\n");
    bg_nle_time_ruler_zoom_out(t->ruler);
    }
  else if(w == t->zoom_fit)
    {
    fprintf(stderr, "zoom fit\n");
    
    }
  }

static void selection_changed_callback(void * data)
  {
  int i;
  bg_nle_timeline_t * t = data;
  
  for(i = 0; i < t->num_tracks; i++)
    {
    bg_nle_track_widget_redraw(t->tracks[i]);
    }
        
  }

static GtkWidget * create_pixmap_button(bg_nle_timeline_t * w,
                                        const char * filename,
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

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(button_callback), w);
  
  gtk_widget_show(button);

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);
  
  return button;
  }

bg_nle_timeline_t * bg_nle_timeline_create(bg_nle_project_t * p)
  {
  GtkWidget * box;
  int i;

  bg_nle_timeline_t * ret = calloc(1, sizeof(*ret));  
  
  ret->p = p;
  
  ret->ruler = bg_nle_time_ruler_create(p);

  bg_nle_time_ruler_set_selection_callback(ret->ruler,
                                           selection_changed_callback,
                                           ret);
    
  ret->table = gtk_table_new(2, 2, 0);

  ret->zoom_in = create_pixmap_button(ret, "time_zoom_in.png",
                                      TRS("Zoom in"));
  ret->zoom_out = create_pixmap_button(ret, "time_zoom_out.png",
                                       TRS("Zoom out"));
  ret->zoom_fit = create_pixmap_button(ret, "time_zoom_fit.png",
                                       TRS("Fit to selection"));

  ret->start_button = create_pixmap_button(ret, "first_16.png",
                                           TRS("Goto start"));
  ret->end_button = create_pixmap_button(ret, "last_16.png",
                                         TRS("Goto end"));
  
  ret->preview_window = gtk_scrolled_window_new((GtkAdjustment *)0,
                                                (GtkAdjustment *)0);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->preview_window),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  ret->panel_window =
    gtk_scrolled_window_new((GtkAdjustment *)0,
                            gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(ret->preview_window)));
  
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->panel_window),
                                 GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  
  ret->preview_box = gtk_vbox_new(FALSE, 2);
  ret->panel_box = gtk_vbox_new(FALSE, 2);
  
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ret->preview_window),
                                        ret->preview_box);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ret->panel_window),
                                        ret->panel_box);

  gtk_widget_show(ret->preview_box);
  gtk_widget_show(ret->panel_box);

  gtk_widget_show(ret->preview_window);
  gtk_widget_show(ret->panel_window);
    
  /* Pack */

  box = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), 
                     ret->zoom_in, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), 
                     ret->zoom_out, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), 
                     ret->zoom_fit, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(box), 
                     ret->start_button, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), 
                     ret->end_button, TRUE, TRUE, 0);

  gtk_widget_show(box);

  gtk_table_attach(GTK_TABLE(ret->table),
                   box,
                   0, 1, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_table_attach(GTK_TABLE(ret->table),
                   bg_nle_time_ruler_get_widget(ret->ruler),
                   1, 2, 0, 1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_table_attach(GTK_TABLE(ret->table),
                   ret->panel_window,
                   0, 1, 1, 2,
                   GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(ret->table),
                   ret->preview_window,
                   1, 2, 1, 2,
                   GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
  
  gtk_widget_show(ret->table);

  /* Add tracks */

  if(ret->p->num_tracks)
    ret->tracks = calloc(ret->p->num_tracks, sizeof(*ret->tracks));

  for(i = 0; i < ret->p->num_tracks; i++)
    {
    ret->tracks[ret->num_tracks] =
      bg_nle_track_widget_create(ret->p->tracks[i], ret->ruler);

    gtk_box_pack_start(GTK_BOX(ret->panel_box),
                       bg_nle_track_widget_get_panel(ret->tracks[ret->num_tracks]),
                       FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(ret->preview_box),
                       bg_nle_track_widget_get_preview(ret->tracks[ret->num_tracks]),
                       FALSE, FALSE, 0);
    ret->num_tracks++;
    }
  
  return ret;
  }

void bg_nle_timeline_destroy(bg_nle_timeline_t * t)
  {
  int i;
  for(i = 0; i < t->num_tracks; i++)
    {
    bg_nle_track_widget_destroy(t->tracks[i]);
    }
  if(t->tracks)
    free(t->tracks);
  
  free(t);
  }

GtkWidget * bg_nle_timeline_get_widget(bg_nle_timeline_t * t)
  {
  return t->table;
  }

void bg_nle_timeline_add_track(bg_nle_timeline_t * t,
                               bg_nle_track_t * track)
  {
  t->tracks = realloc(t->tracks,
                        sizeof(*t->tracks) * (t->num_tracks+1));
  t->tracks[t->num_tracks] = bg_nle_track_widget_create(track, t->ruler);

  gtk_box_pack_start(GTK_BOX(t->panel_box),
                     bg_nle_track_widget_get_panel(t->tracks[t->num_tracks]),
                     FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(t->preview_box),
                     bg_nle_track_widget_get_preview(t->tracks[t->num_tracks]),
                     FALSE, FALSE, 0);
  
  t->num_tracks++;
  }
