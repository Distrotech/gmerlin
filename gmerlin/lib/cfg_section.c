/*****************************************************************
 
  cfg_section.c
 
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
#include <stdio.h>
#include <string.h>

#include <cfg_registry.h>
#include <registry_priv.h>
#include <utils.h>

bg_cfg_section_t * bg_cfg_create_section(const char * name)
  {
  bg_cfg_section_t * ret = calloc(1, sizeof(*ret));
  ret->name = malloc(strlen(name)+1);
  strcpy(ret->name, name);
  return ret;
  }

bg_cfg_section_t * bg_cfg_section_find_subsection(bg_cfg_section_t * s,
                                                  const char * name)
  {
  bg_cfg_section_t * prev_section;
  bg_cfg_section_t * section;

  section = s->children;

  prev_section = (bg_cfg_section_t *)0;

  while(section)
    {
    if(!strcmp(section->name, name))
      return section;
    prev_section = section;
    section = section->next;
    }
  /*  fprintf(stderr, "Creating subsection %s\n", name); */
  if(prev_section)
    {
    prev_section->next = bg_cfg_create_section(name);
    return prev_section->next;
    }
  else
    {
    s->children = bg_cfg_create_section(name);
    return s->children;
    }
  }

int bg_cfg_section_has_subsection(bg_cfg_section_t * s,
                                  const char * name)
  {
  bg_cfg_section_t * section;
  section = s->children;
  while(section)
    {
    if(!strcmp(section->name, name))
      return 1;
    section = section->next;
    }
  return 0;
  }


/*
 *  Get/Set values
 */

bg_cfg_item_t * bg_cfg_section_find_item(bg_cfg_section_t * section,
                                         bg_parameter_info_t * info)
  {
  bg_cfg_item_t * prev;
  bg_cfg_item_t * ret;

  if(!section->items)
    {
    section->items = bg_cfg_create_item(info, NULL);
    return section->items;
    }
  ret = section->items;
  prev = (bg_cfg_item_t*)0;

  while(ret)
    {
    if(!strcmp(ret->name, info->name))
      return ret;
    prev = ret;
    ret = ret->next;
    }
  prev->next = bg_cfg_create_item(info, NULL);
  return prev->next;
  }


void bg_cfg_section_set_parameter(bg_cfg_section_t * section,
                                  bg_parameter_info_t * info,
                                  bg_parameter_value_t * value)
  {
  bg_cfg_item_t * item;
  item = bg_cfg_section_find_item(section, info);

  if(!value)
    return;
  
  switch(item->type)
    {
    case BG_CFG_INT:
      item->value.val_i = value->val_i;
      break;
    case BG_CFG_TIME:
      item->value.val_time = value->val_time;
      break;
    case BG_CFG_FLOAT:
      item->value.val_f = value->val_f;
      break;
    case BG_CFG_STRING:
      if(item->value.val_str)
        {
        free(item->value.val_str);
        item->value.val_str = (char*)0;
        }
      if(value->val_str)
        {
        item->value.val_str = malloc(strlen(value->val_str)+1);
        strcpy(item->value.val_str, value->val_str);
        }
      break;
    case BG_CFG_COLOR:
      item->value.val_color[0] = value->val_color[0];
      item->value.val_color[1] = value->val_color[1];
      item->value.val_color[2] = value->val_color[2];
      item->value.val_color[3] = value->val_color[3];
      break;
      
    }
  
  }

void bg_cfg_section_get_parameter(bg_cfg_section_t * section,
                                  bg_parameter_info_t * info,
                                  bg_parameter_value_t * value)
  {
  bg_cfg_item_t * item;
  item = bg_cfg_section_find_item(section, info);

  if(!value)
    return;
  
  switch(item->type)
    {
    case BG_CFG_INT:
      value->val_i = item->value.val_i;
      break;
    case BG_CFG_TIME:
      value->val_time = item->value.val_time;
      break;
    case BG_CFG_FLOAT:
      value->val_f = item->value.val_f;
      break;
    case BG_CFG_STRING:
      value->val_str = bg_strdup(value->val_str, item->value.val_str);
      break;
    case BG_CFG_COLOR:
      value->val_color[0] = item->value.val_color[0];
      value->val_color[1] = item->value.val_color[1];
      value->val_color[2] = item->value.val_color[2];
      value->val_color[3] = item->value.val_color[3];
      
      break;
    }
  }

