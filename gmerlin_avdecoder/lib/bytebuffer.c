/*****************************************************************
 
  bytebuffer.c
 
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

#include <avdec_private.h>
#include <stdlib.h>
#include <string.h>

void bgav_bytebuffer_append(bgav_bytebuffer_t * b, bgav_packet_t * p, int padding)
  {
  if(b->size + p->data_size + padding > b->alloc)
    {
    b->alloc = b->size + p->data_size + padding + 1024; 
    b->buffer = realloc(b->buffer, b->alloc);
    }
  memcpy(b->buffer + b->size, p->data, p->data_size);
  b->size += p->data_size;
  memset(b->buffer + b->size, 0, padding);
  }

void bgav_bytebuffer_remove(bgav_bytebuffer_t * b, int bytes)
  {
  if(bytes > b->size)
    bytes = b->size;
  
  if(bytes < b->size)
    memmove(b->buffer, b->buffer + bytes, b->size - bytes);
  b->size -= bytes;
  }

void bgav_bytebuffer_free(bgav_bytebuffer_t * b)
  {
  if(b->buffer)
    free(b->buffer);
  }

void bgav_bytebuffer_flush(bgav_bytebuffer_t * b)
  {
  b->size = 0;
  }
