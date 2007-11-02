#include <string.h>

#include <config.h>
#include <translation.h>
#include <pluginregistry.h>
#include <cfg_dialog.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gui_gtk/gtkutils.h>
#include <gui_gtk/audio.h>
#include <gui_gtk/plugin.h>
#include <utils.h>
#include <visualize.h>

#include <gdk/gdkx.h>

#include <log.h>
#define LOG_DOMAIN "gmerlin_visualizer"

typedef struct
  {
  GtkWidget * window;
  GtkWidget * box;
  GtkWidget * socket;
  } window_t;

typedef struct
  {
  GtkWidget * window;
  bg_gtk_plugin_widget_single_t * ra_plugins;
  bg_gtk_plugin_widget_single_t * ov_plugins;
  GtkWidget * close_button;
  } plugin_window_t;

typedef struct
  {
  GtkWidget * config_button;
  GtkWidget * quit_button;
  GtkWidget * plugin_button;
  GtkWidget * restart_button;
  
  GtkWidget * fullscreen_button;
  GtkWidget * nofullscreen_button;
  
  GtkWidget * toolbar; /* A GtkEventBox actually... */
  GtkWidget * fps;
  
  bg_gtk_plugin_widget_single_t * vis_plugins;
  
  /* Windows are created by gtk and the x11 plugin
     is embedded inside after */

  window_t normal_window;
  window_t fullscreen_window;

  window_t *current_window;

  plugin_window_t plugin_window;
  
  bg_gtk_vumeter_t * vumeter;
  
  /* Core stuff */
  
  gavl_audio_frame_t * audio_frame;
  gavl_audio_format_t audio_format;
    
  bg_visualizer_t * visualizer;
  
  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t * cfg_reg;
  
  bg_plugin_handle_t * ra_handle;
  bg_ra_plugin_t * ra_plugin;
  
  const bg_plugin_info_t * ov_info;
  const bg_plugin_info_t * ra_info;
  /* Config stuff */
  
  //  bg_cfg_section_t * vumeter_section;
  bg_cfg_section_t * visualizer_section;
  bg_dialog_t * cfg_dialog;

  int audio_open;
  int vis_open;
  
  int mouse_in_toolbar;
  int do_hide_toolbar;
  int toolbar_visible;
  } visualizer_t;

static char * get_display_string(visualizer_t * v)
  {
  char * ret;
  GdkDisplay * dpy;
  /* Get the display string */

  gtk_widget_realize(v->normal_window.socket);
  gtk_widget_realize(v->fullscreen_window.socket);
  dpy = gdk_display_get_default();
  
  //  ret = bg_sprintf("%s:%08lx:%08lx", gdk_display_get_name(dpy),
  //                   GDK_WINDOW_XID(v->normal_window.window->window),
  //                   GDK_WINDOW_XID(v->fullscreen_window.window->window));

  ret =
    bg_sprintf("%s:%08lx:%08lx", gdk_display_get_name(dpy),
               (long unsigned int)gtk_socket_get_id(GTK_SOCKET(v->normal_window.socket)),
               (long unsigned int)gtk_socket_get_id(GTK_SOCKET(v->fullscreen_window.socket)));
  
  return ret;
  }

static void hide_toolbar(visualizer_t * v)
  {
  gtk_widget_hide(v->toolbar);
  v->toolbar_visible = 0;
  }

static gboolean toolbar_timeout(void * data)
  {
  visualizer_t * v;
  v = (visualizer_t *)data;
  if(!v->toolbar_visible)
    return TRUE;
  
  /* Maybe the toolbar will be hidden next time */
  if(!v->do_hide_toolbar)
    {
    v->do_hide_toolbar = 1;
    return TRUE;
    }
  
  if(!v->mouse_in_toolbar)
    {
    hide_toolbar(v);
    }
  return TRUE;
  }

static gboolean fps_timeout(void * data)
  {
  float fps;
  char * tmp_string;
  visualizer_t * v;
  v = (visualizer_t *)data;
  
  if(!v->toolbar_visible)
    return TRUE;

  fps = bg_visualizer_get_fps(v->visualizer);
  if(fps >= 0.0)
    {
    tmp_string = bg_sprintf("Fps: %.2f", fps);
    gtk_label_set_text(GTK_LABEL(v->fps), tmp_string);
    free(tmp_string);
    }
  
  return TRUE;
  }

