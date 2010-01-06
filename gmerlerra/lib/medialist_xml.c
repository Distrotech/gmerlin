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
static const char * audio_stream_name = "audio_stream";
static const char * video_stream_name = "video_stream";

static const char * duration_name      = "duration";
static const char * name_name          = "name";
static const char * filename_name      = "filename";
static const char * track_name         = "track";
static const char * plugin_name        = "plugin";
static const char * parameters_name    = "parameters";
static const char * start_time_name    = "start_time";
static const char * cache_dir_name    = "cache_dir";

static const char * tc_rate_name    = "tc_rate";
static const char * tc_flags_name    = "tc_flags";

static void load_audio_stream(xmlDocPtr xml_doc,
                              xmlNodePtr node, bg_nle_audio_stream_t * ret)
  {
  char * tmp_string;
  if((tmp_string = BG_XML_GET_PROP(node, "scale")))
    {
    ret->timescale = atoi(tmp_string);
    free(tmp_string);
    }

  node = node->children;
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }

    if(!BG_XML_STRCMP(node->name, start_time_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      ret->start_time = strtoll(tmp_string, (char**)0, 10);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, duration_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      ret->duration = strtoll(tmp_string, (char**)0, 10);
      xmlFree(tmp_string);
      }
    node = node->next;
    }
  
  }

static void load_video_stream(xmlDocPtr xml_doc,
                              xmlNodePtr node, bg_nle_video_stream_t * ret)
  {
  char * tmp_string;
  if((tmp_string = BG_XML_GET_PROP(node, "scale")))
    {
    ret->timescale = atoi(tmp_string);
    free(tmp_string);
    }
  
  node = node->children;
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }

    if(!BG_XML_STRCMP(node->name, tc_rate_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      ret->tc_format.int_framerate = strtol(tmp_string, (char**)0, 10);
      fprintf(stderr, "Timecode rate: %d\n", ret->tc_format.int_framerate);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, tc_flags_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      ret->tc_format.flags = strtol(tmp_string, (char**)0, 10);
      xmlFree(tmp_string);
      }
    node = node->next;
    }
  
  }

bg_nle_file_t * bg_nle_file_load(xmlDocPtr xml_doc, xmlNodePtr node)
  {
  xmlNodePtr child, grandchild;
  bg_nle_file_t * ret;
  char * tmp_string;
  int index;
  int i;
  
  ret = calloc(1, sizeof(*ret));

  if((tmp_string = BG_XML_GET_PROP(node, "id")))
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
      bg_get_filename_hash(ret->filename, ret->filename_hash);
      }
    else if(!BG_XML_STRCMP(child->name, cache_dir_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
      ret->cache_dir = bg_strdup(ret->cache_dir, tmp_string);
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
      tmp_string = BG_XML_GET_PROP(child, "num");
      ret->num_audio_streams = atoi(tmp_string);
      xmlFree(tmp_string);

      ret->audio_streams = calloc(ret->num_audio_streams,
                                  sizeof(*ret->audio_streams));
      index = 0;
      grandchild = child->children;
      while(grandchild)
        {
        if(!grandchild->name)
          {
          grandchild = grandchild->next;
          continue;
          }

        if(!BG_XML_STRCMP(grandchild->name, audio_stream_name))
          {
          load_audio_stream(xml_doc,
                            grandchild, &ret->audio_streams[index]);
          index++;
          }
        grandchild = grandchild->next;
        }
      }
    else if(!BG_XML_STRCMP(child->name, video_streams_name))
      {
      tmp_string = BG_XML_GET_PROP(child, "num");
      ret->num_video_streams = atoi(tmp_string);
      xmlFree(tmp_string);

      ret->video_streams = calloc(ret->num_video_streams,
                                  sizeof(*ret->video_streams));
      index = 0;
      grandchild = child->children;
      while(grandchild)
        {
        if(!grandchild->name)
          {
          grandchild = grandchild->next;
          continue;
          }
        if(!BG_XML_STRCMP(grandchild->name, video_stream_name))
          {
          load_video_stream(xml_doc,
                            grandchild, &ret->video_streams[index]);
          index++;
          }
        grandchild = grandchild->next;
        }
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

  for(i = 0; i < ret->num_video_streams; i++)
    {
    tmp_string = bg_nle_media_list_get_frame_table_filename(ret, i);
    ret->video_streams[i].frametable = gavl_frame_table_load(tmp_string);
    free(tmp_string);
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
            bg_nle_file_load(xml_doc, grandchild);
          ret->num_files++;
          }
        grandchild = grandchild->next;
        }
      }
    
    child = child->next;
    }
  return ret;
  }

