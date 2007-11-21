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

#include "playermsg.h"
#include "pluginregistry.h"
#include "msgqueue.h"

/** \defgroup player Player
 *  \brief Multimedia player
 */

#define BG_PLAYER_VOLUME_MIN (-40.0)

typedef struct bg_player_s bg_player_t;

/* player.c */

/** \ingroup player
 *  \brief Create a player
 *  \param plugin_reg A plugin registry
 *  \returns A newly allocated player
 *
 *  The plugin registry is used for loading audio- and video filters
 */

bg_player_t * bg_player_create(bg_plugin_registry_t * plugin_reg);

/** \ingroup player
 *  \brief Set accelerators
 *  \param player A newly created player 
 *  \param list A list of accelerators, terminated with BG_KEY_NONE
 *
 */

void bg_player_add_accelerators(bg_player_t * player,
                                const bg_accelerator_t * list);

/** \ingroup player
 *  \brief Destroy a player
 *  \param player A player
 */

void bg_player_destroy(bg_player_t * player);

/** \ingroup player
 *  \brief Attach a message queue to a player
 *  \param player A player
 *  \param message_queue A mesage queue
 */

void bg_player_add_message_queue(bg_player_t * player,
                                 bg_msg_queue_t * message_queue);

/** \ingroup player
 *  \brief Detach a message queue from a player
 *  \param player A player
 *  \param message_queue A mesage queue
 */

void bg_player_delete_message_queue(bg_player_t * player,
                                    bg_msg_queue_t * message_queue);

/** \ingroup player
 *  \brief Start the player thread
 *  \param player A player
 */

void bg_player_run(bg_player_t * player);

/** \ingroup player
 *  \brief Quit the player thread
 *  \param player A player
 */

void bg_player_quit(bg_player_t * player);

/*
 *  Thread save functions for controlling the player (see playercmd.c)
 *  These just create messages and send them into the command queue
 */

/** \defgroup player_cmd Commands, which can be sent to the player
 *  \ingroup player
 *
 *  Most of these are called in an aynchronous manner.
 *
 *  @{
 */


#define BG_PLAY_FLAG_IGNORE_IF_PLAYING (1<<0) //!< Ignore play command, if the player is already playing
#define BG_PLAY_FLAG_IGNORE_IF_STOPPED (1<<1) //!< Ignore play command, if the player is stopped
#define BG_PLAY_FLAG_INIT_THEN_PAUSE   (1<<2) //!< Initialize but go to pause status after
#define BG_PLAY_FLAG_RESUME            (1<<3) //!< If the player is paused, resume currently played track

/** \brief Play a track
 *  \param player A player
 *  \param handle Handle of an open input plugin
 *  \param track Track index to select (starting with 0)
 *  \param flags A combination of BG_PLAY_FLAG_* flags
 *  \param track_name Name of the track to broadcast
 *  
 */

void bg_player_play(bg_player_t * player, bg_plugin_handle_t * handle,
                    int track, int flags, const char * track_name);

/** \brief Seek to a specific time
 *  \param player A player
 *  \param time Time to seek to
 */


void bg_player_seek(bg_player_t * player, gavl_time_t time);

/** \brief Seek relative by a specific time
 *  \param player A player
 *  \param time Time offset (can be negative to seek backwards)
 */

void bg_player_seek_rel(bg_player_t * player, gavl_time_t time);

/** \brief Set the volume
 *  \param player A player
 *  \param volume Volume (in dB, max is 0.0)
 */

void bg_player_set_volume(bg_player_t * player, float volume);

/** \brief Set the volume relative
 *  \param player A player
 *  \param volume Volume offset (in dB)
 */

void bg_player_set_volume_rel(bg_player_t * player, float volume);

/** \brief Stop playback
 *  \param player A player
 */

void bg_player_stop(bg_player_t * player);

/** \brief Toggle pause
 *  \param player A player
 */

void bg_player_pause(bg_player_t * player);

/** \brief Trigger an error
 *  \param player A player
 */

void bg_player_error(bg_player_t * player);

/** \brief Set audio output plugin
 *  \param player A player
 *  \param handle A plugin handle
 */

void bg_player_set_oa_plugin(bg_player_t * player, bg_plugin_handle_t * handle);

/** \brief Set video output plugin
 *  \param player A player
 *  \param handle A plugin handle
 */

void bg_player_set_ov_plugin(bg_player_t * player, bg_plugin_handle_t * handle);

/** \brief Set audio stream
 *  \param player A player
 *  \param stream Stream index (starts with 0, -1 means no audio playback)
 */

void bg_player_set_audio_stream(bg_player_t * player, int stream);

/** \brief Set video stream
 *  \param player A player
 *  \param stream Stream index (starts with 0, -1 means no video playback)
 */

void bg_player_set_video_stream(bg_player_t * player, int stream);

/** \brief Set subtitle stream
 *  \param player A player
 *  \param stream Stream index (starts with 0, -1 means no subtitle playback)
 */

void bg_player_set_subtitle_stream(bg_player_t * player, int stream);

/** \brief Shut down playback
 *  \param player A player
 *  \param flags A combination of BG_PLAY_FLAG_* flags
 */

void bg_player_change(bg_player_t * player, int flags);

/** \brief Toggle mute
 *  \param player A player
 */

void bg_player_toggle_mute(bg_player_t * player);

/** \brief Goto a specified chapter
 *  \param player A player
 *  \param chapter Chapter index (starting with 0)
 */

void bg_player_set_chapter(bg_player_t * player, int chapter);

/** \brief Goto the next chapter
 *  \param player A player
 */

void bg_player_next_chapter(bg_player_t * player);

