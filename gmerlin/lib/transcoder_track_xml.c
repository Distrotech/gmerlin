/*****************************************************************
  
  transcoder_track_xml.c
  
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

#include <pluginregistry.h>
#include <tree.h>
#include <transcoder_track.h>
#include <utils.h>



static void track_2_xml(bg_transcoder_track_t * track,
                        xmlNodePtr xml_track)
  {
  
  }

void bg_transcoder_tracks_save(bg_transcoder_track_t * t, const char * filename)
  {
  bg_transcoder_track_t * tmp;

  xmlDocPtr  xml_doc;
  xmlNodePtr node;

  xml_doc = xmlNewDoc("1.0");
  node = xmlNewDocRawNode(xml_doc, NULL, "TRANSCODER_TRACKS", NULL);
  xmlDocSetRootElement(xml_doc, node);

  xmlAddChild(node, xmlNewText("\n"));

  
  }

static void xml_2_track(bg_transcoder_track_t * track,
                        xmlDocPtr xml_doc, xmlNodePtr xml_track)
  {
  
  }

bg_transcoder_track_t * bg_transcoder_tracks_load(const char * filename)
  {
  xmlDocPtr xml_doc;
  xmlNodePtr node;

  bg_transcoder_track_t * ret = (bg_transcoder_track_t *)0;
  bg_transcoder_track_t * end;
    
  if(!filename)
    return (bg_transcoder_track_t*)0;
  
  xml_doc = xmlParseFile(filename);
                                                                                
  if(!xml_doc)
    return (bg_transcoder_track_t*)0;

  node = xml_doc->children;

  if(strcmp(node->name, "TRANSCODER_TRACKS"))
    {
    fprintf(stderr, "File %s contains no transcoder tracks\n", filename);
    xmlFreeDoc(xml_doc);
    return (bg_transcoder_track_t*)0;
    }

  node = node->children;
  
  while(node)
    {
    if(node->name && !strcmp(node->name, "TRACK"))
      {
      /* Load track */

      if(!ret)
        {
        ret = calloc(1, sizeof(ret));
        end = ret;
        }
      else
        {
        end->next = calloc(1, sizeof(*(end->next)));
        end = end->next;
        }
      xml_2_track(end, xml_doc, node);
      }
    node = node->next;
    }
  
  }

