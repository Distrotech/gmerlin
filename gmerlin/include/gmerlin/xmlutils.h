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

/* xml utilities */

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <gavl/gavf.h>


/*  Macro, which calls strcmp, but casts the first argument to char*
 *  This is needed because libxml strings are uint8_t*
 */

#define BG_XML_STRCMP(a, b) strcmp((char*)a, b)
#define BG_XML_GET_PROP(a, b) (char*)xmlGetProp(a, (xmlChar*)b)
#define BG_XML_SET_PROP(a, b, c) xmlSetProp(a, (xmlChar*)b, (xmlChar*)c)
#define BG_XML_NEW_TEXT(a) xmlNewText((xmlChar*)a)

/* 
 * memory writer for libxml
 */
#if 0
typedef struct bg_xml_output_mem_s
  {
  int bytes_written;
  int bytes_allocated;
  char * buffer;
  } bg_xml_output_mem_t;

int bg_xml_write_callback(void * context, const char * buffer,
                          int len);

int bg_xml_close_callback(void * context);
#endif

char * bg_xml_save_to_memory(xmlDocPtr doc);

xmlDocPtr bg_xml_load_FILE(FILE * f);
void bg_xml_save_FILE(xmlDocPtr doc, FILE * f);

xmlDocPtr bg_xml_load_gavf_io(gavf_io_t* io);
void bg_xml_save_io(xmlDocPtr doc, gavf_io_t* io);

xmlDocPtr bg_xml_parse_file(const char * filename, int lock);
void bg_xml_save_file(xmlDocPtr doc, const char * filename, int lock);

xmlNodePtr bg_xml_find_node_child(xmlNodePtr parent, const char * child);
xmlNodePtr bg_xml_find_doc_child(xmlDocPtr parent, const char * child);

xmlNodePtr bg_xml_find_next_node_child(xmlNodePtr parent, xmlNodePtr child);
xmlNodePtr bg_xml_find_next_doc_child(xmlDocPtr parent, xmlNodePtr child);

xmlNodePtr bg_xml_append_child_node(xmlNodePtr parent, const char * name,
                                    const char * content);

/* Get the contents of node like <node>content</node>.
   Returns an empty string if the node has no children or
   NULL
 */

const char * bg_xml_node_get_text_content(xmlNodePtr parent);
