/*****************************************************************
 
  in_pnm.c
 
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
#include "pnm.h"

typedef struct
  {
  char * url;
  int fd;
  int eof;
  pnm_t * s;
  } pnm_priv_t;

static int open_pnm(bgav_input_context_t * ctx, const char * url)
  {
  int port;
  char * path = (char*)0;
  char * host = (char*)0;
  pnm_priv_t * priv = NULL;
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  priv->url = bgav_sprintf(url);
  
  if(!bgav_url_split(url, NULL, &host, &port, &path))
    goto fail;
  
  /* Try to connect to host */

  if(port < 0)
    port = 7070;
  
  if((priv->fd = bgav_tcp_connect(host, port, ctx->connect_timeout, &ctx->error_msg)) == -1)
    return 0;

  priv->s = pnm_connect(priv->fd, path);

  if(!priv->s)
    {
    //    fprintf(stderr, "Cannot open %s\n", url);
    return 0;
    }

  if(host)
    free(host);
  if(path)
    free(path);
  return 1;

  fail:
  ctx->priv = NULL;
  if(host)
    free(host);
  if(path)
    free(path);
  
  if(priv)
    free(priv);
  return 0;
  }

static int     read_pnm(bgav_input_context_t* ctx,
                             uint8_t * buffer, int len)
  {
  int result;
  pnm_priv_t * priv = (pnm_priv_t*)(ctx->priv);

  if(priv->eof)
    return 0;

  result = pnm_read(priv->s, buffer, len);

  if(!result)
    priv->eof = 1;
  //  fprintf(stderr, "Result: %d\n", result);
  return result;
  }

static void    close_pnm(bgav_input_context_t * ctx)
  {
  pnm_priv_t * priv = (pnm_priv_t*)(ctx->priv);

  if(priv->url)
    free(priv->url);

  if(priv->s)
    pnm_close(priv->s);
  free(priv);
  }

bgav_input_t bgav_input_pnm =
  {
    name:      "Real pnm",
    open:      open_pnm,
    read:      read_pnm,
    close:     close_pnm
  };

