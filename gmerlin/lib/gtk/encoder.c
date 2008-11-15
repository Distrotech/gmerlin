/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <config.h>
#include <string.h>

#include <gmerlin/translation.h>

#include <gtk/gtk.h>
#include <gui_gtk/plugin.h>

struct bg_gtk_encoder_widget_s
  {
  bg_gtk_plugin_widget_single_t * video_encoder;
  bg_gtk_plugin_widget_single_t * audio_encoder;
  
  bg_gtk_plugin_widget_single_t * subtitle_text_encoder;
  bg_gtk_plugin_widget_single_t * subtitle_overlay_encoder;
  
  GtkWidget * audio_to_video;
  GtkWidget * subtitle_text_to_video;
  GtkWidget * subtitle_overlay_to_video;
  
  /* Signal handler IDs */
  guint audio_to_video_id;
  guint subtitle_text_to_video_id;
  guint subtitle_overlay_to_video_id;
  
  GtkWidget * widget;
  
  bg_plugin_registry_t * plugin_reg;
  int flags;
  };


static void encoder_widget_get(bg_gtk_encoder_widget_t * wid, const bg_encoder_info_t * info,
                               int copy_sections)
  {
  bg_gtk_plugin_widget_single_set_plugin(wid->video_encoder,
                                         info->video_info);
  
  bg_gtk_plugin_widget_single_set_section(wid->video_encoder,
                                          info->video_encoder_section, copy_sections);
  
  bg_gtk_plugin_widget_single_set_video_section(wid->video_encoder,
                                                info->video_stream_section, copy_sections);
  
  if(info->audio_info)
    {
    g_signal_handler_block(G_OBJECT(wid->audio_to_video), wid->audio_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->audio_to_video), 0);
    g_signal_handler_unblock(G_OBJECT(wid->audio_to_video), wid->audio_to_video_id);

    bg_gtk_plugin_widget_single_set_plugin(wid->audio_encoder,
                                           info->audio_info);
    bg_gtk_plugin_widget_single_set_section(wid->audio_encoder,
                                            info->audio_encoder_section, copy_sections);
    bg_gtk_plugin_widget_single_set_audio_section(wid->audio_encoder,
                                                  info->audio_stream_section, copy_sections);
    }
  else
    {
    g_signal_handler_block(G_OBJECT(wid->audio_to_video), wid->audio_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->audio_to_video), 1);
    g_signal_handler_unblock(G_OBJECT(wid->audio_to_video), wid->audio_to_video_id);

    bg_gtk_plugin_widget_single_set_audio_section(wid->video_encoder,
                                                  info->audio_stream_section, copy_sections);
    }

  if(info->subtitle_text_info)
    {
    g_signal_handler_block(G_OBJECT(wid->subtitle_text_to_video), wid->subtitle_text_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->subtitle_text_to_video), 0);
    g_signal_handler_unblock(G_OBJECT(wid->subtitle_text_to_video), wid->subtitle_text_to_video_id);

    bg_gtk_plugin_widget_single_set_plugin(wid->subtitle_text_encoder,
                                           info->subtitle_text_info);
    bg_gtk_plugin_widget_single_set_section(wid->subtitle_text_encoder,
                                            info->subtitle_text_encoder_section, copy_sections);
    bg_gtk_plugin_widget_single_set_subtitle_text_section(wid->subtitle_text_encoder,
                                                          info->subtitle_text_stream_section, copy_sections);
    }
  else
    {
    g_signal_handler_block(G_OBJECT(wid->subtitle_text_to_video), wid->subtitle_text_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->subtitle_text_to_video), 1);
    g_signal_handler_unblock(G_OBJECT(wid->subtitle_text_to_video), wid->subtitle_text_to_video_id);

    bg_gtk_plugin_widget_single_set_subtitle_text_section(wid->video_encoder,
                                                          info->subtitle_text_stream_section, copy_sections);
    }

  if(info->subtitle_overlay_info)
    {
    g_signal_handler_block(G_OBJECT(wid->subtitle_overlay_to_video), wid->subtitle_overlay_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->subtitle_overlay_to_video), 0);
    g_signal_handler_unblock(G_OBJECT(wid->subtitle_overlay_to_video), wid->subtitle_overlay_to_video_id);

    bg_gtk_plugin_widget_single_set_plugin(wid->subtitle_overlay_encoder,
                                           info->subtitle_overlay_info);
    bg_gtk_plugin_widget_single_set_section(wid->subtitle_overlay_encoder,
                                            info->subtitle_overlay_encoder_section, copy_sections);
    bg_gtk_plugin_widget_single_set_subtitle_overlay_section(wid->subtitle_overlay_encoder,
                                                          info->subtitle_overlay_stream_section, copy_sections);
    }
  else
    {
    g_signal_handler_block(G_OBJECT(wid->subtitle_overlay_to_video), wid->subtitle_overlay_to_video_id);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid->subtitle_overlay_to_video), 1);
    g_signal_handler_unblock(G_OBJECT(wid->subtitle_overlay_to_video), wid->subtitle_overlay_to_video_id);

    bg_gtk_plugin_widget_single_set_subtitle_overlay_section(wid->video_encoder,
                                                             info->subtitle_overlay_stream_section, copy_sections);
    }
  }