void bg_cfg_destroy_section(bg_cfg_section_t * s)
  {
  bg_cfg_item_t    * next_item;
  bg_cfg_section_t * next_section;
  
  while(s->items)
    {
    next_item = s->items->next;
    bg_cfg_destroy_item(s->items);
    s->items = next_item;
    }
  while(s->children)
    {
    next_section = s->children->next;
    bg_cfg_destroy_section(s->children);
    s->children = next_section;
    }
  free(s->name);
  free(s);
  }


void bg_cfg_section_apply(bg_cfg_section_t * section,
                          bg_parameter_info_t * infos,
                          bg_set_parameter_func func,
                          void * callback_data)
  {
  int num;
  bg_cfg_item_t * item;

  num = 0;

  while(infos[num].name)
    {
    item = bg_cfg_section_find_item(section, &(infos[num]));
    func(callback_data, item->name, &(item->value));
    num++;
    }
  func(callback_data, NULL, NULL);
  }

void bg_cfg_section_get(bg_cfg_section_t * section,
                        bg_parameter_info_t * infos,
                        bg_get_parameter_func func,
                        void * callback_data)
  {
  int num;
  bg_cfg_item_t * item;

  if(!func)
    return;
  
  num = 0;

  while(infos[num].name)
    {
    item = bg_cfg_section_find_item(section, &(infos[num]));
    func(callback_data, item->name, &(item->value));
    num++;
    }
  }


static bg_cfg_item_t * find_item_by_name(bg_cfg_section_t * section,
                                         const char * name,
                                         int create_item)
  {
  bg_cfg_item_t * prev;
  bg_cfg_item_t * ret;

  if(!section->items)
    {
    if(create_item)
      section->items = bg_cfg_create_item_empty(name);
    return section->items;
    }
  ret = section->items;
  prev = (bg_cfg_item_t*)0;

  while(ret)
    {
    if(!strcmp(ret->name, name))
      return ret;
    prev = ret;
    ret = ret->next;
    }
  if(create_item)
    {
    prev->next = bg_cfg_create_item_empty(name);
    return prev->next;
    }
  return (bg_cfg_item_t*)0;
  }

void bg_cfg_section_set_parameter_int(bg_cfg_section_t * section,
                                      const char * name, int value)
  {
  bg_cfg_item_t * item;
  item = find_item_by_name(section, name, 1);
  item->type = BG_CFG_INT;
  item->value.val_i = value;
  }

void bg_cfg_section_set_parameter_float(bg_cfg_section_t * section,
                                        const char * name, float value)
  {
  bg_cfg_item_t * item;
  item = find_item_by_name(section, name, 1);
  item->type = BG_CFG_FLOAT;
  item->value.val_f = value;
  }

void bg_cfg_section_set_parameter_string(bg_cfg_section_t * section,
                                         const char * name, const char * value)
  {
  bg_cfg_item_t * item;
  item = find_item_by_name(section, name, 1);
  item->type = BG_CFG_STRING;
  item->value.val_str = bg_strdup(item->value.val_str, value);
  }

/* Get parameter values, return 0 if no such entry */

int bg_cfg_section_get_parameter_int(bg_cfg_section_t * section,
                                     const char * name, int * value)
  {
  bg_cfg_item_t * item;
  item = find_item_by_name(section, name, 0);
  if(!item)
    return 0;
  *value = item->value.val_i;
  return 1;
  }

int bg_cfg_section_get_parameter_float(bg_cfg_section_t * section,
                                        const char * name, float * value)
  {
  bg_cfg_item_t * item;
  item = find_item_by_name(section, name, 0);
  if(!item)
    return 0;
  *value = item->value.val_f;
  return 1;
  
  }

int bg_cfg_section_get_parameter_string(bg_cfg_section_t * section,
                                         const char * name, const char ** value)
  {
  bg_cfg_item_t * item;
  item = find_item_by_name(section, name, 0);
  if(!item)
    return 0;
  *value = item->value.val_str;
  return 1;
  }
