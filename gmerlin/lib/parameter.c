/*****************************************************************
 
  parameter.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <stdlib.h>

#include <utils.h>
#include <parameter.h>

void bg_parameter_value_copy(bg_parameter_value_t * dst,
                             const bg_parameter_value_t * src,
                             bg_parameter_info_t * info)
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
    case BG_PARAMETER_FONT:
    case BG_PARAMETER_DEVICE:
    case BG_PARAMETER_FILE:
    case BG_PARAMETER_DIRECTORY:
    case BG_PARAMETER_STRINGLIST:
    case BG_PARAMETER_MULTI_MENU:
    case BG_PARAMETER_MULTI_LIST:
      dst->val_str = bg_strdup(dst->val_str, src->val_str);
      break;
    case BG_PARAMETER_COLOR_RGB:
      if(!dst->val_color)
        {
        dst->val_color = malloc(4 * sizeof(float));
        }
      memcpy(dst->val_color,
             src->val_color,
             3 * sizeof(float));
      dst->val_color[3] = 1.0;
      break;
    case BG_PARAMETER_COLOR_RGBA:
      if(!dst->val_color)
        {
        dst->val_color = malloc(4 * sizeof(float));
        }
      memcpy(dst->val_color,
             src->val_color,
             4 * sizeof(float));
      break;
    case BG_PARAMETER_TIME:
      dst->val_time = src->val_time;
      break;
    case BG_PARAMETER_SECTION:
      break;
    }
  }

static char ** copy_string_array(char ** arr)
  {
  int i, num;
  char ** ret;
  if(!arr)
    return (char**)0;

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
  dst->type = src->type;
  dst->flags = src->flags;
  
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
    case BG_PARAMETER_FONT:
    case BG_PARAMETER_DEVICE:
    case BG_PARAMETER_FILE:
    case BG_PARAMETER_DIRECTORY:
      dst->val_default.val_str = bg_strdup(dst->val_default.val_str,
                                           src->val_default.val_str);
      break;
    case BG_PARAMETER_MULTI_MENU:
    case BG_PARAMETER_MULTI_LIST:
      dst->val_default.val_str = bg_strdup(dst->val_default.val_str,
                                           src->val_default.val_str);
      
      dst->multi_names        = copy_string_array(src->multi_names);
      dst->multi_labels       = copy_string_array(src->multi_labels);
      dst->multi_descriptions = copy_string_array(src->multi_descriptions);

      i = 0;

      if(src->multi_names)
        {
        num_options = 0;
        
        while(src->multi_names[num_options])
          num_options++;
        
        dst->multi_parameters = calloc(num_options, sizeof(*(src->multi_parameters)));
        i = 0;
        
        while(src->multi_names[i])
          {
          if(src->multi_parameters[i])
            dst->multi_parameters[i] =
              bg_parameter_info_copy_array(src->multi_parameters[i]);
          i++;
          }
        }
      
      break;
    case BG_PARAMETER_STRINGLIST:
      dst->val_default.val_str = bg_strdup(dst->val_default.val_str,
                                           src->val_default.val_str);

      /* Copy stringlist options */
      
      if(src->options)
        dst->options = copy_string_array(src->options);
      break;
    case BG_PARAMETER_COLOR_RGB:
      if(src->val_default.val_color)
        {
        dst->val_default.val_color = malloc(4 * sizeof(float));
        memcpy(dst->val_default.val_color,
               src->val_default.val_color,
               3 * sizeof(float));
        dst->val_default.val_color[3] = 1.0;
        }
      break;
    case BG_PARAMETER_COLOR_RGBA:
      if(src->val_default.val_color)
        {
        dst->val_default.val_color = malloc(4 * sizeof(float));
        memcpy(dst->val_default.val_color,
               src->val_default.val_color,
               4 * sizeof(float));
        }
      break;
    case BG_PARAMETER_TIME:
      dst->val_default.val_time = src->val_default.val_time;
      break;
    case BG_PARAMETER_SECTION:
      break;
    }
  
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
    bg_parameter_info_copy(&(ret[i]), &(src[i]));

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
    switch(info[index].type)
      {
      case BG_PARAMETER_COLOR_RGB:
      case BG_PARAMETER_COLOR_RGBA:
        if(info[index].val_default.val_color)
          free(info[index].val_default.val_color);
        break;
      case BG_PARAMETER_STRINGLIST:
        free_string_array(info[index].options);
        if(info[index].val_default.val_str)
          free(info[index].val_default.val_str);
        break;
      case BG_PARAMETER_STRING:
      case BG_PARAMETER_FONT:
      case BG_PARAMETER_DEVICE:
      case BG_PARAMETER_FILE:
      case BG_PARAMETER_DIRECTORY:
        if(info[index].val_default.val_str)
          free(info[index].val_default.val_str);
        break;
      case BG_PARAMETER_SECTION:
      case BG_PARAMETER_CHECKBUTTON:
      case BG_PARAMETER_INT:
      case BG_PARAMETER_FLOAT:
      case BG_PARAMETER_TIME:
      case BG_PARAMETER_SLIDER_INT:
      case BG_PARAMETER_SLIDER_FLOAT:
        break;
      case BG_PARAMETER_MULTI_MENU:
      case BG_PARAMETER_MULTI_LIST:

        i = 0;
        while(info[index].multi_names[i])
          {
          if(info[index].multi_parameters[i])
            bg_parameter_info_destroy_array(info[index].multi_parameters[i]);
          i++;
          }
        free(info[index].multi_parameters);
        free_string_array(info[index].multi_names);
        free_string_array(info[index].multi_labels);
        free_string_array(info[index].multi_descriptions);
                
        if(info[index].val_default.val_str)
          free(info[index].val_default.val_str);
        
        
      }
    index++;
    }
  free(info);
  }
