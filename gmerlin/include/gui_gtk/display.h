/*****************************************************************
 
  display.h
 
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

/* Display for Time */

typedef enum
  {
    BG_GTK_DISPLAY_SIZE_HUGE,   /* 240 x 96, 1/1 */
    BG_GTK_DISPLAY_SIZE_LARGE,  /* 120 x 48, 1/2 */
    BG_GTK_DISPLAY_SIZE_NORMAL, /*  80 x 32  1/3 */
    BG_GTK_DISPLAY_SIZE_SMALL,  /*  40 x 16  1/6 */
  } BG_GTK_DISPLAY_SIZE;

typedef struct bg_gtk_time_display_s bg_gtk_time_display_t;

bg_gtk_time_display_t * bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE size);

GtkWidget * bg_gtk_time_display_get_widget(bg_gtk_time_display_t *);

void bg_gtk_time_display_update(bg_gtk_time_display_t * d,
                                int seconds);

void bg_gtk_time_display_set_colors(bg_gtk_time_display_t * d,
                                    float * foreground,
                                    float * background);

void bg_gtk_time_display_destroy(bg_gtk_time_display_t * d);
