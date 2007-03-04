/*****************************************************************
 
  pluginreg_private.h
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/** \ingroup plugin_registry
 *  \brief Load plugin infos from an xml file
 *  \param filename The name of the xml file
 */

bg_plugin_info_t * bg_plugin_registry_load(const char * filename);

/** \ingroup plugin_registry
 *  \brief
 */

void bg_plugin_registry_save(bg_plugin_info_t * info);


void bg_plugin_info_destroy(bg_plugin_info_t * info);
