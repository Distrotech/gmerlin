/*****************************************************************
 
  xing.h
 
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


/* XING VBR header (stolen from xmms) */

#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

typedef struct
  {
  int flags;
  
  int frames;             /* total bit stream frames from Xing header data */
  int bytes;              /* total bit stream bytes from Xing header data  */
  unsigned char toc[100]; /* "table of contents" */
  } bgav_xing_header_t;

/* Read the xing header, buf MUST be at least one complete mpeg audio frame */

int bgav_xing_header_read(bgav_xing_header_t * xing, unsigned char *buf);

int64_t bgav_xing_get_seek_position(bgav_xing_header_t * xing, float percent);
