/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#define PLAYER_REMOTE_PORT (BG_REMOTE_PORT_BASE+1)
#define PLAYER_REMOTE_ID "gmerlin_player"
#define PLAYER_REMOTE_ENV "GMERLIN_PLAYER_REMOTE_PORT"


/* Player commands */

/* These resemble the player buttons */

#define PLAYER_COMMAND_PLAY           1
#define PLAYER_COMMAND_STOP           2
#define PLAYER_COMMAND_NEXT           3
#define PLAYER_COMMAND_PREV           4
#define PLAYER_COMMAND_PAUSE          5

/* Add or play a location (arg1: Location) */

#define PLAYER_COMMAND_ADD_LOCATION   6
#define PLAYER_COMMAND_PLAY_LOCATION  7


/* Volume control (arg: Volume in dB) */

#define PLAYER_COMMAND_SET_VOLUME     8
#define PLAYER_COMMAND_SET_VOLUME_REL 9

/* Seek */

#define PLAYER_COMMAND_SEEK           10
#define PLAYER_COMMAND_SEEK_REL       11

/* Open devices */

#define PLAYER_COMMAND_OPEN_DEVICE    12
#define PLAYER_COMMAND_PLAY_DEVICE    13

/* Mute */

#define PLAYER_COMMAND_TOGGLE_MUTE    14

/* Chapters */

#define PLAYER_COMMAND_SET_CHAPTER    15
#define PLAYER_COMMAND_NEXT_CHAPTER   16
#define PLAYER_COMMAND_PREV_CHAPTER   17
