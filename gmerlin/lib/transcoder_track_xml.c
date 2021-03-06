/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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


#include <string.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/tree.h>
#include <gmerlin/transcoder_track.h>
#include <gmerlin/utils.h>
#include <gmerlin/xmlutils.h>

static void section_2_xml(bg_cfg_section_t * s, xmlNodePtr node)
  {
  BG_XML_SET_PROP(node, "name", bg_cfg_section_get_name(s));
  bg_cfg_section_2_xml(s, node);
  }

static void audio_stream_2_xml(xmlNodePtr parent,
                               bg_transcoder_track_audio_t * s)
  {
  xmlNodePtr node;

  if(s->label)
    BG_XML_SET_PROP(parent, "label", s->label);

  node = xmlNewTextChild(parent, NULL, (xmlChar*)"GENERAL", NULL);
  section_2_xml(s->general_section, node);
  
  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));

  if(s->filter_section)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"FILTER", NULL);
    section_2_xml(s->filter_section, node);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }

  if(s->encoder_section)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"ENCODER", NULL);
    section_2_xml(s->encoder_section, node);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }

  if(s->m.num_tags)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"METADATA", NULL);
    bg_metadata_2_xml(node, &s->m);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }
  }

static void video_stream_2_xml(xmlNodePtr parent,
                               bg_transcoder_track_video_t * s)
  {
  xmlNodePtr node;
  if(s->label)
    BG_XML_SET_PROP(parent, "label", s->label);

  node = xmlNewTextChild(parent, NULL, (xmlChar*)"GENERAL", NULL);
  section_2_xml(s->general_section, node);

  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));

  if(s->filter_section)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"FILTER", NULL);
    section_2_xml(s->filter_section, node);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }
  
  if(s->encoder_section)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"ENCODER", NULL);
    section_2_xml(s->encoder_section, node);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }
  if(s->m.num_tags)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"METADATA", NULL);
    bg_metadata_2_xml(node, &s->m);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }

  }

static void text_stream_2_xml(xmlNodePtr parent,
                                       bg_transcoder_track_text_t * s)
  {
  xmlNodePtr node;
  char * tmp_string;
  tmp_string = bg_sprintf("%d", s->in_index);
  BG_XML_SET_PROP(parent, "in_index", tmp_string);
  free(tmp_string);
  
  if(s->label)
    BG_XML_SET_PROP(parent, "label", s->label);

  node = xmlNewTextChild(parent, NULL, (xmlChar*)"GENERAL", NULL);
  section_2_xml(s->general_section, node);

  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));

  node = xmlNewTextChild(parent, NULL, (xmlChar*)"TEXTRENDERER", NULL);
  section_2_xml(s->textrenderer_section, node);
  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));

  if(s->encoder_section_text)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"ENCODER_TEXT", NULL);
    section_2_xml(s->encoder_section_text, node);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }
  if(s->encoder_section_overlay)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"ENCODER_OVERLAY", NULL);
    section_2_xml(s->encoder_section_overlay, node);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }
  if(s->m.num_tags)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"METADATA", NULL);
    bg_metadata_2_xml(node, &s->m);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }
  }

static void overlay_stream_2_xml(xmlNodePtr parent,
                                          bg_transcoder_track_overlay_t * s)
  {
  xmlNodePtr node;
  char * tmp_string;
  tmp_string = bg_sprintf("%d", s->in_index);
  BG_XML_SET_PROP(parent, "in_index", tmp_string);
  free(tmp_string);

  if(s->label)
    BG_XML_SET_PROP(parent, "label", s->label);

  node = xmlNewTextChild(parent, NULL, (xmlChar*)"GENERAL", NULL);
  section_2_xml(s->general_section, node);

  if(s->encoder_section)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"ENCODER", NULL);
    section_2_xml(s->encoder_section, node);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }
  if(s->m.num_tags)
    {
    node = xmlNewTextChild(parent, NULL, (xmlChar*)"METADATA", NULL);
    bg_metadata_2_xml(node, &s->m);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
    }
  
  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
  }

