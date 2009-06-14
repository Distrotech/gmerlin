#include <gtk/gtk.h>

#include <config.h>
#include <gmerlin/utils.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/player.h>
#include <gmerlin/gui_gtk/gtkutils.h>
#include <gmerlin/gui_gtk/audio.h>

#include <gui_gtk/playerwidget.h>

struct bg_nle_player_widget_s
  {
  bg_player_t * player;

  GtkWidget * play_button;
  GtkWidget * stop_button;
  GtkWidget * goto_next_button;
  GtkWidget * goto_prev_button;
  GtkWidget * goto_start_button;
  GtkWidget * goto_end_button;
  GtkWidget * frame_forward_button;
  GtkWidget * frame_backward_button;
  GtkWidget * socket;
  
  GtkWidget * box;
  
  bg_gtk_vumeter_t * vumeter;
  };

static void button_callback(GtkWidget * w, gpointer data)
  {

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


bg_nle_player_widget_t *
bg_nle_player_widget_create(bg_plugin_registry_t * plugin_reg)
  {
  GtkWidget * box;
  bg_nle_player_widget_t * ret;

  ret = calloc(1, sizeof(*ret));

  /* Create buttons */  
  ret->play_button = create_pixmap_button(ret, "gmerlerra_play.png", TRS("Play"));

  ret->stop_button = create_pixmap_button(ret, "gmerlerra_stop.png", TRS("Stop"));

  ret->goto_next_button = create_pixmap_button(ret, "gmerlerra_goto_next.png",
                                               TRS("Goto next label"));
  
  ret->goto_prev_button = create_pixmap_button(ret, "gmerlerra_goto_prev.png",
                                               TRS("Goto previous label"));
  
  ret->goto_start_button = create_pixmap_button(ret, "gmerlerra_goto_start.png",
                                                TRS("Goto start"));
  ret->goto_end_button = create_pixmap_button(ret, "gmerlerra_goto_end.png",
                                                TRS("Goto end"));

  ret->frame_forward_button = create_pixmap_button(ret, "gmerlerra_frame_forward.png",
                                                   TRS("1 frame forward"));
  ret->frame_backward_button = create_pixmap_button(ret, "gmerlerra_frame_backward.png",
                                                    TRS("1 frame backward"));

  /* Create socket */
  
  ret->socket = gtk_socket_new();
  gtk_widget_show(ret->socket);
  
  ret->vumeter = bg_gtk_vumeter_create(2, 1);
  
  /* Pack */
  
  ret->box = gtk_vbox_new(FALSE, 0);

  box = gtk_hbox_new(FALSE, 0);

  gtk_box_pack_start(GTK_BOX(box), ret->socket, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), bg_gtk_vumeter_get_widget(ret->vumeter),
                     FALSE, FALSE, 0);
  
  gtk_widget_show(box);

  gtk_box_pack_start(GTK_BOX(ret->box), box, TRUE, TRUE, 0);
  
  
  box = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->goto_start_button, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->goto_prev_button, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->frame_backward_button, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->stop_button, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->play_button, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->frame_forward_button, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->goto_next_button, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->goto_end_button, TRUE, TRUE, 0);

  gtk_widget_show(box);

  gtk_box_pack_start(GTK_BOX(ret->box), box, FALSE, FALSE, 0);

  gtk_widget_show(ret->box);
  
  return ret;
  }

void
bg_nle_player_widget_destroy(bg_nle_player_widget_t * w)
  {

  }

GtkWidget * bg_nle_player_widget_get_widget(bg_nle_player_widget_t * w)
  {
  return w->box;
  }
