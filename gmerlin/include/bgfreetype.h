/*****************************************************************
 
  bgfreetype.h
 
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

/* Freetype includes */

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

/* The freetype stroker is screwed up in
   versions up to and including 2.1.9 */

#if (FREETYPE_MINOR<1)|| ((FREETYPE_MINOR==1)&&(FREETYPE_PATCH<10))
#undef FT_STROKER_H
#endif

/* Stroker interface */
#ifdef FT_STROKER_H
#include FT_STROKER_H
#endif