static void track_2_xml(bg_transcoder_track_t * track,
                        xmlNodePtr xml_track)
  {
  int i;
  
  xmlNodePtr node, stream_node;

  node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"GENERAL", NULL);
  section_2_xml(track->general_section, node);
  xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));

  node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"GENERAL_PARAMS", NULL);
  bg_parameters_2_xml(track->general_parameters, node);
  xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
  
  node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"METADATA", NULL);
  section_2_xml(track->metadata_section, node);
  xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));

  node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"METADATA_PARAMS", NULL);
  bg_parameters_2_xml(track->metadata_parameters, node);
  xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
  
  if(track->chapter_list)
    {
    node =
      xmlNewTextChild(xml_track, NULL, (xmlChar*)"CHAPTERS", NULL);
    bg_chapter_list_2_xml(track->chapter_list, node);
    xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
    }
  
  if(track->input_section)
    {
    node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"INPUT", NULL);
    section_2_xml(track->input_section, node);
    xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
    }
  if(track->audio_encoder_section)
    {
    node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"AUDIO_ENCODER", NULL);
    section_2_xml(track->audio_encoder_section, node);
    xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
    }

  if(track->video_encoder_section)
    {
    node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"VIDEO_ENCODER", NULL);
    section_2_xml(track->video_encoder_section, node);
    xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
    }

  if(track->text_encoder_section)
    {
    node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"TEXT_ENCODER", NULL);
    section_2_xml(track->text_encoder_section, node);
    xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
    }

  if(track->overlay_encoder_section)
    {
    node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"OVERLAY_ENCODER", NULL);
    section_2_xml(track->overlay_encoder_section, node);
    xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
    }
  
  if(track->num_audio_streams)
    {
    node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"AUDIO_STREAMS", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT("\n"));

    for(i = 0; i < track->num_audio_streams; i++)
      {
      stream_node = xmlNewTextChild(node, NULL, (xmlChar*)"STREAM", NULL);
      xmlAddChild(stream_node, BG_XML_NEW_TEXT("\n"));

      audio_stream_2_xml(stream_node, &track->audio_streams[i]);
      xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
      }
    xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
    }

  
  if(track->num_video_streams)
    {
    node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"VIDEO_STREAMS", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
    
    for(i = 0; i < track->num_video_streams; i++)
      {
      stream_node = xmlNewTextChild(node, NULL, (xmlChar*)"STREAM", NULL);
      xmlAddChild(stream_node, BG_XML_NEW_TEXT("\n"));
      video_stream_2_xml(stream_node, &track->video_streams[i]);
      xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
      }
    xmlAddChild(xml_track, BG_XML_NEW_TEXT("\n"));
    }
  if(track->num_text_streams)
    {
    node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"TEXT_STREAMS", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
    
    for(i = 0; i < track->num_text_streams; i++)
      {
      stream_node = xmlNewTextChild(node, NULL, (xmlChar*)"STREAM", NULL);
      xmlAddChild(stream_node, BG_XML_NEW_TEXT("\n"));
      text_stream_2_xml(stream_node, &track->text_streams[i]);
      xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
      }
    }
  if(track->num_overlay_streams)
    {
    node = xmlNewTextChild(xml_track, NULL, (xmlChar*)"OVERLAY_STREAMS", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
    
    for(i = 0; i < track->num_overlay_streams; i++)
      {
      stream_node = xmlNewTextChild(node, NULL, (xmlChar*)"STREAM", NULL);
      xmlAddChild(stream_node, BG_XML_NEW_TEXT("\n"));
      overlay_stream_2_xml(stream_node, &track->overlay_streams[i]);
      xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
      }
    }


  }

static void global_2_xml(bg_transcoder_track_global_t * g,
                         xmlNodePtr xml_global)
  {
  xmlNodePtr node;
  if(g->pp_plugin)
    {
    node = xmlNewTextChild(xml_global, NULL, (xmlChar*)"PP_PLUGIN", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(g->pp_plugin));
    xmlAddChild(xml_global, BG_XML_NEW_TEXT("\n"));
    
    node = xmlNewTextChild(xml_global, NULL, (xmlChar*)"PP_SECTION", NULL);
    section_2_xml(g->pp_section, node);
    xmlAddChild(xml_global, BG_XML_NEW_TEXT("\n"));
    }
  }

static xmlDocPtr
transcoder_tracks_2_xml(bg_transcoder_track_t * t,
                        bg_transcoder_track_global_t * g,
                        int selected_only)
  {
  bg_transcoder_track_t * tmp;
  xmlDocPtr  xml_doc;
  xmlNodePtr root_node, node;
    
  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  root_node = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)"TRANSCODER_TRACKS", NULL);
  xmlDocSetRootElement(xml_doc, root_node);

  xmlAddChild(root_node, BG_XML_NEW_TEXT("\n"));

  if(g)
    {
    node = xmlNewTextChild(root_node, NULL, (xmlChar*)"GLOBAL", NULL);
    global_2_xml(g, node);
    xmlAddChild(root_node, BG_XML_NEW_TEXT("\n"));
    }
  
  tmp = t;

  while(tmp)
    {
    if(tmp->selected || !selected_only)
      {
      node = xmlNewTextChild(root_node, NULL, (xmlChar*)"TRACK", NULL);
      xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
      track_2_xml(tmp, node);
      xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
      xmlAddChild(root_node, BG_XML_NEW_TEXT("\n"));
      }
    tmp = tmp->next;
    }
  return xml_doc;
  }

