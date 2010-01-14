/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

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
