/*****************************************************************
 
  plugin.h
 
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

/* Gtk widgets for plugins */

typedef struct bg_gtk_plugin_info_s bg_gtk_plugin_info_t;

void bg_gtk_plugin_info_show(const bg_plugin_info_t * info);

/* Browser and configurator for multiple plugins */

typedef struct bg_gtk_plugin_widget_multi_s bg_gtk_plugin_widget_multi_t;

bg_gtk_plugin_widget_multi_t *
bg_gtk_plugin_widget_multi_create(bg_plugin_registry_t * reg,
                                  uint32_t type_mask,
                                  uint32_t flag_mask);

void bg_gtk_plugin_widget_multi_destroy(bg_gtk_plugin_widget_multi_t * w);

GtkWidget *
bg_gtk_plugin_widget_multi_get_widget(bg_gtk_plugin_widget_multi_t * w);

/* Selector and configurator for a single plugin */

typedef struct bg_gtk_plugin_widget_single_s bg_gtk_plugin_widget_single_t;

bg_gtk_plugin_widget_single_t *
bg_gtk_plugin_widget_single_create(bg_plugin_registry_t * reg,
                                   uint32_t type_mask,
                                   uint32_t flag_mask,
                                   void (*set_plugin)(bg_plugin_handle_t *,
                                                      void*),
                                   void * set_plugin_data);

void bg_gtk_plugin_widget_single_destroy(bg_gtk_plugin_widget_single_t * w);

GtkWidget *
bg_gtk_plugin_widget_single_get_widget(bg_gtk_plugin_widget_single_t * w);

const bg_plugin_info_t *
bg_gtk_plugin_widget_single_get_plugin(bg_gtk_plugin_widget_single_t * w);

