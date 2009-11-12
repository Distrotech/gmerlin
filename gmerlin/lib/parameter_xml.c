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

#include <stdlib.h>
#include <string.h>
// #include <locale.h>

#include <gmerlin/parameter.h>
#include <gmerlin/utils.h>
#include <gmerlin/xmlutils.h>

static const struct
  {
  char * name;
  bg_parameter_type_t type;
  }
type_names[] =
  {
    { "section",       BG_PARAMETER_SECTION },
    { "checkbutton",   BG_PARAMETER_CHECKBUTTON },
    { "button",        BG_PARAMETER_BUTTON },
    { "int",           BG_PARAMETER_INT },
    { "float",         BG_PARAMETER_FLOAT },
    { "slider_int",    BG_PARAMETER_SLIDER_INT },
    { "slider_float",  BG_PARAMETER_SLIDER_FLOAT },
    { "string",        BG_PARAMETER_STRING },
    { "string_hidden", BG_PARAMETER_STRING_HIDDEN },
    { "stringlist",    BG_PARAMETER_STRINGLIST },
    { "color_rgb",     BG_PARAMETER_COLOR_RGB },
    { "color_rgba",    BG_PARAMETER_COLOR_RGBA },
    { "font",          BG_PARAMETER_FONT },
    { "device",        BG_PARAMETER_DEVICE },
    { "file",          BG_PARAMETER_FILE },
    { "directory",     BG_PARAMETER_DIRECTORY },
    { "multi_menu",    BG_PARAMETER_MULTI_MENU },
    { "multi_list",    BG_PARAMETER_MULTI_LIST },
    { "multi_chain",   BG_PARAMETER_MULTI_CHAIN },
    { "time",          BG_PARAMETER_TIME },
    { "pos",           BG_PARAMETER_POSITION }
  };

static const char * type_2_name(bg_parameter_type_t type)
  {
  int i;

  for(i = 0; i < sizeof(type_names)/sizeof(type_names[0]); i++)
    {
    if(type_names[i].type == type)
      return type_names[i].name;
    }
  return (char*)0;
  }

static bg_parameter_type_t name_2_type(const char * name)
  {
  int i;

  for(i = 0; i < sizeof(type_names)/sizeof(type_names[0]); i++)
    {
    if(!strcmp(type_names[i].name, name))
      return type_names[i].type;
    }
  return 0;
  }

static const struct
  {
  char * name;
  int flag;
  }
flag_names[] =
  {
    { "sync",        BG_PARAMETER_SYNC        },
    { "hide_dialog", BG_PARAMETER_HIDE_DIALOG },
    { "no_sort",     BG_PARAMETER_NO_SORT     },
    { "plugin",      BG_PARAMETER_PLUGIN      },
  };

static int string_to_flags(const char * str)
  {
  int i;
  int ret = 0;
  const char * start;
  const char * end;

  start = str;
  end = start;
  
  while(1)
    {
    while((*end != '\0') && (*end != '|'))
      end++;
    
    for(i = 0; i < sizeof(flag_names)/sizeof(flag_names[0]); i++)
      {
      if(!strncmp(flag_names[i].name, start, (int)(end-start)))
        ret |= flag_names[i].flag;
      }

    if(*end == '\0')
      return ret;
    
    end++;
    start = end;
    }
  return ret;
  }

static char * flags_to_string(int flags)
  {
  int num = 0;
  int i;
  char * ret = (char*)0;

  for(i = 0; i < sizeof(flag_names)/sizeof(flag_names[0]); i++)
    {
    if(flag_names[i].flag & flags)
      {
      if(num) ret = bg_strcat(ret, "|");
      ret = bg_strcat(ret, flag_names[i].name);
      num++;
      }
    }
  return ret;
  }

