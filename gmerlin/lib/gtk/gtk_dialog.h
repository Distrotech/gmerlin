/*****************************************************************
 
  gtk_dialog.h
 
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

#include <gtk/gtk.h>
#include <cfg_dialog.h>

#include <stdlib.h>

typedef struct bg_gtk_widget_s bg_gtk_widget_t;

typedef struct
  {
  /* value -> widget */
  void (*get_value)(bg_gtk_widget_t * w);
  /* widget -> value */
  void (*set_value)(bg_gtk_widget_t * w);
  void (*destroy)(bg_gtk_widget_t * w);
  void (*attach)(void * priv, GtkWidget * table, int * row, int * num_columns);
  } gtk_widget_funcs_t;

struct bg_gtk_widget_s
  {
  void * priv;
  gtk_widget_funcs_t * funcs;
  bg_parameter_value_t value;
  const bg_parameter_info_t * info;

  /* For change callbacks */
  
  bg_set_parameter_func change_callback;
  void * change_callback_data;
  gulong callback_id;
  GtkWidget * callback_widget;
  };

void 
bg_gtk_create_checkbutton(bg_gtk_widget_t *,
                          bg_parameter_info_t * info);

void 
bg_gtk_create_int(bg_gtk_widget_t *,
                  bg_parameter_info_t * info);

void 
bg_gtk_create_float(bg_gtk_widget_t *,
                    bg_parameter_info_t * info);

void 
bg_gtk_create_slider_int(bg_gtk_widget_t *,
                         bg_parameter_info_t * info);

void 
bg_gtk_create_slider_float(bg_gtk_widget_t *,
                           bg_parameter_info_t * info);

void 
bg_gtk_create_string(bg_gtk_widget_t *,
                     bg_parameter_info_t * info);

void 
bg_gtk_create_stringlist(bg_gtk_widget_t *,
                         bg_parameter_info_t * info);

void 
bg_gtk_create_color_rgb(bg_gtk_widget_t *,
                        bg_parameter_info_t * info);

void 
bg_gtk_create_color_rgba(bg_gtk_widget_t *,
                         bg_parameter_info_t * info);

void 
bg_gtk_create_font(bg_gtk_widget_t *,
                   bg_parameter_info_t * info);

void 
bg_gtk_create_device(bg_gtk_widget_t *,
                     bg_parameter_info_t * info);


void 
bg_gtk_create_time(bg_gtk_widget_t *,
                   bg_parameter_info_t * info);


void
bg_gtk_create_multi_list(bg_gtk_widget_t *, bg_parameter_info_t * info,
                         bg_cfg_section_t * cfg_section,
                         bg_set_parameter_func set_param,
                         void * data);

void
bg_gtk_create_multi_menu(bg_gtk_widget_t *, bg_parameter_info_t * info,
                         bg_cfg_section_t * cfg_section,
                         bg_set_parameter_func set_param,
                         void * data);


void 
bg_gtk_create_directory(bg_gtk_widget_t *,
                        bg_parameter_info_t * info);

void 
bg_gtk_create_file(bg_gtk_widget_t *,
                   bg_parameter_info_t * info);

void bg_gtk_change_callback(GtkWidget * gw, gpointer data);

void bg_gtk_change_callback_block(bg_gtk_widget_t * w, int block);
