/*****************************************************************
 
  pluginreg_xml.c
 
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
#include <locale.h>

#include <pluginregistry.h>
#include <utils.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

static struct
  {
  char * name;
  bg_plugin_type_t type;
  }
type_names[] =
  {
    { "Input",         BG_PLUGIN_INPUT },
    { "OutputAudio",   BG_PLUGIN_OUTPUT_AUDIO },
    { "OutputVideo",   BG_PLUGIN_OUTPUT_VIDEO },
    { "AudioRecorder", BG_PLUGIN_RECORDER_AUDIO },
    { "VideoRecorder", BG_PLUGIN_RECORDER_VIDEO },
    { "EncoderAudio",  BG_PLUGIN_ENCODER_AUDIO },
    { "EncoderVideo",  BG_PLUGIN_ENCODER_VIDEO },
    { "Encoder",       BG_PLUGIN_ENCODER },
    { "ImageReader",   BG_PLUGIN_IMAGE_READER },
    { (char*)0,        BG_PLUGIN_NONE }
  };

static struct
  {
  char * name;
  int flag;
  }
flag_names[] =
  {
    { "Removable", BG_PLUGIN_REMOVABLE }, /* Removable media (CD, DVD etc.) */
    { "Recorder",  BG_PLUGIN_RECORDER  }, /* Plugin can record              */
    { "File",      BG_PLUGIN_FILE      }, /* Plugin reads/writes files      */
    { "URL",       BG_PLUGIN_URL       }, /* Plugin reads URLs or streams   */
    { "Playback",  BG_PLUGIN_PLAYBACK  }, /* Output plugins for playback    */
    { (char*)0,    0                   },
  };

static const char * name_key            = "name";
static const char * long_name_key       = "long_name";
static const char * mimetypes_key       = "mimetypes";
static const char * extensions_key      = "extensions";
static const char * module_filename_key = "module_filename";
static const char * module_time_key     = "module_time";
static const char * type_key            = "type";
static const char * flags_key           = "flags";

static bg_plugin_info_t * load_plugin(xmlDocPtr doc, xmlNodePtr node)
  {
  char * tmp_string;
  xmlNodePtr cur;
  int index;
  char * start_ptr;
  char * end_ptr;
  
  bg_plugin_info_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->name = xmlGetProp(node, name_key);
  cur = node->children;
    
  while(cur)
    {
    if(!cur->name)
      {
      cur = cur->next;
      continue;
      }

    tmp_string = xmlNodeListGetString(doc, cur->children, 1);

    if(!strcmp(cur->name, name_key))
      {
      ret->name = bg_strdup(ret->name, tmp_string);
      }
    else if(!strcmp(cur->name, long_name_key))
      {
      ret->long_name = bg_strdup(ret->long_name, tmp_string);
      }
    else if(!strcmp(cur->name, mimetypes_key))
      {
      ret->mimetypes = bg_strdup(ret->mimetypes, tmp_string);
      }
    else if(!strcmp(cur->name, extensions_key))
      {
      ret->extensions = bg_strdup(ret->extensions, tmp_string);
      }
    else if(!strcmp(cur->name, module_filename_key))
      {
      ret->module_filename = bg_strdup(ret->module_filename, tmp_string);
      }
    else if(!strcmp(cur->name, module_time_key))
      {
      sscanf(tmp_string, "%ld", &(ret->module_time));
      }
    else if(!strcmp(cur->name, type_key))
      {
      index = 0;
      while(type_names[index].name)
        {
        if(!strcmp(tmp_string, type_names[index].name))
          {
          ret->type = type_names[index].type;
          break;
          }
        index++;
        }
      }
    else if(!strcmp(cur->name, flags_key))
      {
      start_ptr = tmp_string;

      while(1)
        {
        end_ptr = strchr(start_ptr, '|');
        if(!end_ptr)
          end_ptr = &(start_ptr[strlen(start_ptr)]);

        index = 0;
        while(flag_names[index].name)
          {
          if(!strncmp(flag_names[index].name, start_ptr, end_ptr - start_ptr))
            ret->flags |= flag_names[index].flag;
          index++;
          }
        if(*end_ptr == '\0')
          break;
        start_ptr = end_ptr;
        
        start_ptr++;
        }
      }
    xmlFree(tmp_string);
    cur = cur->next;
    }
  return ret;
  }

static const char * get_flag_name(uint32_t flag)
  {
  int index = 0;
  
  while(flag_names[index].name)
    {
    if(flag_names[index].flag == flag)
      break;
    else
      index++;
    }
  if(!flag_names[index].name)
    fprintf(stderr, "No string defined for flag %0x08\n", flag);
  return flag_names[index].name;
  }

