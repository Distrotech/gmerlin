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

#include <inttypes.h>
#include <string.h>
#include <gmerlin/edl.h>

#include <gmerlin/utils.h>
#include <gmerlin/xmlutils.h>

static const char * const edl_key           = "GMERLIN_EDL";
static const char * const tracks_key        = "tracks";
static const char * const track_key         = "track";
static const char * const audio_streams_key = "audio_streams";
static const char * const video_streams_key = "video_streams";
static const char * const subtitle_text_streams_key   = "subtitle_text_streams";
static const char * const subtitle_overlay_streams_key   = "subtitle_overlay_streams";
static const char * const stream_key       = "stream";
static const char * const segment_key      = "segment";
static const char * const segments_key     = "segments";
static const char * const url_key          = "url";
static const char * const track_index_key  = "tindex";
static const char * const stream_index_key = "sindex";
static const char * const src_time_key     = "stime";
static const char * const dst_time_key     = "dtime";
static const char * const dst_duration_key = "duration";
static const char * const speed_key        = "speed";
static const char * const metadata_key     = "metadata";


static void load_segments(xmlDocPtr doc, 
                          xmlNodePtr node,
                          bg_edl_stream_t * s)
  {
  char * tmp_string;
  bg_edl_segment_t * seg;
  xmlNodePtr child_node;

  node = node->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, segment_key))
      {
      seg = bg_edl_add_segment(s);
      tmp_string = BG_XML_GET_PROP(node, "scale");
      if(tmp_string)
        {
        seg->timescale = atoi(tmp_string);
        xmlFree(tmp_string);
        }
      child_node = node->children;
      while(child_node)
        {
        if(!child_node->name)
          {
          child_node = child_node->next;
          continue;
          }
        if(!BG_XML_STRCMP(child_node->name, track_index_key))
          {
          tmp_string = (char*)xmlNodeListGetString(doc, child_node->children, 1);
          seg->track = atoi(tmp_string);
          xmlFree(tmp_string);
          }
        if(!BG_XML_STRCMP(child_node->name, url_key))
          {
          tmp_string = (char*)xmlNodeListGetString(doc, child_node->children, 1);
          seg->url = bg_strdup(seg->url, tmp_string);
          xmlFree(tmp_string);
          }
        if(!BG_XML_STRCMP(child_node->name, stream_index_key))
          {
          tmp_string = (char*)xmlNodeListGetString(doc, child_node->children, 1);
          seg->stream = atoi(tmp_string);
          xmlFree(tmp_string);
          }
        if(!BG_XML_STRCMP(child_node->name, src_time_key))
          {
          tmp_string = (char*)xmlNodeListGetString(doc, child_node->children, 1);
          sscanf(tmp_string, "%"PRId64, &seg->src_time);
          xmlFree(tmp_string);
          }
        if(!BG_XML_STRCMP(child_node->name, dst_time_key))
          {
          tmp_string = (char*)xmlNodeListGetString(doc, child_node->children, 1);
          sscanf(tmp_string, "%"PRId64, &seg->dst_time);
          xmlFree(tmp_string);
          }
        if(!BG_XML_STRCMP(child_node->name, dst_duration_key))
          {
          tmp_string = (char*)xmlNodeListGetString(doc, child_node->children, 1);
          sscanf(tmp_string, "%"PRId64, &seg->dst_duration);
          xmlFree(tmp_string);
          }
        if(!BG_XML_STRCMP(child_node->name, speed_key))
          {
          tmp_string = (char*)xmlNodeListGetString(doc, child_node->children, 1);
          sscanf(tmp_string, "%d:%d", &seg->speed_num, &seg->speed_den);
          xmlFree(tmp_string);
          }
        
        child_node = child_node->next;
        }
      }
       
    node = node->next;
    }
  }


static void load_streams(xmlDocPtr doc, 
                         xmlNodePtr node,
                         bg_edl_stream_t * (*add_func)(bg_edl_track_t *),
                         bg_edl_track_t * t)
  {
  char * tmp_string;
  bg_edl_stream_t * s;
  xmlNodePtr child_node;

  node = node->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, stream_key))
      {
      s = add_func(t);
      tmp_string = BG_XML_GET_PROP(node, "scale");
      if(tmp_string)
        {
        s->timescale = atoi(tmp_string);
        xmlFree(tmp_string);
        }
      child_node = node->children;
      while(child_node)
        {
        if(!child_node->name)
          {
          child_node = child_node->next;
          continue;
          }

        if(!BG_XML_STRCMP(child_node->name, segments_key))
          {
          load_segments(doc, child_node, s);
          }
        
        child_node = child_node->next;
        }
      }
       
    node = node->next;
    }
  }

