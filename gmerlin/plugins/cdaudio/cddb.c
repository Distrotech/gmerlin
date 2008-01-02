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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <utils.h>
#include <cddb/cddb.h>


#include "cdaudio.h"

//#define DUMP


int bg_cdaudio_get_metadata_cddb(bg_cdaudio_index_t * idx,
                                 bg_track_info_t * info,
                                 char * cddb_host,
                                 int cddb_port,
                                 char * cddb_path,
                                 char * cddb_proxy_host,
                                 int cddb_proxy_port,
                                 char * cddb_proxy_user,
                                 char * cddb_proxy_pass,
                                 int cddb_timeout)
  {
  int i, success;
  int num_matches;
  unsigned int disc_id_call, date;
  bg_metadata_t *m;
  
  const char *genre, *album; 
  cddb_disc_t * disc = NULL;
  cddb_conn_t * conn = NULL;
  cddb_track_t * track = NULL;
  
  /* create a new disk */
  disc = cddb_disc_new();
  if (disc == NULL)
    {
    return 0;
    }
  
  /* create new tracks and add to disk */
  for( i = 0; i < idx->num_tracks; i++ )
    {
    /* create a new track */
    track = cddb_track_new();
    if (track == NULL)
      {
      return 0;
      }
    cddb_track_set_frame_offset( track, idx->tracks[i].first_sector+150);
    cddb_disc_add_track(disc, track);
    }

  cddb_disc_set_length(disc, (idx->tracks[idx->num_tracks-1].last_sector+1+150)/75);  
  

  /* create new connection */
  conn = cddb_new();
  if (conn == NULL)
    {
    return 0;
    }

  if(cddb_disc_calc_discid(disc) == 1)
    {
    disc_id_call = cddb_disc_get_discid(disc);
    }

  /* HTTP settings */
  cddb_http_enable(conn);
  cddb_set_server_port(conn, cddb_port);
  cddb_set_server_name(conn, cddb_host);
  cddb_set_http_path_query(conn, cddb_path);
  cddb_set_timeout(conn, cddb_timeout);
  
  if(cddb_proxy_host)
    {
    /* HTTP settings with proxy */
    cddb_http_proxy_enable(conn);                          
    cddb_set_http_proxy_server_name(conn, cddb_proxy_host); 
    cddb_set_http_proxy_server_port(conn, cddb_proxy_port);           
    if(cddb_proxy_user && cddb_proxy_pass)
      cddb_set_http_proxy_credentials(conn, cddb_proxy_user, cddb_proxy_pass);
    }

  /* connect cddb to find similar discs */

  cddb_cache_only(conn);
  num_matches = cddb_query(conn, disc);
  if (num_matches == -1)
    {
    /* something went wrong, print error */
    cddb_error_print(cddb_errno(conn));
    return 0;
    }
  
  if(num_matches == 0)
    {
    cddb_cache_disable(conn);
    num_matches = cddb_query(conn, disc);
    if (num_matches == -1)
      {
      /* something went wrong, print error */
      cddb_error_print(cddb_errno(conn));
      return 0;
      }
    cddb_cache_enable(conn);
    }
  
  /* info for gmerlin from the first match */
  genre = cddb_disc_get_category_str(disc);

  disc_id_call = cddb_disc_get_discid(disc); 
  
  cddb_disc_set_category_str(disc, genre);
  cddb_disc_set_discid(disc, disc_id_call);
  
  success = cddb_read(conn, disc);
  if(!success)
    {
    cddb_error_print(cddb_errno(conn));
    return 0;
    }
  
  album = cddb_disc_get_title(disc);
  date = cddb_disc_get_year(disc);

  for(i = 0; i < idx->num_tracks; i++)
    {
    if(!idx->tracks[i].is_audio)
      continue;

    m = &(info[idx->tracks[i].index].metadata);
    track = cddb_disc_get_track(disc, i);

    m->artist = bg_strdup(m->artist, cddb_track_get_artist(track));
    m->title = bg_strdup(m->title, cddb_track_get_title(track));
    m->genre = bg_strdup(m->genre, genre);
    m->genre[0] = toupper(m->genre[0]);
    m->album = bg_strdup(m->album, album);
    if(date)
      m->date = bg_sprintf("%d", date);
    }
  
  /* clean up all trash */
  cddb_destroy(conn);
  cddb_disc_destroy(disc);
  return 1;
  }

static void __cleanup() __attribute__ ((destructor));

static void __cleanup()
  {
  libcddb_shutdown();
  }
