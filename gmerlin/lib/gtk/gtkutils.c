/*****************************************************************
 
  gtkutils.c
 
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

#include <locale.h>

#include <inttypes.h>
#include <gtk/gtk.h>
#include <stdio.h>

#include <gui_gtk/gtkutils.h>

GdkPixbuf * bg_gtk_pixbuf_scale_alpha(GdkPixbuf * src,
                                      int dest_width,
                                      int dest_height,
                                      float * foreground,
                                      float * background)
  {
  int i, j;
  GdkPixbuf * ret;
  int rowstride;
  uint8_t * data;
  uint8_t * ptr;

  int background_i[3];
  int foreground_i[3];

  for(i = 0; i < 3; i++)
    {
    background_i[i] = (int)(255.0 * background[i]);
    foreground_i[i] = (int)(255.0 * foreground[i]);
    }
  ret = gdk_pixbuf_scale_simple(src,
                                dest_width,
                                dest_height,
                                GDK_INTERP_BILINEAR);


  
  rowstride = gdk_pixbuf_get_rowstride(ret);

  data      = gdk_pixbuf_get_pixels(ret);

  for(i = 0; i < dest_height; i++)
    {
    ptr = data;
    for(j = 0; j < dest_width; j++)
      {
      ptr[0] =
        (ptr[3] * foreground_i[0] + (0xFF - ptr[3]) * background_i[0]) >> 8;
      ptr[1] =
        (ptr[3] * foreground_i[1] + (0xFF - ptr[3]) * background_i[1]) >> 8;
      ptr[2] =
        (ptr[3] * foreground_i[2] + (0xFF - ptr[3]) * background_i[2]) >> 8;
      ptr[3] = 0xff;
      ptr += 4;
      }
    data += rowstride;
    }
  
  return ret;
  }

void bg_gtk_init(int * argc, char *** argv)
  {
  //  gtk_disable_setlocale();
  gtk_init(argc, argv);

  /* No, we don't like commas as decimal separators */
  
  setlocale(LC_NUMERIC, "C");
  }
