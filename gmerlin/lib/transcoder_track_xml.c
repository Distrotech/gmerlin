/*****************************************************************
  
  transcoder_track_xml.c
  
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
#include <pluginregistry.h>
#include <tree.h>
#include <transcoder_track.h>
#include <utils.h>

static void section_2_xml(bg_cfg_section_t * s, xmlNodePtr node)
  {
  xmlSetProp(node, "name", bg_cfg_section_get_name(s));
  bg_cfg_section_2_xml(s, node);
  }

static void audio_stream_2_xml(xmlNodePtr parent,
                               bg_transcoder_track_audio_t * s)
  {
  xmlNodePtr node;
  node = xmlNewTextChild(parent, (xmlNsPtr)0, "GENERAL", NULL);
  section_2_xml(s->general_section, node);

  xmlAddChild(parent, xmlNewText("\n"));

  node = xmlNewTextChild(parent, (xmlNsPtr)0, "ENCODER", NULL);
  section_2_xml(s->encoder_section, node);

  xmlAddChild(parent, xmlNewText("\n"));
  
  }

static void video_stream_2_xml(xmlNodePtr parent,
                               bg_transcoder_track_video_t * s)
  {
  xmlNodePtr node;
  node = xmlNewTextChild(parent, (xmlNsPtr)0, "GENERAL", NULL);
  section_2_xml(s->general_section, node);

  xmlAddChild(parent, xmlNewText("\n"));
  
  node = xmlNewTextChild(parent, (xmlNsPtr)0, "ENCODER", NULL);
  section_2_xml(s->encoder_section, node);

  xmlAddChild(parent, xmlNewText("\n"));

  }

static void track_2_xml(bg_transcoder_track_t * track,
                        xmlNodePtr xml_track)
  {
  int i;
  
  xmlNodePtr node, stream_node;

  node = xmlNewTextChild(xml_track, (xmlNsPtr)0, "GENERAL", NULL);
  section_2_xml(track->general_section, node);
  xmlAddChild(xml_track, xmlNewText("\n"));


  node = xmlNewTextChild(xml_track, (xmlNsPtr)0, "METADATA", NULL);
  section_2_xml(track->metadata_section, node);
  xmlAddChild(xml_track, xmlNewText("\n"));

  if(track->audio_encoder_section)
    {
    node = xmlNewTextChild(xml_track, (xmlNsPtr)0, "AUDIO_ENCODER", NULL);
    section_2_xml(track->audio_encoder_section, node);
    xmlAddChild(xml_track, xmlNewText("\n"));
    }

  if(track->video_encoder_section)
    {
    node = xmlNewTextChild(xml_track, (xmlNsPtr)0, "VIDEO_ENCODER", NULL);
    section_2_xml(track->video_encoder_section, node);
    xmlAddChild(xml_track, xmlNewText("\n"));
    }

  
  if(track->num_audio_streams)
    {
    node = xmlNewTextChild(xml_track, (xmlNsPtr)0, "AUDIO_STREAMS", NULL);
    xmlAddChild(node, xmlNewText("\n"));

    for(i = 0; i < track->num_audio_streams; i++)
      {
      stream_node = xmlNewTextChild(node, (xmlNsPtr)0, "STREAM", NULL);
      xmlAddChild(stream_node, xmlNewText("\n"));

      audio_stream_2_xml(stream_node, &(track->audio_streams[i]));
      xmlAddChild(node, xmlNewText("\n"));
      }
    }

  if(track->num_video_streams)
    {
    node = xmlNewTextChild(xml_track, (xmlNsPtr)0, "VIDEO_STREAMS", NULL);
    xmlAddChild(node, xmlNewText("\n"));
    
    for(i = 0; i < track->num_video_streams; i++)
      {
      stream_node = xmlNewTextChild(node, (xmlNsPtr)0, "STREAM", NULL);
      xmlAddChild(stream_node, xmlNewText("\n"));
      video_stream_2_xml(stream_node, &(track->video_streams[i]));
      xmlAddChild(node, xmlNewText("\n"));
      }
    }
  }

void bg_transcoder_tracks_save(bg_transcoder_track_t * t,
                               const char * filename)
  {
  bg_transcoder_track_t * tmp;

  xmlDocPtr  xml_doc;
  xmlNodePtr root_node, xml_track;
    
  xml_doc = xmlNewDoc("1.0");
  root_node = xmlNewDocRawNode(xml_doc, NULL, "TRANSCODER_TRACKS", NULL);
  xmlDocSetRootElement(xml_doc, root_node);

  xmlAddChild(root_node, xmlNewText("\n"));

  tmp = t;

  while(tmp)
    {
    xml_track = xmlNewTextChild(root_node, (xmlNsPtr)0, "TRACK", NULL);
    xmlAddChild(xml_track, xmlNewText("\n"));
    track_2_xml(tmp, xml_track);
    xmlAddChild(xml_track, xmlNewText("\n"));
    tmp = tmp->next;
    xmlAddChild(root_node, xmlNewText("\n"));
    }
  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);
  }

/* Load */

static bg_cfg_section_t * xml_2_section(xmlDocPtr xml_doc, xmlNodePtr xml_section)
  {
  char * name;
  bg_cfg_section_t * ret;

  name = xmlGetProp(xml_section, "name");
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
  
  node = xml_stream->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!strcmp(node->name, "GENERAL"))
      {
      s->general_section = xml_2_section(xml_doc, node);
      }
    else if(!strcmp(node->name, "ENCODER"))
      {
      s->encoder_section = xml_2_section(xml_doc, node);
      }
    node = node->next;
    }
  }

