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

#include <playermsg.h>
#include <pluginregistry.h>
#include <msgqueue.h>

#define BG_PLAYER_VOLUME_MIN (-40.0)

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

void bg_player_key_pressed(bg_player_t * p, int keycode, int mask);

/*
 *  Thread save functions for controlling the player (see playercmd.c)
 *  These just create messages and send them into the command queue
 */

void bg_player_play(bg_player_t *, bg_plugin_handle_t * handle,
                    int track, int flags, const char * track_name);

void bg_player_play_pause(bg_player_t *, bg_plugin_handle_t * handle,
                          int track, int ignore_flags, const char * track_name);

void bg_player_seek(bg_player_t *, gavl_time_t time);

void bg_player_seek_rel(bg_player_t *, gavl_time_t time);

void bg_player_set_volume(bg_player_t *, float volume);
void bg_player_set_volume_rel(bg_player_t *, float volume);


void bg_player_set_track(bg_player_t * p, int track);
void bg_player_set_duration(bg_player_t * p, gavl_time_t duration, int can_seek);

void bg_player_stop(bg_player_t *);
void bg_player_stop_sync(bg_player_t *);

void bg_player_pause(bg_player_t *);

void bg_player_error(bg_player_t *, const char * message);

void bg_player_set_oa_plugin(bg_player_t *, bg_plugin_handle_t * handle);
void bg_player_set_ov_plugin(bg_player_t *, bg_plugin_handle_t * handle);

void bg_player_set_audio_stream(bg_player_t *, int);
void bg_player_set_video_stream(bg_player_t *, int);
void bg_player_set_subtitle_stream(bg_player_t *, int);

void bg_player_set_track_name(bg_player_t *, const char *);
void bg_player_set_metadata(bg_player_t *, const bg_metadata_t *);

bg_parameter_info_t * bg_player_get_audio_parameters(bg_player_t *);
void bg_player_set_audio_parameter(void*, char *, bg_parameter_value_t*);

bg_parameter_info_t * bg_player_get_video_parameters(bg_player_t *);
void bg_player_set_video_parameter(void*, char *, bg_parameter_value_t*);

bg_parameter_info_t * bg_player_get_subtitle_parameters(bg_player_t *);
void bg_player_set_subtitle_parameter(void*, char *, bg_parameter_value_t*);

#endif // __BG_PLAYER_H_
