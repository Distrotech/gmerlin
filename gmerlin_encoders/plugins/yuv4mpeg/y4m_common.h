/*****************************************************************

  y4m_common.h

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

/* Common defintions and routines for creating y4m streams */

typedef struct
  {
  gavl_video_format_t format;
  FILE * file;
  /* For checking if an incoming frame can be read without memcpying */
  int strides[4]; 
  
  int chroma_mode;  

  y4m_stream_info_t si;
  y4m_frame_info_t fi;
  
  y4m_cb_writer_t writer;

  gavl_video_frame_t * frame;
  uint8_t * tmp_planes[4];

  } bg_y4m_common_t;

/* Set pixelformat and chroma placement from chroma_mode */

void bg_y4m_set_pixelformat(bg_y4m_common_t * com);
int bg_y4m_write_header(bg_y4m_common_t * com);

void bg_y4m_write_frame(bg_y4m_common_t * com, gavl_video_frame_t * frame);

void bg_y4m_cleanup(bg_y4m_common_t * com);
