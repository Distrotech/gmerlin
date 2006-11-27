/*****************************************************************
 
  remote.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

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
      locations[1] = (char*)0;
      gmerlin_add_locations(g, locations);
      free(arg_str);
      
      break;
      
    case PLAYER_COMMAND_PLAY_LOCATION:
      arg_str = bg_msg_get_arg_string(msg, 0);
      locations[0] = arg_str;
      locations[1] = (char*)0;
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
    }
  }
