/*****************************************************************
 
  pngwriter.h
 
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

#define BITS_AUTO  0
#define BITS_8     8
#define BITS_16   16

typedef struct
  {
  png_structp png_ptr;
  png_infop   info_ptr;
  int transform_flags;
  FILE * output;
  int bit_mode;
  int compression_level;
  gavl_video_format_t format;
  } bg_pngwriter_t;

int bg_pngwriter_write_header(void * priv, const char * filename,
                              gavl_video_format_t * format);
int bg_pngwriter_write_image(void * priv, gavl_video_frame_t * frame);

void bg_pngwriter_set_parameter(void * p, char * name,
                                bg_parameter_value_t * val);