static void show_toolbar(visualizer_t * v)
  {
  if(!v->toolbar_visible)
    {
    gtk_widget_show(v->toolbar);
    v->toolbar_visible = 1;
    }
  v->do_hide_toolbar = 0;
  }

static void attach_toolbar(visualizer_t * v, window_t * win)
  {
  gtk_box_pack_start(GTK_BOX(win->box), v->toolbar,
                     FALSE, FALSE, 0);
  
  }

static void toggle_fullscreen(visualizer_t * v)
  {
  if(v->current_window == &v->normal_window)
    {
    /* Reparent toolbar */
    gtk_container_remove(GTK_CONTAINER(v->normal_window.box),
                         v->toolbar);
    attach_toolbar(v, &v->fullscreen_window);
    /* Hide normal window, show fullscreen window */
    gtk_widget_show(v->fullscreen_window.window);
    gtk_widget_hide(v->normal_window.window);

    /* Update toolbar */
    gtk_widget_show(v->nofullscreen_button);
    gtk_widget_hide(v->fullscreen_button);

    gtk_widget_hide(v->config_button);
    gtk_widget_hide(v->plugin_button);

    bg_gtk_plugin_widget_single_show_buttons(v->vis_plugins, 0);
    
    v->current_window = &v->fullscreen_window;
    }
  else
    {
    /* Reparent toolbar */
    gtk_container_remove(GTK_CONTAINER(v->fullscreen_window.box),
                         v->toolbar);
    attach_toolbar(v, &v->normal_window);
    
    /* Hide normal window, show fullscreen window */
    gtk_widget_show(v->normal_window.window);
    gtk_widget_hide(v->fullscreen_window.window);
    
    /* Update toolbar */
    gtk_widget_show(v->fullscreen_button);
    gtk_widget_hide(v->nofullscreen_button);

    gtk_widget_show(v->config_button);
    gtk_widget_show(v->plugin_button);
    bg_gtk_plugin_widget_single_show_buttons(v->vis_plugins, 1);
    
    v->current_window = &v->normal_window;
    }
  hide_toolbar(v);
  v->mouse_in_toolbar = 0;
  }

static void open_audio(visualizer_t * v)
  {
  int i;
  int was_open;
  gavl_time_t delay_time = GAVL_TIME_SCALE / 20; /* 50 ms */
  memset(&v->audio_format, 0, sizeof(v->audio_format));
  v->audio_format.num_channels = 2;
  v->audio_format.samplerate = 44100;
  v->audio_format.sample_format = GAVL_SAMPLE_S16;
  gavl_set_channel_setup(&v->audio_format);
  
  if(v->audio_frame)
    {
    gavl_audio_frame_destroy(v->audio_frame);
    v->ra_plugin->close(v->ra_handle->priv);
    v->audio_frame = (gavl_audio_frame_t*)0;
    bg_plugin_unref(v->ra_handle);
    was_open = 1;
    }
  else
    was_open = 0;
  
  v->audio_open = 0;
  
  v->ra_handle = bg_plugin_load(v->plugin_reg,
                                v->ra_info);
  v->ra_plugin = (bg_ra_plugin_t*)(v->ra_handle->plugin);
  
  /* The soundcard might be busy from last time,
     give the kernel some time to free the device */
  
  if(!v->ra_plugin->open(v->ra_handle->priv, &v->audio_format))
    {
    if(!was_open)
      return;
    
    for(i = 0; i < 20; i++)
      {
      gavl_time_delay(&delay_time);
      
      if(v->ra_plugin->open(v->ra_handle->priv, &v->audio_format))
        {
        v->audio_open = 1;
        break;
        }
      
      }
    }
  else
    v->audio_open = 1;
  
  if(v->audio_open)
    {
    v->audio_frame = gavl_audio_frame_create(&v->audio_format);
    bg_gtk_vumeter_set_format(v->vumeter, &v->audio_format);
    }
  }

static void open_vis(visualizer_t * v)
  {
  char * display_string = get_display_string(v);
  bg_visualizer_open_id(v->visualizer, &v->audio_format,
                        v->ov_info, display_string);  
  free(display_string);


  v->vis_open = 1;
  }

static void close_vis(visualizer_t * v)
  {
  bg_visualizer_close(v->visualizer);  
  v->vis_open = 0;
  }

