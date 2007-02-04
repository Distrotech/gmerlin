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
#include <stdio.h>

#include <avdec_private.h>

typedef struct
  {
  uint8_t * data;
  uint8_t * data_ptr;
  bgav_input_context_t* input;
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

static bgav_input_t input_mem =
  {
    open:      NULL, /* Not needed */
    read:      read_mem,
    seek_byte: seek_byte_mem,
    close:     close_mem
  };

bgav_input_context_t * bgav_input_open_memory(uint8_t * data,
                                              uint32_t data_size,
                                              const bgav_options_t * opt)
  {
  bgav_input_context_t * ret;
  mem_priv_t * priv;

  ret = bgav_input_create(opt);
  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  ret->input = &(input_mem);

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

/* Buffer for another input */

static int read_buffer(bgav_input_context_t* ctx,
                       uint8_t * buffer, int len)
  {
  int result;
  int old_size;
  mem_priv_t * priv = (mem_priv_t*)(ctx->priv);
  old_size = priv->data_ptr - priv->data;
  bgav_input_ensure_buffer_size(priv->input, old_size + len);
  priv->data        = priv->input->buffer;
  priv->data_ptr    = priv->input->buffer + old_size;
  
  ctx->total_bytes = priv->input->buffer_size;
  result = read_mem(ctx, buffer, len);
  ctx->total_bytes = 0;
  
  //  fprintf(stderr, "read_mem: old_size: %d, total_bytes: %lld, len: %d, Result: %d\n",
  //          old_size, ctx->total_bytes, len, result);
  
  return result;
  
  }

static bgav_input_t input_buffer =
  {
    open:      NULL, /* Not needed */
    read:      read_buffer,
    //    seek_byte: seek_byte_mem,
    close:     close_mem,
  };



bgav_input_context_t * bgav_input_open_as_buffer(bgav_input_context_t * input)
  {
  bgav_input_context_t * ret;
  mem_priv_t * priv;

  ret = bgav_input_create(input->opt);
  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  ret->input = &(input_buffer);

  priv->input    = input;
  priv->data     = input->buffer;
  priv->data_ptr = input->buffer;
  //  ret->total_bytes = priv->input->buffer_size;
  return ret;
  }
     
