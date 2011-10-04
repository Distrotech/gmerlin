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

#include <string.h>
#include <stdlib.h>

#include <gmerlin/utils.h>
#include <gmerlin/parameter.h>

void bg_parameter_value_copy(bg_parameter_value_t * dst,
                             const bg_parameter_value_t * src,
                             const bg_parameter_info_t * info)
  {
  switch(info->type)
    {
    case BG_PARAMETER_CHECKBUTTON:
      dst->val_i = src->val_i;
      break;
    case BG_PARAMETER_INT:
    case BG_PARAMETER_SLIDER_INT:
      dst->val_i = src->val_i;
      break;
    case BG_PARAMETER_FLOAT:
    case BG_PARAMETER_SLIDER_FLOAT:
      dst->val_f = src->val_f;
      break;
    case BG_PARAMETER_STRING:
    case BG_PARAMETER_STRING_HIDDEN:
    case BG_PARAMETER_FONT:
    case BG_PARAMETER_DEVICE:
    case BG_PARAMETER_FILE:
    case BG_PARAMETER_DIRECTORY:
    case BG_PARAMETER_STRINGLIST:
    case BG_PARAMETER_MULTI_MENU:
    case BG_PARAMETER_MULTI_LIST:
    case BG_PARAMETER_MULTI_CHAIN:
      dst->val_str = bg_strdup(dst->val_str, src->val_str);
      break;
    case BG_PARAMETER_COLOR_RGB:
      memcpy(dst->val_color,
             src->val_color,
             3 * sizeof(dst->val_color[0]));
      dst->val_color[3] = 1.0;
      break;
    case BG_PARAMETER_COLOR_RGBA:
      memcpy(dst->val_color,
             src->val_color,
             4 * sizeof(dst->val_color[0]));
      break;
    case BG_PARAMETER_POSITION:
      memcpy(dst->val_pos,
             src->val_pos,
             2 * sizeof(dst->val_pos[0]));
      break;
    case BG_PARAMETER_TIME:
      dst->val_time = src->val_time;
      break;
    case BG_PARAMETER_SECTION:
    case BG_PARAMETER_BUTTON:
      break;
    }
  }

void bg_parameter_value_free(bg_parameter_value_t * val,
                             bg_parameter_type_t type)
  {
  switch(type)
    {
    case BG_PARAMETER_CHECKBUTTON:
    case BG_PARAMETER_INT:
    case BG_PARAMETER_SLIDER_INT:
    case BG_PARAMETER_FLOAT:
    case BG_PARAMETER_SLIDER_FLOAT:
    case BG_PARAMETER_TIME:
    case BG_PARAMETER_SECTION:
    case BG_PARAMETER_COLOR_RGB:
    case BG_PARAMETER_COLOR_RGBA:
    case BG_PARAMETER_POSITION:
    case BG_PARAMETER_BUTTON:
      break;
    case BG_PARAMETER_STRING:
    case BG_PARAMETER_STRING_HIDDEN:
    case BG_PARAMETER_FONT:
    case BG_PARAMETER_DEVICE:
    case BG_PARAMETER_FILE:
    case BG_PARAMETER_DIRECTORY:
    case BG_PARAMETER_STRINGLIST:
    case BG_PARAMETER_MULTI_MENU:
    case BG_PARAMETER_MULTI_LIST:
    case BG_PARAMETER_MULTI_CHAIN:
      if(val->val_str)
        free(val->val_str);
      break;
    }

  }

static char ** copy_string_array(char const * const * arr)
  {
  int i, num;
  char ** ret;
  if(!arr)
    return NULL;

  num = 0;
  while(arr[num])
    num++;

  ret = calloc(num+1, sizeof(*ret));

  for(i = 0; i < num; i++)
    ret[i] = bg_strdup(ret[i], arr[i]);
  return ret;
  }

static void free_string_array(char ** arr)
  {
  int i = 0;
  if(!arr)
    return;

  while(arr[i])
    {
    free(arr[i]);
    i++;
    }
  free(arr);
  }

