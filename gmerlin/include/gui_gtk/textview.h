/*****************************************************************
 
  textview.h
 
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

typedef struct bg_gtk_textview_s bg_gtk_textview_t;

bg_gtk_textview_t * bg_gtk_textview_create();
void bg_gtk_textview_destroy(bg_gtk_textview_t *);

void bg_gtk_textview_update(bg_gtk_textview_t *, const char * text);

GtkWidget * bg_gtk_textview_get_widget(bg_gtk_textview_t * t);

typedef struct bg_gtk_textwindow_s bg_gtk_textwindow_t;

bg_gtk_textwindow_t *
bg_gtk_textwindow_create(const char * text, const char * title);
void bg_gtk_textwindow_show(bg_gtk_textwindow_t * w, int modal);
