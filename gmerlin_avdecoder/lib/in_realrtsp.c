/*****************************************************************
 
  in_realrtsp.c
 
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

#include <stdio.h>
#include <stdlib.h>
#include <avdec_private.h>
#include <real_rtsp/rtsp_session.h>

typedef struct
  {
  char * host;
  char * path;
  int port;
  int fd;
  
  rtsp_session_t * s;
  } realrtsp_priv_t;

static int open_realrtsp(bgav_input_context_t * ctx, const char * url,
                         int milliseconds)
  {
  int ret;
  char * mrl;
  int got_redirected = 0;
  realrtsp_priv_t * priv = NULL;
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  ret = 1;
  mrl = bgav_sprintf(url);
  
  if(!bgav_url_split(url, NULL, &(priv->host), &(priv->port), &(priv->path)))
    goto fail;

  /* Try to connect to host */

  if(priv->port < 0)
    priv->port = 554;

  //  fprintf(stderr, "Port: %d\n", priv->port);
  
  if((priv->fd = bgav_tcp_connect(priv->host, priv->port, milliseconds)) == -1)
    return 0;
  
  priv->s = rtsp_session_start(priv->fd,
                               &mrl,
                               priv->path,
                               priv->host,
                               priv->port,
                               &got_redirected);

  if(got_redirected)
    {
    fprintf(stderr, "Got redirected to %s\n", mrl);
    ret = open_realrtsp(ctx, mrl, milliseconds);
    }
  free(mrl);
  return ret;
  
  fail:
  ctx->priv = NULL;
  if(priv)
    free(priv);
  return 0;
  }

static int     read_realrtsp(bgav_input_context_t* ctx,
                             uint8_t * buffer, int len)
  {
  realrtsp_priv_t * priv = (realrtsp_priv_t*)(ctx->priv);
  return rtsp_session_read(priv->s, buffer, len);
  }

static void    close_realrtsp(bgav_input_context_t * ctx)
  {
  realrtsp_priv_t * priv = (realrtsp_priv_t*)(ctx->priv);
  if(priv->s)
    rtsp_session_end(priv->s);
  if(priv->host)
    free(priv->host);
  if(priv->path)
    free(priv->path);
  free(priv);
  }

bgav_input_t bgav_input_realrtsp =
  {
    open:      open_realrtsp,
    read:      read_realrtsp,
    close:     close_realrtsp
  };

