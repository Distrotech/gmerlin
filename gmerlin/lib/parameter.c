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
    case BG_PARAMETER_SECTION:
      break;
    }
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
    case BG_PARAMETER_MULTI_MENU:
    case BG_PARAMETER_MULTI_LIST:
      dst->val_default.val_str = bg_strdup(dst->val_default.val_str,
                                           src->val_default.val_str);
      break;
    case BG_PARAMETER_STRINGLIST:
      dst->val_default.val_str = bg_strdup(dst->val_default.val_str,
                                           src->val_default.val_str);

      /* Copy stringlist options */

      if(src->options)
        {
        num_options = 0;
        while(src->options[num_options])
          num_options++;
        dst->options = calloc(num_options+1,sizeof(char*));
        for(i = 0; i < num_options; i++)
          dst->options[i] = bg_strdup(dst->options[i],
                                      src->options[i]);
        }
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
  int num_codecs;
  int option_index;
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
        if(info[index].options)
          {
          option_index = 0;
          while(info[index].options[option_index])
            {
            free(info[index].options[option_index]);
            option_index++;
            }
          free(info[index].options);
          }
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
      case BG_PARAMETER_SLIDER_INT:
      case BG_PARAMETER_SLIDER_FLOAT:
        break;
      case BG_PARAMETER_MULTI_MENU:
      case BG_PARAMETER_MULTI_LIST:
        if(info[index].val_default.val_str)
          free(info[index].val_default.val_str);

        num_codecs = 0;
        
        while(info[index].multi_names[num_codecs])
          {
          free(info[index].multi_names[num_codecs]);
          num_codecs++;
          }

        if(info[index].multi_labels)
          {
          for(i = 0; i < num_codecs; i++)
            {
            free(info[index].multi_labels[i]);
            }
          free(info[index].multi_labels);
          }

        if(info[index].multi_descriptions)
          {
          for(i = 0; i < num_codecs; i++)
            {
            free(info[index].multi_descriptions[i]);
            }
          free(info[index].multi_descriptions);
          }

        if(info[index].multi_parameters)
          {
          for(i = 0; i < num_codecs; i++)
            {
            if(info[index].multi_parameters[i])
              bg_parameter_info_destroy_array(info[index].multi_parameters[i]);
            }
          free(info[index].multi_parameters[i]);
          }
      }
    index++;
    }

  free(info);
  }
