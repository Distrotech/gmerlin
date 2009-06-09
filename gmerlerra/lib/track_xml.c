#include <string.h>

#include <track.h>
#include <gmerlin/utils.h>

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
  if((tmp_string = BG_XML_GET_PROP(node, "name")))
    {
    ret->name = bg_strdup(ret->name, tmp_string);
    xmlFree(tmp_string);
    }
  
  child = node->children;
  
  return ret;
  }

void bg_nle_track_save(bg_nle_track_t * t, xmlNodePtr parent)
  {
  char * tmp_string;
  xmlNodePtr node;
  xmlNodePtr child;
  
  node = xmlNewTextChild(parent, (xmlNsPtr)0,
                         (xmlChar*)"track", NULL);
  BG_XML_SET_PROP(node, "type", type_to_name(t->type));
  
  tmp_string = bg_sprintf("%08x", t->flags);
  BG_XML_SET_PROP(node, "flags", tmp_string);
  free(tmp_string);
  BG_XML_SET_PROP(node, "name", t->name);
  }