void bg_transcoder_tracks_save(bg_transcoder_track_t * t,
                               bg_transcoder_track_global_t * g,
                               const char * filename)
  {
  xmlDocPtr xml_doc;
  
  xml_doc = transcoder_tracks_2_xml(t, g, 0);

  bg_xml_save_file(xml_doc, filename, 1);
  xmlFreeDoc(xml_doc);
  }

char *
bg_transcoder_tracks_selected_to_xml(bg_transcoder_track_t * t)
  {
  char * ret;
  xmlDocPtr xml_doc;
  xml_doc = transcoder_tracks_2_xml(t, NULL, 1);

  ret = bg_xml_save_to_memory(xml_doc);
  xmlFreeDoc(xml_doc);
  return ret;
  }

/* Load */

static bg_cfg_section_t * xml_2_section(xmlDocPtr xml_doc, xmlNodePtr xml_section)
  {
  char * name;
  bg_cfg_section_t * ret;

  name = BG_XML_GET_PROP(xml_section, "name");
  ret = bg_cfg_section_create(name);

  if(name)
    xmlFree(name);
  
  bg_cfg_xml_2_section(xml_doc, xml_section, ret);
  return ret;
  }

static void xml_2_audio(bg_transcoder_track_audio_t * s,
                        xmlDocPtr xml_doc, xmlNodePtr xml_stream)
  {
  xmlNodePtr node;

  char * tmp_string;
  tmp_string = BG_XML_GET_PROP(xml_stream, "label");
  if(tmp_string)
    {
    s->label = gavl_strrep(s->label, tmp_string);
    xmlFree(tmp_string);
    }
  
  node = xml_stream->children;

  
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, "GENERAL"))
      {
      s->general_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "FILTER"))
      {
      s->filter_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "ENCODER"))
      {
      s->encoder_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "METADATA"))
      {
      bg_xml_2_metadata(xml_doc, node, &s->m);
      }
    node = node->next;
    }
  }

static void xml_2_video(bg_transcoder_track_video_t * s,
                        xmlDocPtr xml_doc, xmlNodePtr xml_stream)
  {
  xmlNodePtr node;
  char * tmp_string;
  tmp_string = BG_XML_GET_PROP(xml_stream, "label");
  if(tmp_string)
    {
    s->label = gavl_strrep(s->label, tmp_string);
    xmlFree(tmp_string);
    }

  
  node = xml_stream->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, "GENERAL"))
      {
      s->general_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "FILTER"))
      {
      s->filter_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "ENCODER"))
      {
      s->encoder_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "METADATA"))
      {
      bg_xml_2_metadata(xml_doc, node, &s->m);
      }
    node = node->next;
    }
  
  }

static void xml_2_text(bg_transcoder_track_text_t * s,
                                xmlDocPtr xml_doc, xmlNodePtr xml_stream)
  {
  xmlNodePtr node;
  char * tmp_string;
  tmp_string = BG_XML_GET_PROP(xml_stream, "label");
  if(tmp_string)
    {
    s->label = gavl_strrep(s->label, tmp_string);
    xmlFree(tmp_string);
    }

  tmp_string = BG_XML_GET_PROP(xml_stream, "in_index");
  if(tmp_string)
    {
    s->in_index = atoi(tmp_string);
    xmlFree(tmp_string);
    }
  
  node = xml_stream->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, "GENERAL"))
      {
      s->general_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "TEXTRENDERER"))
      {
      s->textrenderer_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "ENCODER_TEXT"))
      {
      s->encoder_section_text = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "ENCODER_OVERLAY"))
      {
      s->encoder_section_overlay = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "METADATA"))
      {
      bg_xml_2_metadata(xml_doc, node, &s->m);
      }
    node = node->next;
    }
  }

static void xml_2_overlay(bg_transcoder_track_overlay_t * s,
                                   xmlDocPtr xml_doc, xmlNodePtr xml_stream)
  {
  xmlNodePtr node;
  char * tmp_string;
  tmp_string = BG_XML_GET_PROP(xml_stream, "label");
  if(tmp_string)
    {
    s->label = gavl_strrep(s->label, tmp_string);
    xmlFree(tmp_string);
    }
  tmp_string = BG_XML_GET_PROP(xml_stream, "in_index");
  if(tmp_string)
    {
    s->in_index = atoi(tmp_string);
    xmlFree(tmp_string);
    }

  node = xml_stream->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, "GENERAL"))
      {
      s->general_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "ENCODER"))
      {
      s->encoder_section = xml_2_section(xml_doc, node);
      }
    else if(!BG_XML_STRCMP(node->name, "METADATA"))
      {
      bg_xml_2_metadata(xml_doc, node, &s->m);
      }
    
    node = node->next;
    }
  }

