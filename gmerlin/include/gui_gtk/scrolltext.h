/*****************************************************************
 
  scrolltext.h
 
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

typedef struct bg_gtk_scrolltext_s bg_gtk_scrolltext_t;

bg_gtk_scrolltext_t * bg_gtk_scrolltext_create(int width, int height);
GtkWidget * bg_gtk_scrolltext_get_widget(bg_gtk_scrolltext_t *);

void bg_gtk_scrolltext_set_text(bg_gtk_scrolltext_t *, const char * text,
                                float * fg_color, float * bg_color);

void bg_gtk_scrolltext_set_colors(bg_gtk_scrolltext_t *,
                                  float * fg_color, float * bg_color);


void bg_gtk_scrolltext_set_font(bg_gtk_scrolltext_t *, const char * font);

void bg_gtk_scrolltext_destroy(bg_gtk_scrolltext_t *);