static void grab_notify_callback(GtkWidget *widget,
                                 gboolean   was_grabbed,
                                 gpointer   data)
  {
  visualizer_t * win = (visualizer_t*)data;
  if(!was_grabbed)
    {
    GTK_WIDGET_SET_FLAGS(win->current_window->socket, GTK_CAN_FOCUS);
    gtk_widget_grab_focus(win->current_window->socket);
    }
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  visualizer_t * win = (visualizer_t*)data;
  
  if((w == win->quit_button) ||
     (w == win->normal_window.window))
    gtk_main_quit();
  else if(w == win->config_button)
    bg_dialog_show(win->cfg_dialog, win->current_window->window);
  else if((w == win->fullscreen_button) ||
          (w == win->nofullscreen_button))
    toggle_fullscreen(win);
  else if(w == win->plugin_button)
    {
    gtk_widget_show(win->plugin_window.window);
    gtk_widget_set_sensitive(win->plugin_button, 0);
    }
  else if(w == win->restart_button)
    {
    if(win->vis_open)
      {
      close_vis(win);
      open_vis(win);
      hide_toolbar(win);
      }
    }
  }

static gboolean plug_removed_callback(GtkWidget * w, gpointer data)
  {
  /* Reuse socket */
  return TRUE;
  }

static void plug_added_callback(GtkWidget * w, gpointer data)
  {
  visualizer_t * v;
  v = (visualizer_t *)data;
  gtk_widget_hide(v->toolbar);
  
  /* Seems that this is switched off, when an earlier client exited */
  GTK_WIDGET_SET_FLAGS(w, GTK_CAN_FOCUS);
  gtk_widget_grab_focus(w);
  }

static gboolean
delete_callback(GtkWidget * w,
                GdkEventAny * evt, gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }


static void plugin_window_button_callback(GtkWidget * w, gpointer data)
  {
  visualizer_t * v;
  v = (visualizer_t *)data;
  gtk_widget_set_sensitive(v->plugin_button, 1);
  gtk_widget_hide(v->plugin_window.window);
  }

static gboolean plugin_window_delete_callback(GtkWidget * w,
                                          GdkEventAny * evt,
                                          gpointer data)
  {
  plugin_window_button_callback(w, data);
  return TRUE;
  }

static void set_ra_plugin(const bg_plugin_info_t * plugin,
                          void * data)
  {
  visualizer_t * v = (visualizer_t*)data;
  bg_plugin_registry_set_default(v->plugin_reg, BG_PLUGIN_RECORDER_AUDIO,
                                 plugin->name);
  
  v->ra_info = plugin;
  bg_log(BG_LOG_INFO, LOG_DOMAIN,
         "Changed recording plugin to %s", v->ra_info->long_name);
  close_vis(v);
  open_audio(v);
  open_vis(v);
  }


static void plugin_window_init(plugin_window_t * win, visualizer_t * v)
  {
  int row = 0, num_columns = 4;
  GtkWidget * table;
  win->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  win->ra_plugins =
    bg_gtk_plugin_widget_single_create(TR("Audio recorder"),
                                       v->plugin_reg,
                                       BG_PLUGIN_RECORDER_AUDIO,
                                       BG_PLUGIN_ALL);

  bg_gtk_plugin_widget_single_set_change_callback(win->ra_plugins,
                                                  set_ra_plugin,
                                                  v);
  
  win->ov_plugins =
    bg_gtk_plugin_widget_single_create(TR("Video output"),
                                       v->plugin_reg,
                                       BG_PLUGIN_OUTPUT_VIDEO,
                                       BG_PLUGIN_ALL);
  
  win->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  g_signal_connect(win->close_button, "clicked",
                   G_CALLBACK(plugin_window_button_callback),
                   v);
  g_signal_connect(win->window, "delete-event",
                   G_CALLBACK(plugin_window_delete_callback),
                   v);
  
  gtk_widget_show(win->close_button);
  
  table = gtk_table_new(3, num_columns, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_window_set_position(GTK_WINDOW(win->window), GTK_WIN_POS_CENTER);
  
  bg_gtk_plugin_widget_single_attach(win->ra_plugins, table,
                                     &row, &num_columns);
  bg_gtk_plugin_widget_single_attach(win->ov_plugins, table,
                                     &row, &num_columns);
  
  gtk_table_attach(GTK_TABLE(table), win->close_button, 0, num_columns,
                   row, row+1, 
                   GTK_SHRINK, GTK_SHRINK, 0, 0);
  
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(win->window),  table);
  }


