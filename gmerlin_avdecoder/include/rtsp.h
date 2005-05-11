/*****************************************************************
 
  avdec_private.h
 
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

#include <sdp.h>

/* rtsp.c */

typedef struct bgav_rtsp_s bgav_rtsp_t;

/* Housekeeping */

bgav_rtsp_t * bgav_rtsp_create(const bgav_options_t * opt);

void bgav_rtsp_set_user_agent(bgav_rtsp_t * r, const char * user_agent);

/* Open the URL and send the OPTIONS request */

int bgav_rtsp_open(bgav_rtsp_t *, const char * url, int * got_redirected, char ** error_msg);

void bgav_rtsp_close(bgav_rtsp_t *);

/* Return raw socket */

int bgav_rtsp_get_fd(bgav_rtsp_t *);

/* Append a field to the next request header */

void bgav_rtsp_schedule_field(bgav_rtsp_t *, const char * field);

/* Get a field from the last answer */

const char * bgav_rtsp_get_answer(bgav_rtsp_t *, const char * name);

/* Now, the requests follow */

int bgav_rtsp_request_describe(bgav_rtsp_t *, int * got_redirected, char ** error_msg);

/* Get the redirection URL */

const char * bgav_rtsp_get_url(bgav_rtsp_t *);

/* After a successful describe request, the session description is there */

bgav_sdp_t * bgav_rtsp_get_sdp(bgav_rtsp_t *);

/* Issue a SETUP */

int bgav_rtsp_request_setup(bgav_rtsp_t *, const char * arg, char ** error_msg);

/* Set Parameter */

int bgav_rtsp_request_setparameter(bgav_rtsp_t *, char ** error_msg);

/* Play */

int bgav_rtsp_request_play(bgav_rtsp_t *, char ** error_msg);

