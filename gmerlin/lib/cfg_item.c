/*****************************************************************
 
  cfg_item.c
 
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

#include <stdlib.h>
#include <string.h>

#include <cfg_registry.h>
#include <registry_priv.h>
#include <utils.h>

bg_cfg_item_t * bg_cfg_item_create_empty(const char * name)
  {
  bg_cfg_item_t * ret = calloc(1, sizeof(*ret));
  ret->name = bg_strdup(ret->name, name);
  return ret;
  }

bg_cfg_item_t * bg_cfg_item_create(const bg_parameter_info_t * info,
                                   bg_parameter_value_t * value)
  {
  bg_cfg_item_t * ret = calloc(1, sizeof(*ret));
  ret->name = bg_strdup(ret->name, info->name);
  
  switch(info->type)
    {
    case BG_PARAMETER_CHECKBUTTON:
    case BG_PARAMETER_INT:
    case BG_PARAMETER_SLIDER_INT:
      ret->type = BG_CFG_INT;
      break;

    case BG_PARAMETER_FLOAT:
    case BG_PARAMETER_SLIDER_FLOAT:
      ret->type = BG_CFG_FLOAT;
      break;
      
    case BG_PARAMETER_COLOR_RGB:
    case BG_PARAMETER_COLOR_RGBA:
      ret->type = BG_CFG_COLOR;
      break;
    case BG_PARAMETER_STRING:
    case BG_PARAMETER_STRINGLIST:
    case BG_PARAMETER_FONT:
    case BG_PARAMETER_DEVICE:
    case BG_PARAMETER_FILE:
    case BG_PARAMETER_DIRECTORY:
    case BG_PARAMETER_MULTI_MENU:
    case BG_PARAMETER_MULTI_LIST:
    case BG_PARAMETER_MULTI_CHAIN:
      ret->type = BG_CFG_STRING;
      break;
    case BG_PARAMETER_STRING_HIDDEN:
      ret->type = BG_CFG_STRING_HIDDEN;
      break;
    case BG_PARAMETER_TIME:
      ret->type = BG_CFG_TIME;
      break;
    case BG_PARAMETER_SECTION:
      break;
    }

  switch(ret->type)
    {
    case BG_CFG_INT:
      if(value)
        ret->value.val_i = value->val_i;
      else
        ret->value.val_i = info->val_default.val_i;
      break;
    case BG_CFG_TIME:
      if(value)
        ret->value.val_time = value->val_time;
      else
        ret->value.val_time = info->val_default.val_time;
      break;
    case BG_CFG_FLOAT:
      if(value)
        ret->value.val_f = value->val_f;
      else
        ret->value.val_f = info->val_default.val_f;
      break;
    case BG_CFG_COLOR:
      if(value)
        {
        ret->value.val_color[0] = value->val_color[0];
        ret->value.val_color[1] = value->val_color[1];
        ret->value.val_color[2] = value->val_color[2];
        ret->value.val_color[3] = value->val_color[3];
        }
      else if(info->val_default.val_color)
        {
        ret->value.val_color[0] = info->val_default.val_color[0];
        ret->value.val_color[1] = info->val_default.val_color[1];
        ret->value.val_color[2] = info->val_default.val_color[2];
        ret->value.val_color[3] = info->val_default.val_color[3];
        }
      break;
    case BG_CFG_STRING:
    case BG_CFG_STRING_HIDDEN:
      if(value && value->val_str)
        {
        ret->value.val_str = bg_strdup(ret->value.val_str, value->val_str);
        }
      else if(info->val_default.val_str)
        {
        ret->value.val_str = bg_strdup(ret->value.val_str,
                                       info->val_default.val_str);
        }
      break;
    }
  return ret;
  }

void bg_cfg_destroy_item(bg_cfg_item_t * item)
  {
  free(item->name);
  switch(item->type)
    {
    case BG_CFG_STRING:
    case BG_CFG_STRING_HIDDEN:
      if(item->value.val_str)
        free(item->value.val_str);
    default:
      break;
    }
  free(item);
  }

void bg_cfg_item_to_parameter(bg_cfg_item_t * src, bg_parameter_info_t * info)
  {
  memset(info, 0, sizeof(*info));
  info->name = src->name;

  switch(src->type)
    {
    case BG_CFG_INT:
      info->type = BG_PARAMETER_INT;
      break;
    case BG_CFG_FLOAT:
      info->type = BG_PARAMETER_FLOAT;
      break;
    case BG_CFG_STRING:
      info->type = BG_PARAMETER_STRING;
      break;
    case BG_CFG_STRING_HIDDEN:
      info->type = BG_PARAMETER_STRING_HIDDEN;
      break;
    case BG_CFG_COLOR:
      info->type = BG_PARAMETER_COLOR_RGBA;
      break;
    case BG_CFG_TIME: /* int64 */
      info->type = BG_PARAMETER_TIME;
      break;
    }
  }

void bg_cfg_item_transfer(bg_cfg_item_t * src, bg_cfg_item_t * dst)
  {
  bg_parameter_info_t info;
  bg_cfg_item_to_parameter(src, &info);
  bg_parameter_value_copy(&(dst->value), &(src->value), &info);
  }

bg_cfg_item_t * bg_cfg_item_copy(bg_cfg_item_t * src)
  {
  
  bg_cfg_item_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->name = bg_strdup(ret->name, src->name);
  ret->type = src->type;
  bg_cfg_item_transfer(src, ret);
  return ret;
  }

