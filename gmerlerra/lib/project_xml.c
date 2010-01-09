#include <stdlib.h>
#include <string.h>

#include <config.h>

#include <gmerlin/log.h>
#include <gmerlin/cfg_registry.h>
#include <gmerlin/utils.h>


#include <track.h>
#include <project.h>



#define LOG_DOMAIN "project_xml"

static const char * project_name   = "gmerlerra_project";
static const char * tracks_name    = "tracks";
static const char * outstreams_name    = "outstreams";
static const char * selection_name = "selection";
static const char * in_out_name    = "in_out";
static const char * edit_mode_name = "edit_mode";

static const char * visible_name   = "visible";
static const char * media_name     = "media";
static const char * cursor_name   = "cursor";

static const char * parameters_name = "parameters";


bg_nle_project_t *
bg_nle_project_load(const char * filename, bg_plugin_registry_t * plugin_reg)
  {
  xmlDocPtr xml_doc;
  xmlNodePtr node;
  xmlNodePtr child;
  char * tmp_string;
  bg_nle_track_t * track;
  bg_nle_outstream_t * outstream;
  bg_nle_project_t * ret;
  
  xml_doc = bg_xml_parse_file(filename);

  if(!xml_doc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Couldn't open project file %s", filename);
    return (bg_nle_project_t*)0;
    }
  
  node = xml_doc->children;
  
  if(BG_XML_STRCMP(node->name, project_name))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "File %s contains no project", filename);
    xmlFreeDoc(xml_doc);
    return (bg_nle_project_t*)0;
    }
  ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = plugin_reg;
  
  node = node->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }

    if(!BG_XML_STRCMP(node->name, cursor_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      sscanf(tmp_string, "%"PRId64, &ret->cursor_pos);
      free(tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, edit_mode_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      sscanf(tmp_string, "%d", &ret->edit_mode);
      free(tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, visible_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      sscanf(tmp_string, "%"PRId64" %"PRId64, &ret->visible.start,
             &ret->visible.end);
      free(tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, selection_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      sscanf(tmp_string, "%"PRId64" %"PRId64, &ret->selection.start,
             &ret->selection.end);
      free(tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, in_out_name))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      sscanf(tmp_string, "%"PRId64" %"PRId64, &ret->in_out.start,
             &ret->in_out.end);
      free(tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, parameters_name))
      {
      ret->section = bg_cfg_section_create("");
      bg_cfg_xml_2_section(xml_doc, node, ret->section);

      /* Get subsections */
      ret->audio_track_section = bg_cfg_section_find_subsection(ret->section,
                                                                "audio_track");
      ret->video_track_section = bg_cfg_section_find_subsection(ret->section,
                                                                "video_track");
      ret->audio_outstream_section = bg_cfg_section_find_subsection(ret->section,
                                                                "audio_outstream");
      ret->video_outstream_section = bg_cfg_section_find_subsection(ret->section,
                                                                "video_outstream");
      }
    
    else if(!BG_XML_STRCMP(node->name, media_name))
      {
      ret->media_list = bg_nle_media_list_load(ret->plugin_reg,
                                               xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, tracks_name))
      {
      child = node->children;

      while(child)
        {
        if(!child->name)
          {
          child = child->next;
          continue;
          }

        if(!BG_XML_STRCMP(child->name, "track"))
          {
          track = bg_nle_track_load(xml_doc, child);
          bg_nle_project_append_track(ret, track);
          }
        
        child = child->next;
        }
      }
    else if(!BG_XML_STRCMP(node->name, outstreams_name))
      {
      child = node->children;

      while(child)
        {
        if(!child->name)
          {
          child = child->next;
          continue;
          }

        if(!BG_XML_STRCMP(child->name, "outstream"))
          {
          outstream = bg_nle_outstream_load(xml_doc, child);
          bg_nle_project_append_outstream(ret, outstream);
          }
        
        child = child->next;
        }
      }
    node = node->next;
    }
  
  xmlFreeDoc(xml_doc);
  bg_nle_project_resolve_ids(ret);

  ret->cache_parameters = bg_nle_project_create_cache_parameters();

  bg_nle_project_create_sections(ret);
  
  bg_cfg_section_apply(ret->cache_section,
                       ret->cache_parameters,
                       bg_nle_project_set_cache_parameter,
                       ret);

  
  return ret;
  }

void bg_nle_project_save(bg_nle_project_t * p, const char * filename)
  {
  int i;
  char * tmp_string;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_project;
  xmlNodePtr node;
  
  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_project = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)project_name, NULL);
  
  xmlDocSetRootElement(xml_doc, xml_project);
  
  xmlAddChild(xml_project, BG_XML_NEW_TEXT("\n"));

  /* Global data */

  /* Cursor pos */
  
  node = xmlNewTextChild(xml_project, (xmlNsPtr)0,
                         (xmlChar*)cursor_name, NULL);
  tmp_string =
    bg_sprintf("%"PRId64, p->cursor_pos);
  xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
  free(tmp_string);

  /* Edit mode */
  
  node = xmlNewTextChild(xml_project, (xmlNsPtr)0,
                         (xmlChar*)edit_mode_name, NULL);
  tmp_string =
    bg_sprintf("%d", p->edit_mode);
  xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
  free(tmp_string);
  
  
  /* Selection */

  node = xmlNewTextChild(xml_project, (xmlNsPtr)0,
                         (xmlChar*)selection_name, NULL);
  tmp_string =
    bg_sprintf("%"PRId64" %"PRId64, p->selection.start, p->selection.end);
  xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
  free(tmp_string);

  /* In-out */

  node = xmlNewTextChild(xml_project, (xmlNsPtr)0,
                         (xmlChar*)in_out_name, NULL);
  tmp_string =
    bg_sprintf("%"PRId64" %"PRId64, p->in_out.start, p->in_out.end);
  xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
  free(tmp_string);

  
  /* Visible Range */
  
  node = xmlNewTextChild(xml_project, (xmlNsPtr)0,
                         (xmlChar*)visible_name, NULL);
  tmp_string =
    bg_sprintf("%"PRId64" %"PRId64, p->visible.start, p->visible.end);
  xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
  free(tmp_string);

  /* Media list */
  node = xmlNewTextChild(xml_project, (xmlNsPtr)0,
                         (xmlChar*)media_name, NULL);

  bg_nle_media_list_save(p->media_list, node);

  /* Section */

  node = xmlNewTextChild(xml_project, (xmlNsPtr)0,
                          (xmlChar*)parameters_name, NULL);
  bg_cfg_section_2_xml(p->section, node);
  
  /* Add tracks */

  if(p->num_tracks)
    {
    node = xmlNewTextChild(xml_project, (xmlNsPtr)0,
                           (xmlChar*)tracks_name, NULL);

    tmp_string = bg_sprintf("%d", p->num_tracks);
    BG_XML_SET_PROP(node, "num", tmp_string);
    free(tmp_string);
    
    for(i = 0; i < p->num_tracks; i++)
      {
      bg_nle_track_save(p->tracks[i], node);
      }
    }

  /* Add outstreams */

  if(p->num_outstreams)
    {
    node = xmlNewTextChild(xml_project, (xmlNsPtr)0,
                           (xmlChar*)outstreams_name, NULL);

    tmp_string = bg_sprintf("%d", p->num_outstreams);
    BG_XML_SET_PROP(node, "num", tmp_string);
    free(tmp_string);
    
    for(i = 0; i < p->num_outstreams; i++)
      {
      bg_nle_outstream_save(p->outstreams[i], node);
      }
    }
  
  /* Save */
  
  if(filename)
    {
    xmlSaveFile(filename, xml_doc);
    }
  }
