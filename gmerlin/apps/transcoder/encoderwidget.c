#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <pluginregistry.h>
#include <transcoder_track.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>

#include "encoderwidget.h"

static void encoder_widget_get(encoder_widget_t * wid, bg_transcoder_encoder_info_t * info)
  {
  bg_gtk_plugin_widget_single_set_plugin(wid->video_encoder,
                                         info->video_info);
  bg_gtk_plugin_widget_single_set_section(wid->video_encoder,
                                          info->video_encoder_section);
  
  bg_gtk_plugin_widget_single_set_video_section(wid->video_encoder,
                                                info->video_stream_section);
  
  if(info->audio_info)
    {
    g_signal_handler_block(G_OBJECT(wid->audio_to_video), wid->audio_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->audio_to_video), 0);
    g_signal_handler_unblock(G_OBJECT(wid->audio_to_video), wid->audio_to_video_id);

    bg_gtk_plugin_widget_single_set_plugin(wid->audio_encoder,
                                           info->audio_info);
    bg_gtk_plugin_widget_single_set_section(wid->audio_encoder,
                                            info->audio_encoder_section);
    bg_gtk_plugin_widget_single_set_audio_section(wid->audio_encoder,
                                                  info->audio_stream_section);
    }
  else
    {
    g_signal_handler_block(G_OBJECT(wid->audio_to_video), wid->audio_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->audio_to_video), 1);
    g_signal_handler_unblock(G_OBJECT(wid->audio_to_video), wid->audio_to_video_id);

    bg_gtk_plugin_widget_single_set_audio_section(wid->video_encoder,
                                                  info->audio_stream_section);
    }

  if(info->subtitle_text_info)
    {
    g_signal_handler_block(G_OBJECT(wid->subtitle_text_to_video), wid->subtitle_text_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->subtitle_text_to_video), 0);
    g_signal_handler_unblock(G_OBJECT(wid->subtitle_text_to_video), wid->subtitle_text_to_video_id);

    bg_gtk_plugin_widget_single_set_plugin(wid->subtitle_text_encoder,
                                           info->subtitle_text_info);
    bg_gtk_plugin_widget_single_set_section(wid->subtitle_text_encoder,
                                            info->subtitle_text_encoder_section);
    bg_gtk_plugin_widget_single_set_subtitle_text_section(wid->subtitle_text_encoder,
                                                          info->subtitle_text_stream_section);
    }
  else
    {
    g_signal_handler_block(G_OBJECT(wid->subtitle_text_to_video), wid->subtitle_text_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->subtitle_text_to_video), 1);
    g_signal_handler_unblock(G_OBJECT(wid->subtitle_text_to_video), wid->subtitle_text_to_video_id);

    bg_gtk_plugin_widget_single_set_subtitle_text_section(wid->video_encoder,
                                                          info->subtitle_text_stream_section);
    }

  if(info->subtitle_overlay_info)
    {
    g_signal_handler_block(G_OBJECT(wid->subtitle_overlay_to_video), wid->subtitle_overlay_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->subtitle_overlay_to_video), 0);
    g_signal_handler_unblock(G_OBJECT(wid->subtitle_overlay_to_video), wid->subtitle_overlay_to_video_id);

    bg_gtk_plugin_widget_single_set_plugin(wid->subtitle_overlay_encoder,
                                           info->subtitle_overlay_info);
    bg_gtk_plugin_widget_single_set_section(wid->subtitle_overlay_encoder,
                                            info->subtitle_overlay_encoder_section);
    bg_gtk_plugin_widget_single_set_subtitle_overlay_section(wid->subtitle_overlay_encoder,
                                                          info->subtitle_overlay_stream_section);
    }
  else
    {
    g_signal_handler_block(G_OBJECT(wid->subtitle_overlay_to_video), wid->subtitle_overlay_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->subtitle_overlay_to_video), 1);
    g_signal_handler_unblock(G_OBJECT(wid->subtitle_overlay_to_video), wid->subtitle_overlay_to_video_id);

    bg_gtk_plugin_widget_single_set_subtitle_overlay_section(wid->video_encoder,
                                                             info->subtitle_overlay_stream_section);
    }
  }


