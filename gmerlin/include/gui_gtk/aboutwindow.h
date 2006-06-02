/*****************************************************************
 
  aboutwindow.h
 
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

typedef struct bg_gtk_about_window_s bg_gtk_about_window_t;

bg_gtk_about_window_t *
bg_gtk_about_window_create(const char * name, const char * version,
                           const char * icon,
                           void (*close_callback)(bg_gtk_about_window_t*,
                                                  void*),
                           void * close_callback_data);
