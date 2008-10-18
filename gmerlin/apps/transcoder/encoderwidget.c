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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <config.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/transcoder_track.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>

#include "encoderwidget.h"



/* Encoder window */

struct encoder_window_s
  {
  bg_gtk_encoder_widget_t * encoder_widget;
  GtkWidget * window;
  GtkWidget * ok_button;
  GtkWidget * apply_button;
  GtkWidget * close_button;
  bg_plugin_registry_t * plugin_reg;
  bg_transcoder_track_t * tracks;
  };

static void encoder_window_get(encoder_window_t * win)
  {
  bg_encoder_info_t info;
  bg_transcoder_track_t * first_selected;

  /* Get the first selected track */

  first_selected = win->tracks;

  while(!first_selected->selected)
    {
    first_selected = first_selected->next;
    }

  bg_encoder_info_get_from_track(win->plugin_reg, first_selected, &info);
  
  bg_encoder_info_get_sections_from_track(first_selected, &info);
  
  bg_gtk_encoder_widget_get(win->encoder_widget, &info);
  bg_gtk_encoder_widget_update_sensitive(win->encoder_widget);
  }

#define COPY_SECTION(sec) if(info.sec) info.sec = bg_cfg_section_copy(info.sec)
#define FREE_SECTION(sec) if(info.sec) \
   bg_cfg_section_destroy(info.sec); \
   info.sec = (bg_cfg_section_t*)0;

static void encoder_window_apply(encoder_window_t * win)
  {
  bg_transcoder_track_t * track;

  bg_encoder_info_t info;
  
  track = win->tracks;

  bg_gtk_encoder_widget_set(win->encoder_widget, &info);
  
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
      bg_transcoder_track_set_encoders(track, win->plugin_reg, &info);
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
  bg_gtk_encoder_widget_update_sensitive(w->encoder_widget);
  }


encoder_window_t * encoder_window_create(bg_plugin_registry_t * plugin_reg)
  {
  GtkWidget * mainbox;
  GtkWidget * buttonbox;
    
  encoder_window_t * ret;

  ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = plugin_reg;
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   ret);
  
  gtk_window_set_title(GTK_WINDOW(ret->window), TR("Select encoders"));
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
  
  ret->encoder_widget = bg_gtk_encoder_widget_create(plugin_reg,
                                                     BG_PLUGIN_ENCODER |
                                                     BG_PLUGIN_ENCODER_AUDIO | 
                                                     BG_PLUGIN_ENCODER_VIDEO |
                                                     BG_PLUGIN_ENCODER_SUBTITLE_TEXT |
                                                     BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY );

  bg_gtk_encoder_widget_set_video_change_callback(ret->encoder_widget,
                                                  set_video_encoder, ret);
  
  /* Pack */

  buttonbox = gtk_hbutton_box_new();
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->ok_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->apply_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->close_button);
  gtk_widget_show(buttonbox);

  mainbox = gtk_vbox_new(0, 5);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), bg_gtk_encoder_widget_get_widget(ret->encoder_widget));
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
  bg_gtk_encoder_widget_destroy(win->encoder_widget);
  free(win);
  }

