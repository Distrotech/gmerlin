/*****************************************************************
 
  gftkutils.h
 
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

GdkPixbuf * bg_gtk_pixbuf_scale_alpha(GdkPixbuf * src,
                                      int dest_width,
                                      int dest_height,
                                      float * foreground,
                                      float * background);

void bg_gtk_init(int * argc, char *** argv);

void bg_gdk_pixbuf_render_pixmap_and_mask(GdkPixbuf *pixbuf,
                                          GdkPixmap **pixmap_return,
                                          GdkBitmap **mask_return);
