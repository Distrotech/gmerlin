/*****************************************************************
 
  urllink.h
 
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

typedef struct bg_gtk_urllink_s bg_gtk_urllink_t;

bg_gtk_urllink_t * bg_gtk_urllink_create(const char * text, const char * url);

GtkWidget * bg_gtk_urllink_get_widget(bg_gtk_urllink_t *);
void bg_gtk_urllink_destroy(bg_gtk_urllink_t*);