void encoder_widget_update_sensitive(encoder_widget_t * win)
  {
  const bg_plugin_info_t * info;
  info = bg_gtk_plugin_widget_single_get_plugin(win->video_encoder);
  
  if(!info->max_audio_streams)
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

  if(!info->max_subtitle_text_streams)
    {
    gtk_widget_set_sensitive(win->subtitle_text_to_video, 0);

    bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_text_encoder, 1);
    }
  else
    {
    gtk_widget_set_sensitive(win->subtitle_text_to_video, 1);
    bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_text_encoder,
                                              !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->subtitle_text_to_video)));
    }

  if(!info->max_subtitle_overlay_streams)
    {
    gtk_widget_set_sensitive(win->subtitle_overlay_to_video, 0);

    bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_overlay_encoder, 1);
    }
  else
    {
    gtk_widget_set_sensitive(win->subtitle_overlay_to_video, 1);
    bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_overlay_encoder,
                                              !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->subtitle_overlay_to_video)));
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

  if(w == wid->subtitle_text_to_video)
    {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid->subtitle_text_to_video)))
      {
      bg_gtk_plugin_widget_single_set_sensitive(wid->subtitle_text_encoder, 0);
      bg_plugin_registry_set_encode_subtitle_text_to_video(wid->plugin_reg, 1);
      }
    else
      {
      bg_gtk_plugin_widget_single_set_sensitive(wid->subtitle_text_encoder, 1);
      bg_plugin_registry_set_encode_subtitle_text_to_video(wid->plugin_reg, 0);
      }
    }
  if(w == wid->subtitle_overlay_to_video)
    {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid->subtitle_overlay_to_video)))
      {
      bg_gtk_plugin_widget_single_set_sensitive(wid->subtitle_overlay_encoder, 0);
      bg_plugin_registry_set_encode_subtitle_overlay_to_video(wid->plugin_reg, 1);
      }
    else
      {
      bg_gtk_plugin_widget_single_set_sensitive(wid->subtitle_overlay_encoder, 1);
      bg_plugin_registry_set_encode_subtitle_overlay_to_video(wid->plugin_reg, 0);
      }
    }
  }

