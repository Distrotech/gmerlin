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
#include <gui_gtk/outstreamwidget.h>

struct bg_nle_timeline_s
  {
  GtkWidget * paned;
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

  int num_outstreams;
  bg_nle_outstream_widget_t ** outstreams;
  
  GtkWidget * panel_box;
  GtkWidget * preview_box;

  bg_nle_project_t * p;

  void (*motion_callback)(gavl_time_t time, void * data);
  void * motion_callback_data;
  
  };

void bg_nle_timeline_set_motion_callback(bg_nle_timeline_t * t,
                                         void (*callback)(gavl_time_t time, void * data),
                                         void * data)
  {
  t->motion_callback = callback;
  t->motion_callback_data = data;
  }


static void button_callback(GtkWidget * w, gpointer  data)
  {
  bg_nle_timeline_t * t = data;

  if(w == t->zoom_in)
    {
    bg_nle_time_ruler_zoom_in(t->ruler);
    }
  else if(w == t->zoom_out)
    {
    bg_nle_time_ruler_zoom_out(t->ruler);
    }
  else if(w == t->zoom_fit)
    {
    bg_nle_time_ruler_zoom_fit(t->ruler);
    }
  }

static void selection_changed_callback(void * data, int64_t start, int64_t end)
  {
  int i;
  bg_nle_timeline_t * t = data;
  
  for(i = 0; i < t->num_tracks; i++)
    {
    bg_nle_track_widget_redraw(t->tracks[i]);
    }
  for(i = 0; i < t->num_outstreams; i++)
    {
    bg_nle_outstream_widget_redraw(t->outstreams[i]);
    }
        
  }

static void visibility_changed_callback(void * data, int64_t start, int64_t end)
  {
  int i;
  bg_nle_timeline_t * t = data;
  
  for(i = 0; i < t->num_tracks; i++)
    {
    bg_nle_track_widget_redraw(t->tracks[i]);
    }
  for(i = 0; i < t->num_outstreams; i++)
    {
    bg_nle_outstream_widget_redraw(t->outstreams[i]);
    }
        
  }