static void save_plugin(xmlNodePtr parent, const bg_plugin_info_t * info)
  {
  char buffer[1024];
  int index;
  int i;
  int num_flags;
  const char * flag_name;
  
  uint32_t flag;
    
  xmlNodePtr xml_plugin;
  xmlNodePtr xml_item;

  xmlAddChild(parent, xmlNewText("\n"));
    
  xml_plugin = xmlNewTextChild(parent, (xmlNsPtr)0, "PLUGIN", NULL);
  xmlAddChild(xml_plugin, xmlNewText("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, name_key, NULL);
  xmlAddChild(xml_item, xmlNewText(info->name));
  xmlAddChild(xml_plugin, xmlNewText("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, long_name_key, NULL);
  xmlAddChild(xml_item, xmlNewText(info->long_name));
  xmlAddChild(xml_plugin, xmlNewText("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, module_filename_key,
                             NULL);
  xmlAddChild(xml_item, xmlNewText(info->module_filename));
  xmlAddChild(xml_plugin, xmlNewText("\n"));
  
  if(info->extensions)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, extensions_key, NULL);
    xmlAddChild(xml_item, xmlNewText(info->extensions));
    xmlAddChild(xml_plugin, xmlNewText("\n"));
    }
  if(info->mimetypes)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, mimetypes_key, NULL);
    xmlAddChild(xml_item, xmlNewText(info->mimetypes));
    xmlAddChild(xml_plugin, xmlNewText("\n"));
    }

  index = 0;
  while(type_names[index].name)
    {
    if(info->type == type_names[index].type)
      {
      xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, type_key, NULL);
      xmlAddChild(xml_item, xmlNewText(type_names[index].name));
      xmlAddChild(xml_plugin, xmlNewText("\n"));
      break;
      }
    index++;
    }

  /* Write flags */
  
  num_flags = 0;

  for(i = 0; i < 32; i++)
    {
    flag = (1<<i);
    if(info->flags & flag)
      num_flags++;
    }
  buffer[0] = '\0';
  index = 0;
  
  for(i = 0; i < 32; i++)
    {
    flag = (1<<i);
    if(!(info->flags & flag))
      continue;
    
    flag_name = get_flag_name(flag);
    strcat(buffer, flag_name);
    if(index < num_flags-1)
      strcat(buffer, "|");
    index++;
    }
  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, flags_key, NULL);
  xmlAddChild(xml_item, xmlNewText(buffer));
  xmlAddChild(xml_plugin, xmlNewText("\n"));
  
  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, module_time_key, NULL);
  sprintf(buffer, "%ld", info->module_time);
  xmlAddChild(xml_item, xmlNewText(buffer));
  xmlAddChild(xml_plugin, xmlNewText("\n"));
  
  }

bg_plugin_info_t * bg_load_plugin_file(const char * filename)
  {
  bg_plugin_info_t * ret;
  bg_plugin_info_t * end;

  xmlDocPtr xml_doc;
  xmlNodePtr node;

  ret = (bg_plugin_info_t *)0;
  end = (bg_plugin_info_t *)0;

  xml_doc = xmlParseFile(filename);

  if(!xml_doc)
    return (bg_plugin_info_t*)0;

  node = xml_doc->children;

  if(strcmp(node->name, "PLUGIN_REGISTRY"))
    {
    fprintf(stderr, "File %s contains no plugin registry\n", filename);
    xmlFreeDoc(xml_doc);
    return (bg_plugin_info_t*)0;
    }

  node = node->children;
    
  while(node)
    {
    if(node->name && !strcmp(node->name, "PLUGIN"))
      {
      if(!ret)
        {
        ret = load_plugin(xml_doc, node);
        end = ret;
        }
      else
        {
        end->next = load_plugin(xml_doc, node);
        end = end->next;
        }
      }
    node = node->next;
    }
      
  xmlFreeDoc(xml_doc);
  return ret;
  }

void bg_save_plugin_file(bg_plugin_info_t * info, const char * filename)
  {
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_registry;
  char * old_locale;
    
  old_locale = setlocale(LC_NUMERIC, "C");
  xml_doc = xmlNewDoc("1.0");
  xml_registry = xmlNewDocRawNode(xml_doc, NULL, "PLUGIN_REGISTRY", NULL);
  xmlDocSetRootElement(xml_doc, xml_registry);
    
  while(info)
    {
    save_plugin(xml_registry, info);
    info = info->next;
    }
  xmlAddChild(xml_registry, xmlNewText("\n"));
  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);
  setlocale(LC_NUMERIC, old_locale);
  }
