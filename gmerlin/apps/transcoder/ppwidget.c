#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <pluginregistry.h>
#include <transcoder_track.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>

#include "ppwidget.h"

typedef struct
  {
  bg_gtk_plugin_widget_single_t * plugins;
  GtkWidget * pp;

  GtkWidget * widget;

  GtkTooltips * tooltips;
  bg_plugin_registry_t * plugin_reg;
  } encoder_pp_widget_t;

static void button_callback(GtkWidget * w, gpointer data)
  {
  encoder_pp_widget_t * wid = (encoder_pp_widget_t *)data;

  if(w == wid->pp)
    {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid->pp)))
      {
      bg_gtk_plugin_widget_single_set_sensitive(wid->plugins, 1);
      bg_plugin_registry_set_encode_pp(wid->plugin_reg, 1);
      }
    else
      {
      bg_gtk_plugin_widget_single_set_sensitive(wid->plugins, 0);
      bg_plugin_registry_set_encode_pp(wid->plugin_reg, 0);
      }
    }
  }

static void encoder_pp_widget_init(encoder_pp_widget_t * ret, bg_plugin_registry_t * plugin_reg)
  {
  int row, num_columns;
  
  ret->tooltips = gtk_tooltips_new();
  ret->plugin_reg = plugin_reg;
  
  g_object_ref (G_OBJECT (ret->tooltips));
  gtk_object_sink (GTK_OBJECT (ret->tooltips));

  ret->plugins =
    bg_gtk_plugin_widget_single_create("Postprocessing plugin", plugin_reg,
                                       BG_PLUGIN_ENCODER_PP,
                                       0,
                                       ret->tooltips);

  bg_gtk_plugin_widget_single_set_sensitive(ret->plugins, 0);

  ret->pp =
    gtk_check_button_new_with_label("Enable postprocessing");

  g_signal_connect(G_OBJECT(ret->pp), "toggled",
                   G_CALLBACK(button_callback),
                   ret);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->pp),
                               bg_plugin_registry_get_encode_pp(plugin_reg));
  
  gtk_widget_show(ret->pp);
  
  ret->widget = gtk_table_new(1, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(ret->widget), 5);
  gtk_table_set_col_spacings(GTK_TABLE(ret->widget), 5);
  gtk_container_set_border_width(GTK_CONTAINER(ret->widget), 5);

  row = 0;
  num_columns = 4;

  gtk_table_resize(GTK_TABLE(ret->widget), row+1, num_columns);
   
  gtk_table_attach(GTK_TABLE(ret->widget),
                   ret->pp,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);

  row++;
  bg_gtk_plugin_widget_single_attach(ret->plugins,
                                     ret->widget,
                                     &row, &num_columns);

  gtk_widget_show(ret->widget);
  }

static void encoder_pp_widget_destroy(encoder_pp_widget_t * wid)
  {
  bg_gtk_plugin_widget_single_destroy(wid->plugins);
  }

/* Encoder window */

struct encoder_pp_window_s
  {
  encoder_pp_widget_t encoder_pp_widget;
  GtkWidget * window;
  GtkWidget * ok_button;
  GtkWidget * apply_button;
  GtkWidget * close_button;

  bg_transcoder_track_global_t * global;
  };

const
bg_plugin_info_t * encoder_pp_window_get_plugin(encoder_pp_window_t * win)
  {
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->encoder_pp_widget.pp)))
    return bg_gtk_plugin_widget_single_get_plugin(win->encoder_pp_widget.plugins);
  else
    return (const bg_plugin_info_t *)0;
  }

static void encoder_pp_window_get(encoder_pp_window_t * win)
  {
  const bg_plugin_info_t * plugin_info;
  if(!win->global->pp_plugin)
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->encoder_pp_widget.pp), 0);
    }
  else
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->encoder_pp_widget.pp), 1);
    plugin_info = bg_plugin_find_by_name(win->encoder_pp_widget.plugin_reg,
                                         win->global->pp_plugin);
    bg_gtk_plugin_widget_single_set_plugin(win->encoder_pp_widget.plugins,
                                           plugin_info);
    }
  bg_transcoder_track_global_to_reg(win->global,
                                    win->encoder_pp_widget.plugin_reg);
  }

static void encoder_pp_window_apply(encoder_pp_window_t * win)
  {
  bg_transcoder_track_global_from_reg(win->global,
                                      win->encoder_pp_widget.plugin_reg);
  }

static void window_button_callback(GtkWidget * w, gpointer data)
  {
  encoder_pp_window_t * win = (encoder_pp_window_t *)data;

  if((w == win->close_button) || (w == win->window))
    {
    //    fprintf(stderr, "close_button\n");
    gtk_widget_hide(win->window);
    gtk_main_quit();
    }
  if(w == win->apply_button)
    {
    //    fprintf(stderr, "apply_button\n");
    encoder_pp_window_apply(win);
    }
  if(w == win->ok_button)
    {
    //    fprintf(stderr, "ok_button\n");
    encoder_pp_window_apply(win);
    gtk_widget_hide(win->window);
    gtk_main_quit();
    }
  
  }

static gboolean delete_callback(GtkWidget * w, GdkEvent * evt, gpointer data)
  {
  //  fprintf(stderr, "Delete callback\n");
  window_button_callback(w, data);
  return TRUE;
  }
  
encoder_pp_window_t * encoder_pp_window_create(bg_plugin_registry_t * plugin_reg)
  {
  GtkWidget * mainbox;
  GtkWidget * buttonbox;
    
  encoder_pp_window_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);

  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   ret);
  
  gtk_window_set_title(GTK_WINDOW(ret->window), "Postprocessing");
  gtk_window_set_modal(GTK_WINDOW(ret->window), 1);

  ret->apply_button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  ret->ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);

  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(window_button_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->apply_button), "clicked",
                   G_CALLBACK(window_button_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->ok_button), "clicked",
                   G_CALLBACK(window_button_callback),
                   ret);

  /* Show */
  gtk_widget_show(ret->close_button);
  gtk_widget_show(ret->apply_button);
  gtk_widget_show(ret->ok_button);
  
  encoder_pp_widget_init(&(ret->encoder_pp_widget), plugin_reg);

  /* Pack */

  buttonbox = gtk_hbutton_box_new();
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->ok_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->apply_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->close_button);
  gtk_widget_show(buttonbox);

  mainbox = gtk_vbox_new(0, 5);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), ret->encoder_pp_widget.widget);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), buttonbox);
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(ret->window), mainbox);
    
  return ret;
  }

void encoder_pp_window_run(encoder_pp_window_t * win,
                           bg_transcoder_track_global_t * global)
  {
  win->global = global;
  
  encoder_pp_window_get(win);
  
  gtk_widget_show(win->window);
  gtk_main();
  }

void encoder_pp_window_destroy(encoder_pp_window_t * win)
  {
  encoder_pp_widget_destroy(&win->encoder_pp_widget);
  free(win);
  }
