/*****************************************************************
 
  cdaudio_xml.c
 
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

#include "cdaudio.h"

int bg_cdaudio_load(bg_track_info_t * tracks, const char * filename)
  {
  int index;
  xmlDocPtr xml_doc;
  xmlNodePtr node;

  xml_doc = xmlParseFile(filename);
  if(!xml_doc)
    return 0;

  node = xml_doc->children;

  if(strcmp(node->name, "CD"))
    {
    fprintf(stderr, "File %s contains no CD metadata\n", filename);
    xmlFreeDoc(xml_doc);
    return 0;
    }

  node = node->children;

  index = 0;

  while(node)
    {
    if(node->name && !strcmp(node->name, "TRACK"))
      {
      bg_xml_2_metadata(xml_doc, node,
                        &(tracks[index].metadata));
      index++;
      }
    node = node->next;
    }

  
  return 1;
  }

void bg_cdaudio_save(bg_track_info_t * tracks, int num_tracks,
                     const char * filename)
  {
  xmlDocPtr  xml_doc;
  int i;

  xmlNodePtr xml_cd, child;
    
  xml_doc = xmlNewDoc("1.0");
  xml_cd = xmlNewDocRawNode(xml_doc, NULL, "CD", NULL);
  xmlDocSetRootElement(xml_doc, xml_cd);
  xmlAddChild(xml_cd, xmlNewText("\n"));

  for(i = 0; i < num_tracks; i++)
    {
    child = xmlNewTextChild(xml_cd, (xmlNsPtr)0, "TRACK", NULL);
    xmlAddChild(child, xmlNewText("\n"));
    bg_metadata_2_xml(child,
                      &(tracks[i].metadata));
    xmlAddChild(xml_cd, xmlNewText("\n"));
    }

  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);
  }
