/*****************************************************************
 
  player.h
 
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

#ifndef __BG_PLAYER_H_
#define __BG_PLAYER_H_

#include <msgqueue.h>
#include <playermsg.h>
#include <pluginregistry.h>

typedef struct bg_player_s bg_player_t;

/* player.c */

bg_player_t * bg_player_create();

void bg_player_destroy(bg_player_t * player);

bg_msg_queue_t * bg_player_get_command_queue(bg_player_t * player);

void bg_player_add_message_queue(bg_player_t * player,
                                 bg_msg_queue_t * message_queue);

void bg_player_delete_message_queue(bg_player_t * player,
                                    bg_msg_queue_t * message_queue);


void bg_player_run(bg_player_t *);

void bg_player_quit(bg_player_t *);

/*
 *  Thread save functions for controlling the player (see playercmd.c)
 *  These just create messages and send them into the command queue
 */

void bg_player_play(bg_player_t *, bg_plugin_handle_t * handle,
                    int track, int ignore_flags);

void bg_player_seek(bg_player_t *, float percentage);

void bg_player_stop(bg_player_t *);
void bg_player_stop_sync(bg_player_t *);

void bg_player_pause(bg_player_t *);

void bg_player_set_oa_plugin(bg_player_t *, bg_plugin_handle_t * handle);
void bg_player_set_ov_plugin(bg_player_t *, bg_plugin_handle_t * handle);

void bg_player_set_track_name(bg_player_t *, const char *);


#endif // __BG_PLAYER_H_