static const char * const name_key             = "NAME";
static const char * const long_name_key        = "LONG_NAME";
static const char * const opt_key              = "OPT";
static const char * const type_key             = "TYPE";
static const char * const flags_key             = "FLAGS";
static const char * const help_string_key      = "HELP_STRING";
static const char * const default_key          = "DEFAULT";
static const char * const range_key            = "RANGE";
static const char * const num_digits_key       = "NUM_DIGITS";
static const char * const num_key              = "NUM";
static const char * const index_key            = "INDEX";
static const char * const parameter_key        = "PARAMETER";
static const char * const multi_names_key      = "MULTI_NAMES";
static const char * const multi_name_key       = "MULTI_NAME";
static const char * const multi_descs_key       = "MULTI_DESCS";
static const char * const multi_desc_key       = "MULTI_DESC";
static const char * const multi_labels_key     = "MULTI_LABELS";
static const char * const multi_label_key      = "MULTI_LABEL";
static const char * const multi_parameters_key = "MULTI_PARAMETERS";
static const char * const multi_parameter_key  = "MULTI_PARAMETER";

static const char * const gettext_domain_key     = "GETTEXT_DOMAIN";
static const char * const gettext_directory_key  = "GETTEXT_DIRECTORY";

/* */

bg_parameter_info_t * bg_xml_2_parameters(xmlDocPtr xml_doc,
                                          xmlNodePtr xml_parameters)
  {
  xmlNodePtr child, grandchild, cur;
  int index = 0;
  int multi_index, multi_num;
  int num_parameters;
  char * tmp_string;
  bg_parameter_info_t * ret = (bg_parameter_info_t *)0;
  
  tmp_string = BG_XML_GET_PROP(xml_parameters, num_key);
  num_parameters = atoi(tmp_string);
  free(tmp_string);
  
  ret = calloc(num_parameters+1, sizeof(*ret));
  
  cur = xml_parameters->children;

  while(cur)
    {
    if(!cur->name)
      {
      cur = cur->next;
      continue;
      }

    if(!BG_XML_STRCMP(cur->name, parameter_key))
      {
      tmp_string = BG_XML_GET_PROP(cur, type_key);
      ret[index].type = name_2_type(tmp_string);
      free(tmp_string);

      tmp_string = BG_XML_GET_PROP(cur, name_key);
      ret[index].name = bg_strdup(ret[index].name, tmp_string);
      free(tmp_string);
      
      child = cur->children;

      while(child)
        {
        if(!child->name)
          {
          child = child->next;
          continue;
          }
        
        if(!BG_XML_STRCMP(child->name, long_name_key))
          {
          tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
          ret[index].long_name = bg_strdup(ret[index].long_name, tmp_string);
          free(tmp_string);
          }
        else if(!BG_XML_STRCMP(child->name, flags_key))
          {
          tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
          ret[index].flags = string_to_flags(tmp_string);
          free(tmp_string);
          }
        else if(!BG_XML_STRCMP(child->name, opt_key))
          {
          tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
          ret[index].opt = bg_strdup(ret[index].opt, tmp_string);
          free(tmp_string);
          }
        else if(!BG_XML_STRCMP(child->name, help_string_key))
          {
          tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
          ret[index].help_string = bg_strdup(ret[index].help_string, tmp_string);
          free(tmp_string);
          }
        else if(!BG_XML_STRCMP(child->name, gettext_domain_key))
          {
          tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
          ret[index].gettext_domain = bg_strdup(ret[index].gettext_domain, tmp_string);
          free(tmp_string);
          }
        else if(!BG_XML_STRCMP(child->name, gettext_directory_key))
          {
          tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
          ret[index].gettext_directory = bg_strdup(ret[index].gettext_directory, tmp_string);
          free(tmp_string);
          }
        else if(!BG_XML_STRCMP(child->name, multi_names_key))
          {
          tmp_string = BG_XML_GET_PROP(child, num_key);
          multi_num = atoi(tmp_string);
          free(tmp_string);

          ret[index].multi_names_nc = calloc(multi_num+1, sizeof(*(ret[index].multi_names)));
          multi_index = 0;
          
          grandchild = child->children;

          while(grandchild)
            {
            if(!grandchild->name)
              {
              grandchild = grandchild->next;
              continue;
              }
            if(!BG_XML_STRCMP(grandchild->name, multi_name_key))
              {
              tmp_string = (char*)xmlNodeListGetString(xml_doc, grandchild->children, 1);
              ret[index].multi_names_nc[multi_index] =
                bg_strdup(ret[index].multi_names_nc[multi_index], tmp_string);
              free(tmp_string);
              multi_index++;
              }
            grandchild = grandchild->next;
            }
          }
        else if(!BG_XML_STRCMP(child->name, multi_descs_key))
          {
          tmp_string = BG_XML_GET_PROP(child, num_key);
          multi_num = atoi(tmp_string);
          free(tmp_string);
          
          ret[index].multi_descriptions_nc = calloc(multi_num+1, sizeof(*(ret[index].multi_descriptions_nc)));
          multi_index = 0;
          
          grandchild = child->children;

          while(grandchild)
            {
            if(!grandchild->name)
              {
              grandchild = grandchild->next;
              continue;
              }
            if(!BG_XML_STRCMP(grandchild->name, multi_desc_key))
              {
              tmp_string = (char*)xmlNodeListGetString(xml_doc, grandchild->children, 1);
              ret[index].multi_descriptions_nc[multi_index] =
                bg_strdup(ret[index].multi_descriptions_nc[multi_index], tmp_string);
              free(tmp_string);
              multi_index++;
              }
            grandchild = grandchild->next;
            }
          }
        
        else if(!BG_XML_STRCMP(child->name, multi_labels_key))
          {
          tmp_string = BG_XML_GET_PROP(child, num_key);
          multi_num = atoi(tmp_string);
          free(tmp_string);

          ret[index].multi_labels_nc = calloc(multi_num+1, sizeof(*(ret[index].multi_labels)));
          multi_index = 0;
          
          grandchild = child->children;

          while(grandchild)
            {
            if(!grandchild->name)
              {
              grandchild = grandchild->next;
              continue;
              }
            if(!BG_XML_STRCMP(grandchild->name, multi_label_key))
              {
              tmp_string = (char*)xmlNodeListGetString(xml_doc, grandchild->children, 1);
              ret[index].multi_labels_nc[multi_index] =
                bg_strdup(ret[index].multi_labels_nc[multi_index], tmp_string);
              free(tmp_string);
              multi_index++;
              }
            grandchild = grandchild->next;
            }
          }
        else if(!BG_XML_STRCMP(child->name, multi_parameters_key))
          {
          tmp_string = BG_XML_GET_PROP(child, num_key);
          multi_num = atoi(tmp_string);
          free(tmp_string);

          ret[index].multi_parameters_nc = calloc(multi_num+1, sizeof(*(ret[index].multi_labels)));
          grandchild = child->children;

          while(grandchild)
            {
            if(!grandchild->name)
              {
              grandchild = grandchild->next;
              continue;
              }
            if(!BG_XML_STRCMP(grandchild->name, multi_parameter_key))
              {
              tmp_string = BG_XML_GET_PROP(grandchild, index_key);
              multi_index = atoi(tmp_string);
              free(tmp_string);

              ret[index].multi_parameters_nc[multi_index] =
                bg_xml_2_parameters(xml_doc, grandchild);
              }
            grandchild = grandchild->next;
            }
          }
        else if(!BG_XML_STRCMP(child->name, default_key))
          {
          tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
          switch(ret[index].type)
            {
            case BG_PARAMETER_STRING_HIDDEN:
            case BG_PARAMETER_SECTION:
            case BG_PARAMETER_BUTTON:
              break;
            case BG_PARAMETER_SLIDER_INT:
            case BG_PARAMETER_CHECKBUTTON:
            case BG_PARAMETER_INT:
              sscanf(tmp_string, "%d", &(ret[index].val_default.val_i));
              break;
            case BG_PARAMETER_FLOAT:
            case BG_PARAMETER_SLIDER_FLOAT:
              sscanf(tmp_string, "%lf", &(ret[index].val_default.val_f));
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
              ret[index].val_default.val_str = bg_strdup(ret[index].val_default.val_str,
                                                         tmp_string);
              break;
            case BG_PARAMETER_COLOR_RGB:
              sscanf(tmp_string, "%f %f %f",
                     &(ret[index].val_default.val_color[0]),
                     &(ret[index].val_default.val_color[1]),
                     &(ret[index].val_default.val_color[2]));
              break;
            case BG_PARAMETER_COLOR_RGBA:
              sscanf(tmp_string, "%f %f %f %f",
                     &(ret[index].val_default.val_color[0]),
                     &(ret[index].val_default.val_color[1]),
                     &(ret[index].val_default.val_color[2]),
                     &(ret[index].val_default.val_color[3]));
              break;
            case BG_PARAMETER_POSITION:
              sscanf(tmp_string, "%lf %lf",
                     &(ret[index].val_default.val_pos[0]),
                     &(ret[index].val_default.val_pos[1]));
              break;
            case BG_PARAMETER_TIME:
              sscanf(tmp_string, "%" PRId64, &(ret[index].val_default.val_time));
              break;
            }
          free(tmp_string);
          }

        else if(!BG_XML_STRCMP(child->name, range_key))
          {
          tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
          switch(ret[index].type)
            {
            case BG_PARAMETER_STRING_HIDDEN:
            case BG_PARAMETER_SECTION:
            case BG_PARAMETER_CHECKBUTTON:
            case BG_PARAMETER_STRING:
            case BG_PARAMETER_STRINGLIST:
            case BG_PARAMETER_FONT:
            case BG_PARAMETER_DEVICE:
            case BG_PARAMETER_FILE:
            case BG_PARAMETER_DIRECTORY:
            case BG_PARAMETER_MULTI_MENU:
            case BG_PARAMETER_MULTI_LIST:
            case BG_PARAMETER_MULTI_CHAIN:
            case BG_PARAMETER_COLOR_RGB:
            case BG_PARAMETER_COLOR_RGBA:
            case BG_PARAMETER_POSITION:
            case BG_PARAMETER_BUTTON:
              break;
            case BG_PARAMETER_SLIDER_INT:
            case BG_PARAMETER_INT:
              sscanf(tmp_string, "%d %d",
                     &(ret[index].val_min.val_i),
                     &(ret[index].val_max.val_i));
              break;
            case BG_PARAMETER_FLOAT:
            case BG_PARAMETER_SLIDER_FLOAT:
              sscanf(tmp_string, "%lf %lf",
                     &(ret[index].val_min.val_f),
                     &(ret[index].val_max.val_f));
              break;
            case BG_PARAMETER_TIME:
              sscanf(tmp_string, "%" PRId64 " %" PRId64,
                     &(ret[index].val_min.val_time),
                     &(ret[index].val_max.val_time));
              break;
            }
          free(tmp_string);
          }
        
        else if(!BG_XML_STRCMP(child->name, num_digits_key))
          {
          tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
          sscanf(tmp_string, "%d", &(ret[index].num_digits));
          free(tmp_string);
          }
        
        child = child->next;
        }
      bg_parameter_info_set_const_ptrs(&ret[index]);
      index++;
      
      }
    
    cur = cur->next;
    }
  
  return ret;
  }

