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

#include "gmerlin.h"
#include "player_remote.h"

void gmerlin_handle_remote(gmerlin_t * g, bg_msg_t * msg)
  {
  int           id;
  gavl_time_t   arg_time;
  char        * arg_str;
  //  gavl_time_t   arg_time;
  float         arg_f;
  char * locations[2];
  int arg_i;
  
  id = bg_msg_get_id(msg);
  switch(id)
    {
    case PLAYER_COMMAND_PLAY:
      gmerlin_play(g, BG_PLAY_FLAG_IGNORE_IF_PLAYING | BG_PLAY_FLAG_RESUME);
      break;
    case PLAYER_COMMAND_STOP:
      bg_player_stop(g->player);
      break;
    case PLAYER_COMMAND_NEXT:
      bg_media_tree_next(g->tree, 1, g->shuffle_mode);
      gmerlin_play(g, BG_PLAY_FLAG_IGNORE_IF_STOPPED);
      break;
    case PLAYER_COMMAND_PREV:
      bg_media_tree_previous(g->tree, 1,
                             g->shuffle_mode);
      gmerlin_play(g, BG_PLAY_FLAG_IGNORE_IF_STOPPED);
      break;
    case PLAYER_COMMAND_PAUSE:
      gmerlin_pause(g);
      break;
    case PLAYER_COMMAND_ADD_LOCATION:
      arg_str = bg_msg_get_arg_string(msg, 0);
      locations[0] = arg_str;
      locations[1] = NULL;
      gmerlin_add_locations(g, locations);
      free(arg_str);
      break;
    case PLAYER_COMMAND_PLAY_LOCATION:
      arg_str = bg_msg_get_arg_string(msg, 0);
      locations[0] = arg_str;
      locations[1] = NULL;
      gmerlin_play_locations(g, locations);
      free(arg_str);
      break;
    case PLAYER_COMMAND_OPEN_DEVICE:
      arg_str = bg_msg_get_arg_string(msg, 0);
      gmerlin_open_device(g, arg_str);
      free(arg_str);
      break;
    case PLAYER_COMMAND_PLAY_DEVICE:
      arg_str = bg_msg_get_arg_string(msg, 0);
      gmerlin_play_device(g, arg_str);
      free(arg_str);
      break;
      
/* Volume control (arg: Volume in dB) */

    case PLAYER_COMMAND_SET_VOLUME:
      arg_f = bg_msg_get_arg_float(msg, 0);
      bg_player_set_volume(g->player, arg_f);
      break;
    case PLAYER_COMMAND_SET_VOLUME_REL:
      arg_f = bg_msg_get_arg_float(msg, 0);
      bg_player_set_volume_rel(g->player, arg_f);
      break;

/* Seek */

    case PLAYER_COMMAND_SEEK:
      break;
    case PLAYER_COMMAND_SEEK_REL:
      arg_time = bg_msg_get_arg_time(msg, 0);
      bg_player_seek_rel(g->player, arg_time);
      break;
/* Mute */

    case PLAYER_COMMAND_TOGGLE_MUTE:
      bg_player_toggle_mute(g->player);
      break;
      
/* Chapters */
    case PLAYER_COMMAND_SET_CHAPTER:
      arg_i = bg_msg_get_arg_int(msg, 0);
      bg_player_set_chapter(g->player, arg_i);
      break;
    case PLAYER_COMMAND_NEXT_CHAPTER:
      bg_player_next_chapter(g->player);
      break;
    case PLAYER_COMMAND_PREV_CHAPTER:
      bg_player_prev_chapter(g->player);
      break;
    case PLAYER_COMMAND_GET_METADATA:
      msg = bg_remote_server_get_msg_write(g->remote);
      bg_msg_set_id(msg, PLAYER_RESPONSE_GET_METADATA);
      /* Set Metadata */
      bg_msg_set_arg_metadata(msg, 0,
                              &g->remote_data.metadata);
      bg_remote_server_done_msg_write(g->remote);
      break;
    case PLAYER_COMMAND_GET_TIME:
      msg = bg_remote_server_get_msg_write(g->remote);
      bg_msg_set_id(msg, PLAYER_RESPONSE_GET_TIME);
      /* Set Time */
      bg_msg_set_arg_time(msg, 0, g->remote_data.time);
      bg_msg_set_arg_time(msg, 1, g->remote_data.duration);
      bg_remote_server_done_msg_write(g->remote);
      break;
    }
  }
