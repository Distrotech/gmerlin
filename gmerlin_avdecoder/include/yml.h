/*****************************************************************
 
  yml.h
 
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

/* VERY simple and nonstandard xml-like parser */

/*
 *  We need this, because asx files are, according to
 *  Microsoft Documentation, xml-based, but the same doc says,
 *  that tag- and attribute names are case insensitive,
 *  which is explicitely forbidden by the xml-spec.
 *  Since we can't tell libxml to be case insensitive,
 *  we need our own parser.
 *
 *  We prefix everything with yml, so we never forget,
 *  that we don't deal with standard xml here.
 */

typedef struct bgav_yml_attr_s
  {
  struct bgav_yml_attr_s * next;
  char * name;
  char * value;
  } bgav_yml_attr_t;

typedef struct bgav_yml_node_s
  {
  char * name;   /* NULL if we have a pure text node */
  char * str;    /* Text for text nodes              */
  bgav_yml_attr_t * attributes;
  struct bgav_yml_node_s * next;
  struct bgav_yml_node_s * children;
  } bgav_yml_node_t;

bgav_yml_node_t * bgav_yml_parse(bgav_input_context_t * input);

void bgav_yml_dump(bgav_yml_node_t *);
void bgav_yml_free(bgav_yml_node_t *);

const char * bgav_yml_get_attribute(bgav_yml_node_t *, const char * name);
const char * bgav_yml_get_attribute_i(bgav_yml_node_t *, const char * name);
