/*****************************************************************
 
  hexdump.c
 
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

#include <stdio.h>
#include <utils.h>

void bg_hexdump(const void * data, int len)
  {
  int i;
  const unsigned char * str = data;
  
  for(i = 0; i < len; i++)
    {
    fprintf(stderr, "%02x ", (unsigned int)(str[i]));
    if(!((i+1) % 16))
      fprintf(stderr, "\n");
    }
  fprintf(stderr, "\n");
  }