static void load_track(xmlDocPtr doc, xmlNodePtr node,
                       bg_edl_t * ret)
  {
  bg_edl_track_t * et;
  
  et = bg_edl_add_track(ret);
  
  node = node->children;
  
  
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, audio_streams_key))
      load_streams(doc, node, bg_edl_add_audio_stream, et);
    else if(!BG_XML_STRCMP(node->name, video_streams_key))
      load_streams(doc, node, bg_edl_add_video_stream, et);
    else if(!BG_XML_STRCMP(node->name, subtitle_text_streams_key))
      load_streams(doc, node, bg_edl_add_subtitle_text_stream, et);
    else if(!BG_XML_STRCMP(node->name, subtitle_overlay_streams_key))
      load_streams(doc, node, bg_edl_add_subtitle_overlay_stream, et);
    else if(!BG_XML_STRCMP(node->name, metadata_key))
      bg_xml_2_metadata(doc, node, &et->metadata);
    
    node = node->next;
    }
  
  }

bg_edl_t * bg_edl_load(const char * filename)
  {
  bg_edl_t * ret;
  char * tmp_string;
  xmlDocPtr xml_doc;
  xmlNodePtr node;
  xmlNodePtr child_node;
  
  xml_doc = bg_xml_parse_file(filename);

  if(!xml_doc)
    return NULL;

  node = xml_doc->children;

  if(BG_XML_STRCMP(node->name, edl_key))
    {
    xmlFreeDoc(xml_doc);
    return NULL;
    }

  node = node->children;
  
  ret = bg_edl_create();
  
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
      
    if(!BG_XML_STRCMP(node->name, url_key))
      {
      tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
      ret->url = bg_strdup(ret->url, tmp_string);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, tracks_key))
      {
      child_node = node->children;

      while(child_node)
        {
        if(!child_node->name)
          {
          child_node = child_node->next;
          continue;
          }

        if(!BG_XML_STRCMP(child_node->name, track_key))
          {
          load_track(xml_doc, child_node, ret);
          }
        child_node = child_node->next;
        }
      }
    node = node->next;
    }
  return ret;
  }

static void save_streams(xmlNodePtr parent, const bg_edl_stream_t * s, int num)
  {
  int i, j;
  xmlNodePtr stream_node;
  xmlNodePtr segments_node;
  xmlNodePtr segment_node;
  xmlNodePtr node;
  char * tmp_string;
  tmp_string = bg_sprintf("%d", num);
  BG_XML_SET_PROP(parent, "num", tmp_string);
  free(tmp_string);

  for(i = 0; i < num; i++)
    {
    stream_node = xmlNewTextChild(parent, NULL, (xmlChar*)stream_key, NULL);
    xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));

    xmlAddChild(stream_node, BG_XML_NEW_TEXT("\n"));

    tmp_string = bg_sprintf("%d", s[i].timescale);
    BG_XML_SET_PROP(stream_node, "scale", tmp_string);
    free(tmp_string);
    
    segments_node = xmlNewTextChild(stream_node, NULL, (xmlChar*)segments_key, NULL);

    xmlAddChild(stream_node, BG_XML_NEW_TEXT("\n"));

    tmp_string = bg_sprintf("%d", s[i].num_segments);
    BG_XML_SET_PROP(segments_node, "num", tmp_string);
    free(tmp_string);

    xmlAddChild(segments_node, BG_XML_NEW_TEXT("\n"));

    for(j = 0; j < s[i].num_segments; j++)
      {
      segment_node = xmlNewTextChild(segments_node, NULL, (xmlChar*)segment_key, NULL);

      
      tmp_string = bg_sprintf("%d", s[i].segments[j].timescale);
      BG_XML_SET_PROP(segment_node, "scale", tmp_string);
      free(tmp_string);

      xmlAddChild(segment_node, BG_XML_NEW_TEXT("\n"));

      if(s[i].segments[j].url)
        {
        node = xmlNewTextChild(segment_node, NULL, (xmlChar*)url_key, NULL);
        xmlAddChild(node, BG_XML_NEW_TEXT(s[i].segments[j].url));
        xmlAddChild(segment_node, BG_XML_NEW_TEXT("\n"));
        }

      node = xmlNewTextChild(segment_node, NULL, (xmlChar*)track_index_key, NULL);
      tmp_string = bg_sprintf("%d", s[i].segments[j].track);
      xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
      xmlAddChild(segment_node, BG_XML_NEW_TEXT("\n"));
      free(tmp_string);
      
      node = xmlNewTextChild(segment_node, NULL, (xmlChar*)stream_index_key, NULL);
      tmp_string = bg_sprintf("%d", s[i].segments[j].stream);
      xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
      xmlAddChild(segment_node, BG_XML_NEW_TEXT("\n"));
      free(tmp_string);

      node = xmlNewTextChild(segment_node, NULL, (xmlChar*)src_time_key, NULL);
      tmp_string = bg_sprintf("%"PRId64, s[i].segments[j].src_time);
      xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
      xmlAddChild(segment_node, BG_XML_NEW_TEXT("\n"));
      free(tmp_string);

      node = xmlNewTextChild(segment_node, NULL, (xmlChar*)dst_time_key, NULL);
      tmp_string = bg_sprintf("%"PRId64, s[i].segments[j].dst_time);
      xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
      xmlAddChild(segment_node, BG_XML_NEW_TEXT("\n"));
      free(tmp_string);

      node = xmlNewTextChild(segment_node, NULL, (xmlChar*)dst_duration_key, NULL);
      tmp_string = bg_sprintf("%"PRId64, s[i].segments[j].dst_duration);
      xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
      xmlAddChild(segment_node, BG_XML_NEW_TEXT("\n"));
      free(tmp_string);

      node = xmlNewTextChild(segment_node, NULL, (xmlChar*)speed_key, NULL);
      tmp_string = bg_sprintf("%d:%d", s[i].segments[j].speed_num, s[i].segments[j].speed_den);
      xmlAddChild(node, BG_XML_NEW_TEXT(tmp_string));
      xmlAddChild(segment_node, BG_XML_NEW_TEXT("\n"));
      free(tmp_string);
      
      xmlAddChild(segments_node, BG_XML_NEW_TEXT("\n"));

      }
    
    }
  
  }

