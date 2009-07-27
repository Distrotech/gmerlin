#include <gtk/gtk.h>
#include <stdlib.h>
#include <config.h>
#include <string.h>

#include <gmerlin/utils.h>
#include <gmerlin/gui_gtk/gtkutils.h>
#include <gmerlin/cfg_registry.h>

#include <track.h>
#include <project.h>

#include <gui_gtk/timerange.h>
#include <gui_gtk/timeruler.h>
#include <gui_gtk/timeline.h>
#include <gui_gtk/trackwidget.h>
#include <gui_gtk/outstreamwidget.h>

struct bg_nle_timeline_s
  {
  //  GtkWidget * paned;
  bg_nle_time_ruler_t * ruler;

  GtkWidget * table;
  
  GtkWidget * panel_window;
  GtkWidget * preview_window;

  GtkWidget * zoom_in;
  GtkWidget * zoom_out;
  GtkWidget * zoom_fit;
  GtkWidget * start_button;
  GtkWidget * end_button;
  GtkWidget * scrollbar;
  
  int num_tracks;
  bg_nle_track_widget_t ** tracks;

  int num_outstreams;
  bg_nle_outstream_widget_t ** outstreams;
  
  GtkWidget * panel_box;
  GtkWidget * preview_box;

  bg_nle_project_t * p;

  void (*motion_callback)(gavl_time_t time, void * data);
  void * motion_callback_data;
  
  bg_nle_timerange_widget_t tr;
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
    bg_nle_timerange_widget_zoom_in(&t->tr);
    }
  else if(w == t->zoom_out)
    {
    bg_nle_timerange_widget_zoom_out(&t->tr);
    }
  else if(w == t->zoom_fit)
    {
    bg_nle_timerange_widget_zoom_fit(&t->tr);
    }
  }

void bg_nle_timeline_set_selection(bg_nle_timeline_t * t,
                                   bg_nle_time_range_t * selection, int64_t cursor_pos)
  {
  int i;
  bg_nle_time_range_copy(&t->tr.selection, selection);
  t->tr.cursor_pos = cursor_pos;
  for(i = 0; i < t->num_tracks; i++)
    {
    bg_nle_track_widget_update_selection(t->tracks[i]);
    }
  for(i = 0; i < t->num_outstreams; i++)
    {
    bg_nle_outstream_widget_update_selection(t->outstreams[i]);
    }
  bg_nle_time_ruler_update_selection(t->ruler);
  }

void bg_nle_timeline_set_in_out(bg_nle_timeline_t * t, bg_nle_time_range_t * in_out)
  {
  bg_nle_time_range_copy(&t->tr.in_out, in_out);
  bg_nle_time_ruler_update_in_out(t->ruler);
  }


void bg_nle_timeline_set_cursor_pos(bg_nle_timeline_t * t,
                                    int64_t cursor_pos)
  {
  t->tr.cursor_pos = cursor_pos;
  bg_nle_time_ruler_update_cursor_pos(t->ruler);
  }

void bg_nle_timeline_set_visible(bg_nle_timeline_t * t,
                                 bg_nle_time_range_t * visible)
  {
  int i;
  bg_nle_time_range_copy(&t->tr.visible, visible);
  for(i = 0; i < t->num_tracks; i++)
    {
    bg_nle_track_widget_update_visible(t->tracks[i]);
    }
  for(i = 0; i < t->num_outstreams; i++)
    {
    bg_nle_outstream_widget_update_visible(t->outstreams[i]);
    }
  bg_nle_time_ruler_update_visible(t->ruler);
  }

void bg_nle_timeline_set_zoom(bg_nle_timeline_t * t,
                                 bg_nle_time_range_t * visible)
  {
  int i;
  bg_nle_time_range_copy(&t->tr.visible, visible);
  for(i = 0; i < t->num_tracks; i++)
    {
    bg_nle_track_widget_update_zoom(t->tracks[i]);
    }
  for(i = 0; i < t->num_outstreams; i++)
    {
    bg_nle_outstream_widget_update_zoom(t->outstreams[i]);
    }
  bg_nle_time_ruler_update_zoom(t->ruler);
  }

