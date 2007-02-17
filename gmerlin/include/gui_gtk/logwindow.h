/*****************************************************************
 
  logwindow.h
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

typedef struct bg_gtk_log_window_s bg_gtk_log_window_t;

bg_gtk_log_window_t * bg_gtk_log_window_create(void (*close_callback)(bg_gtk_log_window_t*, void*),
                                               void * data);

void bg_gtk_log_window_destroy(bg_gtk_log_window_t *);

void
bg_gtk_log_window_show(bg_gtk_log_window_t *);

void bg_gtk_log_window_hide(bg_gtk_log_window_t *);

bg_parameter_info_t * bg_gtk_log_window_get_parameters(bg_gtk_log_window_t *);

void bg_gtk_log_window_set_parameter(void * data, char * name,
                                     bg_parameter_value_t * v);

int bg_gtk_log_window_get_parameter(void * data, char * name,
                                    bg_parameter_value_t * val);

void bg_gtk_log_window_flush(bg_gtk_log_window_t *);

const char * bg_gtk_log_window_last_error(bg_gtk_log_window_t *);
