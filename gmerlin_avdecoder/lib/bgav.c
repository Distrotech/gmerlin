/*****************************************************************
 
  bgav.c
 
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

#include <avdec_private.h>

int bgav_init(bgav_t * ret)
  {
  bgav_demuxer_t * demuxer;
  bgav_redirector_t * redirector;

  /*
   *  If the input already has it's track table,
   *  we can stop here
   */
  
  if(ret->input->tt)
    {
    ret->tt = ret->input->tt;
    bgav_track_table_ref(ret->tt);
    if(ret->tt->num_tracks > 1)
      return 1;
    }

  if(!ret->input->demuxer)
    {
    demuxer = bgav_demuxer_probe(ret->input);
    
    if(demuxer)
      {
      ret->demuxer = bgav_demuxer_create(demuxer,
                                         ret->input);
      bgav_demuxer_start(ret->demuxer, &(ret->redirector));
      }
    else
      {
      redirector = bgav_redirector_probe(ret->input);
      if(!redirector)
        goto fail;
      ret->redirector = calloc(1, sizeof(*(ret->redirector)));
      ret->redirector->input = ret->input;
      
      if(!redirector->parse(ret->redirector))
        goto fail;
      return 1;
      }
    if(!ret->demuxer)
      goto fail;

    ret->tt = ret->demuxer->tt;
    bgav_track_table_ref(ret->tt);
    return 1;
    }
  else
    {
    ret->demuxer = ret->input->demuxer;
    ret->tt = ret->demuxer->tt;
    bgav_track_table_ref(ret->tt);
    return 1;
    }
  
  fail:
  
  if(ret->demuxer)
    bgav_demuxer_destroy(ret->demuxer);
  if(ret->redirector)
    bgav_redirector_destroy(ret->redirector);
  return 0;
  }

int bgav_num_tracks(bgav_t * b)
  {
  return b->tt->num_tracks;
  }

bgav_t * bgav_create()
  {
  bgav_t * ret;
  bgav_codecs_init();
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

int bgav_open(bgav_t * ret, const char * location)
  {
  ret->input = bgav_input_open(location, ret->connect_timeout);
  if(!ret->input)
    {
    return 0;
    }
  if(!bgav_init(ret))
    goto fail;
  return 1;
  fail:
  return 0;
  }

int bgav_open_fd(bgav_t * ret, int fd, int64_t total_size, const char * mimetype)
  {
  ret->input = bgav_input_open_fd(fd, total_size, mimetype);
  if(!bgav_init(ret))
    return 0;
  return 1;
  }

void bgav_close(bgav_t * b)
  {
  if(b->is_running)
    bgav_stop(b);
  if(b->demuxer)
    bgav_demuxer_destroy(b->demuxer);
  if(b->redirector)
    bgav_redirector_destroy(b->redirector);

  if(b->input)
    bgav_input_close(b->input);
  bgav_track_table_unref(b->tt);
  free(b);
  }


void bgav_dump(bgav_t * bgav)
  {
  bgav_track_dump(bgav, bgav->tt->current_track);
  
  }

gavl_time_t bgav_get_duration(bgav_t * bgav, int track)
  {
  return bgav->tt->tracks[track].duration;
  }

const char * bgav_get_track_name(bgav_t * b, int track)
  {
  return b->tt->tracks[track].name;
  }

void bgav_stop(bgav_t * b)
  {
  bgav_track_stop(b->tt->current_track);
  b->is_running = 0;
  }

void bgav_select_track(bgav_t * b, int track)
  {
  if(b->is_running)
    bgav_stop(b);
  
  if(b->input->input->select_track)
    {
    /* Input switches track, recreate the demuxer */

    bgav_demuxer_stop(b->demuxer);
    
    bgav_track_table_select_track(b->tt, track);
    b->input->input->select_track(b->input, track);
    
    bgav_demuxer_start(b->demuxer, &(b->redirector));
    }
  else if(b->demuxer->demuxer->select_track)
    {
    /* Demuxer switches track */
    bgav_track_table_select_track(b->tt, track);
    b->demuxer->demuxer->select_track(b->demuxer, track);
    }
  }

void bgav_start(bgav_t * b)
  {
  b->is_running = 1;
  /* Create buffers */
  bgav_track_start(b->tt->current_track, b->demuxer);
  }

int bgav_can_seek(bgav_t * b)
  {
  return b->demuxer->can_seek;
  }

const char * bgav_get_author(bgav_t*b, int track)
  {
  return b->tt->tracks[track].metadata.author;
  }

const char * bgav_get_title(bgav_t*b, int track)
  {
  return b->tt->tracks[track].metadata.title;
  }

const char * bgav_get_copyright(bgav_t*b, int track)
  {
  return b->tt->tracks[track].metadata.copyright;
  }

const char * bgav_get_comment(bgav_t*b, int track)
  {
  return b->tt->tracks[track].metadata.comment;
  }

char * bgav_get_album(bgav_t*b, int track)
  {
  return b->tt->tracks[track].metadata.album;
  }

char * bgav_get_artist(bgav_t*b, int track)
  {
  return b->tt->tracks[track].metadata.artist;
  }

char * bgav_get_genre(bgav_t*b, int track)
  {
  return b->tt->tracks[track].metadata.genre;
  }

char * bgav_get_date(bgav_t*b, int track)
  {
  return b->tt->tracks[track].metadata.date;
  }

int bgav_get_track(bgav_t*b, int track)
  {
  return b->tt->tracks[track].metadata.track;
  }

const char * bgav_get_description(bgav_t * b)
  {
  return b->demuxer->stream_description;
  }

void bgav_set_connect_timeout(bgav_t *b, int timeout)
  {
  b->connect_timeout = timeout;
  }

void bgav_set_read_timeout(bgav_t *b, int timeout)
  {
  b->read_timeout = timeout;
  }

/*
 *  Set network bandwidth (in bits per second)
 */

void bgav_set_network_bandwidth(bgav_t *b, int bandwidth)
  {
  b->network_bandwidth = bandwidth;
  }
