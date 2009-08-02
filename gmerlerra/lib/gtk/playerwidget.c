#include <gtk/gtk.h>

#include <config.h>
#include <gmerlin/utils.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/player.h>
#include <gmerlin/gui_gtk/gtkutils.h>
#include <gmerlin/gui_gtk/audio.h>
#include <gmerlin/gui_gtk/display.h>


#include <types.h>

#include <gui_gtk/timerange.h>
#include <gui_gtk/timeruler.h>
#include <gui_gtk/playerwidget.h>

#define DELAY_TIME 10 /* 10 milliseconds */

struct bg_nle_player_widget_s
  {
  bg_player_t * player;
  bg_msg_queue_t * queue;
  
  GtkWidget * play_button;
  GtkWidget * goto_next_button;
  GtkWidget * goto_prev_button;
  GtkWidget * goto_start_button;
  GtkWidget * goto_end_button;
  GtkWidget * frame_forward_button;
  GtkWidget * frame_backward_button;
  GtkWidget * zoom_in;
  GtkWidget * zoom_out;
  GtkWidget * zoom_fit;

  GtkWidget * in_button;
  GtkWidget * out_button;
  
  GtkWidget * socket;
  
  GtkWidget * box;
  
  guint play_id;
  
  bg_plugin_registry_t * plugin_reg;
  
  bg_gtk_vumeter_t * vumeter;
  bg_gtk_time_display_t * display;

  bg_nle_time_ruler_t * ruler;
  bg_nle_time_ruler_t * ruler_priv;

  bg_nle_timerange_widget_t tr;

  int player_state;
  
  int time_changed;
  };

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_nle_player_widget_t * p = data;
  if(w == p->play_button)
    {
    bg_player_pause(p->player);
    }
  else if(w == p->goto_start_button)
    {
    if(p->player_state != BG_PLAYER_STATE_PAUSED)
      bg_player_pause(p->player);
    bg_player_seek(p->player, 0);
    }
  else if(w == p->zoom_in)
    {
    bg_nle_timerange_widget_zoom_in(bg_nle_time_ruler_get_tr(p->ruler));
    }
  else if(w == p->zoom_out)
    {
    bg_nle_timerange_widget_zoom_out(bg_nle_time_ruler_get_tr(p->ruler));
    }
  else if(w == p->zoom_fit)
    {
    bg_nle_timerange_widget_zoom_fit(bg_nle_time_ruler_get_tr(p->ruler));
    }
  else if(w == p->in_button)
    {
    bg_nle_timerange_widget_toggle_in(bg_nle_time_ruler_get_tr(p->ruler));
    }
  else if(w == p->out_button)
    {
    bg_nle_timerange_widget_toggle_out(bg_nle_time_ruler_get_tr(p->ruler));
    }
  }

static GtkWidget * create_pixmap_button(bg_nle_player_widget_t * w,
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

static GtkWidget * create_pixmap_toggle_button(bg_nle_player_widget_t * w,
                                               const char * filename,
                                               const char * tooltip, guint * id)
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
  button = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);

  *id = g_signal_connect(G_OBJECT(button), "toggled",
                         G_CALLBACK(button_callback), w);
  
  gtk_widget_show(button);

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);
  
  return button;
  }


static float display_fg[] = { 0.0, 1.0, 0.0 };
static float display_bg[] = { 0.0, 0.0, 0.0 };

static void load_output_plugins(bg_nle_player_widget_t * p)
  {
  char * display_string;
  bg_plugin_handle_t * handle;
  const bg_plugin_info_t * info;
  GdkDisplay * dpy;

  /* Video */
  dpy = gdk_display_get_default();
  
  info = bg_plugin_registry_get_default(p->plugin_reg,
                                        BG_PLUGIN_OUTPUT_VIDEO);

  display_string =
    bg_sprintf("%s:%08lx:", gdk_display_get_name(dpy),
               (long unsigned int)gtk_socket_get_id(GTK_SOCKET(p->socket)));
  
  handle = bg_ov_plugin_load(p->plugin_reg, info, display_string);
  free(display_string);
  
  bg_player_set_ov_plugin(p->player, handle);

  /* Audio */
  info = bg_plugin_registry_get_default(p->plugin_reg,
                                        BG_PLUGIN_OUTPUT_AUDIO);
  handle = bg_plugin_load(p->plugin_reg, info);
  bg_player_set_oa_plugin(p->player, handle);
  
  }