static void xml_2_video(bg_transcoder_track_video_t * s,
                        xmlDocPtr xml_doc, xmlNodePtr xml_stream)
  {
  xmlNodePtr node;

  node = xml_stream->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!strcmp(node->name, "GENERAL"))
      {
      s->general_section = xml_2_section(xml_doc, node);
      }
    else if(!strcmp(node->name, "ENCODER"))
      {
      s->encoder_section = xml_2_section(xml_doc, node);
      }
    node = node->next;
    }
  
  }

static int xml_2_track(bg_transcoder_track_t * t,
                       xmlDocPtr xml_doc, xmlNodePtr xml_track,
                       bg_plugin_registry_t * plugin_reg,
                       bg_plugin_handle_t ** audio_encoder,
                       bg_plugin_handle_t ** video_encoder)
  {
  xmlNodePtr node, child_node;
  int i;
  char * tmp_string;
  const bg_plugin_info_t * plugin_info;
  
  node = xml_track->children;
  
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!strcmp(node->name, "GENERAL"))
      {
      t->general_section = xml_2_section(xml_doc, node);
      }
    else if(!strcmp(node->name, "METADATA"))
      {
      t->metadata_section = xml_2_section(xml_doc, node);
      }
    else if(!strcmp(node->name, "AUDIO_ENCODER"))
      {
      t->audio_encoder_section = xml_2_section(xml_doc, node);
      }
    else if(!strcmp(node->name, "VIDEO_ENCODER"))
      {
      t->video_encoder_section = xml_2_section(xml_doc, node);
      }
    else if(!strcmp(node->name, "AUDIO_STREAMS"))
      {
      /* Count streams */

      t->num_audio_streams = 0;

      child_node = node->children;
      while(child_node)
        {
        if(child_node->name && !strcmp(child_node->name, "STREAM"))
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
        if(child_node->name && !strcmp(child_node->name, "STREAM"))
          {
          xml_2_audio(&(t->audio_streams[i]), xml_doc, child_node);
          i++;
          }
        child_node = child_node->next;
        }
      
      }
    else if(!strcmp(node->name, "VIDEO_STREAMS"))
      {
      /* Count streams */

      t->num_video_streams = 0;

      child_node = node->children;
      while(child_node)
        {
        if(child_node->name && !strcmp(child_node->name, "STREAM"))
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
        if(child_node->name && !strcmp(child_node->name, "STREAM"))
          {
          xml_2_video(&(t->video_streams[i]), xml_doc, child_node);
          i++;
          }
        child_node = child_node->next;
        }
      
      }
    node = node->next;
    }

  /* Load audio encoder */
  tmp_string = bg_transcoder_track_get_audio_encoder(t);

  if(!(*audio_encoder) || strcmp((*audio_encoder)->info->name, 
                                 tmp_string))
    {
    if(*audio_encoder)
      bg_plugin_unref(*audio_encoder);

    plugin_info = bg_plugin_find_by_name(plugin_reg, tmp_string);

    if(!plugin_info)
      return 0;
    *audio_encoder = bg_plugin_load(plugin_reg, plugin_info);
    }
  free(tmp_string);

  /* Load video encoder */
  tmp_string = bg_transcoder_track_get_video_encoder(t);

  if(!(*video_encoder) || strcmp((*video_encoder)->info->name, 
                                 tmp_string))
    {
    if(*video_encoder)
      bg_plugin_unref(*video_encoder);

    plugin_info = bg_plugin_find_by_name(plugin_reg, tmp_string);

    if(!plugin_info)
      return 0;
    *video_encoder = bg_plugin_load(plugin_reg, plugin_info);
    }
  free(tmp_string);

  bg_transcoder_track_create_parameters(t, *audio_encoder,
                                        *video_encoder);
  
  return 1;
  }

bg_transcoder_track_t *
bg_transcoder_tracks_load(const char * filename,
                          bg_plugin_registry_t * plugin_reg)
  {
  xmlDocPtr xml_doc;
  xmlNodePtr node;

  bg_transcoder_track_t * ret = (bg_transcoder_track_t *)0;
  bg_transcoder_track_t * end;

  bg_plugin_handle_t * audio_encoder = (bg_plugin_handle_t*)0;
  bg_plugin_handle_t * video_encoder = (bg_plugin_handle_t*)0;
  
  if(!filename)
    return (bg_transcoder_track_t*)0;
  
  xml_doc = xmlParseFile(filename);
                                                                               
  if(!xml_doc)
    return (bg_transcoder_track_t*)0;

  node = xml_doc->children;

  if(strcmp(node->name, "TRANSCODER_TRACKS"))
    {
    fprintf(stderr, "File %s contains no transcoder tracks\n", filename);
    xmlFreeDoc(xml_doc);
    return (bg_transcoder_track_t*)0;
    }

  node = node->children;
  
  while(node)
    {
    if(node->name && !strcmp(node->name, "TRACK"))
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
      xml_2_track(end, xml_doc, node, plugin_reg, &audio_encoder,
                  &video_encoder);
      }
    node = node->next;
    }

  if(audio_encoder)
    bg_plugin_unref(audio_encoder);
  if(video_encoder)
    bg_plugin_unref(video_encoder);
  
  
  return ret;  
  }

