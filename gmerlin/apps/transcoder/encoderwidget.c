#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <pluginregistry.h>
#include <transcoder_track.h>

#include <gui_gtk/plugin.h>

#include "encoderwidget.h"

static void set_video_encoder(bg_plugin_handle_t * handle, void * data)
  {
  encoder_widget_t * win = (encoder_widget_t *)data;
  if(handle->info->type == BG_PLUGIN_ENCODER_VIDEO)
    {
    gtk_widget_set_sensitive(win->audio_to_video, 0);

    bg_gtk_plugin_widget_single_set_sensitive(win->audio_encoder, 1);
    }
  else
    {
    gtk_widget_set_sensitive(win->audio_to_video, 1);
    bg_gtk_plugin_widget_single_set_sensitive(win->audio_encoder,
                                              !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->audio_to_video)));
    }
  
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  encoder_widget_t * wid = (encoder_widget_t *)data;

  if(w == wid->audio_to_video)
    {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid->audio_to_video)))
      {
      bg_gtk_plugin_widget_single_set_sensitive(wid->audio_encoder, 0);
      bg_plugin_registry_set_encode_audio_to_video(wid->plugin_reg, 1);
      }
    else
      {
      bg_gtk_plugin_widget_single_set_sensitive(wid->audio_encoder, 1);
      bg_plugin_registry_set_encode_audio_to_video(wid->plugin_reg, 0);
      }
    }
  }

void encoder_widget_init(encoder_widget_t * ret, bg_plugin_registry_t * plugin_reg)
  {
  int row, num_columns;
  
  ret->tooltips = gtk_tooltips_new();
  ret->plugin_reg = plugin_reg;
  
  g_object_ref (G_OBJECT (ret->tooltips));
  gtk_object_sink (GTK_OBJECT (ret->tooltips));

  ret->audio_encoder =
    bg_gtk_plugin_widget_single_create("Audio", plugin_reg,
                                       BG_PLUGIN_ENCODER_AUDIO,
                                       BG_PLUGIN_FILE,
                                       NULL,
                                       ret, ret->tooltips);

  ret->audio_to_video =
    gtk_check_button_new_with_label("Encode audio into video file");

  g_signal_connect(G_OBJECT(ret->audio_to_video), "toggled",
                   G_CALLBACK(button_callback),
                   ret);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->audio_to_video),
                               bg_plugin_registry_get_encode_audio_to_video(plugin_reg));

  ret->video_encoder =
    bg_gtk_plugin_widget_single_create("Video", plugin_reg,
                                       BG_PLUGIN_ENCODER_VIDEO |
                                       BG_PLUGIN_ENCODER,
                                       BG_PLUGIN_FILE,
                                       set_video_encoder,
                                       ret, ret->tooltips);
  gtk_widget_show(ret->audio_to_video);

  ret->widget = gtk_table_new(1, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(ret->widget), 5);
  gtk_table_set_col_spacings(GTK_TABLE(ret->widget), 5);
  gtk_container_set_border_width(GTK_CONTAINER(ret->widget), 5);

  row = 0;
  num_columns = 0;

  bg_gtk_plugin_widget_single_attach(ret->audio_encoder,
                                     ret->widget,
                                     &row, &num_columns);
  bg_gtk_plugin_widget_single_attach(ret->video_encoder,
                                     ret->widget,
                                     &row, &num_columns);

  gtk_table_resize(GTK_TABLE(ret->widget), row+1, num_columns);
   
  gtk_table_attach(GTK_TABLE(ret->widget),
                   ret->audio_to_video,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  
  gtk_widget_show(ret->widget);

  }

void encoder_widget_destroy(encoder_widget_t * wid)
  {
  bg_gtk_plugin_widget_single_destroy(wid->audio_encoder);
  bg_gtk_plugin_widget_single_destroy(wid->video_encoder);
  }

/* Encoder window */

