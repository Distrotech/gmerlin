/*****************************************************************
 
  options.c
 
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

/* Configuration stuff */

void bgav_set_connect_timeout(bgav_options_t *b, int timeout)
  {
  b->connect_timeout = timeout;
  }

void bgav_set_read_timeout(bgav_options_t *b, int timeout)
  {
  b->read_timeout = timeout;
  }

/*
 *  Set network bandwidth (in bits per second)
 */

void bgav_set_network_bandwidth(bgav_options_t *b, int bandwidth)
  {
  b->network_bandwidth = bandwidth;
  }

void bgav_set_network_buffer_size(bgav_options_t *b, int size)
  {
  b->network_buffer_size = size;
  }


void bgav_set_http_use_proxy(bgav_options_t*b, int use_proxy)
  {
  b->http_use_proxy = use_proxy;
  }

void bgav_set_http_proxy_host(bgav_options_t*b, const char * h)
  {
  if(b->http_proxy_host)
    free(b->http_proxy_host);
  b->http_proxy_host = bgav_strndup(h, NULL);
  }

void bgav_set_http_proxy_port(bgav_options_t*b, int p)
  {
  b->http_proxy_port = p;
  }

void bgav_set_http_shoutcast_metadata(bgav_options_t*b, int m)
  {
  b->http_shoutcast_metadata = m;
  }

void bgav_set_ftp_anonymous_password(bgav_options_t*b, const char * h)
  {
  if(b->ftp_anonymous_password)
    free(b->ftp_anonymous_password);
  b->ftp_anonymous_password = bgav_strndup(h, NULL);
  }

#define FREE(ptr) if(ptr) free(ptr);

void bgav_options_free(bgav_options_t*opt)
  {
  FREE(opt->ftp_anonymous_password);
  FREE(opt->http_proxy_host)
    
  }

void bgav_options_set_defaults(bgav_options_t * b)
  {
  b->connect_timeout = 10000;
  b->read_timeout = 10000;
  }