static void save_audio_stream(xmlNodePtr node,
                              bg_nle_audio_stream_t * s)
  {
  xmlNodePtr child;
  char * tmp_string;
  /* Timescale */
  tmp_string = bg_sprintf("%d", s->timescale);
  BG_XML_SET_PROP(node, "scale", tmp_string);
  free(tmp_string);
  
  /* Offset */
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                          (xmlChar*)start_time_name, NULL);
  
  tmp_string = bg_sprintf("%"PRId64, s->start_time);
  xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
  free(tmp_string);

  /* Duration */
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                          (xmlChar*)duration_name, NULL);
  tmp_string = bg_sprintf("%"PRId64, s->duration);
  xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
  free(tmp_string);
  
  }

static void save_video_stream(xmlNodePtr node,
                              bg_nle_video_stream_t * s)
  {
  char * tmp_string;
  xmlNodePtr child;

  /* Timescale */
  tmp_string = bg_sprintf("%d", s->timescale);
  BG_XML_SET_PROP(node, "scale", tmp_string);
  free(tmp_string);

  if(s->tc_format.int_framerate)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)tc_rate_name, NULL);
    
    tmp_string = bg_sprintf("%d", s->tc_format.int_framerate);
    xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
    free(tmp_string);
    }
  if(s->tc_format.flags)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)tc_flags_name, NULL);
    
    tmp_string = bg_sprintf("%d", s->tc_format.flags);
    xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));
    free(tmp_string);
    }
  
  }

void bg_nle_file_save(xmlNodePtr node, bg_nle_file_t * file)
  {
  xmlNodePtr child, grandchild;
  char * tmp_string;
  int i;
  
  /* ID */
  tmp_string = bg_sprintf("%08x", file->id);
  BG_XML_SET_PROP(node, "id", tmp_string);
  free(tmp_string);
  
  /* Filename */
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                          (xmlChar*)filename_name, NULL);
  xmlAddChild(child, BG_XML_NEW_TEXT(file->filename));

  /* Cache directory */
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                          (xmlChar*)cache_dir_name, NULL);
  xmlAddChild(child, BG_XML_NEW_TEXT(file->cache_dir));
  
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
    BG_XML_SET_PROP(child, "num", tmp_string);
    free(tmp_string);

    for(i = 0; i < file->num_audio_streams; i++)
      {
      grandchild = xmlNewTextChild(child, (xmlNsPtr)0,
                                   (xmlChar*)audio_stream_name, NULL);
      save_audio_stream(grandchild, &file->audio_streams[i]);
      }
    }
  
  /* Video streams */

  if(file->num_video_streams)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)video_streams_name, NULL);
    tmp_string =
      bg_sprintf("%d", file->num_video_streams);
    BG_XML_SET_PROP(child, "num", tmp_string);
    free(tmp_string);

    for(i = 0; i < file->num_video_streams; i++)
      {
      grandchild = xmlNewTextChild(child, (xmlNsPtr)0,
                                   (xmlChar*)video_stream_name, NULL);
      save_video_stream(grandchild, &file->video_streams[i]);
      }
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

      bg_nle_file_save(grandchild, list->files[i]);
      }
    }
  
  }
