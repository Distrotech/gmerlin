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

#include <parameter.h>
#include <msgqueue.h>

#define BG_REMOTE_PORT_BASE 10100

/* Remote server */

typedef struct bg_remote_server_s bg_remote_server_t;

bg_remote_server_t * bg_remote_server_create(int default_listen_port,
                                             char * protocol_id);

bg_parameter_info_t * bg_remote_server_get_parameters(bg_remote_server_t *);

void bg_remote_server_set_parameter(void * data,
                                    char * name,
                                    bg_parameter_value_t * v);

int bg_remote_server_init(bg_remote_server_t *);

bg_msg_t * bg_remote_server_get_msg(bg_remote_server_t *);

void bg_remote_server_put_msg(bg_remote_server_t *, bg_msg_t *);

void bg_remote_server_destroy(bg_remote_server_t *);

/* Remote client */

typedef struct bg_remote_client_s bg_remote_client_t;

/* Create a remote for a player on the local host */

bg_remote_client_t * bg_remote_client_create(const char * protocol_id,
                                             int read_messages);

int bg_remote_client_init(bg_remote_client_t *,
                          const char * host, int port, int milliseconds);

void bg_remote_client_destroy(bg_remote_client_t * c);

bg_msg_t * bg_remote_client_get_msg_write(bg_remote_client_t * c);
int bg_remote_client_done_msg_write(bg_remote_client_t * c);

bg_msg_t * bg_remote_client_get_msg_read(bg_remote_client_t * c);


#endif // __BG_REMOTE_H_
