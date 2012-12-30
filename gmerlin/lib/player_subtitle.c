/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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


#include <string.h>
#include <stdio.h>

#include <gmerlin/player.h>
#include <playerprivate.h>

// #define DUMP_SUBTITLE

void bg_player_subtitle_create(bg_player_t * p)
  {
  p->subtitle_stream.renderer = bg_text_renderer_create();
  pthread_mutex_init(&p->subtitle_stream.config_mutex, NULL);
  }

void bg_player_subtitle_destroy(bg_player_t * p)
  {
  pthread_mutex_destroy(&p->subtitle_stream.config_mutex);
  bg_text_renderer_destroy(p->subtitle_stream.renderer);
  }

int bg_player_subtitle_init(bg_player_t * player)
  {
  int index, is_text;
  bg_player_subtitle_stream_t * s;

  if(!DO_SUBTITLE(player->flags))
    return 1;
  
  s = &player->subtitle_stream;
  
  index = bg_player_get_subtitle_index(player->track_info,
                                       player->current_subtitle_stream, &is_text);
  
  if(is_text)
    {
    gavl_packet_source_t * psrc =
      player->input_plugin->get_text_source(player->input_handle->priv, index);

    if(!psrc)
      {
      return 0;
      }
    gavl_packet_source_set_lock_funcs(psrc, bg_plugin_lock, bg_plugin_unlock,
                                      player->input_handle);
    
    pthread_mutex_lock(&s->config_mutex);
    if(DO_SUBTITLE_ONLY(player->flags))
      {
      s->vsrc = bg_text_renderer_connect(s->renderer,
                                         psrc,
                                         NULL,
                                         player->track_info->text_streams[index].timescale);
      
      bg_text_renderer_get_frame_format(player->subtitle_stream.renderer,
                                        &player->video_stream.input_format);
      gavl_video_format_copy(&player->video_stream.output_format,
                             &player->video_stream.input_format);
      }
    else
      {
      s->vsrc = bg_text_renderer_connect(s->renderer,
                                         psrc,
                                         &player->video_stream.output_format,
                                         player->track_info->text_streams[index].timescale);
      }
    pthread_mutex_unlock(&player->subtitle_stream.config_mutex);

    gavl_video_format_copy(&player->subtitle_stream.input_format,
                           gavl_video_source_get_src_format(s->vsrc));
    
    player->subtitle_stream.input_format.timescale =
      player->track_info->text_streams[index].timescale;
    }
  else
    {
    s->vsrc = 
      player->input_plugin->get_overlay_source(player->input_handle->priv, index);
    gavl_video_source_set_lock_funcs(s->vsrc, bg_plugin_lock, bg_plugin_unlock,
                                     player->input_handle);
    
    gavl_video_format_copy(&player->subtitle_stream.input_format,
                           &player->track_info->overlay_streams[index].format);
    
    if(DO_SUBTITLE_ONLY(player->flags))
      {
      gavl_video_format_copy(&player->video_stream.input_format,
                             &player->subtitle_stream.input_format);
      gavl_video_format_copy(&player->video_stream.output_format,
                             &player->video_stream.input_format);
      }
    }
  
  /* Initialize subtitle fifo */
  
  if(!DO_SUBTITLE_ONLY(player->flags))
    {
    /* Video output already initialized */
    bg_player_ov_set_subtitle_format(&player->video_stream);
    bg_player_subtitle_init_converter(player);
    }
  
  return 1;
  }


void bg_player_subtitle_cleanup(bg_player_t * player)
  {
  }

/* Configuration stuff */

static bg_parameter_info_t general_parameters[] =
  {
    {
      .name = "general",
      .long_name = TRS("General"),
      .type = BG_PARAMETER_SECTION,
    },
    {
      .name      = "time_offset",
      .long_name = TRS("Time offset"),
      .flags     = BG_PARAMETER_SYNC,
      .type      = BG_PARAMETER_FLOAT,
      .val_min   = { .val_f = -600.0 },
      .val_max   = { .val_f =  600.0 },
      .num_digits = 3,
    },
    { /* */ }
  };

const bg_parameter_info_t * bg_player_get_subtitle_parameters(bg_player_t * p)
  {
  if(!p->subtitle_stream.parameters)
    {
    const bg_parameter_info_t * info[3];
    info[0] = general_parameters;
    info[1] = bg_text_renderer_get_parameters();
    info[2] = NULL;
    p->subtitle_stream.parameters = bg_parameter_info_concat_arrays(info);
    }
  return p->subtitle_stream.parameters;
  }

void bg_player_set_subtitle_parameter(void * data, const char * name,
                                      const bg_parameter_value_t * val)
  {
  bg_player_t * player = (bg_player_t*)data;

  if(!name)
    return;
  
  pthread_mutex_lock(&player->subtitle_stream.config_mutex);

  if(!strcmp(name, "time_offset"))
    {
    player->subtitle_stream.time_offset =
      (int64_t)(val->val_f * GAVL_TIME_SCALE + 0.5);
    }
  else
    bg_text_renderer_set_parameter(player->subtitle_stream.renderer,
                                   name, val);
  
  pthread_mutex_unlock(&player->subtitle_stream.config_mutex);
  }

void bg_player_subtitle_init_converter(bg_player_t * player)
  {
  bg_player_subtitle_stream_t * s;
  s = &player->subtitle_stream;
  gavl_video_source_set_dst(s->vsrc, 0, &s->output_format);
  }

void bg_player_subtitle_unscale_time(bg_player_subtitle_stream_t * s, gavl_overlay_t * ovl)
  {
  ovl->timestamp =
    gavl_time_unscale(s->input_format.timescale, ovl->timestamp) +
    s->time_offset;
  ovl->duration  =
    gavl_time_unscale(s->input_format.timescale, ovl->duration);
  }

int bg_player_get_subtitle_index(bg_track_info_t * info, int stream_index, int * is_text)
  {
  if(stream_index >= info->num_overlay_streams)
    {
    if(is_text)
      *is_text = 1;
    return stream_index - info->num_overlay_streams;
    }
  else
    {
    if(is_text)
      *is_text = 0;
    return stream_index;
    }
  }
