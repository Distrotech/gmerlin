/*****************************************************************
 
  http.h
 
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

#ifndef __BG_HTTP_H_
#define __BG_HTTP_H_

/* Generic http socket */

typedef struct bg_http_connection_s bg_http_connection_t;

bg_http_connection_t * bg_http_connection_create();
/* Destroying the connection doesn't close it!! */

void bg_http_connection_destroy(bg_http_connection_t *);

/* Set parameters */

void bg_http_connection_set_proxy(bg_http_connection_t *,
                                  const char * host, int port);

void bg_http_connection_set_timeouts(bg_http_connection_t *,
                                     int connect_ms, int read_ms);

/* Connect, return FALSE on failure */

int bg_http_connection_connect(bg_http_connection_t *,
                               const char * url,
                               char ** extra_request);

/* Get Informations about the contents */

const char * bg_http_connection_get_mimetype(bg_http_connection_t *);

/* NULL terminated array of header lines */

int bg_http_connection_get_num_header_lines(bg_http_connection_t *);

const char * bg_http_connection_get_header_line(bg_http_connection_t *, int);

/* Close the connection */

void bg_http_connection_close(bg_http_connection_t *);

/* Return the file descriptor */

int bg_http_connection_get_fd(bg_http_connection_t *);


void bg_http_connection_save(bg_http_connection_t *, const char * filename);

/* Read data after the header, rerurns the number of bytes */

int bg_http_connection_read(bg_http_connection_t *, uint8_t * buf, int len);

#endif // __BG_HTTP_H_
