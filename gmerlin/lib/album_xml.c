/*****************************************************************
 
  album_xml.c
 
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
#include <stdlib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include <tree.h>
#include <treeprivate.h>
#include <utils.h>

/* Load an entry from a node */

static bg_album_entry_t * load_entry(bg_album_t * album,
                                     xmlDocPtr xml_doc,
                                     xmlNodePtr node,
                                     int * current)
  {
  bg_album_entry_t * ret;
  char * tmp_string;

  ret = bg_album_entry_create(album);
  ret->total_tracks = 1;
  if(current)
    *current = 0;
  
  if((tmp_string = xmlGetProp(node, "current")))
    {
    if(current)
      *current = 1;
    xmlFree(tmp_string);
    }

  if((tmp_string = xmlGetProp(node, "error")))
    {
    if(atoi(tmp_string))
      ret->flags |= BG_ALBUM_ENTRY_ERROR;
    xmlFree(tmp_string);
    }
  if((tmp_string = xmlGetProp(node, "privname")))
    {
    if(atoi(tmp_string))
      ret->flags |= BG_ALBUM_ENTRY_PRIVNAME;
    xmlFree(tmp_string);
    }
  
  node = node->children;
  
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    tmp_string = xmlNodeListGetString(xml_doc, node->children, 1);
    
    if(!strcmp(node->name, "NAME"))
      {
      ret->name = bg_strdup(ret->name, tmp_string);
      }
    else if(!strcmp(node->name, "LOCATION"))
      {
      ret->location = (void*)bg_strdup(ret->location, tmp_string);
      }
    else if(!strcmp(node->name, "PLUGIN"))
      {
      ret->plugin = (void*)bg_strdup(ret->plugin, tmp_string);
      }
    else if(!strcmp(node->name, "DURATION"))
      {
      sscanf(tmp_string, "%lld", &(ret->duration));
      }
    else if(!strcmp(node->name, "ASTREAMS"))
      {
      sscanf(tmp_string, "%d", &(ret->num_audio_streams));
      }
    else if(!strcmp(node->name, "VSTREAMS"))
      {
      sscanf(tmp_string, "%d", &(ret->num_video_streams));
      }
    else if(!strcmp(node->name, "INDEX"))
      {
      sscanf(tmp_string, "%d", &(ret->index));
      }
    else if(!strcmp(node->name, "TOTAL_TRACKS"))
      {
      sscanf(tmp_string, "%d", &(ret->total_tracks));
      }
    
    if(tmp_string)
      xmlFree(tmp_string);
    node = node->next;
    }
  return ret;
  }

static bg_album_entry_t * xml_2_album(bg_album_t * album,
                                      xmlDocPtr xml_doc,
                                      bg_album_entry_t ** last,
                                      bg_album_entry_t ** current,
                                      int load_globals)
  {
  xmlNodePtr node;
  int is_current;
  bg_album_entry_t * ret = (bg_album_entry_t*)0;
  bg_album_entry_t * end_ptr = (bg_album_entry_t*)0;
  bg_album_entry_t * new_entry;

  if(current)
    *current = (bg_album_entry_t*)0;
  
  node = xml_doc->children;

  if(strcmp(node->name, "ALBUM"))
    {
    xmlFreeDoc(xml_doc);
    return (bg_album_entry_t *)0;
    }

  node = node->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    else if(!strcmp(node->name, "CFG_SECTION") && load_globals)
      {
      bg_cfg_xml_2_section(xml_doc, node, album->cfg_section);
      }
    else if(!strcmp(node->name, "ENTRY"))
      {
      new_entry = load_entry(album, xml_doc, node, &is_current);
      if(new_entry)
        {
        if(!ret)
          {
          ret = new_entry;
          end_ptr = ret;
          }
        else
          {
          end_ptr->next = new_entry;
          end_ptr = end_ptr->next;
          }
        if(is_current && current)
          {
          *current = new_entry;
          }
        }
      }
    node = node->next;
    }
  if(last)
    *last = end_ptr;
  return ret;
  }