void bg_edl_save(const bg_edl_t * edl, const char * filename)
  {
  int i;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_edl;
  xmlNodePtr node;
  xmlNodePtr track_node;
  xmlNodePtr child_node;
  
  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_edl = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)edl_key, NULL);
  xmlDocSetRootElement(xml_doc, xml_edl);
  
  xmlAddChild(xml_edl, BG_XML_NEW_TEXT("\n"));

  if(edl->url)
    {
    node = xmlNewTextChild(xml_edl, NULL, (xmlChar*)url_key, NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(edl->url));
    xmlAddChild(xml_edl, BG_XML_NEW_TEXT("\n"));
    }
  
  if(edl->num_tracks)
    {
    node = xmlNewTextChild(xml_edl, NULL, (xmlChar*)tracks_key, NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
    for(i = 0; i < edl->num_tracks; i++)
      {
      track_node = xmlNewTextChild(node, NULL, (xmlChar*)track_key, NULL);
      xmlAddChild(track_node, BG_XML_NEW_TEXT("\n"));
      xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
      
      child_node = xmlNewTextChild(track_node, NULL, (xmlChar*)metadata_key, NULL);
      bg_metadata_2_xml(child_node, &edl->tracks[i].metadata);
      
      if(edl->tracks[i].num_audio_streams)
        {
        child_node = xmlNewTextChild(track_node, NULL, (xmlChar*)audio_streams_key, NULL);
        xmlAddChild(child_node, BG_XML_NEW_TEXT("\n"));
        xmlAddChild(track_node, BG_XML_NEW_TEXT("\n"));

        save_streams(child_node, edl->tracks[i].audio_streams,
                     edl->tracks[i].num_audio_streams);
        }
      if(edl->tracks[i].num_video_streams)
        {
        child_node = xmlNewTextChild(track_node, NULL, (xmlChar*)video_streams_key, NULL);
        xmlAddChild(child_node, BG_XML_NEW_TEXT("\n"));
        xmlAddChild(track_node, BG_XML_NEW_TEXT("\n"));

        save_streams(child_node, edl->tracks[i].video_streams,
                     edl->tracks[i].num_video_streams);

        }
      if(edl->tracks[i].num_subtitle_text_streams)
        {
        child_node = xmlNewTextChild(track_node, NULL, (xmlChar*)subtitle_text_streams_key, NULL);
        xmlAddChild(child_node, BG_XML_NEW_TEXT("\n"));
        xmlAddChild(track_node, BG_XML_NEW_TEXT("\n"));

        save_streams(child_node, edl->tracks[i].subtitle_text_streams,
                     edl->tracks[i].num_subtitle_text_streams);

        }
      if(edl->tracks[i].num_subtitle_overlay_streams)
        {
        child_node = xmlNewTextChild(track_node, NULL, (xmlChar*)subtitle_overlay_streams_key, NULL);
        xmlAddChild(child_node, BG_XML_NEW_TEXT("\n"));
        xmlAddChild(track_node, BG_XML_NEW_TEXT("\n"));

        save_streams(child_node, edl->tracks[i].subtitle_overlay_streams,
                     edl->tracks[i].num_subtitle_overlay_streams);
        }
      
      xmlAddChild(xml_edl, BG_XML_NEW_TEXT("\n"));
      }
    }
  
  //  xmlAddChild(xml_edl, BG_XML_NEW_TEXT("\n"));
  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);
  }
