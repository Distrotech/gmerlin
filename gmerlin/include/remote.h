/*****************************************************************
 
  remote.h
 
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

#ifndef __BG_REMOTE_H_
#define __BG_REMOTE_H_

/* Remote server */

typedef struct bg_remote_server_s bg_remote_server_t;

bg_remote_server_t * bg_remote_server_create();

void bg_remote_server_destroy(bg_remote_server_t *);

/* Remote client */

typedef struct bg_remote_client_s bg_remote_client_t;

/* Create a remote for a player on the local host */

bg_remote_client_t * bg_remote_client_create_local();

/* Create a remote for a player on another computer */

bg_remote_client_t *
bg_remote_client_create_remote(const char * host, int port);

#endif // __BG_REMOTE_H_