void bg_parameter_info_copy(bg_parameter_info_t * dst,
                            const bg_parameter_info_t * src)
  {
  int num_options, i;

  dst->name = bg_strdup(dst->name, src->name);
  
  dst->long_name = bg_strdup(dst->long_name, src->long_name);
  dst->opt = bg_strdup(dst->opt, src->opt);
  dst->help_string = bg_strdup(dst->help_string, src->help_string);
  dst->type = src->type;
  dst->flags = src->flags;

  dst->gettext_domain    = bg_strdup(dst->gettext_domain,    src->gettext_domain);
  dst->gettext_directory = bg_strdup(dst->gettext_directory, src->gettext_directory);
  dst->preset_path = bg_strdup(dst->preset_path, src->preset_path);
  
  switch(dst->type)
    {
    case BG_PARAMETER_CHECKBUTTON:
      dst->val_default.val_i = src->val_default.val_i;
      break;
    case BG_PARAMETER_INT:
    case BG_PARAMETER_SLIDER_INT:
      dst->val_default.val_i = src->val_default.val_i;
      dst->val_min.val_i     = src->val_min.val_i;
      dst->val_max.val_i     = src->val_max.val_i;
      break;
    case BG_PARAMETER_FLOAT:
    case BG_PARAMETER_SLIDER_FLOAT:
      dst->val_default.val_f = src->val_default.val_f;
      dst->val_min.val_f     = src->val_min.val_f;
      dst->val_max.val_f     = src->val_max.val_f;
      dst->num_digits        = src->num_digits;
      break;
    case BG_PARAMETER_STRING:
    case BG_PARAMETER_STRING_HIDDEN:
    case BG_PARAMETER_FONT:
    case BG_PARAMETER_DEVICE:
    case BG_PARAMETER_FILE:
    case BG_PARAMETER_DIRECTORY:
      dst->val_default.val_str = bg_strdup(dst->val_default.val_str,
                                           src->val_default.val_str);
      break;
    case BG_PARAMETER_MULTI_MENU:
    case BG_PARAMETER_MULTI_LIST:
    case BG_PARAMETER_MULTI_CHAIN:
      dst->val_default.val_str = bg_strdup(dst->val_default.val_str,
                                           src->val_default.val_str);
      
      dst->multi_names_nc        = copy_string_array(src->multi_names);
      dst->multi_labels_nc       = copy_string_array(src->multi_labels);
      dst->multi_descriptions_nc = copy_string_array(src->multi_descriptions);
      
      i = 0;

      if(src->multi_names)
        {
        num_options = 0;
        
        while(src->multi_names[num_options])
          num_options++;

        if(src->multi_parameters)
          {
          dst->multi_parameters_nc =
            calloc(num_options, sizeof(*(src->multi_parameters_nc)));
          i = 0;
          
          while(src->multi_names[i])
            {
            if(src->multi_parameters[i])
              dst->multi_parameters_nc[i] =
                bg_parameter_info_copy_array(src->multi_parameters[i]);
            i++;
            }
          }
        }
      break;
    case BG_PARAMETER_STRINGLIST:
      dst->val_default.val_str = bg_strdup(dst->val_default.val_str,
                                           src->val_default.val_str);

      /* Copy stringlist options */
      
      if(src->multi_names)
        dst->multi_names_nc = copy_string_array(src->multi_names);
      if(src->multi_labels)
        dst->multi_labels_nc = copy_string_array(src->multi_labels);
      dst->multi_names = (char const **)dst->multi_names_nc;
      dst->multi_labels = (char const **)dst->multi_labels_nc;
      break;
    case BG_PARAMETER_COLOR_RGB:
      if(src->val_default.val_color)
        {
        memcpy(dst->val_default.val_color,
               src->val_default.val_color,
               3 * sizeof(dst->val_default.val_color[0]));
        dst->val_default.val_color[3] = 1.0;
        }
      break;
    case BG_PARAMETER_COLOR_RGBA:
      if(src->val_default.val_color)
        {
        memcpy(dst->val_default.val_color,
               src->val_default.val_color,
               4 * sizeof(dst->val_default.val_color[0]));
        }
      break;
    case BG_PARAMETER_POSITION:
      if(src->val_default.val_color)
        {
        memcpy(dst->val_default.val_pos,
               src->val_default.val_pos,
               2 * sizeof(dst->val_default.val_pos[0]));
        }
      dst->num_digits        = src->num_digits;
      break;
    case BG_PARAMETER_TIME:
      dst->val_default.val_time = src->val_default.val_time;
      break;
    case BG_PARAMETER_SECTION:
    case BG_PARAMETER_BUTTON:
      break;
    }
  bg_parameter_info_set_const_ptrs(dst);

  }

bg_parameter_info_t *
bg_parameter_info_copy_array(const bg_parameter_info_t * src)
  {
  int num_parameters, i;
  bg_parameter_info_t * ret;

  num_parameters = 0;
  
  while(src[num_parameters].name)
    num_parameters++;
      
  ret = calloc(num_parameters + 1, sizeof(bg_parameter_info_t));
  
  for(i = 0; i < num_parameters; i++)
    bg_parameter_info_copy(&ret[i], &src[i]);
  
  return ret;
  }

