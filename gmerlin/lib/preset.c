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

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/preset.h>
#include <gmerlin/utils.h>

static bg_preset_t * append_to_list(bg_preset_t * list, bg_preset_t * p)
  {
  if(list)
    {
    bg_preset_t * end = list;
    while(end->next)
      end = end->next;
    end->next = p;
    }
  else
    list = p;

  p->next = NULL;
  return list;
  }

static bg_preset_t *
load_presets(const char * directory)
  {
  DIR * dir;
  struct dirent * dent_ptr;
  struct stat stat_buf;
  char * filename;
  bg_preset_t * p;
  char * pos;
  
  struct
    {
    struct dirent d;
    char b[NAME_MAX]; /* Make sure there is enough memory */
    } dent;
  bg_preset_t * ret = NULL;

  dir = opendir(directory);
  if(!dir)
    return NULL;

  while(!readdir_r(dir, &dent.d, &dent_ptr))
    {
    if(!dent_ptr)
      break;

    if(dent.d.d_name[0] == '.') /* Don't import hidden files */
      continue;

    filename = bg_sprintf("%s/%s", directory, dent.d.d_name);
    
    if(stat(filename, &stat_buf) ||
       !S_ISREG(stat_buf.st_mode) ||
       access(filename, R_OK | W_OK))
      {
      free(filename);
      continue;
      }

    /* Got a preset */
    p = calloc(1, sizeof(*p));
    p->file = filename;

    /* Preset name is basename of the file */
    pos = strrchr(p->file, '/');
    if(pos)
      {
      pos++;
      p->name = bg_strdup(p->name, pos);
      }
    else
      p->name = bg_strdup(p->name, p->file);
    ret = append_to_list(ret, p);
    }
  closedir(dir);
  
  return ret;
  }

bg_preset_t * bg_presets_load(const char * preset_path)
  {
  const char * home_dir;
  char * directory;
  bg_preset_t * ret = NULL;
  /* First option: Try home directory */
  home_dir = getenv("HOME");

  if(home_dir)
    {
    directory = bg_sprintf("%s/.gmerlin/presets/%s",
                           home_dir, preset_path);
    if(!access(directory, R_OK|W_OK|X_OK))
      ret = load_presets(directory);
    free(directory);
    }
  
  if(!ret)
    {
    directory = bg_sprintf("%s/presets/%s",
                           DATA_DIR, preset_path);
    if(!access(directory, R_OK|W_OK|X_OK))
      ret = load_presets(directory);
    free(directory);
    }
  
  return ret;
  }

bg_preset_t * bg_preset_append(bg_preset_t * p,
                               const char * preset_path,
                               char * name)
  {
  return NULL;
  
  }

void bg_presets_destroy(bg_preset_t * p)
  {
  bg_preset_t * tmp;

  while(p)
    {
    tmp = p->next;

    if(p->name)
      free(p->name);
    if(p->file)
      free(p->file);
    free(p);

    p = tmp;
    }
  }

