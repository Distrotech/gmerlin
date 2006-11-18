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
#include <pluginreg_priv.h>
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
    { "Input",                   BG_PLUGIN_INPUT },
    { "OutputAudio",             BG_PLUGIN_OUTPUT_AUDIO },
    { "OutputVideo",             BG_PLUGIN_OUTPUT_VIDEO },
    { "AudioRecorder",           BG_PLUGIN_RECORDER_AUDIO },
    { "VideoRecorder",           BG_PLUGIN_RECORDER_VIDEO },
    { "EncoderAudio",            BG_PLUGIN_ENCODER_AUDIO },
    { "EncoderVideo",            BG_PLUGIN_ENCODER_VIDEO },
    { "EncoderSubtitleText",     BG_PLUGIN_ENCODER_SUBTITLE_TEXT },
    { "EncoderSubtitleOverlay",  BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY },
    { "Encoder",                 BG_PLUGIN_ENCODER },
    { "EncoderPP",               BG_PLUGIN_ENCODER_PP },
    { "ImageReader",             BG_PLUGIN_IMAGE_READER },
    { "ImageWriter",             BG_PLUGIN_IMAGE_WRITER },
    { (char*)0,                  BG_PLUGIN_NONE }
  };

static struct
  {
  char * name;
  int flag;
  }
flag_names[] =
  {
    { "Removable",    BG_PLUGIN_REMOVABLE      }, /* Removable media (CD, DVD etc.) */
    { "Recorder",     BG_PLUGIN_RECORDER       }, /* Plugin can record              */
    { "File",         BG_PLUGIN_FILE           }, /* Plugin reads/writes files      */
    { "URL",          BG_PLUGIN_URL            }, /* Plugin reads URLs or streams   */
    { "Playback",     BG_PLUGIN_PLAYBACK       }, /* Output plugins for playback    */
    { "Bypass",       BG_PLUGIN_BYPASS         }, /* Plugin can bypass playback engine */
    { "KeepRunning", BG_PLUGIN_KEEP_RUNNING   }, /* Plugin should not be stopped and restarted if tracks change */
    { "HasSync",     BG_PLUGIN_INPUT_HAS_SYNC }, /* FOR INPUTS ONLY: Plugin will set the time via callback */
    { (char*)0,    0                           },
  };

static const char * plugin_key            = "PLUGIN";
static const char * plugin_registry_key   = "PLUGIN_REGISTRY";

static const char * name_key              = "NAME";
static const char * long_name_key         = "LONG_NAME";
static const char * mimetypes_key         = "MIMETYPES";
static const char * extensions_key        = "EXTENSIONS";
static const char * protocols_key         = "PROTOCOLS";
static const char * module_filename_key   = "MODULE_FILENAME";
static const char * module_time_key       = "MODULE_TIME";
static const char * type_key              = "TYPE";
static const char * flags_key             = "FLAGS";
static const char * priority_key          = "PRIORITY";
static const char * device_info_key       = "DEVICE_INFO";
static const char * device_key            = "DEVICE";
static const char * max_audio_streams_key = "MAX_AUDIO_STREAMS";
static const char * max_video_streams_key = "MAX_VIDEO_STREAMS";
static const char * max_subtitle_text_streams_key = "MAX_SUBTITLE_TEXT_STREAMS";
static const char * max_subtitle_overlay_streams_key = "MAX_SUBTITLE_OVERLAY_STREAMS";

static const char * parameters_key       = "PARAMETERS";
static const char * audio_parameters_key = "AUDIO_PARAMETERS";
static const char * video_parameters_key = "VIDEO_PARAMETERS";
static const char * subtitle_text_parameters_key = "SUBTITLE_TEXT_PARAMETERS";
static const char * subtitle_overlay_parameters_key = "SUBTITLE_OVERLAY_PARAMETERS";

static bg_device_info_t *
load_device(bg_device_info_t * arr, xmlDocPtr doc, xmlNodePtr node)
  {
  char * tmp_string;
  xmlNodePtr cur;
  char * device = (char*)0;
  char * name = (char*)0;
  
  cur = node->children;
  while(cur)
    {
    if(!cur->name)
      {
      cur = cur->next;
      continue;
      }
    tmp_string = (char*)xmlNodeListGetString(doc, cur->children, 1);

    if(!BG_XML_STRCMP(cur->name, name_key))
      {
      name = tmp_string;
      tmp_string = (char*)0;
      }
    else if(!BG_XML_STRCMP(cur->name, device_key))
      {
      device = tmp_string;
      tmp_string = (char*)0;
      }
    if(tmp_string)
      free(tmp_string);
    cur =  cur->next;
    }
  
  if(device)
    {
    arr = bg_device_info_append(arr,
                                device,
                                name);
    
    xmlFree(device);
    }
  if(name)
    {
    xmlFree(name);
    }
  return arr;
  }

