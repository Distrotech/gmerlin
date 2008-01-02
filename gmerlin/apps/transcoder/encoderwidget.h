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

typedef struct
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
  } encoder_widget_t;

void encoder_widget_init(encoder_widget_t * ret, bg_plugin_registry_t * plugin_reg);

void encoder_widget_destroy(encoder_widget_t * wid);

void encoder_widget_update_sensitive(encoder_widget_t * win);

void encoder_widget_set_from_registry(encoder_widget_t * w, bg_plugin_registry_t * plugin_reg);
void encoder_widget_set_from_tracks(encoder_widget_t * w, bg_transcoder_track_t * tracks);


/* Encoder window */

typedef struct encoder_window_s encoder_window_t;

encoder_window_t * encoder_window_create(bg_plugin_registry_t * plugin_reg);


void encoder_window_run(encoder_window_t * win, bg_transcoder_track_t * tracks);
void encoder_window_destroy(encoder_window_t * win);
