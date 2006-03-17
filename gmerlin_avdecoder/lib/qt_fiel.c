/*****************************************************************
 
  qt_fiel.c
 
  Copyright (c) 2005-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <avdec_private.h>

#include <stdio.h>
#include <qt.h>

int bgav_qt_fiel_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_fiel_t * ret)
  {
  memcpy(&(ret->h), h, sizeof(*h));
  if(!bgav_input_read_data(ctx, &(ret->fields), 1) ||
     !bgav_input_read_data(ctx, &(ret->detail), 1))
    return 0;
  //  bgav_qt_fiel_dump(ret);
  return 1;
  }

void bgav_qt_fiel_dump(qt_fiel_t * p)
  {
  fprintf(stderr, "fiel:\n");
  fprintf(stderr, "  fields: %d\n", p->fields);
  fprintf(stderr, "  detail: %d\n", p->detail);
  }
