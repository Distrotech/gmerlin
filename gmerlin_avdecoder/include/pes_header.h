/*****************************************************************
 
  pes_header.h
 
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

/* MPEG-1/2 PES packet */

typedef struct
  {
  int64_t pts;
  int stream_id;     /* For private streams: stream_id | (substream_id << 8)   */
  int payload_size;  /* To be read from the input after bgav_pes_header_read() */
  } bgav_pes_header_t;

int bgav_pes_header_read(bgav_input_context_t * input,
                         bgav_pes_header_t * ret);

void bgav_pes_header_dump(bgav_pes_header_t * p);

