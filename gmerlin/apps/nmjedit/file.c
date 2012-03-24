/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include "nmjedit.h"

bg_nmj_file_t * file_scan_internal(const char * directory,
                                   const char * extensions,
                                   bg_nmj_file_t * files,
                                   int * num, int * alloc)
  {
  union
    {
    struct dirent d;
    char padding[sizeof(struct dirent) + 2048];
    }d;

  
  }

bg_nmj_file_t * bg_nmj_file_scan(const char * directory,
                                 const char * extensions)
  {
  bg_nmj_file_t * ret;
  int num_ret;
  int ret_alloc;
  
  ret = file_scan_internal(directory,
                           extensions,
                           ret,
                           &num_ret, ret_alloc);
  }

bg_nmj_file_t * bg_nmj_file_lookup(bg_nmj_file_t * files,
                                   const char * path)
  {
  int i;
  while(files[i].path)
    {
    if(!strcmp(files[i].path, path))
      return &files[i];
    i++;
    }
  return NULL;
  }

void bg_nmj_file_destroy(bg_nmj_file_t * files)
  {
  while(files[i].path)
    {
    free(files[i].path);
    i++;
    }
  
  }
