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

/* Error codes */

#define BG_PLAYER_ERROR_GENERAL   1
#define BG_PLAYER_ERROR_TRACK     2 /* Somethings wrong with the track */


/* Message definition for the player */

/****************************
 *  Commands for the player
 ****************************/

/* Start playing                                        */
/* arg1: Input plugin handle                            */
/* arg2: Track index for plugins with multiple tracks   */
/* arg3: Play flags, see defines below                  */

#define BG_PLAYER_CMD_PLAY     0

#define BG_PLAY_FLAG_IGNORE_IF_PLAYING (1<<0)
#define BG_PLAY_FLAG_IGNORE_IF_STOPPED (1<<1)
#define BG_PLAY_FLAG_INIT_THEN_PAUSE   (1<<2)

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
 *  Set the logo to be displayed when video is inactive
 *  arg1: Format
 *  arg2: Video frame (pointer)
 */
   
#define BG_PLAYER_CMD_SETLOGO       9

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
 *    arg1: String describing the error
 *    arg2: Integer error code (Seek BG_PLAYER_ERROR_* above)
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
 *  Arg3: Number of subpicture streams
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

/* Metadata (is only sent, if information is available) */

/* Arg1 : string */

#define BG_PLAYER_MSG_METADATA                8

#define BG_PLAYER_MSG_AUDIO_DESCRIPTION       9

#define BG_PLAYER_MSG_VIDEO_DESCRIPTION      10

#define BG_PLAYER_MSG_SUBPICTURE_DESCRIPTION 11

#define BG_PLAYER_MSG_STREAM_DESCRIPTION     12

#define BG_PLAYER_MSG_VOLUME_CHANGED         13

#endif // __BG_PLAYERMSG_H_
