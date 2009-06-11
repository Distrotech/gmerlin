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
 * *****************************************************************/

#include <config.h>

#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include <gmerlin/utils.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "thumbnails"

static int ensure_directory(const char * dir)
  {
  if(access(dir, R_OK))
    {
    if(mkdir(dir, S_IRUSR|S_IWUSR|S_IXUSR) == -1)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not create directory %s: %s",
             dir, strerror(errno));
      return 0;
      }
    else
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Created directory %s",
             dir);
    }
  return 1;
  }

char * bg_get_thumbnail_file(const char * gml)
  {
  char * ret;
  char hash[33];

  char * home_dir;

  char * thumbs_dir;
  char * thumbs_dir_normal;
  char * thumbs_dir_fail;
  char * thumbs_dir_fail_gmerlin;
  
  home_dir = getenv("HOME");
  if(!home_dir)
    return (char*)0;
  
  thumbs_dir              = bg_sprintf("%s/.thumbnails",              home_dir);
  thumbs_dir_normal       = bg_sprintf("%s/.thumbnails/normal",       home_dir);
  thumbs_dir_fail         = bg_sprintf("%s/.thumbnails/fail",         home_dir);
  thumbs_dir_fail_gmerlin = bg_sprintf("%s/.thumbnails/fail/gmerlin", home_dir);
  
  if(!ensure_directory(thumbs_dir) ||
     !ensure_directory(thumbs_dir_normal) ||
     !ensure_directory(thumbs_dir_fail) ||
     !ensure_directory(thumbs_dir_fail_gmerlin))
    goto fail;
  
  bg_get_filename_hash(gml, hash);
  
  ret = bg_sprintf("%s/%s.png", thumbs_dir_normal, hash);

  while(access(ret, R_OK))
    {
    /* Check if there is a failed thumbnail */
    
    }

  fail:
  
  free(thumbs_dir);
  free(thumbs_dir_normal);
  free(thumbs_dir_fail);
  free(thumbs_dir_fail_gmerlin);
  
  return ret;
  }