static bg_plugin_info_t * load_plugin(xmlDocPtr doc, xmlNodePtr node)
  {
  char * tmp_string;
  xmlNodePtr cur;
  int index;
  char * start_ptr;
  char * end_ptr;
  
  bg_plugin_info_t * ret;

  ret = calloc(1, sizeof(*ret));
  
  cur = node->children;
    
  while(cur)
    {
    if(!cur->name)
      {
      cur = cur->next;
      continue;
      }

    if(!BG_XML_STRCMP(cur->name, parameters_key))
      {
      ret->parameters = bg_xml_2_parameters(doc, cur);
      cur = cur->next;
      continue;
      }
    else if(!BG_XML_STRCMP(cur->name, audio_parameters_key))
      {
      ret->audio_parameters = bg_xml_2_parameters(doc, cur);
      cur = cur->next;
      continue;
      }
    else if(!BG_XML_STRCMP(cur->name, video_parameters_key))
      {
      ret->video_parameters = bg_xml_2_parameters(doc, cur);
      cur = cur->next;
      continue;
      }
    else if(!BG_XML_STRCMP(cur->name, subtitle_text_parameters_key))
      {
      ret->subtitle_text_parameters = bg_xml_2_parameters(doc, cur);
      cur = cur->next;
      continue;
      }
    else if(!BG_XML_STRCMP(cur->name, subtitle_overlay_parameters_key))
      {
      ret->subtitle_overlay_parameters = bg_xml_2_parameters(doc, cur);
      cur = cur->next;
      continue;
      }
    
    tmp_string = (char*)xmlNodeListGetString(doc, cur->children, 1);

    if(!BG_XML_STRCMP(cur->name, name_key))
      {
      ret->name = bg_strdup(ret->name, tmp_string);
      }
    else if(!BG_XML_STRCMP(cur->name, long_name_key))
      {
      ret->long_name = bg_strdup(ret->long_name, tmp_string);
      }
    else if(!BG_XML_STRCMP(cur->name, mimetypes_key))
      {
      ret->mimetypes = bg_strdup(ret->mimetypes, tmp_string);
      }
    else if(!BG_XML_STRCMP(cur->name, extensions_key))
      {
      ret->extensions = bg_strdup(ret->extensions, tmp_string);
      }
    else if(!BG_XML_STRCMP(cur->name, protocols_key))
      {
      ret->protocols = bg_strdup(ret->protocols, tmp_string);
      }
    else if(!BG_XML_STRCMP(cur->name, module_filename_key))
      {
      ret->module_filename = bg_strdup(ret->module_filename, tmp_string);
      }
    else if(!BG_XML_STRCMP(cur->name, module_time_key))
      {
      sscanf(tmp_string, "%ld", &(ret->module_time));
      }
    else if(!BG_XML_STRCMP(cur->name, priority_key))
      {
      sscanf(tmp_string, "%d", &(ret->priority));
      }
    else if(!BG_XML_STRCMP(cur->name, type_key))
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
    else if(!BG_XML_STRCMP(cur->name, flags_key))
      {
      start_ptr = tmp_string;
      
      while(1)
        {
        if(!start_ptr) break;
        
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
    else if(!BG_XML_STRCMP(cur->name, device_info_key))
      {
      ret->devices = load_device(ret->devices, doc, cur);
      }
    else if(!BG_XML_STRCMP(cur->name, max_audio_streams_key))
      {
      ret->max_audio_streams = atoi(tmp_string);
      }
    else if(!BG_XML_STRCMP(cur->name, max_video_streams_key))
      {
      ret->max_video_streams = atoi(tmp_string);
      }
    else if(!BG_XML_STRCMP(cur->name, max_subtitle_text_streams_key))
      {
      ret->max_subtitle_text_streams = atoi(tmp_string);
      }
    else if(!BG_XML_STRCMP(cur->name, max_subtitle_overlay_streams_key))
      {
      ret->max_subtitle_overlay_streams = atoi(tmp_string);
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
  return flag_names[index].name;
  }

static void save_devices(xmlNodePtr parent, const bg_device_info_t * info)
  {
  int i;
  xmlNodePtr xml_device, xml_item;

  i = 0;
  while(info[i].device)
    {
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    
    xml_device = xmlNewTextChild(parent, (xmlNsPtr)0,
                                 (xmlChar*)device_info_key, NULL);
        
    xmlAddChild(xml_device, BG_XML_NEW_TEXT("\n"));
    
    xml_item = xmlNewTextChild(xml_device, (xmlNsPtr)0, (xmlChar*)device_key, NULL);
    xmlAddChild(xml_item, BG_XML_NEW_TEXT(info[i].device));
    xmlAddChild(xml_device, BG_XML_NEW_TEXT("\n"));

    if(info[i].name)
      {
      xml_item = xmlNewTextChild(xml_device, (xmlNsPtr)0, (xmlChar*)name_key, NULL);
      xmlAddChild(xml_item, BG_XML_NEW_TEXT(info[i].name));
      xmlAddChild(xml_device, BG_XML_NEW_TEXT("\n"));
      }
    i++;
    }
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

  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    
  xml_plugin = xmlNewTextChild(parent, (xmlNsPtr)0,
                               (xmlChar*)plugin_key, NULL);
  xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)name_key, NULL);
  xmlAddChild(xml_item, BG_XML_NEW_TEXT(info->name));
  xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)long_name_key, NULL);
  xmlAddChild(xml_item, BG_XML_NEW_TEXT(info->long_name));
  xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)module_filename_key,
                             NULL);
  xmlAddChild(xml_item, BG_XML_NEW_TEXT(info->module_filename));
  xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
  
  if(info->extensions)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)extensions_key, NULL);
    xmlAddChild(xml_item, BG_XML_NEW_TEXT(info->extensions));
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    }
  if(info->protocols)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)protocols_key, NULL);
    xmlAddChild(xml_item, BG_XML_NEW_TEXT(info->protocols));
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    }
  if(info->mimetypes)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)mimetypes_key, NULL);
    xmlAddChild(xml_item, BG_XML_NEW_TEXT(info->mimetypes));
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    }

  if(info->parameters)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)parameters_key, NULL);
    bg_parameters_2_xml(info->parameters, xml_item);
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    }
  if(info->audio_parameters)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)audio_parameters_key, NULL);
    bg_parameters_2_xml(info->audio_parameters, xml_item);
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    }
  if(info->video_parameters)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)video_parameters_key, NULL);
    bg_parameters_2_xml(info->video_parameters, xml_item);
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    }
  if(info->subtitle_text_parameters)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)subtitle_text_parameters_key, NULL);
    bg_parameters_2_xml(info->subtitle_text_parameters, xml_item);
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    }
  if(info->subtitle_overlay_parameters)
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)subtitle_overlay_parameters_key, NULL);
    bg_parameters_2_xml(info->subtitle_overlay_parameters, xml_item);
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    }
  
  if(info->type & (BG_PLUGIN_ENCODER_AUDIO|
                   BG_PLUGIN_ENCODER_VIDEO|
                   BG_PLUGIN_ENCODER |
                   BG_PLUGIN_ENCODER_SUBTITLE_TEXT |
                   BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY))
    {
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)max_audio_streams_key, NULL);
    sprintf(buffer, "%d", info->max_audio_streams);
    xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));

    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)max_video_streams_key, NULL);
    sprintf(buffer, "%d", info->max_video_streams);
    xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));

    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)max_subtitle_text_streams_key, NULL);
    sprintf(buffer, "%d", info->max_subtitle_text_streams);
    xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));

    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)max_subtitle_overlay_streams_key, NULL);
    sprintf(buffer, "%d", info->max_subtitle_overlay_streams);
    xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    
    }
  
  index = 0;
  while(type_names[index].name)
    {
    if(info->type == type_names[index].type)
      {
      xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)type_key, NULL);
      xmlAddChild(xml_item, BG_XML_NEW_TEXT(type_names[index].name));
      xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
      break;
      }
    index++;
    }

  /* Write flags */

  if(info->flags)
    {
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
    xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)flags_key, NULL);
    xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
    xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));
    }
  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)module_time_key, NULL);
  sprintf(buffer, "%ld", info->module_time);
  xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
  xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));

  xml_item = xmlNewTextChild(xml_plugin, (xmlNsPtr)0, (xmlChar*)priority_key, NULL);
  sprintf(buffer, "%d", info->priority);
  xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
  xmlAddChild(xml_plugin, BG_XML_NEW_TEXT("\n"));

  if(info->devices && info->devices->device)
    save_devices(xml_plugin, info->devices);
  }

bg_plugin_info_t * bg_plugin_registry_load(const char * filename)
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

  if(BG_XML_STRCMP(node->name, plugin_registry_key))
    {
    xmlFreeDoc(xml_doc);
    return (bg_plugin_info_t*)0;
    }

  node = node->children;
    
  while(node)
    {
    if(node->name && !BG_XML_STRCMP(node->name, plugin_key))
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


void bg_plugin_registry_save(bg_plugin_info_t * info)
  {
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_registry;
  char * filename;

  filename = bg_search_file_write("", "plugins.xml");
  if(!filename)
    {
    return;
    }
  
  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_registry = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)plugin_registry_key, NULL);
  xmlDocSetRootElement(xml_doc, xml_registry);
  while(info)
    {
    //    if(info->module_filename) /* We save only external plugins */
    save_plugin(xml_registry, info);
    info = info->next;
    }
  
  xmlAddChild(xml_registry, BG_XML_NEW_TEXT("\n"));
  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);
  free(filename);
  }