static void
selection_changed_callback(bg_nle_time_range_t * selection, int64_t cursor_pos,
                           void * data)
  {
  
  //  int i;
  bg_nle_timeline_t * t = data;
  
  //  fprintf(stderr, "selection changed %ld %ld\n", selection->start, selection->end);
  bg_nle_project_set_selection(t->p, selection, cursor_pos);
  }

static void
in_out_changed_callback(bg_nle_time_range_t * selection,
                        void * data)
  {
  //  int i;
  bg_nle_timeline_t * t = data;
  
  //  fprintf(stderr, "selection changed %ld %ld\n", selection->start, selection->end);
  bg_nle_project_set_in_out(t->p, selection);
  }

static void visibility_changed_callback(bg_nle_time_range_t * visible, void * data)
  {
  //  int i;
  bg_nle_timeline_t * t = data;

  //  fprintf(stderr, "visibility changed %ld %ld\n", visible->start, visible->end);
  bg_nle_project_set_visible(t->p, visible);
  }

static void zoom_changed_callback(bg_nle_time_range_t * visible, void * data)
  {
  //  int i;
  bg_nle_timeline_t * t = data;

  //  fprintf(stderr, "zoom changed %ld %ld\n", visible->start, visible->end);
  bg_nle_project_set_zoom(t->p, visible);
  }

static void timewidget_motion_callback(int64_t time, void * data)
  {
  bg_nle_timeline_t * t = data;
  
  if(t->motion_callback)
    t->motion_callback(time, t->motion_callback_data);
  }

static gboolean motion_notify_callback(GtkWidget * w, GdkEventMotion * evt,
                                       gpointer data)
  {
  bg_nle_timeline_t * t = data;
  
  if(t->motion_callback)
    t->motion_callback(bg_nle_pos_2_time(&t->tr, (int)evt->x),
                       t->motion_callback_data);
  return FALSE;
  }

