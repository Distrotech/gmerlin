
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

/* http.c */

typedef struct
  {
  int num_lines;
  int lines_alloc;
  struct
    {
    char * line;    /* 0 terminated */
    int line_alloc; /* Allocated size */
    } * lines;
  } bgav_http_header_t;

typedef struct bgav_http_s bgav_http_t;

/* Creation/destruction of http header */

bgav_http_header_t * bgav_http_header_create();

void bgav_http_header_reset(bgav_http_header_t*);
void bgav_http_header_destroy(bgav_http_header_t*);

void bgav_http_header_add_line(bgav_http_header_t*, const char * line);
int bgav_http_header_send(bgav_http_header_t*, int fd, char ** error_msg);

int bgav_http_header_status_code(bgav_http_header_t * h);

/* Reading of http header */

void bgav_http_header_revc(bgav_http_header_t*, int fd, int milliseconds);

const char * bgav_http_header_get_var(bgav_http_header_t*,
                                      const char * name);


void bgav_http_header_dump(bgav_http_header_t*);

/* http connection */

bgav_http_t * bgav_http_open(const char * url, int milliseconds, char ** redirect_url,
                             bgav_http_header_t* extra_header, char ** error_msg);

void bgav_http_close(bgav_http_t *);

int bgav_http_get_fd(bgav_http_t *);

bgav_http_header_t* bgav_http_get_header(bgav_http_t *);