static bg_album_entry_t * load_album_file(bg_album_t * album,
                                   const char * filename,
                                   bg_album_entry_t ** last,
                                   bg_album_entry_t ** current,
                                   int load_globals)
  {
  bg_album_entry_t * ret;
  xmlDocPtr xml_doc;

  xml_doc = xmlParseFile(filename);

  if(!xml_doc)
    {
    fprintf(stderr, "Couldn't open album file %s\n", filename);
    return (bg_album_entry_t*)0;
    }
  
  xml_doc = xmlParseFile(filename);
  
  ret = xml_2_album(album, xml_doc, last, current, load_globals);
  
  xmlFreeDoc(xml_doc);
  return ret;
  }

static bg_album_entry_t * load_album_xml(bg_album_t * album,
                                  const char * string, int len,
                                  bg_album_entry_t ** last,
                                  bg_album_entry_t ** current,
                                  int load_globals)
  {
  bg_album_entry_t * ret;
  xmlDocPtr xml_doc;
  xml_doc = xmlParseMemory(string, len);

  ret = xml_2_album(album, xml_doc, last, current, load_globals);
  
  xmlFreeDoc(xml_doc);
  return ret;
  }

bg_album_entry_t *
bg_album_entries_new_from_xml(const char * xml_string,
                             int length)
  {
  bg_album_entry_t * ret;
  xmlDocPtr xml_doc;
  xml_doc = xmlParseMemory(xml_string, length);

  ret = xml_2_album((bg_album_t*)0, xml_doc,
                    (bg_album_entry_t**)0, (bg_album_entry_t**)0, 0);
  
  xmlFreeDoc(xml_doc);
  return ret;
  }



/* Inserts an xml-string */

void bg_album_insert_xml_after(bg_album_t * a,
                               const char * xml_string, int len,
                               bg_album_entry_t * before)
  {
  bg_album_entry_t * new_entries;
  bg_album_entry_t * current_entry;
  new_entries = load_album_xml(a,
                               xml_string, len,
                               (bg_album_entry_t**)0, &current_entry, 0);
  bg_album_insert_entries_after(a, new_entries, before);

  if(current_entry)
    {
    bg_album_set_current(a, current_entry);
    }
  
  return;
  }

void bg_album_insert_xml_before(bg_album_t * a,
                                const char * xml_string, int len,
                                bg_album_entry_t * after)
  {
  bg_album_entry_t * new_entries;
  bg_album_entry_t * current_entry;
  new_entries = load_album_xml(a,
                               xml_string, len, (bg_album_entry_t**)0, &current_entry, 0);
  bg_album_insert_entries_before(a, new_entries, after);

  if(current_entry)
    {
    bg_album_set_current(a, current_entry);
    }
  return;
  }


static bg_album_entry_t * load_albums(bg_album_t * album,
                                      char ** filenames,
                                      bg_album_entry_t ** last,
                                      bg_album_entry_t ** current)
  {
  int i = 0;
  bg_album_entry_t * ret;
  bg_album_entry_t * end = (bg_album_entry_t *)0;
  bg_album_entry_t * tmp_end;

  ret = (bg_album_entry_t*)0;
  
  while(filenames[i])
    {
    if(!ret)
      {
      ret = load_album_file(album, filenames[i], &tmp_end, current, 0);
      end = tmp_end;
      }
    else
      {
      end->next = load_album_file(album, filenames[i], &tmp_end, current, 0);
      end = tmp_end;
      }
    
    i++;
    }
  if(last)
    *last = end;
  return ret;
  }

void bg_album_insert_albums_before(bg_album_t * a,
                                   char ** locations,
                                   bg_album_entry_t * after)
  {
  bg_album_entry_t * new_entries;
  new_entries = load_albums(a, locations, (bg_album_entry_t **)0, NULL);
  bg_album_insert_entries_before(a, new_entries, after);
  }

void bg_album_insert_albums_after(bg_album_t * a,
                                  char ** locations,
                                  bg_album_entry_t * before)
  {
  bg_album_entry_t * new_entries;
  new_entries = load_albums(a, locations, (bg_album_entry_t **)0, NULL);
  bg_album_insert_entries_after(a, new_entries, before);
  }

