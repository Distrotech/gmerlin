/*****************************************************************
 
  playermsg.h
 
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

#ifndef __BG_PLAYERMSG_H_
#define __BG_PLAYERMSG_H_

/** \defgroup player_states Player states
 *  \ingroup player_msg
 *  \brief State definitions for the player
 *
 *  @{
*/

#define BG_PLAYER_STATE_INIT      -1 //!< Initializing
#define BG_PLAYER_STATE_STOPPED   0 //!< Stopped, waiting for play command
#define BG_PLAYER_STATE_PLAYING   1 //!< Playing
#define BG_PLAYER_STATE_SEEKING   2 //!< Seeking
#define BG_PLAYER_STATE_CHANGING  3 //!< Changing the track
#define BG_PLAYER_STATE_BUFFERING 4 //!< Buffering data
#define BG_PLAYER_STATE_PAUSED    5 //!< Paused
#define BG_PLAYER_STATE_FINISHING 6 //!< Finishing playback
#define BG_PLAYER_STATE_STARTING  7 //!< Starting playback
#define BG_PLAYER_STATE_ERROR     8 //!< Error

/**
 *  @}
 */

/* Message definition for the player */

/****************************
 *  Commands for the player
 ****************************/

/* Start playing                                        */
/* arg1: Input plugin handle                            */
/* arg2: Track index for plugins with multiple tracks   */
/* arg3: Play flags, see defines below                  */

#define BG_PLAYER_CMD_PLAY     0

/* Stop playing                              */

#define BG_PLAYER_CMD_STOP     1

/* Seek to a specific point                  */
/* arg1: seek Perfenctage                    */
/* (between 0.0 and 1.0, float)              */ 

#define BG_PLAYER_CMD_SEEK     2

/* Set the state of the player */
/*  arg1: New state             */

/*
 *  if(state == BG_PLAYER_STATE_BUFFERING)
 *    arg2: Buffering percentage (float)
 *  else if(state == BG_PLAYER_STATE_ERROR)
 *    arg2: String describing the error
 *  else if(state == BG_PLAYER_STATE_PLAYING)
 *    arg2: Integer (1 if player can seek withing the current track)
 */

#define BG_PLAYER_CMD_SETSTATE 3

/* Quit playback thread (used by bg_player_quit()) */

#define BG_PLAYER_CMD_QUIT      4

/* Change output plugins, arg1 is plugin handle of the opened plugin */

#define BG_PLAYER_CMD_SET_OV_PLUGIN 5

#define BG_PLAYER_CMD_SET_OA_PLUGIN 6

/* Set track name */

#define BG_PLAYER_CMD_SET_NAME      7

/* Act like a pause button */

#define BG_PLAYER_CMD_PAUSE         8

/*
 *  Seek relative (gavl_time_t)
 */

#define BG_PLAYER_CMD_SEEK_REL      10

/* Set volume (float dB value) */

#define BG_PLAYER_CMD_SET_VOLUME     11

/* Set volume (float dB value) */

#define BG_PLAYER_CMD_SET_VOLUME_REL 12

/* Like BG_PLAYER_CMD_PLAY but go into the pause state right after the
   playback is set up */

#define BG_PLAYER_CMD_PLAY_PAUSE     13

#define BG_PLAYER_CMD_SET_AUDIO_STREAM    14
#define BG_PLAYER_CMD_SET_VIDEO_STREAM    15
#define BG_PLAYER_CMD_SET_SUBTITLE_STREAM 16

/* Argument 1: keycode (see keycodes.h)
   Argument 2: mask (see keycodes.h)
*/

#define BG_PLAYER_CMD_KEY                 17 /* A key was pressed */

#define BG_PLAYER_CMD_CHANGE              18 /* Player should prepare for changing the
                                                track */

#define BG_PLAYER_CMD_TOGGLE_MUTE         19 /* Toggle mute state */

#define BG_PLAYER_CMD_SET_CHAPTER         20 /* Goto chapter */

#define BG_PLAYER_CMD_NEXT_CHAPTER        21 /* Next chapter */

#define BG_PLAYER_CMD_PREV_CHAPTER        22 /* Previous chapter */

#define BG_PLAYER_CMD_INTERRUPT           23 /* Interrupt playback */
#define BG_PLAYER_CMD_INTERRUPT_RESUME    24 /* Resume interrupted playback */

/********************************
 * Messages from the player
********************************/

/** \defgroup player_msg Messages from the player
 *  \ingroup player
*
 *  @{
 */

/** \brief Display time changed
 *
 *  arg0: New time (gavl_time_t)
 *
 *  This is called periodically during playback if the time changed.
 */

#define BG_PLAYER_MSG_TIME_CHANGED            0

/** \brief Track changed
 *
 *  arg0: Track index (int)
 *
 *  This message is only emitted for input plugins,
 *  which do playback themselves.
 */

#define BG_PLAYER_MSG_TRACK_CHANGED           1

/** \brief State changed
 *
 *  arg0: New state (\ref player_states)
 *
 *  arg1 depends on the state:
 *
 *  - BG_PLAYER_STATE_BUFFERING: Buffering percentage (float, 0.0..1.0)
 *  - BG_PLAYER_STATE_ERROR: String describing the error (char*)
 *  - BG_PLAYER_STATE_PLAYING: 1 if player can seek within the current track, 0 else (int)
 *  - BG_PLAYER_STATE_CHANGING: 1 if player needs the next track, 0 else
 */

