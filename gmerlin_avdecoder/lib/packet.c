/*****************************************************************
 
  packet.c
 
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

#include <stdlib.h>

#include <avdec_private.h>

bgav_packet_t * bgav_packet_create()
  {
  bgav_packet_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bgav_packet_destroy(bgav_packet_t * p)
  {
  if(p->data)
    free(p->data);
  free(p);
  }

void bgav_packet_alloc(bgav_packet_t * p, int size)
  {
  if(size > p->data_alloc)
    {
    p->data_alloc = size + 16;
    p->data = realloc(p->data, p->data_alloc);
    }
  }

void bgav_packet_done_write(bgav_packet_t * p)
  {
  p->valid = 1;
  }
