#include <string.h>

#include <track.h>
#include <gmerlin/utils.h>

static const char * parameters_name = "parameters";
static const char * segments_name   = "segments";
static const char * segment_name    = "segment";

static const struct
  {
  bg_nle_track_type_t type;
  const char * name;
  } 
type_names[] =
  {
    { BG_NLE_TRACK_AUDIO, "audio" },
    { BG_NLE_TRACK_VIDEO, "video" },
    { /* End */ }
  };

static const char * type_to_name(bg_nle_track_type_t type)
  {
  int i = 0;
  while(type_names[i].name)
    {
    if(type_names[i].type == type)
      return type_names[i].name;
    i++;
    }
  return NULL;
  }

static bg_nle_track_type_t name_to_type(const char * name)
  {
  int i = 0;
  while(type_names[i].name)
    {
    if(!strcmp(type_names[i].name, name))
      return type_names[i].type;
    i++;
    }
  return BG_NLE_TRACK_NONE;
  }

bg_nle_track_t * bg_nle_track_load(xmlDocPtr xml_doc, xmlNodePtr node)
  {
  bg_nle_track_t * ret;
  char * tmp_string;
  xmlNodePtr child;
  xmlNodePtr grandchild;
  int i;
  
  ret = calloc(1, sizeof(*ret));
  
  if((tmp_string = BG_XML_GET_PROP(node, "type")))
    {
    ret->type = name_to_type(tmp_string);
    xmlFree(tmp_string);
    }
  if((tmp_string = BG_XML_GET_PROP(node, "flags")))
    {
    ret->flags = strtol(tmp_string, NULL, 16);
    xmlFree(tmp_string);
    }
  
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

    if(!BG_XML_STRCMP(child->name, parameters_name))
      {
      ret->section = bg_cfg_section_create("");
      bg_cfg_xml_2_section(xml_doc, child, ret->section);
      }

    if(!BG_XML_STRCMP(child->name, segments_name))
      {
      if((tmp_string = BG_XML_GET_PROP(child, "num")))
        {
        ret->num_segments = strtoll(tmp_string, (char**)0, 10);
        ret->segments = calloc(ret->num_segments, sizeof(*ret->segments));
        free(tmp_string);
        }
      
      grandchild = child->children;
      i = 0;
      while(grandchild)
        {
        if(!grandchild->name)
          {
          grandchild = grandchild->next;
          continue;
          }
        
        if(!BG_XML_STRCMP(grandchild->name, segment_name))
          {
          if((tmp_string = BG_XML_GET_PROP(node, "src_pos")))
            {
            ret->segments[i].src_pos = strtoll(tmp_string, (char**)0, 10);
            free(tmp_string);
            }
          if((tmp_string = BG_XML_GET_PROP(node, "dst_pos")))
            {
            ret->segments[i].dst_pos = strtoll(tmp_string, (char**)0, 10);
            free(tmp_string);
            }
          if((tmp_string = BG_XML_GET_PROP(node, "len")))
            {
            ret->segments[i].len = strtoll(tmp_string, (char**)0, 10);
            free(tmp_string);
            }
          if((tmp_string = BG_XML_GET_PROP(node, "scale")))
            {
            ret->segments[i].scale = strtol(tmp_string, (char**)0, 10);
            free(tmp_string);
            }
          if((tmp_string = BG_XML_GET_PROP(node, "stream")))
            {
            ret->segments[i].stream = strtol(tmp_string, (char**)0, 10);
            free(tmp_string);
            }
          if((tmp_string = BG_XML_GET_PROP(node, "file_id")))
            {
            ret->segments[i].file_id = strtol(tmp_string, (char**)0, 10);
            free(tmp_string);
            
            }
          
          i++;
          }
        }
      
      }
    
    child = child->next;
    }
  
  return ret;
  }

void bg_nle_track_save(bg_nle_track_t * t, xmlNodePtr parent)
  {
  int i;
  char * tmp_string;
  xmlNodePtr node;
  xmlNodePtr child;
  xmlNodePtr grandchild;
  
  node = xmlNewTextChild(parent, (xmlNsPtr)0,
                         (xmlChar*)"track", NULL);
  BG_XML_SET_PROP(node, "type", type_to_name(t->type));

  tmp_string = bg_sprintf("%08x", t->id);
  BG_XML_SET_PROP(node, "id", tmp_string);
  free(tmp_string);
  
  tmp_string = bg_sprintf("%08x", t->flags);
  BG_XML_SET_PROP(node, "flags", tmp_string);
  free(tmp_string);

  if(t->section)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)parameters_name, NULL);
    bg_cfg_section_2_xml(t->section, child);
    }
  
  if(t->num_segments)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)segments_name, NULL);

    tmp_string = bg_sprintf("%d", t->num_segments);
    BG_XML_SET_PROP(child, "num", tmp_string);
    free(tmp_string);
    
    for(i = 0; i < t->num_segments; i++)
      {
      grandchild = xmlNewTextChild(child, (xmlNsPtr)0,
                                   (xmlChar*)segment_name, NULL);

      tmp_string = bg_sprintf("%d", t->segments[i].scale);
      BG_XML_SET_PROP(grandchild, "scale", tmp_string);
      free(tmp_string);

      tmp_string = bg_sprintf("%d", t->segments[i].stream);
      BG_XML_SET_PROP(grandchild, "stream", tmp_string);
      free(tmp_string);
      
      tmp_string = bg_sprintf("%"PRId64, t->segments[i].src_pos);
      BG_XML_SET_PROP(grandchild, "src_pos", tmp_string);
      free(tmp_string);

      tmp_string = bg_sprintf("%"PRId64, t->segments[i].dst_pos);
      BG_XML_SET_PROP(grandchild, "dst_pos", tmp_string);
      free(tmp_string);

      tmp_string = bg_sprintf("%"PRId64, t->segments[i].len);
      BG_XML_SET_PROP(grandchild, "len", tmp_string);
      free(tmp_string);

      
      }
    
    }

  }

