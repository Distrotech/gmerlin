/*****************************************************************
 
  codecinfo.c
 
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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <parameter.h>
#include <utils.h>
#include <gui_gtk/multiinfo.h>
#include <gui_gtk/textview.h>

void bg_gtk_multi_info_show(const bg_parameter_info_t * info, int i)
  {
  char * text;
  bg_gtk_textwindow_t * win;
  
  text = bg_sprintf("Name:\t %s\nLabel:\t %s\nDescription:\t %s",
                    info->multi_names[i],
                    (info->multi_labels?info->multi_labels[i]:info->multi_names[i]),
                    (info->multi_descriptions?info->multi_descriptions[i]:"None"));

  win = bg_gtk_textwindow_create(text, info->long_name);
  free(text);

  bg_gtk_textwindow_show(win, 0);
  }


