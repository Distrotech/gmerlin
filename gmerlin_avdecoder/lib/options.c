/*****************************************************************
 
  options.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <string.h>

#include <avdec_private.h>

/* Configuration stuff */

void bgav_options_set_connect_timeout(bgav_options_t *b, int timeout)
  {
  b->connect_timeout = timeout;
  }

void bgav_options_set_read_timeout(bgav_options_t *b, int timeout)
  {
  b->read_timeout = timeout;
  }

/*
 *  Set network bandwidth (in bits per second)
 */

void bgav_options_set_network_bandwidth(bgav_options_t *b, int bandwidth)
  {
  b->network_bandwidth = bandwidth;
  }

void bgav_options_set_network_buffer_size(bgav_options_t *b, int size)
  {
  b->network_buffer_size = size;
  }


void bgav_options_set_http_use_proxy(bgav_options_t*b, int use_proxy)
  {
  b->http_use_proxy = use_proxy;
  }

void bgav_options_set_http_proxy_host(bgav_options_t*b, const char * h)
  {
  if(b->http_proxy_host)
    free(b->http_proxy_host);
  b->http_proxy_host = bgav_strdup(h);
  }

void bgav_options_set_http_proxy_port(bgav_options_t*b, int p)
  {
  b->http_proxy_port = p;
  }

void bgav_options_set_http_proxy_auth(bgav_options_t*b, int i)
  {
  b->http_proxy_auth = i;
  }

void bgav_options_set_http_proxy_user(bgav_options_t*b, const char * h)
  {
  if(b->http_proxy_user)
    free(b->http_proxy_user);
  b->http_proxy_user = bgav_strdup(h);
  }

void bgav_options_set_http_proxy_pass(bgav_options_t*b, const char * h)
  {
  if(b->http_proxy_pass)
    free(b->http_proxy_pass);
  b->http_proxy_pass = bgav_strdup(h);
  }


void bgav_options_set_http_shoutcast_metadata(bgav_options_t*b, int m)
  {
  b->http_shoutcast_metadata = m;
  }

void bgav_options_set_ftp_anonymous_password(bgav_options_t*b, const char * h)
  {
  if(b->ftp_anonymous_password)
    free(b->ftp_anonymous_password);
  b->ftp_anonymous_password = bgav_strdup(h);
  }

void bgav_options_set_ftp_anonymous(bgav_options_t*b, int anonymous)
  {
  b->ftp_anonymous = anonymous;
  }

void bgav_options_set_dvd_chapters_as_tracks(bgav_options_t* opt,
                                             int chapters_as_tracks)
  {
  opt->dvd_chapters_as_tracks = chapters_as_tracks;
  }

void bgav_options_set_audio_dynrange(bgav_options_t* opt, int audio_dynrange)
  {
  opt->audio_dynrange = audio_dynrange;
  }



void bgav_options_set_default_subtitle_encoding(bgav_options_t* b,
                                                const char* encoding)
  {
  if(b->default_subtitle_encoding)
    free(b->default_subtitle_encoding);
  b->default_subtitle_encoding = bgav_strdup(encoding);
  }

void bgav_options_set_seamless(bgav_options_t* opt,
                               int seamless)
  {
  opt->seamless = seamless;
  }

void bgav_options_set_seek_subtitles(bgav_options_t* opt,
                                     int seek_subtitles)
  {
  opt->seek_subtitles = seek_subtitles;
  }

#define FREE(ptr) if(ptr) free(ptr)

void bgav_options_free(bgav_options_t*opt)
  {
  FREE(opt->ftp_anonymous_password);
  FREE(opt->http_proxy_host);
  FREE(opt->http_proxy_user);
  FREE(opt->http_proxy_pass);
  FREE(opt->default_subtitle_encoding);
  }

void bgav_options_set_defaults(bgav_options_t * b)
  {
  memset(b, 0, sizeof(*b));
  b->connect_timeout = 10000;
  b->read_timeout = 10000;
  b->ftp_anonymous = 1;
  b->default_subtitle_encoding = bgav_strdup("LATIN1");
  b->dvd_chapters_as_tracks = 1;
  b->audio_dynrange = 1;
  }

bgav_options_t * bgav_options_create()
  {
  bgav_options_t * ret;
  ret = calloc(1, sizeof(*ret));
  bgav_options_set_defaults(ret);
  return ret;
  }

void bgav_options_destroy(bgav_options_t * opt)
  {
  bgav_options_free(opt);
  free(opt);
  }

#define CP_INT(i) dst->i = src->i
#define CP_STR(s) if(dst->s) free(dst->s); dst->s = bgav_strdup(src->s)

void bgav_options_copy(bgav_options_t * dst, const bgav_options_t * src)
  {
  /* Generic network options */
  CP_INT(connect_timeout);
  CP_INT(read_timeout);

  CP_INT(network_bandwidth);
  CP_INT(network_buffer_size);

  /* http options */

  CP_INT(http_use_proxy);
  CP_STR(http_proxy_host);
  CP_INT(http_proxy_port);

  CP_INT(http_proxy_auth);
  
  CP_STR(http_proxy_user);
  CP_STR(http_proxy_pass);
  
  CP_INT(http_shoutcast_metadata);

  /* ftp options */
    
  CP_STR(ftp_anonymous_password);
  CP_INT(ftp_anonymous);

  /* Subtitle */
  
  CP_STR(default_subtitle_encoding);
  CP_INT(seek_subtitles);

  /* DVD */

  CP_INT(dvd_chapters_as_tracks);

  /* Audio */

  CP_INT(audio_dynrange);

  
  /* Callbacks */
  
  CP_INT(name_change_callback);
  CP_INT(name_change_callback_data);

  CP_INT(metadata_change_callback);
  CP_INT(metadata_change_callback_data);

  CP_INT(track_change_callback);
  CP_INT(track_change_callback_data);

  CP_INT(buffer_callback);
  CP_INT(buffer_callback_data);

  CP_INT(user_pass_callback);
  CP_INT(user_pass_callback_data);
  
  }

#undef CP_INT
#undef CP_STR