/* Upgrade saved tracks with older versions */

static void purge_track(bg_transcoder_track_t * t)
  {
  const char * video_name = NULL;
  const char * name;

  bg_cfg_section_get_parameter_string(t->general_section,
                                      "video_encoder", &video_name);
  if(!video_name)
    return;

  bg_cfg_section_get_parameter_string(t->general_section,
                                      "audio_encoder", &name);
  if(name && !strcmp(name, video_name))
    bg_cfg_section_set_parameter_string(t->general_section,
                                        "audio_encoder", NULL);

  bg_cfg_section_get_parameter_string(t->general_section,
                                      "text_encoder", &name);

  if(name && !strcmp(name, video_name))
    bg_cfg_section_set_parameter_string(t->general_section,
                                        "text_encoder", NULL);
  
  bg_cfg_section_get_parameter_string(t->general_section,
                                      "overlay_encoder", &name);
  if(name && !strcmp(name, video_name))
    bg_cfg_section_set_parameter_string(t->general_section,
                                        "overlay_encoder", NULL);
  }

static int xml_2_track(bg_transcoder_track_t * t,
                       xmlDocPtr xml_doc, xmlNodePtr xml_track,
                       bg_plugin_registry_t * plugin_reg)
  {
  int ret = 0;

  xmlNodePtr node, child_node;
  int i;
  
  node = xml_track->children;
  
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, "GENERAL"))
      t->general_section = xml_2_section(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "GENERAL_PARAMS"))
      t->general_parameters = bg_xml_2_parameters(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "METADATA"))
      t->metadata_section = xml_2_section(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "METADATA_PARAMS"))
      t->metadata_parameters = bg_xml_2_parameters(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "CHAPTERS"))
      t->chapter_list = bg_xml_2_chapter_list(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "INPUT"))
      t->input_section = xml_2_section(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "AUDIO_ENCODER"))
      t->audio_encoder_section = xml_2_section(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "VIDEO_ENCODER"))
      t->video_encoder_section = xml_2_section(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "TEXT_ENCODER"))
      t->text_encoder_section = xml_2_section(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "OVERLAY_ENCODER"))
      t->overlay_encoder_section = xml_2_section(xml_doc, node);
    else if(!BG_XML_STRCMP(node->name, "AUDIO_STREAMS"))
      {
      /* Count streams */

      t->num_audio_streams = 0;

      child_node = node->children;
      while(child_node)
        {
        if(child_node->name && !BG_XML_STRCMP(child_node->name, "STREAM"))
          t->num_audio_streams++;
        child_node = child_node->next;
        }

      /* Allocate streams */
      
      t->audio_streams =
        calloc(t->num_audio_streams, sizeof(*(t->audio_streams)));

      /* Load streams */
      
      child_node = node->children;
      i = 0;

      while(child_node)
        {
        if(child_node->name && !BG_XML_STRCMP(child_node->name, "STREAM"))
          {
          xml_2_audio(&t->audio_streams[i], xml_doc, child_node);
          i++;
          }
        child_node = child_node->next;
        }
      
      }
    else if(!BG_XML_STRCMP(node->name, "VIDEO_STREAMS"))
      {
      /* Count streams */

      t->num_video_streams = 0;

      child_node = node->children;
      while(child_node)
        {
        if(child_node->name && !BG_XML_STRCMP(child_node->name, "STREAM"))
          t->num_video_streams++;
        child_node = child_node->next;
        }

      
      /* Allocate streams */
      
      t->video_streams =
        calloc(t->num_video_streams, sizeof(*(t->video_streams)));

      /* Load streams */
      
      child_node = node->children;
      i = 0;

      while(child_node)
        {
        if(child_node->name && !BG_XML_STRCMP(child_node->name, "STREAM"))
          {
          xml_2_video(&t->video_streams[i], xml_doc, child_node);
          i++;
          }
        child_node = child_node->next;
        }
      
      }

    /* Subtitle text streams */

    else if(!BG_XML_STRCMP(node->name, "TEXT_STREAMS"))
      {
      /* Count streams */

      t->num_text_streams = 0;

      child_node = node->children;
      while(child_node)
        {
        if(child_node->name && !BG_XML_STRCMP(child_node->name, "STREAM"))
          t->num_text_streams++;
        child_node = child_node->next;
        }

      /* Allocate streams */
      
      t->text_streams =
        calloc(t->num_text_streams, sizeof(*(t->text_streams)));
      
      /* Load streams */
      
      child_node = node->children;
      i = 0;

      while(child_node)
        {
        if(child_node->name && !BG_XML_STRCMP(child_node->name, "STREAM"))
          {
          xml_2_text(&t->text_streams[i], xml_doc, child_node);
          i++;
          }
        child_node = child_node->next;
        }
      }

    /* Subtitle overlay streams */

    else if(!BG_XML_STRCMP(node->name, "OVERLAY_STREAMS"))
      {
      /* Count streams */

      t->num_overlay_streams = 0;

      child_node = node->children;
      while(child_node)
        {
        if(child_node->name && !BG_XML_STRCMP(child_node->name, "STREAM"))
          t->num_overlay_streams++;
        child_node = child_node->next;
        }

      /* Allocate streams */
      
      t->overlay_streams =
        calloc(t->num_overlay_streams, sizeof(*(t->overlay_streams)));
      
      /* Load streams */
      
      child_node = node->children;
      i = 0;

      while(child_node)
        {
        if(child_node->name && !BG_XML_STRCMP(child_node->name, "STREAM"))
          {
          xml_2_overlay(&t->overlay_streams[i], xml_doc, child_node);
          i++;
          }
        child_node = child_node->next;
        }
      }
    node = node->next;
    }
  
  bg_transcoder_track_create_parameters(t, plugin_reg);

  purge_track(t);
  
  ret = 1;
  
  return ret;
  }

