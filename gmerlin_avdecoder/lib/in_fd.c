/*****************************************************************
 
  in_fd.c
 
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <avdec_private.h>

typedef struct
  {
  int fd;
  } fd_priv_t;

static int read_fd(bgav_input_context_t* ctx,
                    uint8_t * buffer, int len)
  {
  fd_priv_t * priv = (fd_priv_t*)(ctx->priv);
  return read(priv->fd, buffer, len);
  }

static void    close_fd(bgav_input_context_t * ctx)
  {
  fd_priv_t * priv = (fd_priv_t*)(ctx->priv);
  free(priv);
  }

static bgav_input_t bgav_input_fd =
  {
    open:   NULL, /* Not needed */
    read:   read_fd,
    close:  close_fd
  };

bgav_input_context_t * bgav_input_open_fd(int fd, int64_t total_bytes, const char * mimetype)
  {
  bgav_input_context_t * ret;
  fd_priv_t * priv;
  
  ret = calloc(1, sizeof(*ret));
  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  ret->input = &(bgav_input_fd);

  priv->fd     = fd;
  ret->total_bytes = total_bytes;
  ret->mimetype = bgav_strdup(mimetype);
  return ret;
  }