static gboolean scroll_callback(GtkWidget * w, GdkEventScroll * evt,
                                gpointer data)
  {
  bg_nle_timeline_t * t = data;
  double new_val;
  GtkAdjustment * adj = gtk_viewport_get_vadjustment(GTK_VIEWPORT(t->preview_window));
  
  //  gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(t->scrollbar));
  
  if(evt->direction == GDK_SCROLL_UP)
    {
    new_val = adj->value - 3 * adj->step_increment;
    gtk_adjustment_set_value(adj, new_val);
    return TRUE;
    }
  else if(evt->direction == GDK_SCROLL_DOWN)
    {
    new_val = adj->value + 3 * adj->step_increment;
    if(new_val > adj->upper - adj->page_size)
      new_val = adj->upper - adj->page_size;
    gtk_adjustment_set_value(adj, new_val);
    return TRUE;
    }
  
  return FALSE;
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


static void size_allocate_callback(GtkWidget     *widget,
                                   GtkAllocation *allocation,
                                   gpointer       user_data)
  {
  bg_nle_timeline_t * w = user_data;
  bg_nle_timerange_widget_set_width(&w->tr, allocation->width);
  }


bg_nle_timeline_t * bg_nle_timeline_create(bg_nle_project_t * p)
  {
  GtkWidget * box;
  //  GtkWidget * table;
  GtkSizeGroup * size_group;
  //  GtkWidget * table1;
  GtkStyle *style;
  GtkWidget * frame;
  
  int i;

  bg_nle_timeline_t * ret = calloc(1, sizeof(*ret));  

  //  GtkObject *adj = gtk_adjustment_new( 0, 0, 100, 1, 10, 0 );

  ret->p = p;

  bg_nle_time_range_copy(&ret->tr.selection,
                         &p->selection);
  bg_nle_time_range_copy(&ret->tr.visible,
                         &p->visible);
  bg_nle_time_range_copy(&ret->tr.in_out,
                         &p->in_out);
  ret->tr.cursor_pos = p->cursor_pos;
  
  ret->tr.set_visible = visibility_changed_callback;
  ret->tr.set_zoom = zoom_changed_callback;
  ret->tr.set_selection = selection_changed_callback;
  ret->tr.set_in_out = in_out_changed_callback;
  ret->tr.motion_callback = timewidget_motion_callback;
  ret->tr.callback_data = ret;
  
  ret->ruler = bg_nle_time_ruler_create(&ret->tr);
  
  //  ret->paned = gtk_hpaned_new();
  
  ret->zoom_in = create_pixmap_button(ret, "gmerlerra/time_zoom_in.png",
                                      TRS("Zoom in"));
  ret->zoom_out = create_pixmap_button(ret, "gmerlerra/time_zoom_out.png",
                                       TRS("Zoom out"));
  ret->zoom_fit = create_pixmap_button(ret, "gmerlerra/time_zoom_fit.png",
                                       TRS("Fit to selection"));

  ret->start_button = create_pixmap_button(ret, "first_16.png",
                                           TRS("Goto start"));
  ret->end_button = create_pixmap_button(ret, "last_16.png",
                                         TRS("Goto end"));
  ret->preview_window = gtk_viewport_new(NULL, NULL);

  gtk_widget_set_events(ret->preview_window,
                        GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK);

  g_signal_connect(ret->preview_window,
                   "scroll-event",
                   G_CALLBACK(scroll_callback), ret);
  g_signal_connect(ret->preview_window,
                   "motion-notify-event",
                   G_CALLBACK(motion_notify_callback), ret);
  g_signal_connect(ret->preview_window,
                   "size-allocate", G_CALLBACK(size_allocate_callback),
                   ret);

  
  ret->scrollbar =
    gtk_vscrollbar_new(gtk_viewport_get_vadjustment(GTK_VIEWPORT(ret->preview_window)));
  gtk_widget_show(ret->scrollbar);
  
  //  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->preview_window),
  //                                 GTK_POLICY_NEVER, GTK_POLICY_NEVER);


  ret->panel_window =
    gtk_viewport_new(NULL, gtk_viewport_get_vadjustment(GTK_VIEWPORT(ret->preview_window)));

  gtk_widget_set_events(ret->panel_window,
                        GDK_BUTTON_PRESS_MASK);

  g_signal_connect(ret->panel_window,
                   "scroll-event",
                   G_CALLBACK(scroll_callback), ret);

  
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(ret->preview_window),
                               GTK_SHADOW_NONE);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(ret->panel_window),
                               GTK_SHADOW_NONE);

  gtk_widget_set_size_request(ret->panel_window, -1, 0);
  gtk_widget_set_size_request(ret->preview_window, -1, 0);
  
  //  ret->panel_window =
  //    gtk_scrolled_window_new((GtkAdjustment *)0,
  //                            GTK_ADJUSTMENT(adj));
  
  //  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->panel_window),
  //                                 GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  
  ret->preview_box = gtk_vbox_new(FALSE, 1);
  ret->panel_box = gtk_vbox_new(FALSE, 1);

  gtk_container_add(GTK_CONTAINER(ret->preview_window), ret->preview_box);
  gtk_container_add(GTK_CONTAINER(ret->panel_window), ret->panel_box);
  
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

  
  ret->table = gtk_table_new(2, 3, 0);

  /* Button box */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 0); 

  style = gtk_style_copy(frame->style);
  style->xthickness = 1;
  style->ythickness = 1;
  gtk_widget_set_style(frame, style);
  g_object_unref(style);


  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(frame);
  
  gtk_table_attach(GTK_TABLE(ret->table),
                   frame,
                   0, 1, 0, 1,
                   GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);

  /* Panel window */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 0); 

  style = gtk_style_copy(frame->style);
  style->xthickness = 1;
  style->ythickness = 1;
  gtk_widget_set_style(frame, style);
  g_object_unref(style);
  
  gtk_container_add(GTK_CONTAINER(frame), ret->panel_window);
  gtk_widget_show(frame);
  
  
  gtk_table_attach(GTK_TABLE(ret->table),
                   frame,
                   0, 1, 1, 2,
                   GTK_FILL|GTK_SHRINK, GTK_EXPAND|GTK_FILL, 0, 0);
  
  //  gtk_widget_show(table);

  //  gtk_paned_add1(GTK_PANED(ret->paned), table);
  
  //  table = gtk_table_new(2, 1, 0);

  /* Time ruler */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 0); 

  style = gtk_style_copy(frame->style);
  style->xthickness = 1;
  style->ythickness = 1;
  gtk_widget_set_style(frame, style);
  g_object_unref(style);
  
  gtk_container_add(GTK_CONTAINER(frame), bg_nle_time_ruler_get_widget(ret->ruler));
  gtk_widget_show(frame);
  
  gtk_table_attach(GTK_TABLE(ret->table),
                   frame,
                   1, 2, 0, 1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL|GTK_SHRINK, 0, 0);
  
  /* Preview window */

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_container_set_border_width(GTK_CONTAINER(frame), 0); 

  style = gtk_style_copy(frame->style);
  style->xthickness = 1;
  style->ythickness = 1;
  gtk_widget_set_style(frame, style);
  g_object_unref(style);
  
  gtk_container_add(GTK_CONTAINER(frame), ret->preview_window);
  gtk_widget_show(frame);

  
  gtk_table_attach(GTK_TABLE(ret->table),
                   frame,
                   1, 2, 1, 2,
                   GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0);
  
  gtk_table_attach(GTK_TABLE(ret->table),
                   ret->scrollbar,
                   2, 3, 1, 2,
                   GTK_FILL|GTK_SHRINK, GTK_EXPAND|GTK_FILL, 0, 0);
  
  gtk_widget_show(ret->table);
  //  gtk_paned_add2(GTK_PANED(ret->paned), table);

  //  gtk_widget_show(ret->paned);
  /* Add tracks */

  if(ret->p->num_tracks)
    ret->tracks = calloc(ret->p->num_tracks, sizeof(*ret->tracks));

  for(i = 0; i < ret->p->num_tracks; i++)
    {
    ret->tracks[ret->num_tracks] =
      bg_nle_track_widget_create(ret->p->tracks[i], &ret->tr, ret->ruler);

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
        bg_nle_outstream_widget_create(ret->p->outstreams[i], ret->ruler, &ret->tr);

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
  return t->table;
  }

