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

bg_cfg_section_t * bg_cfg_section_create(const char * name)
  {
  bg_cfg_section_t * ret = calloc(1, sizeof(*ret));
  ret->name = bg_strdup(ret->name, name);
  return ret;
  }

bg_cfg_section_t *
bg_cfg_section_create_from_parameters(const char * name,
                                      bg_parameter_info_t * parameters)
  {
  bg_cfg_section_t * ret;
  bg_cfg_section_t * last_child = (bg_cfg_section_t*)0;
  bg_cfg_section_t * last_subchild;
  
  int i, j;
  
  ret = bg_cfg_section_create(name);

  i = 0;
  while(parameters[i].name)
    {
    bg_cfg_section_find_item(ret, &(parameters[i]));
    i++;
    }

  /* Add child sections */
  
  i = 0;
  while(parameters[i].name)
    {
    if((parameters[i].type == BG_PARAMETER_MULTI_LIST) ||
       (parameters[i].type == BG_PARAMETER_MULTI_MENU))
      {
      if(!last_child)
        {
        ret->children = bg_cfg_section_create(parameters[i].name);
        last_child = ret->children;
        }
      else
        {
        last_child->next = bg_cfg_section_create(parameters[i].name);
        last_child = last_child->next;
        }
      j = 0;

      last_subchild = (bg_cfg_section_t*)0;
      while(parameters[i].multi_names[j])
        {
        if(parameters[i].multi_parameters[j])
          {
          if(!last_subchild)
            {
            last_child->children =
              bg_cfg_section_create_from_parameters(parameters[i].multi_names[j],
                                                    parameters[i].multi_parameters[j]);
            last_subchild = last_child->children;
            }
          else
            {
            last_subchild->next =
              bg_cfg_section_create_from_parameters(parameters[i].multi_names[j],
                                                    parameters[i].multi_parameters[j]);
            last_subchild = last_subchild->next;
            }
          }
        j++;
        }
      }
    i++;
    }
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
    prev_section->next = bg_cfg_section_create(name);
    return prev_section->next;
    }
  else
    {
    s->children = bg_cfg_section_create(name);
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
    section->items = bg_cfg_item_create(info, NULL);
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
  prev->next = bg_cfg_item_create(info, NULL);
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
    case BG_CFG_STRING_HIDDEN:
      item->value.val_str = bg_strdup(item->value.val_str,
                                      value->val_str);
      break;
    case BG_CFG_COLOR:
      item->value.val_color[0] = value->val_color[0];
      item->value.val_color[1] = value->val_color[1];
      item->value.val_color[2] = value->val_color[2];
      item->value.val_color[3] = value->val_color[3];
      break;
      
    }
  
  }

