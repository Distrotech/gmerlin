/*****************************************************************
 
  r_asx.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>


#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

static char * plugin_name = "i_avdec";

typedef struct
  {
  int num_streams;
  char ** names;
  char ** locations;
  } asx_t;

static void free_info(asx_t * asx)
  {
  int i;
  for(i = 0; i < asx->num_streams; i++)
    {
    if(asx->names && asx->names[i])
      free(asx->names[i]);
    if(asx->locations && asx->locations[i])
      free(asx->locations[i]);
    }
  if(asx->locations)
    free(asx->locations);
  if(asx->names)
    free(asx->names);
  }

static int parse_asx(void * priv, const char * filename)
  {
  int index;
  char * title;
  char * tmp_string;
  xmlDocPtr xml_doc;
  xmlNodePtr node;
  xmlNodePtr child_node;
  asx_t * p = (asx_t*)priv;

  /* Parse the xml file */

  xml_doc = xmlParseFile(filename);

  if(!xml_doc)
    return 0;

  /* Count the entries and get the global title */
  
  node = xml_doc->children;

  p->num_streams = 0;
  
  
  if(xmlStrcasecmp(node->name, "ASX"))
    {
    xmlFreeDoc(xml_doc);
    return 0;
    }

  node = node->children;

  title = (char*)0;
  while(node)
    {

    if(!xmlStrcasecmp(node->name, "Title"))
      {
      title = xmlNodeListGetString(xml_doc, node->children, 1);
      }
    else if(!xmlStrcasecmp(node->name, "Entry"))
      {
      p->num_streams++;
      }
    node = node->next;
    }

  p->names = calloc(p->num_streams, sizeof(char*));
  p->locations = calloc(p->num_streams, sizeof(char*));
    
  /* Now, loop through all streams and collect the values */

  node = xml_doc->children->children;
  index = 0;

  while(node)
    {
    if(!xmlStrcasecmp(node->name, "Entry"))
      {
      child_node = node->children;
      while(child_node)
        {
        if(!xmlStrcasecmp(child_node->name, "Title"))
          {
          tmp_string = xmlNodeListGetString(xml_doc, child_node->children, 1);

          if(title)
            p->names[index] = bg_sprintf("%s (%s)", title, tmp_string);
          else
            p->names[index] = bg_sprintf("%s", tmp_string);
          xmlFree(tmp_string);
          }
        else if(!xmlStrcasecmp(child_node->name, "Ref"))
          {
          tmp_string = xmlGetProp(child_node, "href");
          if(!p->locations[index])
            p->locations[index] = bg_strdup(p->locations[index],
                                            tmp_string);
          xmlFree(tmp_string);
          }
        child_node = child_node->next;
        }
      if(!p->names[index])
        p->names[index] = bg_sprintf("Stream %d (%s)",
                                     index+1, p->locations[index]);
      index++;
      }
    node = node->next;
    }
  if(title)
    free(title);
  xmlFreeDoc(xml_doc);
  return 1;
  }

static int get_num_streams_asx(void * priv)
  {
  asx_t * p;
  p = (asx_t *)priv;
  return p->num_streams;
  }
                                                                                                         
static const char * get_stream_name_asx(void * priv, int stream)
  {
  asx_t * p;
  p = (asx_t *)priv;
  return p->names[stream];
  }
                                                                                                         
static const char * get_stream_url_asx(void * priv, int stream)
  {
  asx_t * p;
  p = (asx_t *)priv;
  return p->locations[stream];
  }

static const char * get_stream_plugin_asx(void * priv, int stream)
  {
  return plugin_name;
  }
                                                                                                         
static void * create_asx()
  {
  asx_t * p;
  p = calloc(1, sizeof(*p));
  return p;
  }
                                                                                                         
static void destroy_asx(void * data)
  {
  asx_t * p = (asx_t *)data;
                                                                                                         
  free_info(p);
  free(p);
  }

bg_redirector_plugin_t the_plugin =
  {
    common:
    {
      name:          "r_asx",
      long_name:     "ASX Redirector",
      mimetypes:     "video/x-ms-asf",
      extensions:    "asx",
      type:          BG_PLUGIN_REDIRECTOR,
      flags:         BG_PLUGIN_FILE|BG_PLUGIN_URL,
      create:        create_asx,
      destroy:       destroy_asx,
                                                                                                         
      get_parameters: NULL,
      set_parameter:  NULL,
    },
    parse:      parse_asx,
                                                                                                         
    get_num_streams:   get_num_streams_asx,
    get_stream_name:   get_stream_name_asx,
    get_stream_url:    get_stream_url_asx,
    get_stream_plugin: get_stream_plugin_asx
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
API_VERSION;