/** \brief Goto the previous chapter
 *  \param player A player
 */

void bg_player_prev_chapter(bg_player_t * player);

/** \brief Interrupt playback
 *  \param player A player
 *
 *  This function works synchonously, this means it
 *  is garantueed, that all playback threads are stopped
 *  until \ref bg_player_interrupt_resume is called.
 */

void bg_player_interrupt(bg_player_t * player);

/** \brief Resume an interrupted playback
 *  \param player A player
 */

void bg_player_interrupt_resume(bg_player_t * player);

/** @} */

/** \defgroup player_cfg Player configuration
 * \ingroup player
 * @{
 */

/** \brief Get input parameters
 *  \param player A player
 *  \returns Null terminated parameter array.
 *
 *  Returned parameters can be passed to \ref bg_player_set_input_parameter
 */

bg_parameter_info_t * bg_player_get_input_parameters(bg_player_t *  player);

/** \brief Set an input parameter
 *  \param data Player casted to void*
 *  \param name Name
 *  \param val Value
 */
void bg_player_set_input_parameter(void * data, const char * name,
                                   const bg_parameter_value_t * val);

/** \brief Get audio parameters
 *  \param player A player
 *  \returns Null terminated parameter array.
 *
 *  Returned parameters can be passed to \ref bg_player_set_audio_parameter
 */

bg_parameter_info_t * bg_player_get_audio_parameters(bg_player_t * player);

/** \brief Get audio filter parameters
 *  \param player A player
 *  \returns Null terminated parameter array.
 *
 *  Returned parameters can be passed to \ref bg_player_set_audio_filter_parameter
 */

bg_parameter_info_t * bg_player_get_audio_filter_parameters(bg_player_t * player);

/** \brief Set an audio parameter
 *  \param data Player casted to void*
 *  \param name Name
 *  \param val Value
 */
void bg_player_set_audio_parameter(void*data, const char * name,
                                   const bg_parameter_value_t*val);

/** \brief Set an audio filter parameter
 *  \param data Player casted to void*
 *  \param name Name
 *  \param val Value
 */
void bg_player_set_audio_filter_parameter(void*data, const char * name,
                                          const bg_parameter_value_t*val);

/** \brief Get video parameters
 *  \param player A player
 *  \returns Null terminated parameter array.
 *
 *  Returned parameters can be passed to \ref bg_player_set_video_parameter
 */
bg_parameter_info_t * bg_player_get_video_parameters(bg_player_t * player);

/** \brief Get video filter parameters
 *  \param player A player
 *  \returns Null terminated parameter array.
 *
 *  Returned parameters can be passed to \ref bg_player_set_video_parameter
 */
bg_parameter_info_t * bg_player_get_video_filter_parameters(bg_player_t * player);

/** \brief Set a video parameter
 *  \param data Player casted to void*
 *  \param name Name
 *  \param val Value
 */
void bg_player_set_video_parameter(void*data, const char * name,
                                   const bg_parameter_value_t*val);

/** \brief Set a video filter parameter
 *  \param data Player casted to void*
 *  \param name Name
 *  \param val Value
 */
void bg_player_set_video_filter_parameter(void*data, const char * name,
                                          const bg_parameter_value_t*val);

/** \brief Get subtitle parameters
 *  \param player A player
 *  \returns Null terminated parameter array.
 *
 *  Returned parameters can be passed to \ref bg_player_set_subtitle_parameter
 */
bg_parameter_info_t * bg_player_get_subtitle_parameters(bg_player_t * player);
/** \brief Set a subtitle parameter
 *  \param data Player casted to void*
 *  \param name Name
 *  \param val Value
 */
void bg_player_set_subtitle_parameter(void*data, const char * name, const bg_parameter_value_t*val);

/** \brief Get OSD parameters
 *  \param player A player
 *  \returns Null terminated parameter array.
 *
 *  Returned parameters can be passed to \ref bg_player_set_osd_parameter
 */
bg_parameter_info_t * bg_player_get_osd_parameters(bg_player_t * player);
/** \brief Set an OSD parameter
 *  \param data Player casted to void*
 *  \param name Name
 *  \param val Value
 */
void bg_player_set_osd_parameter(void*data, const char * name, const bg_parameter_value_t*val);


/** \brief En- or disable music visualizations
 *  \param p A player
 *  \param enable 1 to enable visualizations, 0 to disable them
 *
 *  Visualizations are only enabled if you passed 1 to this function
 *  and the video window is not used otherwise
 */

void
bg_player_set_visualization(bg_player_t * p, int enable);

/** \brief Set plugin used for visualizations
 *  \param p A player
 *  \param plugin_info Plugin info
 */

void
bg_player_set_visualization_plugin(bg_player_t * p, const bg_plugin_info_t * plugin_info);


/** \brief Get visualization parameters
 *  \param player A player
 *  \returns Null terminated parameter array.
 *
 *  Returned parameters can be passed to
 *  \ref bg_player_set_visualization_parameter
 */

bg_parameter_info_t *
bg_player_get_visualization_parameters(bg_player_t *  player);

/** \brief Set a visualization parameter
 *  \param data Player casted to void*
 *  \param name Name
 *  \param val Value
 */
void
bg_player_set_visualization_parameter(void*data,
                                      const char * name, const bg_parameter_value_t*val);

/** \brief Set a parameter for a visuaization plugin
 *  \param data Player casted to void*
 *  \param name Name
 *  \param val Value
 */
void
bg_player_set_visualization_plugin_parameter(void*data,
                                             const char * name,
                                             const bg_parameter_value_t*val);


/** @} */



#endif // __BG_PLAYER_H_
