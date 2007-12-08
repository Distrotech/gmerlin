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

/* Needed for chmod and mode_t */
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>

#include <tree.h>
#include <treeprivate.h>
#include <utils.h>
#include <xmlutils.h>

#include <log.h>
#define LOG_DOMAIN "album"

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
  
  if((tmp_string = BG_XML_GET_PROP(node, "current")))
    {
    if(current)
      *current = 1;
    xmlFree(tmp_string);
    }

  if((tmp_string = BG_XML_GET_PROP(node, "error")))
    {
    if(atoi(tmp_string))
      ret->flags |= BG_ALBUM_ENTRY_ERROR;
    xmlFree(tmp_string);
    }
  if((tmp_string = BG_XML_GET_PROP(node, "save_auth")))
    {
    if(atoi(tmp_string))
      ret->flags |= BG_ALBUM_ENTRY_SAVE_AUTH;
    xmlFree(tmp_string);
    }
  if((tmp_string = BG_XML_GET_PROP(node, "privname")))
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
    tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);
    
    if(!BG_XML_STRCMP(node->name, "NAME"))
      {
      ret->name = bg_strdup(ret->name, tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, "LOCATION"))
      {
      ret->location = (void*)bg_uri_to_string(tmp_string, -1);
      }
    else if(!BG_XML_STRCMP(node->name, "USER"))
      {
      ret->username = bg_strdup(ret->username, tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, "PASS"))
      {
      ret->password = bg_descramble_string(tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, "PLUGIN"))
      {
      ret->plugin = (void*)bg_strdup(ret->plugin, tmp_string);
      }
    else if(!BG_XML_STRCMP(node->name, "DURATION"))
      {
      sscanf(tmp_string, "%" PRId64, &(ret->duration));
      }
    else if(!BG_XML_STRCMP(node->name, "ASTREAMS"))
      {
      sscanf(tmp_string, "%d", &(ret->num_audio_streams));
      }
    else if(!BG_XML_STRCMP(node->name, "STSTREAMS"))
      {
      sscanf(tmp_string, "%d", &(ret->num_still_streams));
      }
    else if(!BG_XML_STRCMP(node->name, "VSTREAMS"))
      {
      sscanf(tmp_string, "%d", &(ret->num_video_streams));
      }
    else if(!BG_XML_STRCMP(node->name, "INDEX"))
      {
      sscanf(tmp_string, "%d", &(ret->index));
      }
    else if(!BG_XML_STRCMP(node->name, "TOTAL_TRACKS"))
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

  if(BG_XML_STRCMP(node->name, "ALBUM"))
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
    else if(!BG_XML_STRCMP(node->name, "CFG_SECTION") && load_globals)
      {
      bg_cfg_xml_2_section(xml_doc, node, album->cfg_section);
      }
    else if(!BG_XML_STRCMP(node->name, "ENTRY"))
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

  xml_doc = bg_xml_parse_file(filename);

  if(!xml_doc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't open album file %s", filename);
    return (bg_album_entry_t*)0;
    }
  ret = xml_2_album(album, xml_doc, last, current, load_globals);
  
  xmlFreeDoc(xml_doc);
  return ret;
  }

static bg_album_entry_t * load_album_xml(bg_album_t * album,
                                  const char * string,
                                  bg_album_entry_t ** last,
                                  bg_album_entry_t ** current,
                                  int load_globals)
  {
  bg_album_entry_t * ret;
  xmlDocPtr xml_doc;
  xml_doc = xmlParseMemory(string, strlen(string));
  
  ret = xml_2_album(album, xml_doc, last, current, load_globals);
  
  xmlFreeDoc(xml_doc);
  return ret;
  }

bg_album_entry_t *
bg_album_entries_new_from_xml(const char * xml_string)
  {
  bg_album_entry_t * ret;
  xmlDocPtr xml_doc;
  xml_doc = xmlParseMemory(xml_string, strlen(xml_string));

  ret = xml_2_album((bg_album_t*)0, xml_doc,
                    (bg_album_entry_t**)0, (bg_album_entry_t**)0, 0);
  
  xmlFreeDoc(xml_doc);
  return ret;
  }



/* Inserts an xml-string */

