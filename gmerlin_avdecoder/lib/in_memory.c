/*****************************************************************
 
  in_memory.c
 
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

#include <string.h>
#include <stdlib.h>

#include <avdec_private.h>

typedef struct
  {
  uint8_t * data;
  uint8_t * data_ptr;
  } mem_priv_t;

static int read_mem(bgav_input_context_t* ctx,
                    uint8_t * buffer, int len)
  {
  int bytes_to_read;
  int bytes_left;
  
  mem_priv_t * priv = (mem_priv_t*)(ctx->priv);
  bytes_left = ctx->total_bytes - (uint32_t)(priv->data_ptr - priv->data);
  bytes_to_read = (len < bytes_left) ? len : bytes_left;
  memcpy(buffer, priv->data_ptr, bytes_to_read);
  priv->data_ptr += bytes_to_read;
  return bytes_to_read;
  }

static int64_t seek_byte_mem(bgav_input_context_t * ctx,
                             int64_t pos, int whence)
  {
  mem_priv_t * priv = (mem_priv_t*)(ctx->priv);
  priv->data_ptr = priv->data + ctx->position;
  return ctx->position;
  }

static void    close_mem(bgav_input_context_t * ctx)
  {
  mem_priv_t * priv = (mem_priv_t*)(ctx->priv);
  free(priv);
  }

static bgav_input_t bgav_input_mem =
  {
    open:      NULL, /* Not needed */
    read:      read_mem,
    seek_byte: seek_byte_mem,
    close:     close_mem
  };

bgav_input_context_t * bgav_input_open_memory(uint8_t * data,
                                              uint32_t data_size)
  {
  bgav_input_context_t * ret;
  mem_priv_t * priv;

  ret = calloc(1, sizeof(*ret));
  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  ret->input = &(bgav_input_mem);

  priv->data     = data;
  priv->data_ptr = data;
  ret->total_bytes = data_size;
  
  return ret;
  }

void bgav_input_reopen_memory(bgav_input_context_t * ctx,
                              uint8_t * data,
                              uint32_t data_size)
  {
  mem_priv_t * priv;
  priv = (mem_priv_t *)(ctx->priv);

  priv->data     = data;
  priv->data_ptr = data;
  ctx->total_bytes = data_size;
  ctx->position = 0;
  ctx->buffer_size = 0;
  }
