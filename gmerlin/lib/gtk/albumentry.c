/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/


#include <stdlib.h>

#include <gtk/gtk.h>

#include <config.h>
#include <gmerlin/tree.h>
#include <gmerlin/utils.h>
#include <gui_gtk/albumentry.h>
#include <gui_gtk/textview.h>
#include <gui_gtk/gtkutils.h>

/* This pops up a which shows all informations about a selected track */

#define S(s) (s?s:"(NULL)")

void bg_gtk_album_enrty_show(const bg_album_entry_t * entry, GtkWidget * parent)
  {
  char * text, * utf8_location;
  bg_gtk_textwindow_t * win;
  char duration[GAVL_TIME_STRING_LEN];

  gavl_time_prettyprint(entry->duration, duration);

  if(entry->location)
    utf8_location = bg_system_to_utf8(entry->location, -1);
  else
    utf8_location = (char*)0;
  
  text = bg_sprintf(TR("Name:\t %s\nLocation:\t %s\nTrack:\t %d/%d%s\nPlugin:\t %s\nDuration:\t %s\nAudio Streams:\t %d\nVideo Streams:\t %d\nSubtitle Streams:\t %d"),
                    S(entry->name), S(utf8_location),
                    entry->index+1,
                    entry->total_tracks,
                    (entry->flags & BG_ALBUM_ENTRY_EDL ? " (EDL)" : ""),
                    (entry->plugin ? entry->plugin : TR("Auto detect")),
                    duration,
                    entry->num_audio_streams,
                    entry->num_video_streams,
                    entry->num_subtitle_streams
                    );

  win = bg_gtk_textwindow_create(text, entry->name);
  free(text);

  bg_gtk_textwindow_show(win, 0, parent);
  if(utf8_location)
    free(utf8_location);
  }