static gboolean mouse_button_callback(GtkWidget * w,
                                      GdkEventButton * evt,
                                      gpointer data)
  {
  visualizer_t * v = (visualizer_t*)data;
  show_toolbar(v);
  return TRUE;
  }

static gboolean motion_callback(GtkWidget * w,
                                GdkEventMotion * evt,
                                gpointer data)
  {
  visualizer_t * v = (visualizer_t*)data;
  show_toolbar(v);
  return FALSE;
  }

static gboolean key_callback(GtkWidget * w,
                             GdkEventKey * evt,
                             gpointer data)
  {
  visualizer_t * v = (visualizer_t*)data;
  //  gtk_widget_show(v->toolbar);
  //  g_timeout_add(2000, toolbar_timeout, v);
  fprintf(stderr, "Vis: Got key press\n");

  switch(evt->keyval)
    {
    case GDK_Tab:
    case GDK_f:
      toggle_fullscreen(v);
      return TRUE;
      break;
    case GDK_Escape:
      if(v->current_window == &v->fullscreen_window)
        toggle_fullscreen(v);
      return TRUE;
      break;
    case GDK_m:
      show_toolbar(v);
      return TRUE;
      break;
    }
  return FALSE;
  }

static void window_init(visualizer_t * v,
                        window_t * w, int fullscreen)
  {
  GtkWidget * table;
  w->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(w->window), "Gmerlin visualizer");
  w->socket = gtk_socket_new();
  w->box = gtk_vbox_new(0, 0);
  gtk_widget_show(w->box);
  gtk_widget_show(w->socket);

  table = gtk_table_new(1, 1, 0);

  gtk_table_attach_defaults(GTK_TABLE(table),w->socket,
                            0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table),w->box,
                            0, 1, 0, 1);
  
  gtk_widget_show(table);
  
  gtk_widget_set_events(w->socket,
                        GDK_BUTTON_PRESS_MASK |
                        GDK_KEY_PRESS_MASK | 
                        GDK_POINTER_MOTION_MASK);
  
  //  gtk_window_set_focus_on_map(w->window, 0);
  
  gtk_container_add(GTK_CONTAINER(w->window), table);
  
  g_signal_connect(G_OBJECT(w->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   v);
  
  g_signal_connect(G_OBJECT(w->socket), "motion-notify-event",
                   G_CALLBACK(motion_callback),
                   v);
  
  g_signal_connect(G_OBJECT(w->socket), "plug-removed",
                   G_CALLBACK(plug_removed_callback),
                   v);

  g_signal_connect(G_OBJECT(w->socket), "grab-notify",
                   G_CALLBACK(grab_notify_callback),
                   v);

  g_signal_connect(G_OBJECT(w->socket), "plug-added",
                   G_CALLBACK(plug_added_callback),
                   v);
  
  g_signal_connect(G_OBJECT(w->socket), "button-press-event",
                   G_CALLBACK(mouse_button_callback),
                   v);
  g_signal_connect(G_OBJECT(w->socket), "key-press-event",
                   G_CALLBACK(key_callback),
                   v);
  
  if(fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(w->window));
  else
    gtk_widget_set_size_request(w->window, 640, 480);
  
  }

