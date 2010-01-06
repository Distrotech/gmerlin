#include <inttypes.h>
#include <string.h>

#include <types.h>
#include <track.h>
#include <medialist.h>
#include <clipboard.h>
#include <project.h>

#include <gmerlin/utils.h>

char * bg_nle_clipboard_to_string(const bg_nle_clipboard_t * c)
  {
  xmlDocPtr  xml_doc;
  bg_xml_output_mem_t ctx;
  xmlOutputBufferPtr b;
  xmlNodePtr xml_clipboard;
  xmlNodePtr node;
  xmlNodePtr child;
  char * tmp_string;
  int i;
  
  memset(&ctx, 0, sizeof(ctx));

  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_clipboard = xmlNewDocRawNode(xml_doc, NULL,
                                   (xmlChar*)"GMERLERRA_CLIPBOARD", NULL);
  xmlDocSetRootElement(xml_doc, xml_clipboard);

  /* Add files */
  if(c->num_files)
    {
    node =
      xmlNewTextChild(xml_clipboard, (xmlNsPtr)0, (xmlChar*)"files", NULL);

    tmp_string = bg_sprintf("%d", c->num_files);
    BG_XML_SET_PROP(node, "num", tmp_string);
    free(tmp_string);

    for(i = 0; i < c->num_files; i++)
      {
      child = xmlNewTextChild(node, (xmlNsPtr)0,
                              (xmlChar*)"file", NULL);

      bg_nle_file_save(child, c->files[i]);
      }
    }
  if(c->num_tracks)
    {
    node =
      xmlNewTextChild(xml_clipboard, (xmlNsPtr)0, (xmlChar*)"files", NULL);
    
    tmp_string = bg_sprintf("%d", c->num_tracks);
    BG_XML_SET_PROP(node, "num", tmp_string);
    free(tmp_string);

    for(i = 0; i < c->num_tracks; i++)
      bg_nle_track_save(c->tracks[i], node);
    }
  
  //  xml_doc = entries_2_xml(a, preserve_current, 1);
  
  b = xmlOutputBufferCreateIO(bg_xml_write_callback,
                              bg_xml_close_callback,
                              &ctx,
                              (xmlCharEncodingHandlerPtr)0);
  
  xmlSaveFileTo(b,
                xml_doc,
                (const char*)0);
  xmlFreeDoc(xml_doc);
  return ctx.buffer;
  }

int bg_nle_clipboard_from_string(bg_nle_clipboard_t * c, const char * str)
  {
  xmlDocPtr  xml_doc;
  xmlNodePtr node;
  xmlNodePtr child;
  char * tmp_string;
  int index;
  
  bg_nle_clipboard_free(c);

  xml_doc = xmlParseMemory(str, strlen(str));
  if(!xml_doc)
    return 0;
  node = xml_doc->children;

  if(BG_XML_STRCMP(node->name, "GMERLERRA_CLIPBOARD"))
    {
    xmlFreeDoc(xml_doc);
    return 0;
    }
  
  node = node->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, "files"))
      {
      if((tmp_string = BG_XML_GET_PROP(node, "num")))
        {
        index = 0;
        c->num_files = atoi(tmp_string);
        xmlFree(tmp_string);

        c->files = calloc(c->num_files, sizeof(*c->files));

        child = node->children;

        while(child)
          {
          if(!child->name)
            {
            child = child->next;
            continue;
            }

          if(!BG_XML_STRCMP(child->name, "file"))
            {
            c->files[index] = bg_nle_file_load(xml_doc, child);
            index++;
            }
          child = child->next;
          }
        
        }
      
      }
    else if(!BG_XML_STRCMP(node->name, "tracks"))
      {
      if((tmp_string = BG_XML_GET_PROP(node, "num")))
        {
        index = 0;
        c->num_tracks = atoi(tmp_string);
        xmlFree(tmp_string);

        c->tracks = calloc(c->num_tracks, sizeof(*c->tracks));

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
            c->tracks[index] = bg_nle_track_load(xml_doc, child);
            index++;
            }
          child = child->next;
          }
        
        }
      
      }
    node = node->next;
    }
  
  return 1;
  }
