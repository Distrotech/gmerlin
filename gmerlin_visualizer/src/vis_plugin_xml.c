#include <string.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <gmerlin/utils.h>

#include "vis_plugin.h"

static struct
  {
  char * name;
  vis_plugin_type_t type;
  }
type_names[] = 
  {
    { "xmms1", VIS_PLUGIN_TYPE_XMMS1 },
    { /* End of names */             },
  };

static const char * name_from_type(vis_plugin_type_t type)
  {
  int i = 0;
  while(type_names[i].name)
    {
    if(type_names[i].type == type)
      return type_names[i].name;
    i++;
    }
  return (const char*)0;
  }

static const vis_plugin_type_t type_from_name(char * name)
  {
  int i = 0;
  while(type_names[i].name)
    {
    if(!strcmp(type_names[i].name, name))
      return type_names[i].type;
    i++;
    }
  return VIS_PLUGIN_TYPE_UNDEFINED;
  }

static const char * type_name            = "type";
static const char * name_name            = "name";
static const char * module_filename_name = "module_filename";
static const char * module_time_name     = "module_time";
static const char * enabled_name         = "enabled";
static const char * plugins_name         = "vis_plugins";
static const char * plugin_name          = "plugin";

static vis_plugin_info_t * load_plugin(xmlDocPtr xml_doc, xmlNodePtr node)
  {
  vis_plugin_info_t * ret;
  char * tmp_string;

  ret = calloc(1, sizeof(*ret));
  
  if((tmp_string = xmlGetProp(node, enabled_name)))
    {
    ret->enabled = 1;
    xmlFree(tmp_string);
    }
  
  node = node->children;
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    tmp_string = xmlNodeListGetString(xml_doc, node->children, 1);
    
    if(!strcmp(node->name, type_name))
      {
      ret->type = type_from_name(tmp_string);
      }
    else if(!strcmp(node->name, name_name))
      {
      ret->name = bg_strdup(ret->name, tmp_string);
      }
    else if(!strcmp(node->name, module_filename_name))
      {
      ret->module_filename = bg_strdup(ret->module_filename, tmp_string);
      }
    else if(!strcmp(node->name, module_time_name))
      {
      sscanf(tmp_string, "%ld", &(ret->module_time));
      }
    if(tmp_string)
      xmlFree(tmp_string);
    node = node->next;
    }
  return ret;
  }

vis_plugin_info_t * vis_plugins_load()
  {
  char * filename;
  xmlDocPtr xml_doc;
  xmlNodePtr node;
  vis_plugin_info_t * ret = (vis_plugin_info_t *)0;
  vis_plugin_info_t * new_info, *ret_end = (vis_plugin_info_t *)0;

  filename = bg_search_file_read("visualizer", "vis_plugins.xml");
  if(!filename)
    return (vis_plugin_info_t*)0;
  
  xml_doc = xmlParseFile(filename);

  if(!xml_doc)
    {
    fprintf(stderr, "Couldn't open file %s\n", filename);
    return (vis_plugin_info_t*)0;
    }

  node = xml_doc->children;

  if(strcmp(node->name, plugins_name))
    {
    xmlFreeDoc(xml_doc);
    return (vis_plugin_info_t*)0;
    }

  node = node->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    else if(!strcmp(node->name, plugin_name))
      {
      new_info = load_plugin(xml_doc, node);
      if(!ret_end)
        {
        ret = new_info;
        ret_end = new_info;
        }
      else
        {
        ret_end->next = new_info;
        ret_end = ret_end->next;
        }
      }
    node = node->next;
    }
  xmlFreeDoc(xml_doc);

  free(filename);
  return ret;
  }

static void save_plugin(xmlNodePtr parent, const vis_plugin_info_t * info)
  {
  char * tmp_string;
  xmlNodePtr xml_plugin;
  xmlNodePtr xml_item;
  
  xml_plugin = xmlNewTextChild(parent, (xmlNsPtr)0,
                               plugin_name, NULL);

  if(info->enabled)
    {
    xmlSetProp(xml_plugin, enabled_name, "1");
    }
    
  xmlAddChild(xml_plugin, xmlNewText("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, type_name, NULL);
  xmlAddChild(xml_item, xmlNewText(name_from_type(info->type)));
  xmlAddChild(xml_plugin, xmlNewText("\n"));
  
  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, name_name, NULL);
  xmlAddChild(xml_item, xmlNewText(info->name));
  xmlAddChild(xml_plugin, xmlNewText("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, module_filename_name, NULL);
  xmlAddChild(xml_item, xmlNewText(info->module_filename));
  xmlAddChild(xml_plugin, xmlNewText("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, module_time_name, NULL);

  tmp_string = bg_sprintf("%ld", info->module_time);
  xmlAddChild(xml_item, xmlNewText(tmp_string));
  free(tmp_string);
  
  xmlAddChild(xml_plugin, xmlNewText("\n"));
  
  }


void vis_plugins_save(vis_plugin_info_t * info)
  {
  char * filename;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_plugins;

  filename = bg_search_file_write("visualizer", "vis_plugins.xml");
  if(!filename)
    return;

  
  xml_doc = xmlNewDoc("1.0");
  xml_plugins = xmlNewDocRawNode(xml_doc, NULL, plugins_name, NULL);
  xmlDocSetRootElement(xml_doc, xml_plugins);
  
  while(info)
    {
    save_plugin(xml_plugins, info);
    info = info->next;
    }
  xmlAddChild(xml_plugins, xmlNewText("\n"));
  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);

  
  free(filename);
  }

