#include <string.h>
#include <gmerlin/xmlutils.h>
#include <gmerlin/utils.h>

#include <stdlib.h>
#include <medialist.h>

static const char * open_path_name = "open_path";
static const char * files_name = "files";
static const char * file_name = "file";

static const char * audio_streams_name = "audio_streams";
static const char * video_streams_name = "video_streams";
static const char * duration_name      = "duration";
static const char * name_name          = "name";
static const char * filename_name      = "filename";
static const char * track_name         = "track";
static const char * plugin_name        = "plugin";
static const char * parameters_name    = "parameters";

static bg_nle_file_t * load_file(xmlDocPtr xml_doc, xmlNodePtr node)
  {
  xmlNodePtr child;
  bg_nle_file_t * ret;
  char * tmp_string;
  ret = calloc(1, sizeof(*ret));

  if((tmp_string = BG_XML_GET_PROP(child, "id")))
    {
    ret->id = strtoll(tmp_string, (char**)0, 16);
    free(tmp_string);
    }
  
  child = node->children;

  while(child)
    {
    if(!child->name)
      {
      child = child->next;
      continue;
      }

    
    if(!BG_XML_STRCMP(child->name, filename_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
      ret->filename = bg_strdup(ret->filename, tmp_string);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(child->name, name_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
      ret->name = bg_strdup(ret->name, tmp_string);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(child->name, plugin_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
      ret->plugin = bg_strdup(ret->plugin, tmp_string);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(child->name, track_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
      ret->track = atoi(tmp_string);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(child->name, audio_streams_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
      ret->num_audio_streams = atoi(tmp_string);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(child->name, video_streams_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
      ret->num_video_streams = atoi(tmp_string);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(child->name, duration_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
      ret->duration = strtoll(tmp_string, (char**)0, 10);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(child->name, parameters_name))
      {
      ret->section = bg_cfg_section_create("");
      bg_cfg_xml_2_section(xml_doc, child, ret->section);
      }
    child = child->next;
    }
  
  return ret;
  
  }

bg_nle_media_list_t *
bg_nle_media_list_load(bg_plugin_registry_t * plugin_reg,
                       xmlDocPtr xml_doc, xmlNodePtr node)
  {
  xmlNodePtr child;
  xmlNodePtr grandchild;
  char * tmp_string;
  
  bg_nle_media_list_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = plugin_reg;
  
  child = node->children;

  while(child)
    {
    if(!child->name)
      {
      child = child->next;
      continue;
      }

    if(!BG_XML_STRCMP(child->name, open_path_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
      ret->open_path = bg_strdup(ret->open_path, tmp_string);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(child->name, files_name))
      {
      tmp_string = BG_XML_GET_PROP(child, "num");
      ret->files_alloc = atoi(tmp_string);
      xmlFree(tmp_string);

      ret->files = calloc(ret->files_alloc, sizeof(*ret->files));
      
      grandchild = child->children;
      
      while(grandchild)
        {
        if(!grandchild->name)
          {
          grandchild = grandchild->next;
          continue;
          }
        if(!BG_XML_STRCMP(grandchild->name, file_name))
          {
          ret->files[ret->num_files] =
            load_file(xml_doc, grandchild);
          ret->num_files++;
          }
        grandchild = grandchild->next;
        }
      
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      ret->open_path = bg_strdup(ret->open_path, tmp_string);
      xmlFree(tmp_string);
      }
    
    child = child->next;
    }
  return ret;
  }

static void save_file(xmlNodePtr node, bg_nle_file_t * file)
  {
  xmlNodePtr child;
  char * tmp_string;

  /* ID */
  tmp_string = bg_sprintf("%08x", file->id);
  BG_XML_SET_PROP(node, "id", tmp_string);
  free(tmp_string);
  
  /* Filename */
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                          (xmlChar*)filename_name, NULL);
  xmlAddChild(child, BG_XML_NEW_TEXT(file->filename));
  
  /* Name */
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                          (xmlChar*)name_name, NULL);
  xmlAddChild(child, BG_XML_NEW_TEXT(file->name));

  /* Plugin */
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                          (xmlChar*)plugin_name, NULL);
  xmlAddChild(child, BG_XML_NEW_TEXT(file->plugin));

  /* Section */
  if(file->section)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)parameters_name, NULL);
    bg_cfg_section_2_xml(file->section, child);
    }
  
  /* Track */
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                         (xmlChar*)track_name, NULL);
  tmp_string = bg_sprintf("%d", file->track);
  xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
  free(tmp_string);
  
  /* Audio streams */
  
  if(file->num_audio_streams)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)audio_streams_name, NULL);
    tmp_string =
      bg_sprintf("%d", file->num_audio_streams);
    xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
    free(tmp_string);
    }
  
  /* Video streams */

  if(file->num_video_streams)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)video_streams_name, NULL);
    tmp_string =
      bg_sprintf("%d", file->num_video_streams);
    xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
    free(tmp_string);
    }
  
  /* Duration */
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                          (xmlChar*)duration_name, NULL);
  tmp_string =
    bg_sprintf("%"PRId64, file->duration);
  xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
  free(tmp_string);
  }

void
bg_nle_media_list_save(bg_nle_media_list_t * list, xmlNodePtr node)
  {
  char * tmp_string;
  int i;
  xmlNodePtr child;
  xmlNodePtr grandchild;
  
  if(list->open_path)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)open_path_name, NULL);
    xmlAddChild(child, BG_XML_NEW_TEXT(list->open_path));
    }

  if(list->num_files)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)files_name, NULL);
    tmp_string = bg_sprintf("%d", list->num_files);
    BG_XML_SET_PROP(child, "num", tmp_string);
    free(tmp_string);

    for(i = 0; i < list->num_files; i++)
      {
      grandchild = xmlNewTextChild(child, (xmlNsPtr)0,
                                   (xmlChar*)file_name, NULL);

      save_file(grandchild, list->files[i]);
      }
    }
  
  }
