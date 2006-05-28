/*****************************************************************
 
  pngreader.h
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

typedef struct bgav_png_reader_s bgav_png_reader_t;

bgav_png_reader_t * bgav_png_reader_create(int depth);
void bgav_png_reader_destroy(bgav_png_reader_t* png);


void bgav_png_reader_reset(bgav_png_reader_t * png);

int bgav_png_reader_read_image(bgav_png_reader_t * png,
                               gavl_video_frame_t * frame);
int bgav_png_reader_read_header(bgav_png_reader_t * png,
                                uint8_t * buffer, int buffer_size,
                                gavl_video_format_t * format);


