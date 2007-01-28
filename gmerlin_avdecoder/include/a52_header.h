/*****************************************************************
 
  a52_header.h
 
  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

typedef struct
  {
  int total_bytes;
  int samplerate;
  int bitrate;

  int acmod;
  int lfe;
  int dolby;

  float cmixlev;
  float smixlev;
  
  } bgav_a52_header_t;

#define BGAV_A52_HEADER_BYTES 7

int bgav_a52_header_read(bgav_a52_header_t * ret, uint8_t * buf);

void bgav_a52_header_dump(bgav_a52_header_t * h);
