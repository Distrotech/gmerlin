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
bg_gtk_plugin_widget_single_create(char * label,
                                   bg_plugin_registry_t * reg,
                                   uint32_t type_mask,
                                   uint32_t flag_mask);

void
bg_gtk_plugin_widget_single_set_change_callback(bg_gtk_plugin_widget_single_t * w,
                                                void (*set_plugin)(const bg_plugin_info_t * plugin,
                                                                   void * data),
                                                void * set_plugin_data);


void
bg_gtk_plugin_widget_single_set_parameter_callback(bg_gtk_plugin_widget_single_t * w,
                                                   bg_set_parameter_func_t func,
                                                   void * priv);


void bg_gtk_plugin_widget_single_destroy(bg_gtk_plugin_widget_single_t * w);

// GtkWidget *
// bg_gtk_plugin_widget_single_get_widget(bg_gtk_plugin_widget_single_t * w);

void bg_gtk_plugin_widget_single_attach(bg_gtk_plugin_widget_single_t * w,
                                        GtkWidget * table,
                                        int * row, int * num_columns);

void bg_gtk_plugin_widget_single_set_sensitive(bg_gtk_plugin_widget_single_t * w,
                                               int sensitive);

const bg_plugin_info_t *
bg_gtk_plugin_widget_single_get_plugin(bg_gtk_plugin_widget_single_t * w);

bg_plugin_handle_t *
bg_gtk_plugin_widget_single_load_plugin(bg_gtk_plugin_widget_single_t * w);

void bg_gtk_plugin_widget_single_set_plugin(bg_gtk_plugin_widget_single_t * w,
                                            const bg_plugin_info_t *);


bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_section(bg_gtk_plugin_widget_single_t * w);

bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_audio_section(bg_gtk_plugin_widget_single_t * w);

bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_video_section(bg_gtk_plugin_widget_single_t * w);

bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_subtitle_text_section(bg_gtk_plugin_widget_single_t * w);

bg_cfg_section_t *
bg_gtk_plugin_widget_single_get_subtitle_overlay_section(bg_gtk_plugin_widget_single_t * w);


void
bg_gtk_plugin_widget_single_set_section(bg_gtk_plugin_widget_single_t * w,
                                        bg_cfg_section_t * s);

void
bg_gtk_plugin_widget_single_set_audio_section(bg_gtk_plugin_widget_single_t * w,
                                              bg_cfg_section_t * s);

void
bg_gtk_plugin_widget_single_set_video_section(bg_gtk_plugin_widget_single_t * w,
                                              bg_cfg_section_t * s);

void
bg_gtk_plugin_widget_single_set_subtitle_text_section(bg_gtk_plugin_widget_single_t * w,
                                                      bg_cfg_section_t * s);

void
bg_gtk_plugin_widget_single_set_subtitle_overlay_section(bg_gtk_plugin_widget_single_t * w,
                                                         bg_cfg_section_t * s);




/* Menu for plugins, will be used for file selectors to select the plugin */

typedef struct bg_gtk_plugin_menu_s bg_gtk_plugin_menu_t;

bg_gtk_plugin_menu_t *
bg_gtk_plugin_menu_create(int auto_supported,
                          bg_plugin_registry_t * plugin_reg,
                          int type_mask, int flag_mask);

const char * bg_gtk_plugin_menu_get_plugin(bg_gtk_plugin_menu_t *);
GtkWidget * bg_gtk_plugin_menu_get_widget(bg_gtk_plugin_menu_t *);
void bg_gtk_plugin_menu_destroy(bg_gtk_plugin_menu_t *);

void bg_gtk_plugin_menu_set_change_callback(bg_gtk_plugin_menu_t *,
                                            void (*callback)(bg_gtk_plugin_menu_t *, void*),
                                            void * callback_data);

void bg_gtk_plugin_menu_attach(bg_gtk_plugin_menu_t * m, GtkWidget * table,
                               int row,
                               int column);

