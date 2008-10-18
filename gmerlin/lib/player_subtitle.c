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


#include <string.h>
#include <stdio.h>

#include <gmerlin/player.h>
#include <playerprivate.h>

// #define NUM_SUBTITLE_FRAMES 2 /* How many overlays are in the fifo */

void bg_player_subtitle_create(bg_player_t * p)
  {
  p->subtitle_stream.cnv = gavl_video_converter_create();
  p->subtitle_stream.renderer = bg_text_renderer_create();
  pthread_mutex_init(&(p->subtitle_stream.config_mutex),(pthread_mutexattr_t *)0);

  }

void bg_player_subtitle_destroy(bg_player_t * p)
  {
  gavl_video_converter_destroy(p->subtitle_stream.cnv);
  pthread_mutex_destroy(&(p->subtitle_stream.config_mutex));
  bg_text_renderer_destroy(p->subtitle_stream.renderer);
  }

int bg_player_subtitle_init(bg_player_t * player, int subtitle_stream)
  {
  bg_player_subtitle_stream_t * s;

  if(!DO_SUBTITLE(player->flags))
    return 1;
  
  s = &(player->subtitle_stream);

  bg_player_input_get_subtitle_format(player->input_context);
  
  if(DO_SUBTITLE_TEXT(player->flags))
    {
    pthread_mutex_lock(&(player->subtitle_stream.config_mutex));
    if(DO_SUBTITLE_ONLY(player->flags))
      {
      bg_text_renderer_init(player->subtitle_stream.renderer,
                            (gavl_video_format_t*)0,
                            &(player->subtitle_stream.input_format));
      
      bg_text_renderer_get_frame_format(player->subtitle_stream.renderer,
                                        &(player->video_stream.input_format));
      gavl_video_format_copy(&(player->video_stream.output_format),
                             &(player->video_stream.input_format));
      
      
      }
    else
      {
      bg_text_renderer_init(player->subtitle_stream.renderer,
                            &(player->video_stream.output_format),
                            &(player->subtitle_stream.input_format));
      }
    pthread_mutex_unlock(&(player->subtitle_stream.config_mutex));
    }
  else
    {
    if(DO_SUBTITLE_ONLY(player->flags))
      {
      gavl_video_format_copy(&(player->video_stream.input_format),
                             &player->subtitle_stream.input_format);
      gavl_video_format_copy(&(player->video_stream.output_format),
                             &(player->video_stream.input_format));
      }
    }
  
  /* Initialize subtitle fifo */
  
  if(!DO_SUBTITLE_ONLY(player->flags))
    {
    /* Video output already initialized */
    bg_player_ov_set_subtitle_format(player->ov_context);
    bg_player_subtitle_init_converter(player);
    }
  
  return 1;
  }

void bg_player_subtitle_cleanup(bg_player_t * player)
  {
  if(player->subtitle_stream.fifo)
    {
    bg_fifo_destroy(player->subtitle_stream.fifo,
                    bg_player_ov_destroy_subtitle_overlay, player->ov_context);
    player->subtitle_stream.fifo = (bg_fifo_t*)0;
    }
  if(player->subtitle_stream.in_ovl.frame)
    {
    gavl_video_frame_destroy(player->subtitle_stream.in_ovl.frame);
    player->subtitle_stream.in_ovl.frame = (gavl_video_frame_t*)0;
    }
  }

/* Configuration stuff */

const bg_parameter_info_t * bg_player_get_subtitle_parameters(bg_player_t * p)
  {
  return bg_text_renderer_get_parameters();
  }

void bg_player_set_subtitle_parameter(void * data, const char * name,
                                      const bg_parameter_value_t * val)
  {
  bg_player_t * player = (bg_player_t*)data;

  if(!name)
    return;
  
  pthread_mutex_lock(&(player->subtitle_stream.config_mutex));
  
  bg_text_renderer_set_parameter(player->subtitle_stream.renderer,
                                 name, val);
  
  pthread_mutex_unlock(&(player->subtitle_stream.config_mutex));
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
    s->in_ovl.frame = gavl_video_frame_create(&s->input_format);

  /* Initialize FIFO */
  player->subtitle_stream.fifo =
    bg_fifo_create(NUM_SUBTITLE_FRAMES,
                   bg_player_ov_create_subtitle_overlay, player->ov_context);
  }