#define BG_PLAYER_MSG_STATE_CHANGED           2

/** \brief Track name
 *
 *  arg0: Track name (char*)
 *
 *  This is set whenever the track name changes. For internet radio stations, it can be
 *  sent multiple times for one URL.
 */

#define BG_PLAYER_MSG_TRACK_NAME              3

/** \brief Duration changed
 *
 *  arg0: Total duration in seconds (gavl_time_t)
 */

#define BG_PLAYER_MSG_TRACK_DURATION          5

/** \brief Get info about the streams
 *
 *  arg0: Number of audio streams (int)
 *
 *  arg1: Number of video streams (int)
 *
 *  arg2: Number of subtitle streams (int)
 */

#define BG_PLAYER_MSG_TRACK_NUM_STREAMS       4

/** \brief Get information about the current audio stream
 *
 *  arg0: Stream index (int)
 *
 *  arg1: Input Format (gavl_audio_format_t)
 *
 *  arg2: Output Format (gavl_audio_format_t)
 */

#define BG_PLAYER_MSG_AUDIO_STREAM            6

/** \brief Get information about the current video stream
 *
 *  arg0: Stream index (int)
 *
 *  arg1: Input Format (gavl_video_format_t)
 *
 *  arg2: Output Format (gavl_video_format_t)
 */

#define BG_PLAYER_MSG_VIDEO_STREAM            7


/** \brief Get information about the current subtitle stream
 *
 *  arg0: Stream index (int)
 *
 *  arg1: 1 if the subtitle is a text subtitle, 0 else
 *
 *  arg2: Format (gavl_video_format_t)
 */

#define BG_PLAYER_MSG_SUBTITLE_STREAM         9

/* Metadata (is only sent, if information is available) */

/** \brief Metadata changed
 *
 *  arg0: Metadata (bg_metadata_t)
 */

#define BG_PLAYER_MSG_METADATA               10

/** \brief Audio description changed
 *
 *  arg0: Audio description (char*)
 */

#define BG_PLAYER_MSG_AUDIO_DESCRIPTION      11

/** \brief Video description changed
 *
 *  arg0: Video description (char*)
 */
#define BG_PLAYER_MSG_VIDEO_DESCRIPTION      12


/** \brief Subtitle description changed
 *
 *  arg0: Subtitle description (char*)
 */

#define BG_PLAYER_MSG_SUBTITLE_DESCRIPTION   14

/** \brief Description of the track changed
 *
 *  arg0: Stream description (char*)
 */
#define BG_PLAYER_MSG_STREAM_DESCRIPTION     15

/** \brief Volume changed
 *
 *  arg0: New volume in dB (float)
 */
#define BG_PLAYER_MSG_VOLUME_CHANGED         16

/** \brief Audio stream info
 *
 *  arg0: stream index (int)
 *
 *  arg1: stream name (char*)
 *
 *  arg2: stream language (char*)
 *
 *  This message is sent for all available audio streams
 *  regardless of what you selected
 */

#define BG_PLAYER_MSG_AUDIO_STREAM_INFO      17

/** \brief Video stream info
 *
 *  arg0: stream index (int)
 *
 *  arg1: stream name (char*)
 *
 *  arg2: stream language (char*)
 *
 *  This message is sent for all available video streams
 *  regardless of what you selected
 */

#define BG_PLAYER_MSG_VIDEO_STREAM_INFO      18

/** \brief Subtitle stream info
 *
 *  arg0: stream index (int)
 *
 *  arg1: stream name (char*)
 *
 *  arg2: stream language (char*)
 *
 *  This message is sent for all available video streams
 *  regardless of what you selected
 */
#define BG_PLAYER_MSG_SUBTITLE_STREAM_INFO   19

/** \brief A key was pressed in the video window
 *
 *  arg0: keycode (see \ref keycodes)
 *
 *  arg1: mask (see \ref keycodes)
 *
 *  This message is only emitted if key+mask were not handled
 *  by the video plugin or by the player.
 */

#define BG_PLAYER_MSG_KEY                    20 /* A key was pressed */

/** \brief Player just cleaned up
 *
 *  A previously triggerend cleanup operation is finished.
 */

#define BG_PLAYER_MSG_CLEANUP                21

/** \brief Player changed the mute state
 *
 *  arg0: 1 when player is muted now, 0 else
 *
 */

#define BG_PLAYER_MSG_MUTE                   22

/** \brief Number of chapters
 *
 *  arg0: Number
 */

#define BG_PLAYER_MSG_NUM_CHAPTERS           23

/** \brief Chapter info
 *
 *  arg0: Chapter index
 *  arg1: Name (string)
 *  arg2: Start time (time)
 */

#define BG_PLAYER_MSG_CHAPTER_INFO           24

/** \brief Chapter changed
 *
 *  arg0: Chapter index
 */

#define BG_PLAYER_MSG_CHAPTER_CHANGED        25

/** \brief Playback interrupted
 */

#define BG_PLAYER_MSG_INTERRUPT              26

/** \brief Interrupted playback resumed 
 */

#define BG_PLAYER_MSG_INTERRUPT_RESUME       27

/** \brief Input info 
 *  arg0: Plugin name (string)
 *  arg1: Location (string)
 *  arg2: Track (int)
 */

#define BG_PLAYER_MSG_INPUT                  28


/**  @}
 */

#endif // __BG_PLAYERMSG_H_
