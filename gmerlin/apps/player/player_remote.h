/*****************************************************************
 
  player_remote.h
 
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

#define PLAYER_REMOTE_PORT (BG_REMOTE_PORT_BASE+1)
#define PLAYER_REMOTE_ID "gmerlin_player"


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