void encoder_widget_init(encoder_widget_t * ret, bg_plugin_registry_t * plugin_reg)
  {
  int row, num_columns;
  
  ret->tooltips = gtk_tooltips_new();
  ret->plugin_reg = plugin_reg;
  
  g_object_ref (G_OBJECT (ret->tooltips));

#if GTK_MINOR_VERSION < 10
  gtk_object_sink (GTK_OBJECT (ret->tooltips));
#else
  g_object_ref_sink(G_OBJECT(ret->tooltips));
#endif

  ret->audio_encoder =
    bg_gtk_plugin_widget_single_create("Audio", plugin_reg,
                                       BG_PLUGIN_ENCODER_AUDIO,
                                       BG_PLUGIN_FILE,
                                       ret->tooltips);

  ret->subtitle_text_encoder =
    bg_gtk_plugin_widget_single_create("Text subtitles", plugin_reg,
                                       BG_PLUGIN_ENCODER_SUBTITLE_TEXT,
                                       BG_PLUGIN_FILE,
                                       ret->tooltips);

  ret->subtitle_overlay_encoder =
    bg_gtk_plugin_widget_single_create("Overlay subtitles", plugin_reg,
                                       BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY,
                                       BG_PLUGIN_FILE,
                                       ret->tooltips);

  ret->audio_to_video =
    gtk_check_button_new_with_label("Encode audio into video file");
  ret->subtitle_text_to_video =
    gtk_check_button_new_with_label("Encode text subtitles into video file");
  ret->subtitle_overlay_to_video =
    gtk_check_button_new_with_label("Encode overlay subtitles into video file");
  
  ret->audio_to_video_id =
    g_signal_connect(G_OBJECT(ret->audio_to_video), "toggled",
                     G_CALLBACK(button_callback),
                     ret);

  ret->subtitle_text_to_video_id =
    g_signal_connect(G_OBJECT(ret->subtitle_text_to_video), "toggled",
                     G_CALLBACK(button_callback),
                     ret);

  ret->subtitle_overlay_to_video_id =
    g_signal_connect(G_OBJECT(ret->subtitle_overlay_to_video), "toggled",
                     G_CALLBACK(button_callback),
                     ret);
  
  ret->video_encoder =
    bg_gtk_plugin_widget_single_create("Video", plugin_reg,
                                       BG_PLUGIN_ENCODER_VIDEO |
                                       BG_PLUGIN_ENCODER,
                                       BG_PLUGIN_FILE,
                                       ret->tooltips);
  gtk_widget_show(ret->audio_to_video);
  gtk_widget_show(ret->subtitle_text_to_video);
  gtk_widget_show(ret->subtitle_overlay_to_video);

  ret->widget = gtk_table_new(1, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(ret->widget), 5);
  gtk_table_set_col_spacings(GTK_TABLE(ret->widget), 5);
  gtk_container_set_border_width(GTK_CONTAINER(ret->widget), 5);

  row = 0;
  num_columns = 6;

  /* Audio */
  
  gtk_table_resize(GTK_TABLE(ret->widget), row+1, num_columns);
   
  gtk_table_attach(GTK_TABLE(ret->widget),
                   ret->audio_to_video,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  row++;
  
  bg_gtk_plugin_widget_single_attach(ret->audio_encoder,
                                     ret->widget,
                                     &row, &num_columns);

  /* Text subtitles */
    
  gtk_table_resize(GTK_TABLE(ret->widget), row+1, num_columns);
   
  gtk_table_attach(GTK_TABLE(ret->widget),
                   ret->subtitle_text_to_video,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  row++;

  bg_gtk_plugin_widget_single_attach(ret->subtitle_text_encoder,
                                     ret->widget,
                                     &row, &num_columns);

  /* Overlay subtitles */
    
  gtk_table_resize(GTK_TABLE(ret->widget), row+1, num_columns);
   
  gtk_table_attach(GTK_TABLE(ret->widget),
                   ret->subtitle_overlay_to_video,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  row++;

  bg_gtk_plugin_widget_single_attach(ret->subtitle_overlay_encoder,
                                     ret->widget,
                                     &row, &num_columns);
  
  bg_gtk_plugin_widget_single_attach(ret->video_encoder,
                                     ret->widget,
                                     &row, &num_columns);
  
  encoder_widget_update_sensitive(ret);
  gtk_widget_show(ret->widget);
  }

void encoder_widget_set_from_registry(encoder_widget_t * wid,
                                      bg_plugin_registry_t * plugin_reg)
  {
  bg_transcoder_encoder_info_t info;
  bg_transcoder_encoder_info_get_from_registry(plugin_reg, &info);
  encoder_widget_get(wid, &info);
  encoder_widget_update_sensitive(wid);
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
  bg_transcoder_encoder_info_t info;
  bg_transcoder_track_t * first_selected;

  /* Get the first selected track */

  first_selected = win->tracks;

  while(!first_selected->selected)
    {
    first_selected = first_selected->next;
    }

  bg_transcoder_encoder_info_get_from_track(win->encoder_widget.plugin_reg,
                                            first_selected,
                                            &info);
  
  bg_transcoder_encoder_info_get_sections_from_track(first_selected,
                                                     &info);
  
  encoder_widget_get(&win->encoder_widget, &info);
  encoder_widget_update_sensitive(&win->encoder_widget);
  }

#define COPY_SECTION(sec) if(info.sec) info.sec = bg_cfg_section_copy(info.sec)
#define FREE_SECTION(sec) if(info.sec) free(info.sec)

static void encoder_window_apply(encoder_window_t * win)
  {
  bg_transcoder_track_t * track;

  bg_transcoder_encoder_info_t info;
  memset(&info, 0, sizeof(info));
  
  track = win->tracks;

  info.video_info = bg_gtk_plugin_widget_single_get_plugin(win->encoder_widget.video_encoder);
  info.video_encoder_section =
    bg_gtk_plugin_widget_single_get_section(win->encoder_widget.video_encoder);
  info.video_stream_section =
    bg_gtk_plugin_widget_single_get_video_section(win->encoder_widget.video_encoder);

  
  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->encoder_widget.audio_to_video)) ||
     (!info.video_info->max_audio_streams))
    {
    info.audio_info = bg_gtk_plugin_widget_single_get_plugin(win->encoder_widget.audio_encoder);
    info.audio_encoder_section =
      bg_gtk_plugin_widget_single_get_section(win->encoder_widget.audio_encoder);
    info.audio_stream_section =
      bg_gtk_plugin_widget_single_get_audio_section(win->encoder_widget.audio_encoder);
    }
  else
    {
    info.audio_stream_section =
      bg_gtk_plugin_widget_single_get_audio_section(win->encoder_widget.video_encoder);
    }
  
  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->encoder_widget.subtitle_text_to_video)) ||
     (!info.video_info->max_subtitle_text_streams))
    {
    info.subtitle_text_info =
      bg_gtk_plugin_widget_single_get_plugin(win->encoder_widget.subtitle_text_encoder);
    info.subtitle_text_encoder_section =
      bg_gtk_plugin_widget_single_get_section(win->encoder_widget.subtitle_text_encoder);
    info.subtitle_text_stream_section =
      bg_gtk_plugin_widget_single_get_subtitle_text_section(win->encoder_widget.subtitle_text_encoder);
    }
  else
    {
    info.subtitle_text_stream_section =
      bg_gtk_plugin_widget_single_get_subtitle_text_section(win->encoder_widget.video_encoder);
    }

  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->encoder_widget.subtitle_overlay_to_video)) ||
     (!info.video_info->max_subtitle_overlay_streams))
    {
    info.subtitle_overlay_info =
      bg_gtk_plugin_widget_single_get_plugin(win->encoder_widget.subtitle_overlay_encoder);
    info.subtitle_overlay_encoder_section =
      bg_gtk_plugin_widget_single_get_section(win->encoder_widget.subtitle_overlay_encoder);
    info.subtitle_overlay_stream_section =
      bg_gtk_plugin_widget_single_get_subtitle_overlay_section(win->encoder_widget.subtitle_overlay_encoder);
    }
  else
    {
    info.subtitle_overlay_stream_section =
      bg_gtk_plugin_widget_single_get_subtitle_overlay_section(win->encoder_widget.video_encoder);
    }

  /* The sections come either out of the registry or from the first transcoder track.
     Now, we need to make local copies of all sections to avoid some crashes */
  
  COPY_SECTION(audio_encoder_section);
  COPY_SECTION(video_encoder_section);
  COPY_SECTION(subtitle_text_encoder_section);
  COPY_SECTION(subtitle_overlay_encoder_section);
  COPY_SECTION(audio_stream_section);
  COPY_SECTION(video_stream_section);
  COPY_SECTION(subtitle_text_stream_section);
  COPY_SECTION(subtitle_overlay_stream_section);
  
  while(track)
    {
    if(track->selected)
      {
      bg_transcoder_track_set_encoders(track, win->encoder_widget.plugin_reg,
                                       &info);
      }
    track = track->next;
    }

  FREE_SECTION(audio_encoder_section);
  FREE_SECTION(video_encoder_section);
  FREE_SECTION(subtitle_text_encoder_section);
  FREE_SECTION(subtitle_overlay_encoder_section);
  FREE_SECTION(audio_stream_section);
  FREE_SECTION(video_stream_section);
  FREE_SECTION(subtitle_text_stream_section);
  FREE_SECTION(subtitle_overlay_stream_section);
  }