static void socket_realize(GtkWidget * w, gpointer data)
  {
  bg_nle_player_widget_t * p = data;
  
  //  fprintf(stderr, "Socket realize\n");
  load_output_plugins(p);
  }

static void size_allocate_callback(GtkWidget     *widget,
                                   GtkAllocation *allocation,
                                   gpointer       user_data)
  {
  bg_nle_player_widget_t * w = user_data;
  bg_nle_timerange_widget_set_width(&w->tr, allocation->width);
  }

static gboolean timeout_func(void * data)
  {
  bg_msg_t * msg;
  bg_nle_player_widget_t * w = data;
  double peaks[2];
  
  while((msg = bg_msg_queue_try_lock_read(w->queue)))
    {
    switch(bg_msg_get_id(msg))
      {
      case BG_PLAYER_MSG_TIME_CHANGED:
        {
        gavl_time_t t = bg_msg_get_arg_time(msg, 0);
        bg_gtk_time_display_update(w->display, t, BG_GTK_DISPLAY_MODE_HMSMS);
        bg_nle_timerange_widget_set_cursor_pos(bg_nle_time_ruler_get_tr(w->ruler), t);
        }
        break;
      case BG_PLAYER_MSG_AUDIO_PEAK:
        {
        int samples;
        samples = bg_msg_get_arg_int(msg, 0);
        peaks[0] = bg_msg_get_arg_float(msg, 1);
        peaks[1] = bg_msg_get_arg_float(msg, 2);
        bg_gtk_vumeter_update_peak(w->vumeter, peaks, samples);
        //  fprintf(stderr, "Got peak %d %f %f\n", samples, peaks[0], peaks[1]);
        }
        break;
      case BG_PLAYER_MSG_STATE_CHANGED:
        w->player_state = bg_msg_get_arg_int(msg, 0);
        fprintf(stderr, "State changed: %d\n", w->player_state);

        if(w->player_state == BG_PLAYER_STATE_PAUSED)
          {
          if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->play_button)))
            {
            g_signal_handler_block(w->play_button, w->play_id);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->play_button), 0);
            g_signal_handler_unblock(w->play_button, w->play_id);
            }
          }
        break;
      }
    bg_msg_queue_unlock_read(w->queue);
    }

  if(w->player_state == BG_PLAYER_STATE_PAUSED)
    {
    peaks[0] = 0.0;
    peaks[1] = 0.0;
    bg_gtk_vumeter_update_peak(w->vumeter, peaks, 480);
    }
  
  return TRUE;
  }


static void visibility_changed_callback(bg_nle_time_range_t * visible, void * data)
  {
  //  int i;
  bg_nle_player_widget_t * w = data;
  
  // fprintf(stderr, "visibility changed %ld %ld\n", visible->start, visible->end);
  // bg_nle_project_set_visible(w->p, visible);
  bg_nle_time_range_copy(&w->tr.visible, visible);
  bg_nle_time_ruler_update_visible(w->ruler);
  }

static void zoom_changed_callback(bg_nle_time_range_t * visible, void * data)
  {
  //  int i;
  bg_nle_player_widget_t * w = data;
  
  //  fprintf(stderr, "zoom changed %ld %ld\n", visible->start, visible->end);
  //  bg_nle_project_set_zoom(w->p, visible);
  bg_nle_time_range_copy(&w->tr.visible, visible);
  bg_nle_time_ruler_update_zoom(w->ruler);
  }

static void selection_changed_callback(bg_nle_time_range_t * selection, int64_t cursor_pos, void * data)
  {
  bg_nle_player_widget_t * w = data;
  
  //  fprintf(stderr, "selection changed %ld %ld\n", selection->start, selection->end);
  //  bg_nle_project_set_selection(t->p, selection);
  bg_nle_time_range_copy(&w->tr.selection, selection);
  w->tr.cursor_pos = cursor_pos;
  bg_nle_time_ruler_update_selection(w->ruler);

  if(w->player_state != BG_PLAYER_STATE_PAUSED)
    bg_player_pause(w->player);
  
  bg_player_seek(w->player, cursor_pos);
  
  }

