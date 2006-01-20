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

#include <string.h>
#include <ctype.h>
#include <avdec_private.h>
#include <stdio.h>
#include <yml.h>
#include <stdlib.h>

static int probe_asx(bgav_input_context_t * input)
  {
  char * pos;
  char buf[4];

  /* We accept all files, which end with .asx */

  if(input->filename)
    {
    pos = strrchr(input->filename, '.');
    if(pos)
      {
      pos++;
      if(!strcasecmp(pos, "asx"))
        return 1;
      }
    }
  if(bgav_input_get_data(input, (uint8_t*)buf, 4) < 4)
    return 0;
  if((buf[0] == '<') &&
     (tolower(buf[1]) == 'a') && 
     (tolower(buf[2]) == 's') && 
     (tolower(buf[3]) == 'x'))
    return 1;
  return 0;
  }

static int sc(const char * str1, const char * str2)
  {
  if((!str1) || (!str2))
    return -1;
  return strcasecmp(str1, str2);
  }

static int count_urls(bgav_yml_node_t * n)
  {
  int ret = 0;

  while(n)
    {
    if(!sc(n->name, "Entry"))
      {
      ret++;
      }
    else if(!sc(n->name, "Repeat"))
      {
      ret += count_urls(n->children);
      }
    n = n->next;
    }
  return ret;
  }

static void get_url(bgav_yml_node_t * n, bgav_url_info_t * ret,
                    const char * title, int * index)
  {
  while(n)
    {
    if(!sc(n->name, "Title") && n->children)
      {
      if(title)
        ret->name = bgav_sprintf("%s (%s)", title, n->children->str);
      else
        ret->name = bgav_sprintf("%s", n->children->str);
      }
    else if(!sc(n->name, "Ref"))
      {
      if(!ret->url)
        ret->url =
          bgav_strndup(bgav_yml_get_attribute_i(n, "href"),
                       NULL);
      }
    n = n->next;
    }

  if(!ret->name)
    ret->name = bgav_sprintf("Stream %d (%s)", (*index)+1, ret->url);
  (*index)++;
  }

static int get_urls(bgav_yml_node_t * n,
                    bgav_redirector_context_t * r, const char * title, int * index)
  {
  while(n)
    {
    if(!sc(n->name, "Entry"))
      {
      get_url(n->children, &(r->urls[*index]), title, index);
      }
    else if(!sc(n->name, "Repeat"))
      {
      get_urls(n->children, r, title, index);
      }
    n = n->next;
    }
  return 1;
  }

static int xml_2_asx(bgav_redirector_context_t * r, bgav_yml_node_t * n)
  {
  int index;
  char * title;
  bgav_yml_node_t * node;

  /* Count the entries and get the global title */
  
  node = n->children;
  r->num_urls = 0;
  
  //  fprintf(stderr, "Node: %s\n", node->name);
  
  if(sc(n->name, "ASX"))
    return 0;
  
  node = n->children;

  title = (char*)0;

  /* Try to fetch a title */
    
  while(node)
    {
    if(!sc(node->name, "Title") && node->children)
      {
      title = bgav_strndup(node->children->str, NULL);
      }
    node = node->next;
    }

  /* Count the entries */

  r->num_urls = count_urls(n->children);
  
  //  fprintf(stderr, "Num entries: %d\n", r->num_urls);
  r->urls = calloc(r->num_urls, sizeof(*(r->urls)));
  
  /* Now, loop through all streams and collect the values */
  
  index = 0;

  get_urls(n->children, r, title, &index);
  
  if(title)
    free(title);
  return 1;
  }


static int parse_asx(bgav_redirector_context_t * r)
  {
  int result;
  bgav_yml_node_t * node;
  
  //  fprintf(stderr, "parse_asx\n");
  //  bgav_input_get_dump(r->input, 32);
  
  node = bgav_yml_parse(r->input);

  //  fprintf(stderr,"Node: %p\n", node);

  if(!node)
    {
    fprintf(stderr, "Parse asx failed (yml error)\n");
    return 0;
    }
  result = xml_2_asx(r, node);

  bgav_yml_free(node);

  if(!result)
    fprintf(stderr, "Parse asx failed\n");

  return result;
  }

bgav_redirector_t bgav_redirector_asx = 
  {
    name:  "ASX (Windows Media)",
    probe: probe_asx,
    parse: parse_asx
  };
