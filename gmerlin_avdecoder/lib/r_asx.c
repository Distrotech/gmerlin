/*****************************************************************
 
  r_asx.c
 
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
#include <ctype.h>
#include <avdec_private.h>
#include <yml.h>
#include <stdlib.h>

int probe_asx(bgav_input_context_t * input)
  {
  char buf[4];
  if(bgav_input_get_data(input, buf, 4) < 4)
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

int xml_2_asx(bgav_redirector_context_t * r, bgav_yml_node_t * n)
  {
  int index;
  char * title;
  bgav_yml_node_t * node;
  bgav_yml_node_t * child_node;

  /* Count the entries and get the global title */
  
  node = n->children;
  r->num_urls = 0;
  
  //  fprintf(stderr, "Node: %s\n", node->name);
  
  if(sc(n->name, "ASX"))
    return 0;
  
  node = n->children;

  title = (char*)0;
  while(node)
    {
    //    fprintf(stderr, "Node: %s\n", node->name);
    
    if(!sc(node->name, "Title"))
      {
      title = bgav_strndup(node->children->str, NULL);
      }
    else if(!sc(node->name, "Entry"))
      {
      r->num_urls++;
      }
    node = node->next;
    }
  fprintf(stderr, "Num entries: %d\n", r->num_urls);
  r->urls = calloc(r->num_urls, sizeof(*(r->urls)));

  
  /* Now, loop through all streams and collect the values */

  node = n->children;
  index = 0;
  
  while(node)
    {
    if(!sc(node->name, "Entry"))
      {
      child_node = node->children;
      while(child_node)
        {
        if(!sc(child_node->name, "Title"))
          {
          
          if(title)
            r->urls[index].name = bgav_sprintf("%s (%s)", title,
                                               child_node->children->str);
          else
            r->urls[index].name = bgav_sprintf("%s",
                                                child_node->children->str);
          }
        else if(!sc(child_node->name, "Ref"))
          {
          if(!r->urls[index].url)
            r->urls[index].url =
              bgav_strndup(bgav_yml_get_attribute_i(child_node, "href"), NULL);
          }
        else
          {
          fprintf(stderr, "Child name: %s\n", child_node->name);
          }
        child_node = child_node->next;
        }
      //      fprintf(stderr, "Name: %s\nLocation: %s\n", r->names[index],
      //              r->locations[index]);
      if(!r->urls[index].name)
        r->urls[index].name = bgav_sprintf("Stream %d (%s)", index+1,
                                           r->urls[index].url);
      index++;
      }
    node = node->next;
    }
  if(title)
    free(title);
  return 1;
  }


int parse_asx(bgav_redirector_context_t * r)
  {
  int result;

  bgav_yml_node_t * node;

  node = bgav_yml_parse(r->input);
    
  result = xml_2_asx(r, node);

  bgav_yml_free(node);
  return result;
  }

bgav_redirector_t bgav_redirector_asx = 
  {
    probe: probe_asx,
    parse: parse_asx
  };
