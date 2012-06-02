/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
  p->subtitle_stream.cnv = gavl_video_converter_create();
  p->subtitle_stream.renderer = bg_text_renderer_create();
  pthread_mutex_init(&p->subtitle_stream.config_mutex, NULL);

  }

void bg_player_subtitle_destroy(bg_player_t * p)
  {
  gavl_video_converter_destroy(p->subtitle_stream.cnv);
  pthread_mutex_destroy(&p->subtitle_stream.config_mutex);
  bg_text_renderer_destroy(p->subtitle_stream.renderer);
  }

int bg_player_subtitle_init(bg_player_t * player, int subtitle_stream)
  {
  bg_player_subtitle_stream_t * s;

  if(!DO_SUBTITLE(player->flags))
    return 1;
  
  s = &player->subtitle_stream;

  bg_player_input_get_subtitle_format(player);

  
  if(DO_SUBTITLE_TEXT(player->flags))
    {
    pthread_mutex_lock(&s->config_mutex);
    if(DO_SUBTITLE_ONLY(player->flags))
      {
      bg_text_renderer_init(s->renderer,
                            NULL,
                            &player->subtitle_stream.input_format);
      
      bg_text_renderer_get_frame_format(player->subtitle_stream.renderer,
                                        &player->video_stream.input_format);
      gavl_video_format_copy(&player->video_stream.output_format,
                             &player->video_stream.input_format);
      }
    else
      {
      bg_text_renderer_init(player->subtitle_stream.renderer,
                            &player->video_stream.output_format,
                            &player->subtitle_stream.input_format);
      }
    pthread_mutex_unlock(&player->subtitle_stream.config_mutex);
    }
  else
    {
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

#define FREE_OVERLAY(ovl) if(ovl.frame) { gavl_video_frame_destroy(ovl.frame); ovl.frame = NULL; }

void bg_player_subtitle_cleanup(bg_player_t * player)
  {
  FREE_OVERLAY(player->subtitle_stream.input_subtitle);
  }

#undef FREE_OVERLAY
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
  
  s->do_convert =
    gavl_video_converter_init(s->cnv,
                              &s->input_format,
                              &s->output_format);
  if(s->do_convert)
    s->input_subtitle.frame = gavl_video_frame_create(&s->input_format);
  }

int bg_player_has_subtitle(bg_player_t * p)
  {
  int ret;
  bg_plugin_lock(p->input_handle);
  ret= p->input_plugin->has_subtitle(p->input_priv,
                                     p->current_subtitle_stream);
  bg_plugin_unlock(p->input_handle);
  return ret;
  }

int bg_player_read_subtitle(bg_player_t * p, gavl_overlay_t * ovl)
  {
  gavl_time_t start, duration;
  bg_player_subtitle_stream_t * s = &p->subtitle_stream;
  
  if(DO_SUBTITLE_TEXT(p->flags))
    {
    bg_plugin_lock(p->input_handle);
    if(!p->input_plugin->read_subtitle_text(p->input_priv,
                                            &s->buffer,
                                            &s->buffer_alloc,
                                            &start, &duration,
                                            p->current_subtitle_stream))
      {
      bg_plugin_unlock(p->input_handle);
      return 0;
      }
    bg_plugin_unlock(p->input_handle);
    if(s->do_convert)
      {
      bg_text_renderer_render(s->renderer, s->buffer, &s->input_subtitle);
      gavl_video_convert(s->cnv, s->input_subtitle.frame, ovl->frame);
      gavl_rectangle_i_copy(&ovl->ovl_rect, &s->input_subtitle.ovl_rect);
      ovl->dst_x = s->input_subtitle.dst_x;
      ovl->dst_y = s->input_subtitle.dst_y;
      }
    else
      {
      bg_text_renderer_render(s->renderer, s->buffer, ovl);
      }
    ovl->frame->timestamp = start;
    ovl->frame->duration = duration;
    }
  else
    {
    if(s->do_convert)
      {
      bg_plugin_lock(p->input_handle);
      if(!p->input_plugin->read_subtitle_overlay(p->input_priv, &s->input_subtitle,
                                                 p->current_subtitle_stream))
        {
        bg_plugin_unlock(p->input_handle);
        return 0;
        }
      bg_plugin_unlock(p->input_handle);
      gavl_video_convert(s->cnv, s->input_subtitle.frame, ovl->frame);
      gavl_rectangle_i_copy(&ovl->ovl_rect, &s->input_subtitle.ovl_rect);
      ovl->dst_x = s->input_subtitle.dst_x;
      ovl->dst_y = s->input_subtitle.dst_y;
      }
    else
      {
      if(!p->input_plugin->read_subtitle_overlay(p->input_priv, ovl,
                                                 p->current_subtitle_stream))
        {
        return 0;
        }
      }
    }

  /* Unscale the overlay times */
  ovl->frame->timestamp =
    gavl_time_unscale(s->input_format.timescale, ovl->frame->timestamp) +
    s->time_offset;
  ovl->frame->duration  =
    gavl_time_unscale(s->input_format.timescale, ovl->frame->duration);
#ifdef DUMP_SUBTITLE
  bg_dprintf("Got subtitle %f -> %f (%f)\n",
             gavl_time_to_seconds(ovl->frame->timestamp),
             gavl_time_to_seconds(ovl->frame->timestamp +
                                  ovl->frame->duration),
             gavl_time_to_seconds(ovl->frame->duration));
  
#endif
  
  return 1;
  }
