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

#include <config.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "nmjedit.h"

#include <gmerlin/utils.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "nmjedit.file"



static bg_nmj_file_t * file_scan_internal(const char * directory,
                                          const char * extensions,
                                          bg_nmj_file_t * files,
                                          int * num_p, int * alloc_p,
                                          int64_t * size)
  {
  char * filename;
  DIR * d;
  struct dirent * e;
  struct stat st;
  char * pos;
  bg_nmj_file_t * file;
  int num = *num_p;
  int alloc = *alloc_p;
  
  d = opendir(directory);
  if(!d)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Opening directory %s failed: %s",
           directory, strerror(errno));
    return files;
    }

  while((e = readdir(d)))
    {
    /* Skip hidden files and "." and ".." */
    if(e->d_name[0] == '.')
      continue;
    
    filename = bg_sprintf("%s/%s", directory, e->d_name);
    if(!stat(filename, &st))
      {
      /* Check for directory */
      if(S_ISDIR(st.st_mode))
        {
        files = file_scan_internal(filename,
                                   extensions,
                                   files, &num, &alloc, size);
        }
      else if(S_ISREG(st.st_mode))
        {
        pos = strrchr(filename, '.');
        if(pos && bg_string_match(pos+1, extensions))
          {
          /* Add file to list */
          if(num + 2 > alloc)
            {
            alloc += 256;
            files = realloc(files, alloc * sizeof(*files));
            memset(files + num, 0,
                   (alloc - num) * sizeof(*files));
            }
          file = files + num;

          file->path = filename;
          filename = NULL;
          file->size = st.st_size;
          file->time = st.st_mtime;
          num += 1;
          }
        *size += st.st_size;
        }
      }
    if(filename)
      free(filename);
    }
  
  closedir(d);
  *num_p = num;
  *alloc_p = alloc;
  return files;
  }

bg_nmj_file_t * bg_nmj_file_scan(const char * directory,
                                 const char * extensions,
                                 int64_t * size)
  {
  bg_nmj_file_t * ret = NULL;
  int num_ret = 0;
  int ret_alloc = 0;
  *size = 0;
  return file_scan_internal(directory,
                            extensions,
                            ret,
                            &num_ret, &ret_alloc, size);
  }

bg_nmj_file_t * bg_nmj_file_lookup(bg_nmj_file_t * files,
                                   const char * path)
  {
  int i = 0;
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
  int i = 0;
  while(files[i].path)
    {
    free(files[i].path);
    i++;
    }
  free(files);
  }

void bg_nmj_file_remove(bg_nmj_file_t * files,
                        bg_nmj_file_t * file)
  {
  /* Count entries after this one */
  int num = 1;
  
  while(file[num].path)
    num++;

  free(file->path);
  memmove(file, file+1, num * sizeof(*file));
  
  }
