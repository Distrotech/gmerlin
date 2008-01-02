/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <gavl/gavl.h>
#include <video.h>
#include <colorspace.h>

#define DO_ROUND
#define HQ

#include "../c/colorspace_tables.h"
#include "../c/colorspace_macros.h"

#include "../c/_rgb_rgb_c.c"
#include "../c/_rgb_yuv_c.c"
#include "../c/_yuv_rgb_c.c"
#include "../c/_yuv_yuv_c.c"
