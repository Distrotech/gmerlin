/*****************************************************************
 
  cddb.c
 
  Copyright (c) 2006 by Michael Gruenert - one78@web.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

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

#ifdef DUMP
  const char *buf;
#endif
  const char *genre, *album; 
  cddb_disc_t * disc = NULL;
  cddb_conn_t * conn = NULL;
  cddb_track_t * track = NULL;
#ifdef DUMP
  cddb_disc_t * possible_match;
  bg_cdaudio_index_dump(idx);
  fprintf(stderr, "  DUMP cddb.c:\n");
  fprintf(stderr, "    num_tracks:            %d\n", idx->num_tracks); 
  fprintf(stderr, "    cddb_host:             %s\n", cddb_host);
  fprintf(stderr, "    cddb_port:             %d\n", cddb_port);
  fprintf(stderr, "    cddb_path:             %s\n", cddb_path);
  fprintf(stderr, "    cddb_proxy_host:       %s\n", cddb_proxy_host);
  fprintf(stderr, "    cddb_proxy_port:       %d\n", cddb_proxy_port);
  //fprintf(stderr, "    cddb_proxy_user:       %s\n", cddb_proxy_user);
  //fprintf(stderr, "    cddb_proxy_pass:       %s\n", cddb_proxy_pass);
#endif
  
  /* create a new disk */
  disc = cddb_disc_new();
  if (disc == NULL)
    {
    fprintf(stderr, "out of memory, unable to create disc");
    return 0;
    }
  
  /* create new tracks and add to disk */
  for( i = 0; i < idx->num_tracks; i++ )
    {
    /* create a new track */
    track = cddb_track_new();
    if (track == NULL)
      {
      fprintf(stderr, "out of memory, unable to create track");
      return 0;
      }
    cddb_track_set_frame_offset( track, idx->tracks[i].first_sector+150);
#ifdef DUMP
    fprintf(stderr, "    frame_offset:          %d\n",idx->tracks[i].first_sector+150);
#endif
    cddb_disc_add_track(disc, track);
    }

  cddb_disc_set_length(disc, (idx->tracks[idx->num_tracks-1].last_sector+1+150)/75);  
  
#ifdef DUMP
  fprintf(stderr, "    last_sector:           %d\n", (idx->tracks[idx->num_tracks-1].last_sector+1)/75);
#endif

  /* create new connection */
  conn = cddb_new();
  if (conn == NULL)
    {
    fprintf(stderr, "out of memory, unable to create connection structure");
    return 0;
    }

  if(cddb_disc_calc_discid(disc) == 1)
    {
    disc_id_call = cddb_disc_get_discid(disc);
#ifdef DUMP
    fprintf(stderr, "    cddb_disc_get_discid:  %x(calc)\n", disc_id_call);
#endif
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
#ifdef DUMP
  buf = cddb_cache_get_dir(conn);
  fprintf(stderr, "    cache:                 %s\n", buf);   
#endif
  num_matches = cddb_query(conn, disc);
  if (num_matches == -1)
    {
    /* something went wrong, print error */
    cddb_error_print(cddb_errno(conn));
    return 0;
    }
#ifdef DUMP
  fprintf(stderr, "    num_matches:           %d(cache)\n", num_matches);
#endif
  
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
#ifdef DUMP
    fprintf(stderr, "    num_matches:           %d(server)\n", num_matches);
#endif
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
  
#ifdef DUMP
  /* infos to stderr for debuging */
  fprintf(stderr, "    num_matches:           %d\n", num_matches);
  while(num_matches > 0)
    {
    possible_match = cddb_disc_clone(disc);
    buf = cddb_disc_get_category_str(possible_match);
    disc_id_call = cddb_disc_get_discid(possible_match);
    fprintf(stderr, "    cddb_disc_get_discid:  %x\n", disc_id_call);
    cddb_disc_set_category_str(possible_match, buf);
    cddb_disc_set_discid(possible_match, disc_id_call);

    success = cddb_read(conn, possible_match);
    if (!success)
      {
      cddb_error_print(cddb_errno(conn));
      return 0;
      }
    
    buf = cddb_disc_get_artist(possible_match);
    fprintf(stderr, "    cddb_disc_get_artist:  %s\n", buf);
    buf = cddb_disc_get_title(possible_match);
    fprintf(stderr, "    cddb_disc_get_title:   %s\n", buf);
    date = cddb_disc_get_year(disc);
    fprintf(stderr, "    cddb_disc_get_year:    %d\n", date);

    i = 1;
    for (track = cddb_disc_get_track_first(possible_match); track != NULL; track = cddb_disc_get_track_next(possible_match))
      {
      title = cddb_track_get_title(track);
      fprintf(stderr, "      cddb_track_get_title %d:   %s\n", i, title);
      i++;
      }
    
    fprintf(stderr, "\n");
    
    num_matches--;
    if (num_matches > 0)
      {
      if (!cddb_query_next(conn, disc))
        {
        fprintf(stderr, "query index out of bounds");
        return 0;
        }
      }
    }
#endif

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
