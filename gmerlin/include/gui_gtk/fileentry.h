/*****************************************************************
 
  fileentry.h
 
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

typedef struct bg_gtk_file_entry_s bg_gtk_file_entry_t;

bg_gtk_file_entry_t * bg_gtk_file_entry_create(int dir,
                                               void (*name_changed_callback)(bg_gtk_file_entry_t *,
                                                                    void * data),
                                               void * name_changed_callback_data);

void bg_gtk_file_entry_destroy(bg_gtk_file_entry_t *);

const char * bg_gtk_file_entry_get_filename(bg_gtk_file_entry_t *);
void bg_gtk_file_entry_set_filename(bg_gtk_file_entry_t *, const char *);

GtkWidget * bg_gtk_file_entry_get_entry(bg_gtk_file_entry_t *);
GtkWidget * bg_gtk_file_entry_get_button(bg_gtk_file_entry_t *);

