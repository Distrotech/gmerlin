/*****************************************************************
 
  playercmd.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdio.h>
#include <player.h>
#include <playerprivate.h>

void bg_player_set_oa_plugin(bg_player_t * p, bg_plugin_handle_t * handle)
  {
  bg_msg_t * msg;
  msg = bg_msg_queue_lock_write(p->command_queue);

  bg_msg_set_id(msg, BG_PLAYER_CMD_SET_OA_PLUGIN);
  bg_msg_set_arg_ptr_nocopy(msg, 0, handle);
  bg_msg_queue_unlock_write(p->command_queue);
  }

void bg_player_set_ov_plugin(bg_player_t * p, bg_plugin_handle_t * handle)
  {
  bg_msg_t * msg;
  msg = bg_msg_queue_lock_write(p->command_queue);

  bg_msg_set_id(msg, BG_PLAYER_CMD_SET_OV_PLUGIN);
  bg_msg_set_arg_ptr_nocopy(msg, 0, handle);
  bg_msg_queue_unlock_write(p->command_queue);
  }

void bg_player_play(bg_player_t * p, bg_plugin_handle_t * handle,
                    int track, int ignore_flags)
  {
  bg_msg_t * msg;
  msg = bg_msg_queue_lock_write(p->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_PLAY);

  bg_msg_set_arg_ptr_nocopy(msg, 0, handle);
  bg_msg_set_arg_int(msg, 1, track);
  bg_msg_set_arg_int(msg, 2, ignore_flags);
  bg_msg_queue_unlock_write(p->command_queue);
  }

void bg_player_stop(bg_player_t * p)
  {
  bg_msg_t * msg;
  msg = bg_msg_queue_lock_write(p->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_STOP);
  bg_msg_queue_unlock_write(p->command_queue);
  
  }

void bg_player_pause(bg_player_t * p)
  {
  bg_msg_t * msg;
  msg = bg_msg_queue_lock_write(p->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_PAUSE);
  bg_msg_queue_unlock_write(p->command_queue);
  
  }

void bg_player_set_track_name(bg_player_t * p, const char * name)
  {
  bg_msg_t * msg;
  
  msg = bg_msg_queue_lock_write(p->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_SET_NAME);
  bg_msg_set_arg_string(msg, 0, name);
  bg_msg_queue_unlock_write(p->command_queue);
  }

void bg_player_seek(bg_player_t * p, float percentage)
  {
  bg_msg_t * msg;
  
  msg = bg_msg_queue_lock_write(p->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_SEEK);
  bg_msg_set_arg_float(msg, 0, percentage);
  bg_msg_queue_unlock_write(p->command_queue);
  }