void bg_nle_timeline_add_track(bg_nle_timeline_t * t,
                               bg_nle_track_t * track, int pos)
  {
  GtkWidget * w;
  t->tracks = realloc(t->tracks,
                        sizeof(*t->tracks) * (t->num_tracks+1));

  if(pos < t->num_tracks)
    memmove(t->tracks + pos + 1, t->tracks + pos, (t->num_tracks - pos) * sizeof(*t->tracks));
  
  t->tracks[pos] = bg_nle_track_widget_create(track, &t->tr, t->ruler);

  w = bg_nle_track_widget_get_panel(t->tracks[pos]);
  
  gtk_box_pack_start(GTK_BOX(t->panel_box), w, FALSE, FALSE, 0);

  gtk_box_reorder_child(GTK_BOX(t->panel_box), w, pos);
  
  w = bg_nle_track_widget_get_preview(t->tracks[pos]);
  
  gtk_box_pack_start(GTK_BOX(t->preview_box),
                     w, FALSE, FALSE, 0);
  gtk_box_reorder_child(GTK_BOX(t->preview_box), w, pos);
  
  t->num_tracks++;
  }

void bg_nle_timeline_add_outstream(bg_nle_timeline_t * t,
                                   bg_nle_outstream_t * outstream, int pos)
  {
  GtkWidget * w;
  t->outstreams = realloc(t->outstreams,
                          sizeof(*t->outstreams) * (t->num_outstreams+1));

  if(pos < t->num_outstreams)
    memmove(t->outstreams + pos + 1, t->outstreams + pos,
            (t->num_outstreams - pos) * sizeof(*t->outstreams));
  
  t->outstreams[pos] =
    bg_nle_outstream_widget_create(outstream, t->ruler, &t->tr);

  w = bg_nle_outstream_widget_get_panel(t->outstreams[pos]);
  gtk_box_pack_start(GTK_BOX(t->panel_box), w, FALSE, FALSE, 0);

  if(pos < t->num_outstreams)
    gtk_box_reorder_child(GTK_BOX(t->panel_box), w, t->num_tracks + pos);

  w = bg_nle_outstream_widget_get_preview(t->outstreams[pos]);
  gtk_box_pack_start(GTK_BOX(t->preview_box), w, FALSE, FALSE, 0);
  if(pos < t->num_outstreams)
    gtk_box_reorder_child(GTK_BOX(t->preview_box), w, t->num_tracks + pos);
  
  t->num_outstreams++;
  }

