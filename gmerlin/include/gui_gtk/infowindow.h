/*****************************************************************
 
  infowindow.h
 
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

#ifndef __BG_GTK_INFOWINDOW_H_
#define __BG_GTK_INFOWINDOW_H_

typedef struct bg_gtk_info_window_s bg_gtk_info_window_t;

bg_gtk_info_window_t *
bg_gtk_info_window_create(bg_player_t * player,
                          void (*close_callback)(bg_gtk_info_window_t*, void*),
                          void * data);

void bg_gtk_info_window_destroy(bg_gtk_info_window_t *);

/* Configuration (doesn't need a dialog) */

bg_parameter_info_t *
bg_gtk_info_window_get_parameters(bg_gtk_info_window_t * win);
  
void bg_gtk_info_window_set_parameter(void * data, const char * name,
                                      const bg_parameter_value_t * val);

int bg_gtk_info_window_get_parameter(void * data, const char * name,
                                     bg_parameter_value_t * val);


/* Show/hide the window */

void bg_gtk_info_window_show(bg_gtk_info_window_t *);
void bg_gtk_info_window_hide(bg_gtk_info_window_t *);

#endif // __BG_GTK_INFOWINDOW_H_