void bg_album_insert_xml_after(bg_album_t * a,
                               const char * xml_string,
                               bg_album_entry_t * before)
  {
  bg_album_entry_t * new_entries;
  bg_album_entry_t * current_entry;
  new_entries = load_album_xml(a,
                               xml_string,
                               (bg_album_entry_t**)0, &current_entry, 0);
  bg_album_insert_entries_after(a, new_entries, before);

  if(current_entry)
    bg_album_set_current(a, current_entry);
  
  //  bg_album_changed(a);
  return;
  }

void bg_album_insert_xml_before(bg_album_t * a,
                                const char * xml_string,
                                bg_album_entry_t * after)
  {
  bg_album_entry_t * new_entries;
  bg_album_entry_t * current_entry;
  new_entries = load_album_xml(a,
                               xml_string, (bg_album_entry_t**)0, &current_entry, 0);
  bg_album_insert_entries_before(a, new_entries, after);

  if(current_entry)
    bg_album_set_current(a, current_entry);
  
  //  bg_album_changed(a);
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
  bg_album_changed(a);
  }

void bg_album_insert_albums_after(bg_album_t * a,
                                  char ** locations,
                                  bg_album_entry_t * before)
  {
  bg_album_entry_t * new_entries;
  new_entries = load_albums(a, locations, (bg_album_entry_t **)0, NULL);
  bg_album_insert_entries_after(a, new_entries, before);
  bg_album_changed(a);
  }

void bg_album_load(bg_album_t * a, const char * filename)
  {
  bg_album_entry_t * current;
  current = (bg_album_entry_t*)0;


  a->entries = load_album_file(a, filename, (bg_album_entry_t**)0, &current, 1);
  if(current)
    {
    bg_album_set_current(a, current);
    }
  }

/* Save album */

static void save_entry(bg_album_t * a, bg_album_entry_t * entry, xmlNodePtr parent, int preserve_current)
  {
  xmlNodePtr xml_entry;
  xmlNodePtr node;
  char * c_tmp;
  
  xml_entry = xmlNewTextChild(parent, (xmlNsPtr)0, (xmlChar*)"ENTRY", NULL);

  if(a && bg_album_entry_is_current(a, entry) && preserve_current)
    {
    BG_XML_SET_PROP(xml_entry, "current", "1");
    }
  if(entry->flags & BG_ALBUM_ENTRY_ERROR)
    {
    BG_XML_SET_PROP(xml_entry, "error", "1");
    }
  if(entry->flags & BG_ALBUM_ENTRY_PRIVNAME)
    {
    BG_XML_SET_PROP(xml_entry, "privname", "1");
    }
  if(entry->flags & BG_ALBUM_ENTRY_SAVE_AUTH)
    {
    BG_XML_SET_PROP(xml_entry, "save_auth", "1");
    }
  
  xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));

  /* Name */

  node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"NAME", NULL);
  xmlAddChild(node, BG_XML_NEW_TEXT(entry->name));
  xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
  
  /* Location */

  if(entry->location)
    {
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"LOCATION", NULL);
    c_tmp = bg_string_to_uri((char*)(entry->location), -1);
    xmlAddChild(node, BG_XML_NEW_TEXT(c_tmp));
    free(c_tmp);
    xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
    }

  /* Authentication */

  if((entry->flags & BG_ALBUM_ENTRY_SAVE_AUTH) && entry->username && entry->password)
    {
    /* Username */
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"USER", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(entry->username));
    xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));

    /* Password */
    c_tmp = bg_scramble_string(entry->password);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"PASS", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(c_tmp));
    xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
    free(c_tmp);
    }
  
  /* Plugin */

  if(entry->plugin)
    {
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"PLUGIN", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(entry->plugin));
    xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
    }
  
  /* Audio streams */

  if(entry->num_audio_streams)
    {
    c_tmp = bg_sprintf("%d", entry->num_audio_streams);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"ASTREAMS", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(c_tmp));
    xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
    free(c_tmp);
    }
  
  /* Still streams */

  if(entry->num_still_streams)
    {
    c_tmp = bg_sprintf("%d", entry->num_still_streams);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"STSTREAMS", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(c_tmp));
    xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
    free(c_tmp);
    }
  
  /* Video streams */

  if(entry->num_video_streams)
    {
    c_tmp = bg_sprintf("%d", entry->num_video_streams);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"VSTREAMS", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(c_tmp));
    free(c_tmp);
    xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
    }

  /* Index */

  if(entry->total_tracks > 1)
    {
    c_tmp = bg_sprintf("%d", entry->index);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"INDEX", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(c_tmp));
    free(c_tmp);
    xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));

    c_tmp = bg_sprintf("%d", entry->total_tracks);
    node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"TOTAL_TRACKS", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(c_tmp));
    free(c_tmp);
    xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
    }
  
  /* Duration */

  c_tmp = bg_sprintf("%" PRId64, entry->duration);
  node = xmlNewTextChild(xml_entry, (xmlNsPtr)0, (xmlChar*)"DURATION", NULL);
  xmlAddChild(node, BG_XML_NEW_TEXT(c_tmp));
  free(c_tmp);
  xmlAddChild(xml_entry, BG_XML_NEW_TEXT("\n"));
  }