static void motion_notify_callback(GtkWidget * w, GdkEventMotion * evt,
                                   gpointer data)
  {
  bg_nle_timeline_t * t = data;
  
  if(t->motion_callback)
    t->motion_callback(bg_nle_time_ruler_pos_2_time(t->ruler,
                                                    (int)evt->x), t->motion_callback_data);
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
  GtkWidget * table;
  GtkWidget * eventbox;
  GtkSizeGroup * size_group;
  //  GtkWidget * table1;
  
  int i;

  bg_nle_timeline_t * ret = calloc(1, sizeof(*ret));  
  
  ret->p = p;
  
  ret->ruler = bg_nle_time_ruler_create();

  bg_nle_time_ruler_set_selection(ret->ruler,
                                  p->start_selection,
                                  p->end_selection);
  bg_nle_time_ruler_set_visible(ret->ruler,
                                p->start_visible,
                                p->end_visible, 1);
  
  bg_nle_time_ruler_set_selection_callback(ret->ruler,
                                           selection_changed_callback,
                                           ret);
  bg_nle_time_ruler_set_visibility_callback(ret->ruler,
                                            visibility_changed_callback,
                                            ret);

  ret->paned = gtk_hpaned_new();
  
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

  eventbox = gtk_event_box_new();
  
  gtk_widget_set_events(eventbox,
                        GDK_POINTER_MOTION_MASK);

  g_signal_connect(eventbox,
                   "motion-notify-event",
                   G_CALLBACK(motion_notify_callback), ret);

  gtk_container_add(GTK_CONTAINER(eventbox), ret->preview_box);
  gtk_widget_show(eventbox);
  
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ret->preview_window),
                                        eventbox);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ret->panel_window),
                                        ret->panel_box);

  gtk_widget_show(ret->preview_box);
  gtk_widget_show(ret->panel_box);

  gtk_widget_show(ret->preview_window);
  gtk_widget_show(ret->panel_window);
    
  /* Pack */
  
  box = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), 
                     ret->zoom_in, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), 
                     ret->zoom_out, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), 
                     ret->zoom_fit, FALSE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(box), 
                     ret->start_button, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), 
                     ret->end_button, FALSE, TRUE, 0);

  gtk_widget_show(box);

  size_group = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget(size_group, box);
  gtk_size_group_add_widget(size_group, bg_nle_time_ruler_get_widget(ret->ruler));
  g_object_unref(size_group);

  
  table = gtk_table_new(2, 1, 0);
  
  gtk_table_attach(GTK_TABLE(table),
                   box,
                   0, 1, 0, 1,
                   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   ret->panel_window,
                   0, 1, 1, 2,
                   GTK_FILL|GTK_EXPAND, GTK_EXPAND|GTK_FILL, 0, 0);

  gtk_widget_show(table);

  gtk_paned_add1(GTK_PANED(ret->paned), table);
  
  table = gtk_table_new(2, 1, 0);
    
  gtk_table_attach(GTK_TABLE(table),
                   bg_nle_time_ruler_get_widget(ret->ruler),
                   0, 1, 0, 1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_table_attach(GTK_TABLE(table),
                   ret->preview_window,
                   0, 1, 1, 2,
                   GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
  
  gtk_widget_show(table);
  gtk_paned_add2(GTK_PANED(ret->paned), table);

  gtk_widget_show(ret->paned);
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

  /* Add outstreams */

  if(ret->p->num_outstreams)
    {
    ret->outstreams = calloc(ret->p->num_outstreams, sizeof(*ret->outstreams));

    for(i = 0; i < ret->p->num_outstreams; i++)
      {
      ret->outstreams[ret->num_outstreams] =
        bg_nle_outstream_widget_create(ret->p->outstreams[i], ret->ruler);

      gtk_box_pack_start(GTK_BOX(ret->panel_box),
                         bg_nle_outstream_widget_get_panel(ret->outstreams[ret->num_outstreams]),
                         FALSE, FALSE, 0);
    
      gtk_box_pack_start(GTK_BOX(ret->preview_box),
                         bg_nle_outstream_widget_get_preview(ret->outstreams[ret->num_outstreams]),
                         FALSE, FALSE, 0);
      ret->num_outstreams++;
      }
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
  return t->paned;
  }

void bg_nle_timeline_add_track(bg_nle_timeline_t * t,
                               bg_nle_track_t * track)
  {
  GtkWidget * w;
  t->tracks = realloc(t->tracks,
                        sizeof(*t->tracks) * (t->num_tracks+1));
  t->tracks[t->num_tracks] = bg_nle_track_widget_create(track, t->ruler);

  w = bg_nle_track_widget_get_panel(t->tracks[t->num_tracks]);
  
  gtk_box_pack_start(GTK_BOX(t->panel_box), w, FALSE, FALSE, 0);

  gtk_box_reorder_child(GTK_BOX(t->panel_box), w, t->num_tracks);

  w = bg_nle_track_widget_get_preview(t->tracks[t->num_tracks]);
  
  gtk_box_pack_start(GTK_BOX(t->preview_box),
                     w, FALSE, FALSE, 0);
  gtk_box_reorder_child(GTK_BOX(t->preview_box), w, t->num_tracks);
  
  t->num_tracks++;
  }

void bg_nle_timeline_add_outstream(bg_nle_timeline_t * t,
                                   bg_nle_outstream_t * outstream)
  {
  t->outstreams = realloc(t->outstreams,
                          sizeof(*t->outstreams) * (t->num_outstreams+1));
  t->outstreams[t->num_outstreams] = bg_nle_outstream_widget_create(outstream, t->ruler);

  gtk_box_pack_start(GTK_BOX(t->panel_box),
                     bg_nle_outstream_widget_get_panel(t->outstreams[t->num_outstreams]),
                     FALSE, FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(t->preview_box),
                     bg_nle_outstream_widget_get_preview(t->outstreams[t->num_outstreams]),
                     FALSE, FALSE, 0);

  
  
  t->num_outstreams++;
  }


bg_nle_time_ruler_t * bg_nle_timeline_get_ruler(bg_nle_timeline_t * t)
  {
  return t->ruler;
  }
