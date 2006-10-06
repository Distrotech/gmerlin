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

/* State definitions for the player */

#define BG_PLAYER_STATE_STOPPED   0
#define BG_PLAYER_STATE_PLAYING   1
#define BG_PLAYER_STATE_SEEKING   2
#define BG_PLAYER_STATE_CHANGING  3
#define BG_PLAYER_STATE_BUFFERING 4
#define BG_PLAYER_STATE_PAUSED    5
#define BG_PLAYER_STATE_FINISHING 6
#define BG_PLAYER_STATE_STARTING  7
#define BG_PLAYER_STATE_ERROR     8
#define BG_PLAYER_STATE_STILL     9 /* Still image */

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

/********************************
 * Messages from the player
********************************/

/* Display time changed            */
/* arg1: New time in seconds (int) */

#define BG_PLAYER_MSG_TIME_CHANGED            0

/* Track changed   */
/* Arg1: new track */

#define BG_PLAYER_MSG_TRACK_CHANGED           1

/*
 *  State changed:
 *  arg1: New state (see above)
 *  If (state == BG_PLAYER_STATE_BUFFERING)
 *    arg2: Buffering percentage (float)
 *  else if(state == BG_PLAYER_STATE_ERROR)
 *    arg2: String describing the error
 *  else if(state == BG_PLAYER_STATE_PLAYING)
 *    arg2: Integer (1 if player can seek withing the current track)
 *  else if(state == BG_PLAYER_STATE_CHANGING)
 *    arg2: Integer (1 if player needs the next track)
 *
 */

#define BG_PLAYER_MSG_STATE_CHANGED           2

/*
 *  Informations about the currently playing
 *  Track.
 *  These are sent in the order given.
 *  Information, which isn't available, isn't sent.
 *  Consider the info to be finished, when the state
 *  switches to BG_PLAYER_STATE_PLAYING
 */

/* Arg1: Name (For track display) */

#define BG_PLAYER_MSG_TRACK_NAME              3

/*
 *  Arg1: Number of audio streams
 *  Arg2: Number of video streams
 *  Arg3: Number of subtitle streams
 */

#define BG_PLAYER_MSG_TRACK_NUM_STREAMS       4

/* Arg1: Total duration in seconds */

#define BG_PLAYER_MSG_TRACK_DURATION          5

/* Arg1: Stream index (int) */
/* Arg2: Input Format       */
/* Arg3: Output Format      */

#define BG_PLAYER_MSG_AUDIO_STREAM            6

/* Arg1: Stream index (int) */
/* Arg2: Input Format       */
/* Arg3: Output Format      */

#define BG_PLAYER_MSG_VIDEO_STREAM            7

/* Arg1: Stream index (int) */
/* Arg2: Input Format       */
/* Arg3: Output Format      */

#define BG_PLAYER_MSG_STILL_STREAM            8

#define BG_PLAYER_MSG_SUBTITLE_STREAM         9



/* Metadata (is only sent, if information is available) */

/* Arg1 : string */

#define BG_PLAYER_MSG_METADATA               10

#define BG_PLAYER_MSG_AUDIO_DESCRIPTION      11

#define BG_PLAYER_MSG_VIDEO_DESCRIPTION      12

#define BG_PLAYER_MSG_STILL_DESCRIPTION      13

#define BG_PLAYER_MSG_SUBTITLE_DESCRIPTION   14

#define BG_PLAYER_MSG_STREAM_DESCRIPTION     15

#define BG_PLAYER_MSG_VOLUME_CHANGED         16

/*
 * Arg 1: stream index (int)
 * Arg 2: stream name (string)
 * Arg 3: stream language (string)
 */

#define BG_PLAYER_MSG_AUDIO_STREAM_INFO      17
#define BG_PLAYER_MSG_VIDEO_STREAM_INFO      18
#define BG_PLAYER_MSG_SUBTITLE_STREAM_INFO   19

/* Argument 1: keycode (see keycodes.h)
   Argument 2: mask (see keycodes.h)
*/

#define BG_PLAYER_MSG_KEY                    20 /* A key was pressed */

/* A previously triggerend cleanup operation is finished */
#define BG_PLAYER_MSG_CLEANUP                21


#endif // __BG_PLAYERMSG_H_