static GtkWidget * create_pixmap_button(visualizer_t * w,
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

static void set_vis_param(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  visualizer_t * v;
  v = (visualizer_t *)data;
  bg_visualizer_set_parameter(v->visualizer, name, val);

  if(bg_visualizer_need_restart(v->visualizer) && v->vis_open &&
     !name)
    {
    gtk_widget_hide(v->toolbar);
    close_vis(v);
    open_vis(v);
    }
  }
                         


static bg_dialog_t * create_cfg_dialog(visualizer_t * win)
  {
  bg_parameter_info_t * info;
  bg_dialog_t * ret;
  ret = bg_dialog_create_multi(TR("Visualizer configuration"));

  info = bg_visualizer_get_parameters(win->visualizer);

  bg_dialog_add(ret,
                TR("Visualizer"),
                win->visualizer_section,
                set_vis_param,
                (void*)(win),
                info);

#if 0
  info = bg_gtk_vumeter_get_parameters(win->vumeter);
  
  bg_dialog_add(ret,
                TR("Vumeter"),
                win->vumeter_section,
                bg_gtk_vumeter_set_parameter,
                (void*)(win->vumeter),
                info);
#endif
  return ret;
  }


static void apply_config(visualizer_t * v)
  {
  bg_parameter_info_t * parameters;
  
  parameters = bg_visualizer_get_parameters(v->visualizer);
  
  bg_cfg_section_apply(v->visualizer_section, parameters,
                       bg_visualizer_set_parameter,
                       (void*)(v->visualizer));
  }

static gboolean idle_func(void * data)
  {
  visualizer_t * v;
  v = (visualizer_t *)data;
  if(v->audio_open)
    {
    v->ra_plugin->read_frame(v->ra_handle->priv,
                             v->audio_frame, v->audio_format.samples_per_frame);
    bg_visualizer_update(v->visualizer, v->audio_frame);
    
    if(v->toolbar_visible)
      bg_gtk_vumeter_update(v->vumeter, v->audio_frame);
    
    bg_visualizer_update(v->visualizer, v->audio_frame);
    }
  return TRUE;
  }


static gboolean crossing_callback(GtkWidget *widget,
                                  GdkEventCrossing *event,
                                  gpointer data)
  {
  visualizer_t * v = (visualizer_t*)data;
  
  if(event->detail == GDK_NOTIFY_INFERIOR)
    return FALSE;
  
  v->mouse_in_toolbar = (event->type == GDK_ENTER_NOTIFY) ? 1 : 0;
  return FALSE;
  }

static void set_vis_plugin(const bg_plugin_info_t * plugin,
                           void * data)
  {
  visualizer_t * v = (visualizer_t*)data;
  bg_visualizer_set_vis_plugin(v->visualizer, plugin);
  bg_plugin_registry_set_default(v->plugin_reg, BG_PLUGIN_VISUALIZATION,
                                 plugin->name);
  if(bg_visualizer_need_restart(v->visualizer))
    {
    close_vis(v);
    open_vis(v);
    }
  hide_toolbar(v);
  }

static void set_vis_parameter(void * data, const char * name,
                              const bg_parameter_value_t * val)
  {
  visualizer_t * v = (visualizer_t*)data;
  bg_visualizer_set_vis_parameter(v->visualizer, name, val);
  }


static visualizer_t * visualizer_create()
  {
  const bg_plugin_info_t * info;
  char * tmp_path;
  int row, col;
  
  GtkWidget * main_table;
  GtkWidget * table;
  GtkWidget * box;
  
  visualizer_t * ret;
  bg_cfg_section_t * cfg_section;
  
  ret = calloc(1, sizeof(*ret));

  window_init(ret, &ret->normal_window, 0);
  window_init(ret, &ret->fullscreen_window, 1);
  ret->current_window = &ret->normal_window;
  
  ret->config_button =
    create_pixmap_button(ret, "config_16.png", TRS("Configure"));
  ret->plugin_button =
    create_pixmap_button(ret, "plugin_16.png", TRS("Recording and display plugins"));
  ret->restart_button =
    create_pixmap_button(ret, "refresh_16.png", TRS("Restart visualization"));
  ret->quit_button =
    create_pixmap_button(ret, "quit_16.png", TRS("Quit"));
  ret->fullscreen_button =
    create_pixmap_button(ret, "fullscreen_16.png", TRS("Fullscreen mode"));
  ret->nofullscreen_button =
    create_pixmap_button(ret, "windowed_16.png", TRS("Leave fullscreen mode"));
  
  ret->fps = gtk_label_new("Fps: --:--");
  gtk_misc_set_alignment(GTK_MISC(ret->fps), 0.0, 0.5);
  gtk_widget_show(ret->fps);
  
  gtk_widget_hide(ret->nofullscreen_button);
  
  //  gtk_box_pack_start_defaults(GTK_BOX(mainbox),
  //                              bg_gtk_vumeter_get_widget(ret->vumeter));
  
  ret->toolbar = gtk_event_box_new();

  gtk_widget_set_events(ret->toolbar,
                        GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  
  g_signal_connect(G_OBJECT(ret->toolbar), "enter-notify-event",
                   G_CALLBACK(crossing_callback), (gpointer*)ret);
  
  g_signal_connect(G_OBJECT(ret->toolbar), "leave-notify-event",
                   G_CALLBACK(crossing_callback), (gpointer*)ret);

  
  g_object_ref(ret->toolbar); /* Must be done for widgets, which get
                                 reparented after */
  
  ret->vumeter = bg_gtk_vumeter_create(2, 0);
  
  /* Create actual objects */

  /* Create plugin regsitry */
  ret->cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("visualizer", "config.xml");
  bg_cfg_registry_load(ret->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  cfg_section = bg_cfg_registry_find_section(ret->cfg_reg, "plugins");
  ret->plugin_reg = bg_plugin_registry_create(cfg_section);
  ret->visualizer = bg_visualizer_create(ret->plugin_reg);

  /* Create vis plugin widget */

  ret->vis_plugins =
    bg_gtk_plugin_widget_single_create(TR("Visualization"),
                                       ret->plugin_reg,
                                       BG_PLUGIN_VISUALIZATION,
                                       BG_PLUGIN_VISUALIZE_FRAME |
                                       BG_PLUGIN_VISUALIZE_GL);

  bg_gtk_plugin_widget_single_set_change_callback(ret->vis_plugins,
                                                  set_vis_plugin,
                                                  ret);

  bg_gtk_plugin_widget_single_set_parameter_callback(ret->vis_plugins,
                                                     set_vis_parameter,
                                                     ret);
  
  
  /* Create audio and video plugin widgets */

  plugin_window_init(&ret->plugin_window, ret);

  /* Get ov info */

  ret->ov_info =
    bg_gtk_plugin_widget_single_get_plugin(ret->plugin_window.ov_plugins);
  
  /* Load recording plugin */
  ret->ra_info =
    bg_gtk_plugin_widget_single_get_plugin(ret->plugin_window.ra_plugins);
  
  /* Create config stuff */
  
  ret->visualizer_section =
    bg_cfg_registry_find_section(ret->cfg_reg, "visualizer");
  //  ret->vumeter_section =
  //    bg_cfg_registry_find_section(ret->cfg_reg, "vumeter");
  
  ret->cfg_dialog = create_cfg_dialog(ret);


  
  /* Pack everything */

  main_table = gtk_table_new(2, 2, 0);

  gtk_table_set_row_spacings(GTK_TABLE(main_table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(main_table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(main_table), 5);
  
  table = gtk_table_new(1, 4, 0);

  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  
  
  row = 0;
  col = 0;
  bg_gtk_plugin_widget_single_attach(ret->vis_plugins,
                                     table,
                                     &row, &col);
  gtk_widget_show(table);
  
  gtk_table_attach(GTK_TABLE(main_table), table, 0, 1, 0, 1,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  
  box = gtk_hbox_new(0, 5);
  gtk_box_pack_start(GTK_BOX(box), ret->plugin_button,
                     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->restart_button,
                     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->config_button,
                     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->fullscreen_button,
                     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->nofullscreen_button,
                     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->quit_button,
                     FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(box), ret->fps);
  gtk_widget_show(box);
  
  gtk_table_attach(GTK_TABLE(main_table), box, 0, 1, 1, 2,
                   GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(main_table),
                   bg_gtk_vumeter_get_widget(ret->vumeter),
                   1, 2, 0, 2,
                   GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  
  gtk_widget_show(main_table);
  
  gtk_container_add(GTK_CONTAINER(ret->toolbar), main_table);
  
  //  gtk_widget_show(ret->toolbar);
  
  /* Start with non-fullscreen mode */
  attach_toolbar(ret, &ret->normal_window);
  
  apply_config(ret);
  
  /* Get visualization plugin */
  info = bg_gtk_plugin_widget_single_get_plugin(ret->vis_plugins);
  bg_visualizer_set_vis_plugin(ret->visualizer, info);
  
  /* Initialize stuff */
  open_audio(ret);
  open_vis(ret);

  gtk_widget_show(ret->current_window->window);
  
  while(gdk_events_pending() || gtk_events_pending())
    gtk_main_iteration();
  
  
  g_idle_add(idle_func, ret);
  
  g_timeout_add(3000, toolbar_timeout, ret);
  g_timeout_add(1000, fps_timeout, ret);
  
  return ret;
  }

static void visualizer_destroy(visualizer_t * v)
  {
  char * tmp_path;
  tmp_path =  bg_search_file_write("visualizer", "config.xml");
  bg_cfg_registry_save(v->cfg_reg, tmp_path);
  fprintf(stderr, "Saving registry to %s\n",
          tmp_path);
  if(tmp_path)
    free(tmp_path);
  free(v);
  }

int main(int argc, char ** argv)
  {
  visualizer_t * win;
  
  bg_gtk_init(&argc, &argv, "visualizer_icon.png");

  win = visualizer_create();
  gtk_main();
  
  if(win->vis_open)
    bg_visualizer_stop(win->visualizer);
  visualizer_destroy(win);
  return 0;
  }
