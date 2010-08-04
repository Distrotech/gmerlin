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

#include <config.h>
#include <gmerlin/cfg_registry.h>
#include <gmerlin/preset.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "preset"

static bg_preset_t *
append_to_list(bg_preset_t * list, bg_preset_t * p)
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
load_presets(const char * directory, bg_preset_t * ret, int private)
  {
  DIR * dir;
  struct dirent * dent_ptr;
  struct stat stat_buf;
  char * filename;
  bg_preset_t * p;
  char * pos;
  char * name;
  
  struct
    {
    struct dirent d;
    char b[NAME_MAX]; /* Make sure there is enough memory */
    } dent;
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
    pos = strrchr(filename, '/');
    if(pos)
      {
      pos++;
      name = bg_strdup(NULL, pos);
      }
    else
      name = bg_strdup(NULL, filename);

    /* Check, if a preset with that name aready exists */
    
    p = ret;
    while(p)
      {
      if(!strcmp(name, p->name))
        break;
      p = p->next;
      }

    if(p)
      {
      free(name);
      free(filename);
      continue;
      }

    p = calloc(1, sizeof(*p));
    p->name = name;
    p->file = filename;
    if(private)
      p->flags |= BG_PRESET_PRIVATE;
    ret = append_to_list(ret, p);
    }
  closedir(dir);
  
  return ret;
  }

static int compare_func(const void * p1, const void * p2)
  {
  const bg_preset_t * const * preset1 = p1;
  const bg_preset_t * const * preset2 = p2;
  return strcmp((*preset1)->name, (*preset2)->name);
  }

static bg_preset_t * sort_presets(bg_preset_t * p)
  {
  int i, num = 0;
  bg_preset_t ** arr;
  bg_preset_t * tmp;
  bg_preset_t * ret;
  
  /* Count presets */
  tmp = p;
  while(tmp)
    {
    num++;
    tmp = tmp->next;
    }

  if(num < 2)
    return p;
  
  /* Create array */
  arr = malloc(num * sizeof(*arr));
  tmp = p;
  for(i = 0; i < num; i++)
    {
    arr[i] = tmp;
    tmp = tmp->next;
    }

  /* Sort */
  qsort(arr, num, sizeof(*arr), compare_func);

  /* Array -> chain */

  ret = arr[0];
  tmp = ret;

  for(i = 1; i < num; i++)
    {
    tmp->next = arr[i];
    tmp = tmp->next;
    }
  tmp->next = NULL;

  free(arr);
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
      ret = load_presets(directory, ret, 1);
    free(directory);
    }

  /* Second option: system wide directory. We only
     load presets, which are not available in $HOME */
  
  directory = bg_sprintf("%s/presets/%s",
                         DATA_DIR, preset_path);
  if(!access(directory, R_OK|W_OK|X_OK))
    ret = load_presets(directory, ret, 0);
  free(directory);

  return sort_presets(ret);
  }

bg_preset_t * bg_preset_find_by_file(bg_preset_t * presets,
                                     const char * file)
  {
  while(presets)
    {
    if(!strcmp(presets->file, file))
      return presets;
    presets = presets->next;
    }
  return NULL;
  }

bg_preset_t * bg_preset_find_by_name(bg_preset_t * presets,
                                     const char * name)
  {
  while(presets)
    {
    if(!strcmp(presets->name, name))
      return presets;
    presets = presets->next;
    }
  return NULL;
  }

bg_preset_t * bg_preset_delete(bg_preset_t * presets,
                               bg_preset_t * preset)
  {
  bg_preset_t * p;
  bg_preset_t * ret = NULL;
  
  /* First (or only) preset */
  if(presets == preset)
    {
    ret = preset->next;
    }
  /* Later preset */
  else
    {
    p = presets;
    while(1)
      {
      if(p->next == preset)
        break;
      p = p->next;
      }
    if(p)
      p->next = preset->next;
    ret = presets;
    }
  
  return ret;
  }

bg_preset_t * bg_preset_add(bg_preset_t * presets,
                            const char * preset_path,
                            const char * name,
                            const bg_cfg_section_t * s)
  {
  char * home_dir;
  bg_preset_t * p;

  home_dir = getenv("HOME");
  if(!home_dir)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot make new preset: No home directory");
    return presets;
    }
  
  /* Check if the preset already exists */
  
  p = bg_preset_find_by_name(presets, name);

  if(!p)
    {
    char * dir;
    dir = bg_sprintf("%s/.gmerlin/presets/%s", home_dir,
                     preset_path);

    if(!bg_ensure_directory(dir))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Could not create directory: %s", dir);
      free(dir);
      return presets;
      }
    p = calloc(1, sizeof(*p));
    p->name = bg_strdup(p->name, name);
    p->file = bg_sprintf("%s/%s", dir, name);
    p->next = presets;
    presets = p;
    free(dir);
    }
  
  bg_preset_save(p, s);
  
  return sort_presets(presets);
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

