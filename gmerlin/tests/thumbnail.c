/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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
 ******************************************************************/

#include <stdio.h>
#include <utils.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv)
  {
  char * th = bg_get_tumbnail_file(argv[1]);
  printf("Thumbnail: %s\n", th);
  free(th);
  return 0;
  }


