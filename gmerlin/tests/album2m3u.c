/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>

#include <gmerlin/tree.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "album2m3u"

int main(int argc, char ** argv)
  {
  bg_album_entry_t * entries;
  char * album_xml;
  
  if(argc < 3)
    {
    fprintf(stderr, "Usage: %s <album> <outfile>\n", argv[0]);
    return -1;
    }

  album_xml = bg_read_file(argv[1], NULL);
  if(!album_xml)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s could not be opened",
           argv[1]);
    return 0;
    }

  entries = bg_album_entries_new_from_xml(album_xml);
  free(album_xml);
  
  if(!entries)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s contains no entries",
           argv[1]);
    return 0;
    }
  bg_album_entries_save_extm3u(entries, argv[2]);
  bg_album_entries_destroy(entries);
  return 0;
  }