void bg_album_load(bg_album_t * a, const char * filename)
  {
  bg_album_entry_t * current;
  current = (bg_album_entry_t*)0;


  a->entries = load_album_file(a, filename, (bg_album_entry_t**)0, &current, 1);
  if(current)
    {
    bg_album_set_current(a, current);
    //    fprintf(stderr, "Current: %s\n", current->name);
    }
  }

/* Save album */

static void save_entry(bg_album_t * a, bg_album_entry_t * entry, xmlNodePtr parent)
  {
  xmlNodePtr xml_entry;
  xmlNodePtr node;
  char * c_tmp;
  
  xml_entry = xmlNewTextChild(parent, (xmlNsPtr)0, "ENTRY", NULL);

  if(a && bg_album_entry_is_current(a, entry))  
    {
    //    fprintf(stderr, "Save Current: %s\n", entry->name);
    xmlSetProp(xml_entry, "current", "1");
    }
  if(entry->flags & BG_ALBUM_ENTRY_ERROR)
    {
    xmlSetProp(xml_entry, "error", "1");
    }
  if(entry->flags & BG_ALBUM_ENTRY_PRIVNAME)
    {
    xmlSetProp(xml_entry, "privname", "1");
    }
  
  xmlAddChild(xml_entry, xmlNewText("\n"));
  xmlAddChild(parent, xmlNewText("\n"));

  /* Name */

  node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, "NAME", NULL);
  xmlAddChild(node, xmlNewText(entry->name));
  xmlAddChild(xml_entry, xmlNewText("\n"));
  
  /* Location */

  if(entry->location)
    {
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, "LOCATION", NULL);
    xmlAddChild(node, xmlNewText((char*)(entry->location)));
    xmlAddChild(xml_entry, xmlNewText("\n"));
    }

  /* Plugin */

  if(entry->plugin)
    {
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, "PLUGIN", NULL);
    xmlAddChild(node, xmlNewText(entry->plugin));
    xmlAddChild(xml_entry, xmlNewText("\n"));
    }
  
  /* Audio streams */

  if(entry->num_audio_streams)
    {
    c_tmp = bg_sprintf("%d", entry->num_audio_streams);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, "ASTREAMS", NULL);
    xmlAddChild(node, xmlNewText(c_tmp));
    xmlAddChild(xml_entry, xmlNewText("\n"));
    free(c_tmp);
    }

  /* Video streams */

  if(entry->num_video_streams)
    {
    c_tmp = bg_sprintf("%d", entry->num_video_streams);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, "VSTREAMS", NULL);
    xmlAddChild(node, xmlNewText(c_tmp));
    free(c_tmp);
    xmlAddChild(xml_entry, xmlNewText("\n"));
    }

  /* Index */

  if(entry->total_tracks > 1)
    {
    c_tmp = bg_sprintf("%d", entry->index);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, "INDEX", NULL);
    xmlAddChild(node, xmlNewText(c_tmp));
    free(c_tmp);
    xmlAddChild(xml_entry, xmlNewText("\n"));

    c_tmp = bg_sprintf("%d", entry->total_tracks);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, "TOTAL_TRACKS", NULL);
    xmlAddChild(node, xmlNewText(c_tmp));
    free(c_tmp);
    xmlAddChild(xml_entry, xmlNewText("\n"));
    }
  
  /* Duration */

  c_tmp = bg_sprintf("%lld", entry->duration);
  node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, "DURATION", NULL);
  xmlAddChild(node, xmlNewText(c_tmp));
  free(c_tmp);
  xmlAddChild(xml_entry, xmlNewText("\n"));
  }

static xmlDocPtr album_2_xml(bg_album_t * a)
  {
  bg_album_entry_t * entry;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_album;
  xmlNodePtr node;
  
  /* First step: Build the xml tree */

  xml_doc = xmlNewDoc("1.0");
  xml_album = xmlNewDocRawNode(xml_doc, NULL, "ALBUM", NULL);
  
  xmlDocSetRootElement(xml_doc, xml_album);

  xmlAddChild(xml_album, xmlNewText("\n"));

  /* Save config data */

  if(a->cfg_section)
    {
    node = xmlNewTextChild(xml_album, (xmlNsPtr)0, "CFG_SECTION", NULL);
    bg_cfg_section_2_xml(a->cfg_section, node);
    xmlAddChild(xml_album, xmlNewText("\n"));
    }
  
  entry = a->entries;

  while(entry)
    {
    save_entry(a, entry, xml_album);
    entry = entry->next;
    }
  return xml_doc;
  }