static void in_out_changed_callback(bg_nle_time_range_t * in_out, void * data)
  {
  bg_nle_player_widget_t * w = data;
  
  //  fprintf(stderr, "selection changed %ld %ld\n", selection->start, selection->end);
  //  bg_nle_project_set_selection(t->p, selection);
  bg_nle_time_range_copy(&w->tr.in_out, in_out);

  bg_nle_time_ruler_update_in_out(w->ruler);
  }

static void cursor_changed_callback(int64_t cursor_pos, void * data)
  {
  bg_nle_player_widget_t * w = data;
  w->tr.cursor_pos = cursor_pos;
  bg_nle_time_ruler_update_cursor_pos(w->ruler);
  }

#if 0
static void timewidget_motion_callback(int64_t time, void * data)
  {
  bg_nle_player_widget_t * w = data;
  
  //  if(t->motion_callback)
  //    t->motion_callback(time, t->motion_callback_data);
  
  }
#endif

bg_nle_player_widget_t *
bg_nle_player_widget_create(bg_plugin_registry_t * plugin_reg,
                            bg_nle_time_ruler_t * ruler)
  {
  GtkWidget * box;
  GtkWidget * sep;
  bg_nle_player_widget_t * ret;
  bg_nle_time_range_t r;
  bg_parameter_value_t val;
  r.start = 0;
  r.end = 10 * GAVL_TIME_SCALE;
  
  ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = plugin_reg;

  ret->tr.visible.start = 0;
  ret->tr.visible.end = GAVL_TIME_SCALE * 10;

  ret->tr.in_out.start = -1;
  ret->tr.in_out.end   = -1;

  
  ret->tr.set_visible = visibility_changed_callback;
  ret->tr.set_zoom = zoom_changed_callback;
  ret->tr.set_selection = selection_changed_callback;
  ret->tr.set_in_out = in_out_changed_callback;
  ret->tr.set_cursor_pos = cursor_changed_callback;

  ret->tr.callback_data = ret;

  
  if(!ruler)
    {
    ret->ruler_priv = bg_nle_time_ruler_create(&ret->tr);
    
    bg_nle_time_ruler_update_visible(ret->ruler_priv);
    
    ret->ruler = ret->ruler_priv;
    }
  else
    ret->ruler = ruler;
  
  /* Create buttons */  
  ret->play_button = create_pixmap_toggle_button(ret, "gmerlerra/play.png", TRS("Play"), &ret->play_id);
  
  ret->goto_next_button = create_pixmap_button(ret, "gmerlerra/goto_next.png",
                                               TRS("Goto next label"));
  
  ret->goto_prev_button = create_pixmap_button(ret, "gmerlerra/goto_prev.png",
                                               TRS("Goto previous label"));
  
  ret->goto_start_button = create_pixmap_button(ret, "gmerlerra/goto_start.png",
                                                TRS("Goto start"));
  ret->goto_end_button = create_pixmap_button(ret, "gmerlerra/goto_end.png",
                                                TRS("Goto end"));

  ret->frame_forward_button = create_pixmap_button(ret, "gmerlerra/frame_forward.png",
                                                   TRS("1 frame forward"));
  ret->frame_backward_button = create_pixmap_button(ret, "gmerlerra/frame_backward.png",
                                                    TRS("1 frame backward"));
  
  ret->zoom_in = create_pixmap_button(ret, "gmerlerra/time_zoom_in.png",
                                      TRS("Zoom in"));
  ret->zoom_out = create_pixmap_button(ret, "gmerlerra/time_zoom_out.png",
                                       TRS("Zoom out"));
  ret->zoom_fit = create_pixmap_button(ret, "gmerlerra/time_zoom_fit.png",
                                       TRS("Fit to selection"));

  ret->in_button = create_pixmap_button(ret, "gmerlerra/in.png",
                                        TRS("Toggle in point at cursor position"));
  ret->out_button = create_pixmap_button(ret, "gmerlerra/out.png",
                                         TRS("Toggle out point at cursor position"));
  /* Create socket */
  
  ret->socket = gtk_socket_new();

  g_signal_connect(G_OBJECT(ret->socket), "realize",
                   G_CALLBACK(socket_realize), ret);
  
  gtk_widget_show(ret->socket);
  
  ret->vumeter = bg_gtk_vumeter_create(2, 1);
  ret->display = bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL, 4,
                                            BG_GTK_DISPLAY_MODE_TIMECODE |
                                            BG_GTK_DISPLAY_MODE_HMSMS);
  bg_gtk_time_display_set_colors(ret->display, display_fg, display_bg );
  //  bg_gtk_time_display_update(ret->display, 0, BG_GTK_DISPLAY_MODE_TIMECODE);
  bg_gtk_time_display_update(ret->display, 0, BG_GTK_DISPLAY_MODE_HMSMS);
  /* Pack */
  
  ret->box = gtk_vbox_new(FALSE, 0);

  box = gtk_hbox_new(FALSE, 0);

  gtk_box_pack_start(GTK_BOX(box), ret->socket, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), bg_gtk_vumeter_get_widget(ret->vumeter),
                     FALSE, FALSE, 0);
  
  gtk_widget_show(box);
  gtk_box_pack_start(GTK_BOX(ret->box), box, TRUE, TRUE, 0);

  if(ret->ruler_priv)
    {
    g_signal_connect(ret->box, "size-allocate", G_CALLBACK(size_allocate_callback), ret);
    gtk_box_pack_start(GTK_BOX(ret->box), bg_nle_time_ruler_get_widget(ret->ruler_priv),
                       FALSE, FALSE, 0);
    }
  
  box = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->goto_start_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->goto_prev_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->frame_backward_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->play_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->frame_forward_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->goto_next_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->goto_end_button, FALSE, FALSE, 0);

  sep = gtk_vseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_start(GTK_BOX(box), sep,  FALSE, TRUE, 0);
  
  gtk_box_pack_start(GTK_BOX(box), ret->zoom_in,  FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->zoom_out, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->zoom_fit, FALSE, TRUE, 0);

  sep = gtk_vseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_start(GTK_BOX(box), sep,  FALSE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(box), ret->in_button,  FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->out_button,  FALSE, TRUE, 0);
  
  gtk_box_pack_end(GTK_BOX(box),
                   bg_gtk_time_display_get_widget(ret->display),
                   FALSE, FALSE, 0);

  gtk_widget_show(box);

  gtk_box_pack_start(GTK_BOX(ret->box), box, FALSE, FALSE, 0);

  gtk_widget_show(ret->box);

  /* Setup player */

  ret->queue = bg_msg_queue_create();
  
  ret->player = bg_player_create(ret->plugin_reg);
  
  bg_player_add_message_queue(ret->player, ret->queue);

  val.val_str = "frame";
  bg_player_set_parameter(ret->player, "time_update", &val);

  val.val_str = "pause";
  bg_player_set_parameter(ret->player, "finish_mode", &val);
  
  val.val_i = 1;
  bg_player_set_parameter(ret->player, "report_peak", &val);
  bg_player_set_parameter(ret->player, NULL, NULL);
      
  bg_player_run(ret->player);
  bg_player_set_volume(ret->player, 0.0);
  
  /* Add idle callback */
  g_timeout_add(DELAY_TIME, timeout_func, ret);
  
  return ret;
  }

void
bg_nle_player_widget_destroy(bg_nle_player_widget_t * w)
  {
  bg_player_destroy(w->player);
  bg_msg_queue_destroy(w->queue);
  }

GtkWidget * bg_nle_player_widget_get_widget(bg_nle_player_widget_t * w)
  {
  return w->box;
  }

void bg_nle_player_set_track(bg_nle_player_widget_t * w,
                             bg_plugin_handle_t * input_plugin, int track,
                             const char * track_name)
  {
  bg_player_play(w->player, input_plugin,
                 track, BG_PLAY_FLAG_INIT_THEN_PAUSE, track_name);
  }