void bg_gtk_encoder_widget_get(bg_gtk_encoder_widget_t * wid, const bg_encoder_info_t * info)
  {
  encoder_widget_get(wid, info, 1);
  }

void bg_gtk_encoder_widget_set(bg_gtk_encoder_widget_t * wid, bg_encoder_info_t * info)
  {
  memset(info, 0, sizeof(*info));
  info->video_info = bg_gtk_plugin_widget_single_get_plugin(wid->video_encoder);
  info->video_encoder_section =
    bg_gtk_plugin_widget_single_get_section(wid->video_encoder);
  info->video_stream_section =
    bg_gtk_plugin_widget_single_get_video_section(wid->video_encoder);

  
  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid->audio_to_video)) ||
     (!info->video_info->max_audio_streams))
    {
    info->audio_info = bg_gtk_plugin_widget_single_get_plugin(wid->audio_encoder);
    info->audio_encoder_section =
      bg_gtk_plugin_widget_single_get_section(wid->audio_encoder);
    info->audio_stream_section =
      bg_gtk_plugin_widget_single_get_audio_section(wid->audio_encoder);
    }
  else
    {
    info->audio_stream_section =
      bg_gtk_plugin_widget_single_get_audio_section(wid->video_encoder);
    }
  
  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid->subtitle_text_to_video)) ||
     (!info->video_info->max_subtitle_text_streams))
    {
    info->subtitle_text_info =
      bg_gtk_plugin_widget_single_get_plugin(wid->subtitle_text_encoder);
    info->subtitle_text_encoder_section =
      bg_gtk_plugin_widget_single_get_section(wid->subtitle_text_encoder);
    info->subtitle_text_stream_section =
      bg_gtk_plugin_widget_single_get_subtitle_text_section(wid->subtitle_text_encoder);
    }
  else
    {
    info->subtitle_text_stream_section =
      bg_gtk_plugin_widget_single_get_subtitle_text_section(wid->video_encoder);
    }

  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid->subtitle_overlay_to_video)) ||
     (!info->video_info->max_subtitle_overlay_streams))
    {
    info->subtitle_overlay_info =
      bg_gtk_plugin_widget_single_get_plugin(wid->subtitle_overlay_encoder);
    info->subtitle_overlay_encoder_section =
      bg_gtk_plugin_widget_single_get_section(wid->subtitle_overlay_encoder);
    info->subtitle_overlay_stream_section =
      bg_gtk_plugin_widget_single_get_subtitle_overlay_section(wid->subtitle_overlay_encoder);
    }
  else
    {
    info->subtitle_overlay_stream_section =
      bg_gtk_plugin_widget_single_get_subtitle_overlay_section(wid->video_encoder);
    }
  
  }

