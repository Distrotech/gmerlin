/*****************************************************************
 
  mms.h
 
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

typedef struct bgav_mms_s bgav_mms_t;

/* Open an mms url, return opaque structure */

bgav_mms_t * bgav_mms_open(const char * url,
                           int connect_timeout,
                           int read_timeout, char ** error_msg);

/* After a successful open call, the ASF header can obtained
   with the following function */

uint8_t * bgav_mms_get_header(bgav_mms_t * mms, int * len);

/* Select the streams, right now, all streams MUST be switched on */

int bgav_mms_select_streams(bgav_mms_t * mms,
                            int * stream_ids, int num_streams, char ** error_msg);

/*
 *  This reads data (usually one asf packet)
 *  NULL is returned on EOF 
 */

uint8_t * bgav_mms_read_data(bgav_mms_t * mms, int * len, int block, char ** error_msg);

void bgav_mms_close(bgav_mms_t*);