static xmlDocPtr album_2_xml(bg_album_t * a)
  {
  bg_album_entry_t * entry;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_album;
  xmlNodePtr node;
  
  /* First step: Build the xml tree */

  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_album = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)"ALBUM", NULL);
  
  xmlDocSetRootElement(xml_doc, xml_album);

  xmlAddChild(xml_album, BG_XML_NEW_TEXT("\n"));

  /* Save config data */

  if(a->cfg_section)
    {
    node = xmlNewTextChild(xml_album, (xmlNsPtr)0, (xmlChar*)"CFG_SECTION", NULL);
    bg_cfg_section_2_xml(a->cfg_section, node);
    xmlAddChild(xml_album, BG_XML_NEW_TEXT("\n"));
    }
  
  entry = a->entries;

  while(entry)
    {
    save_entry(a, entry, xml_album, 1);
    entry = entry->next;
    }
  return xml_doc;
  }

static xmlDocPtr selected_2_xml(bg_album_t * a, int preserve_current)
  {
  bg_album_entry_t * entry;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_album;
  
  /* First step: Build the xml tree */

  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_album = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)"ALBUM", NULL);
  xmlDocSetRootElement(xml_doc, xml_album);

  xmlAddChild(xml_album, BG_XML_NEW_TEXT("\n"));

  entry = a->entries;

  while(entry)
    {
    if(entry->flags & BG_ALBUM_ENTRY_SELECTED)
      save_entry(a, entry, xml_album, preserve_current);
    entry = entry->next;
    }
  return xml_doc;
  
  }

/* Output routines for writing to memory */

#if 0
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

  if(o->bytes_allocated == o->bytes_written)
    {
    o->bytes_allocated++;
    o->buffer = realloc(o->buffer, o->bytes_allocated);
    }
  o->buffer[o->bytes_written] = '\0';
  return 0;
  }
#endif

char * bg_album_save_to_memory(bg_album_t * a)
  {
  xmlDocPtr  xml_doc;
  bg_xml_output_mem_t ctx;
  xmlOutputBufferPtr b;
  memset(&ctx, 0, sizeof(ctx));
  
  xml_doc = album_2_xml(a);
  
  b = xmlOutputBufferCreateIO (bg_xml_write_callback,
                               bg_xml_close_callback,
                               &ctx,
                               (xmlCharEncodingHandlerPtr)0);
  
  xmlSaveFileTo(b, xml_doc, (const char*)0);
  xmlFreeDoc(xml_doc);
  return ctx.buffer;
  }

char * bg_album_save_selected_to_memory(bg_album_t * a, int preserve_current)
  {
  xmlDocPtr  xml_doc;
  bg_xml_output_mem_t ctx;
  xmlOutputBufferPtr b;
  
  memset(&ctx, 0, sizeof(ctx));
  
  xml_doc = selected_2_xml(a, preserve_current);
  
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

static void set_permissions(const char * filename)
  {
  chmod(filename, S_IRUSR | S_IWUSR);
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
    set_permissions(filename);
    }
  else
    {
    if(!a->xml_file)
      {
      bg_album_set_default_location(a);
      }
    tmp_filename = bg_sprintf("%s/%s", a->com->directory, a->xml_file);
    xmlSaveFile(tmp_filename, xml_doc);
    set_permissions(tmp_filename);
    free(tmp_filename);
    }
  xmlFreeDoc(xml_doc);
  }