/* */

void bg_parameters_2_xml(const bg_parameter_info_t * info, xmlNodePtr xml_parameters)
  {
  int multi_num, i;
  xmlNodePtr xml_info;
  xmlNodePtr child, grandchild;
  int num_parameters = 0;
  char * tmp_string;
  
  xmlAddChild(xml_parameters, BG_XML_NEW_TEXT("\n"));
  while(info[num_parameters].name)
    {
    xml_info = xmlNewTextChild(xml_parameters, (xmlNsPtr)0, (xmlChar*)parameter_key, NULL);
    BG_XML_SET_PROP(xml_info, name_key, info[num_parameters].name);
    BG_XML_SET_PROP(xml_info, type_key, type_2_name(info[num_parameters].type));
    xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));

    if(info[num_parameters].long_name)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)long_name_key, NULL);
      xmlAddChild(child, BG_XML_NEW_TEXT(info[num_parameters].long_name));
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }
    
    if(info[num_parameters].opt)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)opt_key, NULL);
      xmlAddChild(child, BG_XML_NEW_TEXT(info[num_parameters].opt));
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }

    if(info[num_parameters].flags)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)flags_key, NULL);

      tmp_string = flags_to_string(info[num_parameters].flags);
      xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
      free(tmp_string);
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }


    if(info[num_parameters].help_string)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)help_string_key, NULL);
      xmlAddChild(child, BG_XML_NEW_TEXT(info[num_parameters].help_string));
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }
    if(info[num_parameters].gettext_domain)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)gettext_domain_key, NULL);
      xmlAddChild(child, BG_XML_NEW_TEXT(info[num_parameters].gettext_domain));
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }
    if(info[num_parameters].gettext_directory)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)gettext_directory_key, NULL);
      xmlAddChild(child, BG_XML_NEW_TEXT(info[num_parameters].gettext_directory));
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }
    
    multi_num = 0;
    if(info[num_parameters].multi_names)
      {
      while(info[num_parameters].multi_names[multi_num])
        multi_num++;

      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)multi_names_key, NULL);
      xmlAddChild(child, BG_XML_NEW_TEXT("\n"));
      
      tmp_string = bg_sprintf("%d", multi_num);
      BG_XML_SET_PROP(child, num_key, tmp_string);
      free(tmp_string);
            

      for(i = 0; i < multi_num; i++)
        {
        grandchild = xmlNewTextChild(child, (xmlNsPtr)0, (xmlChar*)multi_name_key, NULL);
        xmlAddChild(grandchild, BG_XML_NEW_TEXT(info[num_parameters].multi_names[i]));
        xmlAddChild(child, BG_XML_NEW_TEXT("\n"));
        }
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }

    if(info[num_parameters].multi_labels)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)multi_labels_key, NULL);
      xmlAddChild(child, BG_XML_NEW_TEXT("\n"));
      
      tmp_string = bg_sprintf("%d", multi_num);
      BG_XML_SET_PROP(child, num_key, tmp_string);
      free(tmp_string);
      
      for(i = 0; i < multi_num; i++)
        {
        grandchild = xmlNewTextChild(child, (xmlNsPtr)0, (xmlChar*)multi_label_key, NULL);
        xmlAddChild(grandchild, BG_XML_NEW_TEXT(info[num_parameters].multi_labels[i]));
        xmlAddChild(child, BG_XML_NEW_TEXT("\n"));
        }
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }

    if(info[num_parameters].multi_descriptions)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)multi_descs_key, NULL);
      xmlAddChild(child, BG_XML_NEW_TEXT("\n"));
      
      tmp_string = bg_sprintf("%d", multi_num);
      BG_XML_SET_PROP(child, num_key, tmp_string);
      free(tmp_string);
      
      for(i = 0; i < multi_num; i++)
        {
        grandchild = xmlNewTextChild(child, (xmlNsPtr)0, (xmlChar*)multi_desc_key, NULL);
        xmlAddChild(grandchild, BG_XML_NEW_TEXT(info[num_parameters].multi_descriptions[i]));
        xmlAddChild(child, BG_XML_NEW_TEXT("\n"));
        }
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }
    
    if(info[num_parameters].multi_parameters)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)multi_parameters_key, NULL);
      xmlAddChild(child, BG_XML_NEW_TEXT("\n"));
      
      tmp_string = bg_sprintf("%d", multi_num);
      BG_XML_SET_PROP(child, num_key, tmp_string);
      free(tmp_string);
      
      for(i = 0; i < multi_num; i++)
        {
        if(info[num_parameters].multi_parameters[i])
          {
          grandchild = xmlNewTextChild(child, (xmlNsPtr)0, (xmlChar*)multi_parameter_key, NULL);

          tmp_string = bg_sprintf("%d", i);
          BG_XML_SET_PROP(grandchild, index_key, tmp_string);
          free(tmp_string);

          bg_parameters_2_xml(info[num_parameters].multi_parameters[i], grandchild);
          
          xmlAddChild(child, BG_XML_NEW_TEXT("\n"));
          }
        }
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }
    
    switch(info[num_parameters].type)
      {
      case BG_PARAMETER_SECTION:
      case BG_PARAMETER_BUTTON:
        break;
      case BG_PARAMETER_CHECKBUTTON:
        if(info[num_parameters].val_default.val_i)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)default_key, NULL);
          xmlAddChild(child, BG_XML_NEW_TEXT("1"));
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }
        break;
      case BG_PARAMETER_INT:
      case BG_PARAMETER_SLIDER_INT:
        if(info[num_parameters].val_default.val_i)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)default_key, NULL);

          tmp_string = bg_sprintf("%d", info[num_parameters].val_default.val_i);
          xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }
        if(info[num_parameters].val_min.val_i < info[num_parameters].val_max.val_i)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)range_key, NULL);

          tmp_string = bg_sprintf("%d %d",
                                  info[num_parameters].val_min.val_i,
                                  info[num_parameters].val_max.val_i);
          xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }


        break;
      case BG_PARAMETER_FLOAT:
      case BG_PARAMETER_SLIDER_FLOAT:
        if(info[num_parameters].val_default.val_f != 0.0)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)default_key, NULL);

          tmp_string = bg_sprintf("%f", info[num_parameters].val_default.val_f);
          xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }
        if(info[num_parameters].val_min.val_f <
           info[num_parameters].val_max.val_f)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0,
                                  (xmlChar*)range_key, NULL);

          tmp_string = bg_sprintf("%f %f",
                                  info[num_parameters].val_min.val_f,
                                  info[num_parameters].val_max.val_f);
          xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }
        break;
      case BG_PARAMETER_TIME:
        if(info[num_parameters].val_default.val_time)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)default_key, NULL);

          tmp_string = bg_sprintf("%d", info[num_parameters].val_default.val_i);
          xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }
        if(info[num_parameters].val_min.val_time <
           info[num_parameters].val_max.val_time)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)range_key, NULL);

          tmp_string = bg_sprintf("%" PRId64 " %" PRId64, info[num_parameters].val_min.val_time,
                                  info[num_parameters].val_max.val_time);
          xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }
        break;
      case BG_PARAMETER_STRING:
      case BG_PARAMETER_FONT:
      case BG_PARAMETER_FILE:
      case BG_PARAMETER_DIRECTORY:
      case BG_PARAMETER_DEVICE:
      case BG_PARAMETER_MULTI_MENU:
      case BG_PARAMETER_MULTI_LIST:
      case BG_PARAMETER_MULTI_CHAIN:
      case BG_PARAMETER_STRINGLIST:
        if(info[num_parameters].val_default.val_str)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)default_key, NULL);

          xmlAddChild(child, BG_XML_NEW_TEXT(info[num_parameters].val_default.val_str));
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }
        break;

      case BG_PARAMETER_STRING_HIDDEN:
        break; /* Hidden strings never have defaults */
        
      case BG_PARAMETER_COLOR_RGB:
        if(info[num_parameters].val_default.val_color)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)default_key, NULL);

          tmp_string = bg_sprintf("%f %f %f",
                                  info[num_parameters].val_default.val_color[0],
                                  info[num_parameters].val_default.val_color[1],
                                  info[num_parameters].val_default.val_color[2]);
          xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));

          }
        break;
      case BG_PARAMETER_COLOR_RGBA:
        if(info[num_parameters].val_default.val_color)
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)default_key, NULL);

          tmp_string = bg_sprintf("%f %f %f %f",
                                  info[num_parameters].val_default.val_color[0],
                                  info[num_parameters].val_default.val_color[1],
                                  info[num_parameters].val_default.val_color[2],
                                  info[num_parameters].val_default.val_color[3]);
          xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }
        break;
      case BG_PARAMETER_POSITION:
        if((info[num_parameters].val_default.val_pos[0] != 0.0) ||
           (info[num_parameters].val_default.val_pos[1] != 0.0))
          {
          child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)default_key, NULL);

          tmp_string = bg_sprintf("%f %f",
                                  info[num_parameters].val_default.val_pos[0],
                                  info[num_parameters].val_default.val_pos[1]);
          xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          
          xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
          }
        break;
      }

    if(info[num_parameters].num_digits)
      {
      child = xmlNewTextChild(xml_info, (xmlNsPtr)0, (xmlChar*)num_digits_key, NULL);
      
      tmp_string = bg_sprintf("%d", info[num_parameters].num_digits);
      xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
      free(tmp_string);
      
      xmlAddChild(xml_info, BG_XML_NEW_TEXT("\n"));
      }
    
    xmlAddChild(xml_parameters, BG_XML_NEW_TEXT("\n"));
    
    num_parameters++;
    }
  tmp_string = bg_sprintf("%d", num_parameters);
  BG_XML_SET_PROP(xml_parameters, num_key, tmp_string);
  free(tmp_string);
  }