#undef COPY_SECTION
#undef FREE_SECTION

static void window_button_callback(GtkWidget * w, gpointer data)
  {
  encoder_window_t * win = (encoder_window_t *)data;

  if((w == win->close_button) || (w == win->window))
    {
    gtk_widget_hide(win->window);
    gtk_main_quit();
    }
  if(w == win->apply_button)
    {
    encoder_window_apply(win);
    }
  if(w == win->ok_button)
    {
    encoder_window_apply(win);
    gtk_widget_hide(win->window);
    gtk_main_quit();
    }
  
  }

static gboolean delete_callback(GtkWidget * w, GdkEvent * evt, gpointer data)
  {
  window_button_callback(w, data);
  return TRUE;
  }

static void set_video_encoder(const bg_plugin_info_t * info, void * data)
  {
  encoder_window_t * w = (encoder_window_t*)data;
  encoder_widget_update_sensitive(&w->encoder_widget);
  }


encoder_window_t * encoder_window_create(bg_plugin_registry_t * plugin_reg)
  {
  GtkWidget * mainbox;
  GtkWidget * buttonbox;
    
  encoder_window_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   ret);
  
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

  bg_gtk_plugin_widget_single_set_change_callback(ret->encoder_widget.video_encoder,
                                                  set_video_encoder, ret);

  
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
