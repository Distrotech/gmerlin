/*****************************************************************
 
  transcoder_remote.h
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#define TRANSCODER_REMOTE_PORT (BG_REMOTE_PORT_BASE+2)
#define TRANSCODER_REMOTE_ID "gmerlin_transcoder"

/* Remote commands */

/* Add file (arg1) */

#define TRANSCODER_REMOTE_ADD_FILE  1

/* Add album (arg1 points to an xml encoded file) */

#define TRANSCODER_REMOTE_ADD_ALBUM 2