void bg_gtk_encoder_widget_update_sensitive(bg_gtk_encoder_widget_t * win)
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
  bg_gtk_encoder_widget_t * wid = (bg_gtk_encoder_widget_t *)data;

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

bg_gtk_encoder_widget_t * bg_gtk_encoder_widget_create(bg_plugin_registry_t * plugin_reg,
                                                       int flags)
  {
  int row, num_columns;
  bg_gtk_encoder_widget_t * ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = plugin_reg;

  ret->audio_encoder =
    bg_gtk_plugin_widget_single_create("Audio", plugin_reg,
                                       BG_PLUGIN_ENCODER_AUDIO,
                                       BG_PLUGIN_FILE);

  ret->subtitle_text_encoder =
    bg_gtk_plugin_widget_single_create("Text subtitles", plugin_reg,
                                       BG_PLUGIN_ENCODER_SUBTITLE_TEXT,
                                       BG_PLUGIN_FILE);

  ret->subtitle_overlay_encoder =
    bg_gtk_plugin_widget_single_create("Overlay subtitles", plugin_reg,
                                       BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY,
                                       BG_PLUGIN_FILE);

  ret->audio_to_video =
    gtk_check_button_new_with_label(TR("Encode audio into video file"));
  ret->subtitle_text_to_video =
    gtk_check_button_new_with_label(TR("Encode text subtitles into video file"));
  ret->subtitle_overlay_to_video =
    gtk_check_button_new_with_label(TR("Encode overlay subtitles into video file"));
  
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
                                       BG_PLUGIN_FILE);
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
  
  bg_gtk_encoder_widget_update_sensitive(ret);
  gtk_widget_show(ret->widget);
  return ret;
  }

void bg_gtk_encoder_widget_set_from_registry(bg_gtk_encoder_widget_t * wid,
                                      bg_plugin_registry_t * plugin_reg)
  {
  bg_encoder_info_t info;
  bg_encoder_info_get_from_registry(plugin_reg, &info);
  encoder_widget_get(wid, &info, 0);
  bg_gtk_encoder_widget_update_sensitive(wid);
  }

void bg_gtk_encoder_widget_destroy(bg_gtk_encoder_widget_t * wid)
  {
  bg_gtk_plugin_widget_single_destroy(wid->audio_encoder);
  bg_gtk_plugin_widget_single_destroy(wid->video_encoder);
  free(wid);
  }

void bg_gtk_encoder_widget_set_audio_change_callback(bg_gtk_encoder_widget_t * w,
                                                     void (*set_plugin)(const bg_plugin_info_t * plugin,
                                                                        void * data),
                                                     void * set_plugin_data)
  {
  bg_gtk_plugin_widget_single_set_change_callback(w->audio_encoder, set_plugin, set_plugin_data);
  }

void bg_gtk_encoder_widget_set_video_change_callback(bg_gtk_encoder_widget_t * w,
                                                     void (*set_plugin)(const bg_plugin_info_t * plugin,
                                                                        void * data),
                                                     void * set_plugin_data)
  {
  bg_gtk_plugin_widget_single_set_change_callback(w->video_encoder, set_plugin, set_plugin_data);
  }

void bg_gtk_encoder_widget_set_subtitle_text_change_callback(bg_gtk_encoder_widget_t * w,
                                                             void (*set_plugin)(const bg_plugin_info_t * plugin,
                                                                                void * data),
                                                             void * set_plugin_data)
  {
  bg_gtk_plugin_widget_single_set_change_callback(w->subtitle_text_encoder, set_plugin, set_plugin_data);
  }

void bg_gtk_encoder_widget_set_subtitle_overlay_change_callback(bg_gtk_encoder_widget_t * w,
                                                                void (*set_plugin)(const bg_plugin_info_t * plugin,
                                                                                   void * data),
                                                                void * set_plugin_data)
  {
  bg_gtk_plugin_widget_single_set_change_callback(w->subtitle_overlay_encoder, set_plugin, set_plugin_data);
  }


GtkWidget * bg_gtk_encoder_widget_get_widget(bg_gtk_encoder_widget_t * w)
  {
  return w->widget;
  }
