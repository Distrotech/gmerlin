/*****************************************************************
 
  albumentry.c
 
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

#include <tree.h>
#include <utils.h>
#include <gui_gtk/albumentry.h>
#include <gui_gtk/textview.h>

/* This pops up a which shows all informations about a selected track */

#define S(s) (s?s:"(NULL)")

void bg_gtk_album_enrty_show(const bg_album_entry_t * entry)
  {
  char * text, * utf8_location;
  bg_gtk_textwindow_t * win;
  char duration[GAVL_TIME_STRING_LEN];

  gavl_time_prettyprint(entry->duration, duration);

  if(entry->location)
    utf8_location = bg_system_to_utf8(entry->location, -1);
  else
    utf8_location = (char*)0;
  
  text = bg_sprintf("Name:\t %s\nLocation:\t %s\nTrack:\t %d/%d\nPlugin:\t %s\nDuration:\t %s\nAudio Streams:\t %d\nVideo Streams:\t %d\nSubtitle Streams:\t %d",
                    S(entry->name), S(utf8_location),
                    entry->index+1,
                    entry->total_tracks,
                    (entry->plugin ? entry->plugin : "Auto detect"),
                    duration,
                    entry->num_audio_streams,
                    entry->num_video_streams,
                    entry->num_subtitle_streams
                    );

  win = bg_gtk_textwindow_create(text, entry->name);
  free(text);

  bg_gtk_textwindow_show(win, 0);
  if(utf8_location)
    free(utf8_location);
  }


