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

#include <stdlib.h>
#include <string.h>

#include <gmerlin/cfg_registry.h>
#include <registry_priv.h>
#include <gmerlin/utils.h>

bg_cfg_item_t * bg_cfg_item_create_empty(const char * name)
  {
  bg_cfg_item_t * ret = calloc(1, sizeof(*ret));
  ret->name = bg_strdup(ret->name, name);
  return ret;
  }

bg_cfg_item_t * bg_cfg_item_create(const bg_parameter_info_t * info,
                                   bg_parameter_value_t * value)
  {
  bg_cfg_type_t type;
  bg_cfg_item_t * ret;
  switch(info->type)
    {
    case BG_PARAMETER_CHECKBUTTON:
    case BG_PARAMETER_INT:
    case BG_PARAMETER_SLIDER_INT:
      type = BG_CFG_INT;
      break;

    case BG_PARAMETER_FLOAT:
    case BG_PARAMETER_SLIDER_FLOAT:
      type = BG_CFG_FLOAT;
      break;
      
    case BG_PARAMETER_COLOR_RGB:
    case BG_PARAMETER_COLOR_RGBA:
      type = BG_CFG_COLOR;
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
      type = BG_CFG_STRING;
      break;
    case BG_PARAMETER_STRING_HIDDEN:
      type = BG_CFG_STRING_HIDDEN;
      break;
    case BG_PARAMETER_TIME:
      type = BG_CFG_TIME;
      break;
    case BG_PARAMETER_POSITION:
      type = BG_CFG_POSITION;
      break;
    case BG_PARAMETER_SECTION:
    case BG_PARAMETER_BUTTON:
      return (bg_cfg_item_t *)0;
      break;
    }
  
  ret = calloc(1, sizeof(*ret));
  ret->type = type;
  ret->name = bg_strdup(ret->name, info->name);
    
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
      else
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
    case BG_CFG_POSITION:
      if(value)
        {
        ret->value.val_pos[0] = value->val_pos[0];
        ret->value.val_pos[1] = value->val_pos[1];
        }
      else
        {
        ret->value.val_pos[0] = info->val_default.val_pos[0];
        ret->value.val_pos[1] = info->val_default.val_pos[1];
        }
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
    case BG_CFG_POSITION:
      info->type = BG_PARAMETER_POSITION;
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

