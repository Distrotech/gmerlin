/*****************************************************************
 
  in_http.h
 
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
#include <unistd.h>
#include <avdec_private.h>
#include <http.h>

/* Generic http input module */

static int open_http(bgav_input_context_t * ctx, const char * url,
                     int milliseconds)
  {
  bgav_http_t * h;
  bgav_http_header_t * header;
  char * redirect_url = (char*)0;
  const char * var;
  
  h = bgav_http_open(url, milliseconds, &redirect_url);
  
  if(!h)
    return 0;
  ctx->priv = h;

  header = bgav_http_get_header(h);

  var = bgav_http_header_get_var(header, "Content-length");
  if(var)
    ctx->total_bytes = atoi(var);

  var = bgav_http_header_get_var(header, "Content-type");
  if(var)
    ctx->mimetype = bgav_strndup(var, NULL);
  
  return 1;
  }

static int     read_http(bgav_input_context_t* ctx,
                         uint8_t * buffer, int len)
  {
  int fd = bgav_http_get_fd((bgav_http_t*)(ctx->priv));
  return read(fd, buffer, len); 
  }

static void    close_http(bgav_input_context_t * ctx)
  {
  if(ctx->priv)
    bgav_http_close((bgav_http_t*)(ctx->priv));
  }

bgav_input_t bgav_input_http =
  {
    open:      open_http,
    read:      read_http,
    close:     close_http
  };

