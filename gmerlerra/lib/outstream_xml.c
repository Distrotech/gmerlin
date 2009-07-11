#include <string.h>

#include <track.h>
#include <outstream.h>
#include <gmerlin/utils.h>

static const char * parameters_name    = "parameters";
static const char * source_tracks_name = "source_tracks";
static const char * source_track_name = "source_track";

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

bg_nle_outstream_t * bg_nle_outstream_load(xmlDocPtr xml_doc, xmlNodePtr node)
  {
  bg_nle_outstream_t * ret;
  char * tmp_string;
  xmlNodePtr child;
  xmlNodePtr grandchild;
  
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

    if(!BG_XML_STRCMP(child->name, parameters_name))
      {
      ret->section = bg_cfg_section_create("");
      bg_cfg_xml_2_section(xml_doc, child, ret->section);
      }

    if(!BG_XML_STRCMP(child->name, source_tracks_name))
      {
      if((tmp_string = BG_XML_GET_PROP(child, "num")))
        {
        ret->source_tracks_alloc = strtol(tmp_string, NULL, 10);
        xmlFree(tmp_string);
        
        ret->source_track_ids = calloc(ret->source_tracks_alloc,
                                       sizeof(*ret->source_track_ids));

        grandchild = child->children;

        while(grandchild)
          {
          if(!grandchild->name)
            {
            grandchild = grandchild->next;
            continue;
            }
          if(!BG_XML_STRCMP(child->name, source_track_name))
            {
            if((tmp_string = BG_XML_GET_PROP(child, "id")))
              {
              ret->source_track_ids[ret->num_source_tracks] =
                strtol(tmp_string, NULL, 10);
              xmlFree(tmp_string);
              ret->num_source_tracks++;
              }
            }
          grandchild = grandchild->next;
          }
        }
      }
    child = child->next;
    }
  
  return ret;
  }

void bg_nle_outstream_save(bg_nle_outstream_t * t, xmlNodePtr parent)
  {
  int i;
  char * tmp_string;
  xmlNodePtr node;
  xmlNodePtr child;
  xmlNodePtr grandchild;
  
  node = xmlNewTextChild(parent, (xmlNsPtr)0,
                         (xmlChar*)"outstream", NULL);
  BG_XML_SET_PROP(node, "type", type_to_name(t->type));
  
  tmp_string = bg_sprintf("%08x", t->flags);
  BG_XML_SET_PROP(node, "flags", tmp_string);
  free(tmp_string);
  
  tmp_string = bg_sprintf("%08x", t->id);
  BG_XML_SET_PROP(node, "id", tmp_string);
  free(tmp_string);
  
  child = xmlNewTextChild(node, (xmlNsPtr)0,
                          (xmlChar*)parameters_name, NULL);
  bg_cfg_section_2_xml(t->section, child);

  /* Save source tracks */
  if(t->num_source_tracks)
    {
    child = xmlNewTextChild(node, (xmlNsPtr)0,
                            (xmlChar*)source_tracks_name, NULL);

    tmp_string = bg_sprintf("%d", t->num_source_tracks);
    BG_XML_SET_PROP(child, "num", tmp_string);
    free(tmp_string);

    for(i = 0; i < t->num_source_tracks; i++)
      {
      grandchild = xmlNewTextChild(child, (xmlNsPtr)0,
                                   (xmlChar*)source_track_name, NULL);
      tmp_string = bg_sprintf("%08x", t->source_tracks[i]->id);
      BG_XML_SET_PROP(grandchild, "id", tmp_string);
      free(tmp_string);
      }
    }
  
  }