void bg_parameter_info_destroy_array(bg_parameter_info_t * info)
  {
  int index = 0;
  int i;
  while(info[index].name)
    {
    free(info[index].name);
    if(info[index].long_name)
      free(info[index].long_name);
    if(info[index].opt)
      free(info[index].opt);
    if(info[index].help_string)
      free(info[index].help_string);
    if(info[index].gettext_domain)
      free(info[index].gettext_domain);
    if(info[index].gettext_directory)
      free(info[index].gettext_directory);
    if(info[index].preset_path)
      free(info[index].preset_path);
    switch(info[index].type)
      {
      case BG_PARAMETER_STRINGLIST:
        free_string_array(info[index].multi_names_nc);
        free_string_array(info[index].multi_labels_nc);
        
        if(info[index].val_default.val_str)
          free(info[index].val_default.val_str);
        break;
      case BG_PARAMETER_STRING:
      case BG_PARAMETER_STRING_HIDDEN:
      case BG_PARAMETER_FONT:
      case BG_PARAMETER_DEVICE:
      case BG_PARAMETER_FILE:
      case BG_PARAMETER_DIRECTORY:
        if(info[index].val_default.val_str)
          free(info[index].val_default.val_str);
        break;
      case BG_PARAMETER_SECTION:
      case BG_PARAMETER_BUTTON:
      case BG_PARAMETER_CHECKBUTTON:
      case BG_PARAMETER_INT:
      case BG_PARAMETER_FLOAT:
      case BG_PARAMETER_TIME:
      case BG_PARAMETER_SLIDER_INT:
      case BG_PARAMETER_SLIDER_FLOAT:
      case BG_PARAMETER_COLOR_RGB:
      case BG_PARAMETER_COLOR_RGBA:
      case BG_PARAMETER_POSITION:
        break;
      case BG_PARAMETER_MULTI_MENU:
      case BG_PARAMETER_MULTI_LIST:
      case BG_PARAMETER_MULTI_CHAIN:
        i = 0;

        if(info[index].multi_parameters)
          {
          while(info[index].multi_names[i])
            {
            if(info[index].multi_parameters[i])
              bg_parameter_info_destroy_array(info[index].multi_parameters_nc[i]);
            i++;
            }
          free(info[index].multi_parameters_nc);
          }
        
        free_string_array(info[index].multi_names_nc);
        free_string_array(info[index].multi_labels_nc);
        free_string_array(info[index].multi_descriptions_nc);
                
        if(info[index].val_default.val_str)
          free(info[index].val_default.val_str);
        
        
      }
    index++;
    }
  free(info);
  }

bg_parameter_info_t *
bg_parameter_info_concat_arrays(bg_parameter_info_t const ** srcs)
  {
  int i, j, dst, num_parameters;

  bg_parameter_info_t * ret;

  /* Count the parameters */
  num_parameters = 0;
  i = 0;

  while(srcs[i])
    {
    j = 0;
    while(srcs[i][j].name)
      {
      num_parameters++;
      j++;
      }
    i++;
    }

  /* Allocate destination */

  ret = calloc(num_parameters+1, sizeof(*ret));

  /* Copy stuff */
  
  i = 0;
  dst = 0;

  while(srcs[i])
    {
    j = 0;
    while(srcs[i][j].name)
      {
      bg_parameter_info_copy(&ret[dst], &srcs[i][j]);
      dst++;
      j++;
      }
    i++;
    }
  return ret;
  }

int bg_parameter_get_selected(const bg_parameter_info_t * info,
                              const char * val)
  {
  int ret = -1, i;

  if(val)
    {
    i = 0;
    while(info->multi_names[i])
      {
      if(!strcmp(val, info->multi_names[i]))
        {
        ret = i;
        break;
        }
      i++;
      }
    }
  
  if((ret < 0) && info->val_default.val_str)
    {
    i = 0;
    /* Try default value */
    while(info->multi_names[i])
      {
      if(!strcmp(info->val_default.val_str, info->multi_names[i]))
        {
        ret = i;
        break;
        }
      i++;
      }
    }
  
  if(ret < 0)
    return 0;
  else
    return ret;
  }

const bg_parameter_info_t *
bg_parameter_find(const bg_parameter_info_t * info,
                  const char * name)
  {
  int i, j;
  const bg_parameter_info_t * child_ret;
  i = 0;
  while(info[i].name)
    {
    if(!strcmp(name, info[i].name))
      return &info[i];

    if(info[i].multi_parameters && info[i].multi_names)
      {
      j = 0;
      while(info[i].multi_names[j])
        {
        if(info[i].multi_parameters[j])
          {
          child_ret = bg_parameter_find(info[i].multi_parameters[j], name);
          if(child_ret)
            return child_ret;
          }
        j++;
        }
      }
    i++;
    }
  return NULL;
  }

void bg_parameter_info_set_const_ptrs(bg_parameter_info_t * ret)
  {
  ret->multi_names = (char const **)ret->multi_names_nc;
  ret->multi_labels = (char const **)ret->multi_labels_nc;
  ret->multi_descriptions = (char const **)ret->multi_descriptions_nc;
  ret->multi_parameters =
    (bg_parameter_info_t const * const *)ret->multi_parameters_nc;
  }
  