static int xml_2_global(bg_transcoder_track_global_t * g,
                        xmlDocPtr xml_doc, xmlNodePtr node,
                        bg_plugin_registry_t * plugin_reg)
  {
  int ret = 0;
  char * tmp_string;

  bg_transcoder_track_global_free(g);
  node = node->children;
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, "PP_PLUGIN"))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);

      if(tmp_string)
        {
        g->pp_plugin = gavl_strrep(g->pp_plugin, tmp_string);
        xmlFree(tmp_string);
        }
      }
    else if(!BG_XML_STRCMP(node->name, "PP_SECTION"))
      {
      g->pp_section = xml_2_section(xml_doc, node);
      }
    node = node->next;
    }

  ret = 1;
  
  return ret;
  }

static bg_transcoder_track_t *
transcoder_tracks_load(xmlDocPtr xml_doc,
                       bg_transcoder_track_global_t * g,
                       bg_plugin_registry_t * plugin_reg)
  {
  xmlNodePtr node;

  bg_transcoder_track_t * ret = NULL;
  bg_transcoder_track_t * end = NULL;
  
  if(!xml_doc)
    return NULL;

  node = xml_doc->children;

  if(BG_XML_STRCMP(node->name, "TRANSCODER_TRACKS"))
    return NULL;

  node = node->children;
  
  while(node)
    {
    if(node->name && !BG_XML_STRCMP(node->name, "TRACK"))
      {
      /* Load track */

      if(!ret)
        {
        ret = calloc(1, sizeof(*ret));
        end = ret;
        }
      else
        {
        end->next = calloc(1, sizeof(*(end->next)));
        end = end->next;
        }
      xml_2_track(end, xml_doc, node, plugin_reg);
      }
    else if(node->name && !BG_XML_STRCMP(node->name, "GLOBAL") && g)
      {
      xml_2_global(g, xml_doc, node, plugin_reg);
      }
    
    node = node->next;
    }

  
  
  return ret;  
  

  }


bg_transcoder_track_t *
bg_transcoder_tracks_load(const char * filename,
                          bg_transcoder_track_global_t * g,
                          bg_plugin_registry_t * plugin_reg)
  {
  xmlDocPtr xml_doc;
  bg_transcoder_track_t * ret;
  if(!filename)
    return NULL;
  
  xml_doc = bg_xml_parse_file(filename, 1);
  
  ret = transcoder_tracks_load(xml_doc,
                               g, plugin_reg);
    
  
  xmlFreeDoc(xml_doc);
  return ret;
  }

bg_transcoder_track_t * 
bg_transcoder_tracks_from_xml(char * str, bg_plugin_registry_t * plugin_reg)
  {
  xmlDocPtr xml_doc;
  bg_transcoder_track_t * ret;
  
  xml_doc = xmlParseMemory(str, strlen(str));
  
  ret = transcoder_tracks_load(xml_doc,
                               NULL,
                               plugin_reg);
  
  xmlFreeDoc(xml_doc);
  return ret;
  
  }
