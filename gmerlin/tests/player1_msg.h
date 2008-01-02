/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

/* Commands for the player */
/* Comments are arguments passed along with the message */

#define PLAYER_CMD_PLAY      0
#define PLAYER_CMD_NEXT      1
#define PLAYER_CMD_PREV      2
#define PLAYER_CMD_SEL_VIDEO 3 /* int */
#define PLAYER_CMD_SEL_AUDIO 4 /* int */
#define PLAYER_CMD_SEL_SUBP  5 /* int */

#define PLAYER_MSG_TIME_CHANGED  0 /* int (in seconds) */
#define PLAYER_MSG_TRACK_CHANGED 1 /* */
#define PLAYER_MSG_STATE_CHANGED 2 /* int, see defines below */

#define PLAYER_STATE_STOPPED      0
#define PLAYER_STATE_CHANGING     1
#define PLAYER_STATE_BUFFERING    2
#define PLAYER_STATE_PAUSED       3
#define PLAYER_STATE_PLAYING      4
#define PLAYER_STATE_SEEKING      5