void bg_nle_timeline_delete_track(bg_nle_timeline_t * t, int index)
  {
  bg_nle_track_widget_destroy(t->tracks[index]);
  if(index < t->num_tracks-1)
    memmove(t->tracks + index, t->tracks + index + 1,
            (t->num_tracks-1 - index) * sizeof(*t->tracks));
  t->num_tracks--;
  }

void bg_nle_timeline_move_track(bg_nle_timeline_t * t, int old_index, int new_index)
  {
  GtkWidget * w;
  bg_nle_track_widget_t * wid;
  w = bg_nle_track_widget_get_preview(t->tracks[old_index]);
  gtk_box_reorder_child(GTK_BOX(t->preview_box), w, new_index);
  w = bg_nle_track_widget_get_panel(t->tracks[old_index]);
  gtk_box_reorder_child(GTK_BOX(t->panel_box), w, new_index);

  /* Reorder widget array */
  wid = t->tracks[old_index];
  if(old_index < t->num_tracks-1)
    memmove(t->tracks + old_index, t->tracks + old_index + 1,
            (t->num_tracks-1 - old_index) * sizeof(*t->tracks));

  if(new_index < t->num_tracks-1)
    memmove(t->tracks + new_index + 1, t->tracks + new_index,
            (t->num_tracks-1 - new_index) * sizeof(*t->tracks));
  t->tracks[new_index] = wid;
  }

void bg_nle_timeline_move_outstream(bg_nle_timeline_t * t, int old_index, int new_index)
  {
  GtkWidget * w;
  bg_nle_outstream_widget_t * wid;

  w = bg_nle_outstream_widget_get_preview(t->outstreams[old_index]);
  gtk_box_reorder_child(GTK_BOX(t->preview_box), w, t->num_tracks + new_index);
  w = bg_nle_outstream_widget_get_panel(t->outstreams[old_index]);
  gtk_box_reorder_child(GTK_BOX(t->panel_box), w, t->num_tracks + new_index);

  /* Reorder widget array */
  wid = t->outstreams[old_index];
  if(old_index < t->num_outstreams-1)
    memmove(t->outstreams + old_index, t->outstreams + old_index + 1,
            (t->num_outstreams-1 - old_index) * sizeof(*t->outstreams));

  if(new_index < t->num_outstreams-1)
    memmove(t->outstreams + new_index + 1, t->outstreams + new_index,
            (t->num_outstreams-1 - new_index) * sizeof(*t->outstreams));
  t->outstreams[new_index] = wid;

  }

void bg_nle_timeline_delete_outstream(bg_nle_timeline_t * t, int index)
  {
  bg_nle_outstream_widget_destroy(t->outstreams[index]);
  if(index < t->num_outstreams-1)
    memmove(t->outstreams + index, t->outstreams + index + 1,
            (t->num_outstreams-1 - index) * sizeof(*t->outstreams));
  t->num_outstreams--;
  }


bg_nle_time_ruler_t * bg_nle_timeline_get_ruler(bg_nle_timeline_t * t)
  {
  return t->ruler;
  }

void bg_nle_timeline_set_track_flags(bg_nle_timeline_t * t,
                                     bg_nle_track_t * track, int flags)
  {
  int index = bg_nle_project_track_index(t->p, track);
  bg_nle_track_widget_set_flags(t->tracks[index], flags);
  }

void bg_nle_timeline_update_track_parameters(bg_nle_timeline_t * t, int index,
                                             bg_cfg_section_t * s)
  {
  bg_nle_track_widget_update_parameters(t->tracks[index], s);
  
  }

void bg_nle_timeline_update_outstream_parameters(bg_nle_timeline_t * t, int index,
                                                 bg_cfg_section_t * s)
  {
  bg_nle_outstream_widget_update_parameters(t->outstreams[index], s);
  }

void bg_nle_timeline_set_outstream_flags(bg_nle_timeline_t * t,
                                         bg_nle_outstream_t * outstream, int flags)
  {
  int index = bg_nle_project_outstream_index(t->p, outstream);
  bg_nle_outstream_widget_set_flags(t->outstreams[index], flags);
  }

void bg_nle_timeline_outstreams_make_current(bg_nle_timeline_t * t)
  {
  int i;
  for(i = 0; i < t->num_outstreams; i++)
    {
    bg_nle_outstream_widget_update_current(t->outstreams[i]);
    }
  }
