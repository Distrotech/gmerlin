/*****************************************************************
 
  singlepic.h
 
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

/*
 * Singlepicture "metaplugins" are for the application like
 * all other plugin. The internal difference is, that they
 * are created based on the available image writers.

 * They just translate the Video stream API into the one for
 * single pictures
 */

/*
 *  Create info structures.
 *  Called by the registry after all external plugins
 *  are detected
 */

bg_plugin_info_t * bg_singlepic_input_info(bg_plugin_registry_t * reg);
bg_plugin_info_t * bg_singlepic_stills_input_info(bg_plugin_registry_t * reg);
bg_plugin_info_t * bg_singlepic_encoder_info(bg_plugin_registry_t * reg);

/*
 *  Get the static plugin infos
 */

bg_plugin_common_t * bg_singlepic_input_get();
bg_plugin_common_t * bg_singlepic_stills_input_get();
bg_plugin_common_t * bg_singlepic_encoder_get();

/*
 *  Create the plugins (These are a replacement for the create() methods
 */

void * bg_singlepic_input_create(bg_plugin_registry_t * reg);
void * bg_singlepic_stills_input_create(bg_plugin_registry_t * reg);
void * bg_singlepic_encoder_create(bg_plugin_registry_t * reg);

#define bg_singlepic_encoder_name        "e_singlepic"
#define bg_singlepic_input_name        "i_singlepic"
#define bg_singlepic_stills_input_name "i_singlepic_stills"