/* Returns characters read or 0 */
int bg_cfg_section_set_parameter_from_string(bg_cfg_section_t * section,
                                             bg_parameter_info_t * info,
                                             const char * str)
  {
  char * end;
  const char * end_c;
  const char * pos;
  bg_cfg_item_t * item;
  item = bg_cfg_section_find_item(section, info);
  
  if(!str)
    return 0;
  
  switch(item->type)
    {
    case BG_CFG_INT:
      item->value.val_i = strtol(str, &end, 10);
      return end - str;
      break;
    case BG_CFG_TIME:
      return gavl_time_parse(str, &(item->value.val_time));
      break;
    case BG_CFG_FLOAT:
      item->value.val_f = strtod(str, &end);
      return end - str;
      break;
    case BG_CFG_STRING:
    case BG_CFG_STRING_HIDDEN:
      end_c = str;
      while(!((*end_c == ',') && (*(end_c-1) != '\\')) &&
            (*end_c != '\0'))
        end_c++;
      
      item->value.val_str = bg_strndup(item->value.val_str,
                                       str, end_c);
      //      fprintf(stderr, "String: %s\n", item->value.val_str);
      return end_c - str;
      break;
    case BG_CFG_COLOR:
      pos = str;
      if(*pos == '\0')
        return 0;
      item->value.val_color[0] = strtod(pos, &end);
      pos = end;

      if(*pos == '\0')
        return 0;
      pos++; // ,
      item->value.val_color[1] = strtod(pos, &end);
      pos = end;

      if(*pos == '\0')
        return 0;
      pos++; // ,
      item->value.val_color[2] = strtod(pos, &end);
      if(pos == end)
        return 0;
      
      pos = end;

      if(info->type == BG_PARAMETER_COLOR_RGBA)
        {
        if(*pos == '\0')
          return 0;
        pos++; // ,
        item->value.val_color[3] = strtod(pos, &end);
        
        if(pos == end)
          return 0;
        }
      return end - pos;
      break;
    }
  return 0;
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
    case BG_CFG_STRING_HIDDEN:
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

void bg_cfg_section_destroy(bg_cfg_section_t * s)
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
    bg_cfg_section_destroy(s->children);
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
  int num, selected;
  bg_cfg_item_t * item;
  bg_cfg_section_t * subsection;
  bg_cfg_section_t * subsubsection;

  num = 0;

  while(infos[num].name)
    {
    item = bg_cfg_section_find_item(section, &(infos[num]));
    func(callback_data, item->name, &(item->value));

    /* If this was a menu, apply children */ 

    if((infos[num].type == BG_PARAMETER_MULTI_MENU) &&
       infos[num].multi_names && infos[num].multi_parameters)
      {
      selected = 0;
      while(infos[num].multi_names[selected] &&
            strcmp(infos[num].multi_names[selected], item->value.val_str))
        selected++;
      if(infos[num].multi_names[selected] && infos[num].multi_parameters[selected])
        {
        subsection    = bg_cfg_section_find_subsection(section, infos[num].name);
        subsubsection = bg_cfg_section_find_subsection(subsection,
                                                       item->value.val_str);
        bg_cfg_section_apply(subsubsection,
                             infos[num].multi_parameters[selected],
                             func, callback_data);
        
        }
      }
    
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
      section->items = bg_cfg_item_create_empty(name);
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
    prev->next = bg_cfg_item_create_empty(name);
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

void bg_cfg_section_set_parameter_time(bg_cfg_section_t * section,
                                       const char * name, gavl_time_t value)
  {
  bg_cfg_item_t * item;
  item = find_item_by_name(section, name, 1);
  item->type = BG_CFG_TIME;
  item->value.val_time = value;
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

int bg_cfg_section_get_parameter_time(bg_cfg_section_t * section,
                                      const char * name, gavl_time_t * value)
  {
  bg_cfg_item_t * item;
  item = find_item_by_name(section, name, 0);
  if(!item)
    return 0;
  *value = item->value.val_time;
  return 1;
  }

/* Copy one config section to another */

bg_cfg_section_t * bg_cfg_section_copy(bg_cfg_section_t * src)
  {
  bg_cfg_item_t * src_item;
  bg_cfg_item_t * end_item = (bg_cfg_item_t*)0;
  bg_cfg_section_t * ret;
  bg_cfg_section_t * src_child;
  bg_cfg_section_t * end_child = (bg_cfg_section_t*)0;
  
  ret = calloc(1, sizeof(*ret));

  ret->name = bg_strdup(ret->name, src->name);

  /* Copy items */
  
  src_item = src->items;
  
  while(src_item)
    {
    if(!ret->items)
      {
      ret->items = bg_cfg_item_copy(src_item);
      end_item = ret->items;
      }
    else
      {
      end_item->next = bg_cfg_item_copy(src_item);
      end_item = end_item->next;
      }
    src_item = src_item->next;
    }

  /* Copy children */

  src_child = src->children;

  while(src_child)
    {
    if(!ret->children)
      {
      ret->children = bg_cfg_section_copy(src_child);
      end_child = ret->children;
      }
    else
      {
      end_child->next = bg_cfg_section_copy(src_child);
      end_child = end_child->next;
      }
    src_child = src_child->next;
    }
  return ret;
  }

void bg_cfg_section_transfer(bg_cfg_section_t * src, bg_cfg_section_t * dst)
  {
  bg_cfg_item_t * src_item;
  
  bg_cfg_section_t * src_child;
  bg_cfg_section_t * dst_child;
  bg_parameter_info_t  info;
  /* Copy items */
  
  src_item = src->items;
  
  while(src_item)
    {
    bg_cfg_item_to_parameter(src_item, &info);
    bg_cfg_section_set_parameter(dst, &info, &(src_item->value));
    src_item = src_item->next;
    }
  
  /* Copy children */
  
  src_child = src->children;
  
  while(src_child)
    {
    dst_child = bg_cfg_section_find_subsection(dst, src_child->name);
    bg_cfg_section_transfer(src_child, dst_child);
    src_child = src_child->next;
    }
  }


const char * bg_cfg_section_get_name(bg_cfg_section_t * s)
  {
  if(!s)
    return (const char*)0;
  return s->name;
  }

void bg_cfg_section_set_name(bg_cfg_section_t * s, const char * name)
  {
  s->name = bg_strdup(s->name, name);
  }

void bg_cfg_section_create_items(bg_cfg_section_t * section,
                                 bg_parameter_info_t * info)
  {
  int i;
  int j;

  bg_cfg_section_t * subsection;
  bg_cfg_section_t * subsubsection;

  i = 0;

  while(info[i].name)
    {
    bg_cfg_section_find_item(section, &(info[i]));

    if((info[i].type == BG_PARAMETER_MULTI_LIST) ||
       (info[i].type == BG_PARAMETER_MULTI_MENU))
      {
      j = 0;

      subsection = bg_cfg_section_find_subsection(section, info[i].name);
  
      while(info[i].multi_names[j])
        {
        if(info[i].multi_parameters[j])
          {
          subsubsection =
            bg_cfg_section_find_subsection(subsection, info[i].multi_names[j]);
          
          bg_cfg_section_create_items(subsubsection,
                                      info[i].multi_parameters[j]);
          }
        j++;
        }
      }
    i++;
    }
  }



void bg_cfg_section_delete_subsection(bg_cfg_section_t * section,
                                   bg_cfg_section_t * subsection)
  {
  bg_cfg_section_t * before;

  if(!section->children)
    return;

  if(section->children == subsection)
    {
    section->children = section->children->next;
    bg_cfg_section_destroy(subsection);
    }
  else
    {
    before = section->children;
    while(before->next)
      {
      if(before->next == subsection)
        break;
      before = before->next;
      }

    if(before->next)
      {
      before->next = before->next->next;
      bg_cfg_section_destroy(subsection);
      }
    }
  
  }

                                  