static xmlDocPtr selected_2_xml(bg_album_t * a)
  {
  bg_album_entry_t * entry;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_album;
  
  /* First step: Build the xml tree */

  xml_doc = xmlNewDoc("1.0");
  xml_album = xmlNewDocRawNode(xml_doc, NULL, "ALBUM", NULL);
  xmlDocSetRootElement(xml_doc, xml_album);

  xmlAddChild(xml_album, xmlNewText("\n"));

  entry = a->entries;

  while(entry)
    {
    if(entry->flags & BG_ALBUM_ENTRY_SELECTED)
      save_entry(a, entry, xml_album);
    entry = entry->next;
    }
  return xml_doc;
  
  }

/* Output routines for writing to memory */

typedef struct output_mem_s
  {
  int bytes_written;
  int bytes_allocated;
  char * buffer;
  } output_mem_t;

#define BLOCK_SIZE 2048

static int xml_write_callback(void * context, const char * buffer,
                       int len)
  {
  output_mem_t * o = (output_mem_t*)context;

  if(o->bytes_allocated - o->bytes_written < len)
    {
    o->bytes_allocated += BLOCK_SIZE;
    while(o->bytes_allocated < o->bytes_written + len)
      o->bytes_allocated += BLOCK_SIZE;
    o->buffer = realloc(o->buffer, o->bytes_allocated);
    }
  memcpy(&(o->buffer[o->bytes_written]), buffer, len);
  o->bytes_written += len;
  return len;
  }

static int xml_close_callback(void * context)
  {
  output_mem_t * o = (output_mem_t*)context;
  o->buffer[o->bytes_written] = '\0';
  return 0;
  }

char * bg_album_save_to_memory(bg_album_t * a, int * len)
  {
  xmlDocPtr  xml_doc;
  output_mem_t * ctx;
  xmlOutputBufferPtr b;
  char * ret;

  xml_doc = album_2_xml(a);
  
  ctx = calloc(1, sizeof(*ctx));
  
  b = xmlOutputBufferCreateIO (xml_write_callback,
                               xml_close_callback,
                               ctx,
                               (xmlCharEncodingHandlerPtr)0);
  
  xmlSaveFileTo(b,
                xml_doc,
                (const char*)0);
  xmlFreeDoc(xml_doc);
  ret = ctx->buffer;
  if(len)
    *len = ctx->bytes_written;
  free(ctx);
  return ret;
  }

char * bg_album_save_selected_to_memory(bg_album_t * a, int * len)
  {
  xmlDocPtr  xml_doc;
  output_mem_t * ctx;
  xmlOutputBufferPtr b;
  char * ret;

  xml_doc = selected_2_xml(a);
  
  ctx = calloc(1, sizeof(*ctx));
  
  b = xmlOutputBufferCreateIO(xml_write_callback,
                              xml_close_callback,
                              ctx,
                              (xmlCharEncodingHandlerPtr)0);
  
  xmlSaveFileTo(b,
                xml_doc,
                (const char*)0);
  xmlFreeDoc(xml_doc);
  ret = ctx->buffer;
  if(len)
    *len = ctx->bytes_written;
  free(ctx);
  return ret;
  }


void bg_album_save(bg_album_t * a, const char * filename)
  {
  char * tmp_filename;

  xmlDocPtr  xml_doc;

  if((a->type == BG_ALBUM_TYPE_REMOVABLE) ||
     (a->type == BG_ALBUM_TYPE_PLUGIN))
    return;
  
  
  xml_doc = album_2_xml(a);
    
  /* Get the filename */

  if(filename)
    {
    xmlSaveFile(filename, xml_doc);
    }
  else if(a->location)
    {
    tmp_filename = bg_sprintf("%s/%s", a->com->directory, a->location);
    xmlSaveFile(tmp_filename, xml_doc);
    free(tmp_filename);
    }
  xmlFreeDoc(xml_doc);
  }
