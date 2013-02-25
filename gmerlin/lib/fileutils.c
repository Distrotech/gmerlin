/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <stdio.h>



#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "fileutils"

#include <unistd.h>
#include <fcntl.h>

int bg_lock_file(FILE * f, int wr)
  {
  struct flock fl;

  fl.l_type   = wr ? F_WRLCK : F_RDLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
  fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
  fl.l_start  = 0;        /* Offset from l_whence         */
  fl.l_len    = 0;        /* length, 0 = to EOF           */
  fl.l_pid    = getpid(); /* our PID                      */

  if(fcntl(fileno(f), F_SETLKW, &fl))
    return 0;
  
  return 1;
  }

int bg_unlock_file(FILE * f)
  {
  struct flock fl;

  fl.l_type   = F_UNLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
  fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
  fl.l_start  = 0;        /* Offset from l_whence         */
  fl.l_len    = 0;        /* length, 0 = to EOF           */
  fl.l_pid    = getpid(); /* our PID                      */

  if(fcntl(fileno(f), F_SETLK, &fl))
    return 0;
  
  return 1;
  
  }

size_t bg_file_size(FILE * f)
  {
  size_t ret;
  size_t oldpos;

  oldpos = ftell(f);

  fseek(f, 0, SEEK_END);
  ret = ftell(f);
  fseek(f, oldpos, SEEK_SET);
  return ret;
  }


void * bg_read_file(const char * filename, int * len_ret)
  {
  uint8_t * ret;
  FILE * file;
  size_t len;
  
  file = fopen(filename, "r");
  if(!file)
    return NULL;

  len = bg_file_size(file);
  
  ret = malloc(len + 1);

  if(fread(ret, 1, len, file) < len)
    {
    fclose(file);
    free(ret);
    return NULL;
    }
  ret[len] = '\0';
  fclose(file);

  if(len_ret)
    *len_ret = len;
  
  return ret;
  }


int bg_write_file(const char * filename, void * data, int len)
  {
  FILE * file;
  
  file = fopen(filename, "w");
  if(!file)
    return 0;
  
  if(fwrite(data, 1, len, file) < len)
    {
    fclose(file);
    return 0;
    }
  fclose(file);
  return 1;
  }
