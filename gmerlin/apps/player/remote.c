#include "gmerlin.h"
#include "player_remote.h"

void gmerlin_handle_remote(gmerlin_t * g, bg_msg_t * msg)
  {
  int           id;
  //  int           arg_i;
  char        * arg_str;
  //  gavl_time_t   arg_time;
  float         arg_f;
  char * locations[2];
    
  id = bg_msg_get_id(msg);
  //  fprintf(stderr, "Got message %d\n", id);
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
      //      fprintf(stderr, "PLAYER_COMMAND_ADD_LOCATION: %s\n", arg_str);
      locations[0] = arg_str;
      locations[1] = (char*)0;
      gmerlin_add_locations(g, locations);
      
      break;
      
    case PLAYER_COMMAND_PLAY_LOCATION:
      arg_str = bg_msg_get_arg_string(msg, 0);
      //      fprintf(stderr, "PLAYER_COMMAND_PLAY_LOCATION: %s\n", arg_str);
      locations[0] = arg_str;
      locations[1] = (char*)0;
      gmerlin_play_locations(g, locations);
      break;

/* Volume control (arg: Volume in dB) */

    case PLAYER_COMMAND_SET_VOLUME:
      arg_f = bg_msg_get_arg_float(msg, 0);
      //      fprintf(stderr, "PLAYER_COMMAND_SET_VOLUME: %f\n", arg_f);
      bg_player_set_volume(g->player, arg_f);
      break;
    case PLAYER_COMMAND_SET_VOLUME_REL:
      arg_f = bg_msg_get_arg_float(msg, 0);
      //      fprintf(stderr, "PLAYER_COMMAND_SET_VOLUME_REL: %f\n", arg_f);
      bg_player_set_volume_rel(g->player, arg_f);
      break;

/* Seek */

    case PLAYER_COMMAND_SEEK:
    case PLAYER_COMMAND_SEEK_REL:
      break;
    }
  }
