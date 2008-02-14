/*****************************************************************
 
  bgavdump.c
 
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

#include <avdec_private.h>

int main(int argc, char ** argv)
  {
  bgav_t * b;
  bgav_options_t * opt;

  b = bgav_create();
  opt = bgav_get_options(b);
  bgav_options_set_sample_accurate(opt, 1);
  //  bgav_options_set_index_callback(opt, index_callback, NULL);


  if(!bgav_open(b, argv[1]))
    return -1;

  bgav_file_index_dump(b);

  bgav_close(b);
  return 0;
  }
