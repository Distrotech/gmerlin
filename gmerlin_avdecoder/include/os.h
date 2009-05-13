/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

/* Functions which miss on some systems */

#include <config.h>

/* poll() */

#ifndef HAVE_POLL 

#else

#endif // HAVE_POLL

/* getaddrinfo */

#ifndef HAVE_GETADDRINFO

#else

#endif // HAVE_GETADDRINFO

/* readdir_r */

#ifndef HAVE_READDIR_R

#else

#endif // HAVE_READDIR_R

#ifndef HAVE_CLOSESOCKET
#define closesocket(fd) close(fd)
#else

#endif // HAVE_CLOSESOCKET