struct encoder_window_s
  {
  encoder_widget_t encoder_widget;
  GtkWidget * window;
  GtkWidget * ok_button;
  GtkWidget * apply_button;
  GtkWidget * close_button;

  bg_transcoder_track_t * tracks;
  };

static void encoder_window_get(encoder_window_t * win)
  {
  char * audio_encoder;
  char * video_encoder;

  bg_transcoder_track_t * first_selected;

  /* Get the first selected track */

  first_selected = win->tracks;

  while(!first_selected->selected)
    {
    first_selected = first_selected->next;
    }
  
  audio_encoder = bg_transcoder_track_get_audio_encoder(first_selected);
  video_encoder = bg_transcoder_track_get_video_encoder(first_selected);

  fprintf(stderr, "Audio encoder: %s\n", audio_encoder);
  fprintf(stderr, "Video encoder: %s\n", video_encoder);

  if(!strcmp(audio_encoder, video_encoder))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->encoder_widget.audio_to_video), 1);
    }
  else
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->encoder_widget.audio_to_video), 0);
    bg_gtk_plugin_widget_single_set_plugin(win->encoder_widget.audio_encoder, audio_encoder);
    }
  
  bg_gtk_plugin_widget_single_set_plugin(win->encoder_widget.video_encoder, video_encoder);
  
  free(audio_encoder);
  free(video_encoder);
  
  }

static void encoder_window_apply(encoder_window_t * win)
  {
  bg_plugin_handle_t * audio_encoder;
  bg_plugin_handle_t * video_encoder;

  bg_transcoder_track_t * track;

  track = win->tracks;

  video_encoder = bg_gtk_plugin_widget_single_get_plugin(win->encoder_widget.video_encoder);

  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->encoder_widget.audio_to_video)) ||
     (video_encoder->info->type == BG_PLUGIN_ENCODER_VIDEO))
    {
    audio_encoder = bg_gtk_plugin_widget_single_get_plugin(win->encoder_widget.audio_encoder);
    }
  else
    audio_encoder = (bg_plugin_handle_t*)0;
  
  while(track)
    {
    if(track->selected)
      {
      bg_transcoder_track_set_encoders(track, win->encoder_widget.plugin_reg,
                                       audio_encoder, video_encoder);
      }
    track = track->next;
    }
  }

static void window_button_callback(GtkWidget * w, gpointer data)
  {
  encoder_window_t * win = (encoder_window_t *)data;

  if(w == win->close_button)
    {
    //    fprintf(stderr, "close_button\n");
    gtk_widget_hide(win->window);
    gtk_main_quit();
    }
  if(w == win->apply_button)
    {
    //    fprintf(stderr, "apply_button\n");
    encoder_window_apply(win);
    }
  if(w == win->ok_button)
    {
    //    fprintf(stderr, "ok_button\n");
    encoder_window_apply(win);
    gtk_widget_hide(win->window);
    gtk_main_quit();
    }
  
  }

encoder_window_t * encoder_window_create(bg_plugin_registry_t * plugin_reg)
  {
  GtkWidget * mainbox;
  GtkWidget * buttonbox;
    
  encoder_window_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  
  gtk_window_set_title(GTK_WINDOW(ret->window), "Select encoders");
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
  
  encoder_widget_init(&(ret->encoder_widget), plugin_reg);

  /* Pack */

  buttonbox = gtk_hbutton_box_new();
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->ok_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->apply_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->close_button);
  gtk_widget_show(buttonbox);

  mainbox = gtk_vbox_new(0, 5);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), ret->encoder_widget.widget);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), buttonbox);
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(ret->window), mainbox);

  
  return ret;
  }

void encoder_window_run(encoder_window_t * win, bg_transcoder_track_t * tracks)
  {
  win->tracks = tracks;
  
  encoder_window_get(win);
  
  gtk_widget_show(win->window);
  gtk_main();
  }

void encoder_window_destroy(encoder_window_t * win)
  {
  free(win);
  }
