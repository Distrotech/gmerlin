/*****************************************************************
 
  packetbuffer.c
 
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

struct bgav_packet_buffer_s
  {
  bgav_packet_t * packets;
  bgav_packet_t * read_packet;
  bgav_packet_t * write_packet;
  };

bgav_packet_buffer_t * bgav_packet_buffer_create()
  {
  bgav_packet_buffer_t * ret;
  ret = calloc(1, sizeof(*ret));
  /* Create 2 packets */

  ret->packets = bgav_packet_create();
  ret->packets->next = bgav_packet_create();
  ret->packets->next->next = ret->packets;

  ret->read_packet  = ret->packets;
  ret->write_packet = ret->packets;
  
  return ret;
  }

void bgav_packet_buffer_destroy(bgav_packet_buffer_t * b)
  {
  bgav_packet_t * tmp_packet;
  bgav_packet_t * packet = b->packets;
  
  do{
  tmp_packet = packet->next;
  bgav_packet_destroy(packet);
  packet = tmp_packet;
  }while(packet != b->packets);
  free(b);
  }

void bgav_packet_buffer_clear(bgav_packet_buffer_t * b)
  {
  bgav_packet_t * tmp_packet;
  tmp_packet = b->packets;

  do{
  tmp_packet->data_size = 0;
  tmp_packet->timestamp = GAVL_TIME_UNDEFINED;
  tmp_packet->valid = 0;
  tmp_packet = tmp_packet->next;
  }while(tmp_packet != b->packets);
  b->read_packet = b->write_packet;
  }


bgav_packet_t * bgav_packet_buffer_get_packet_read(bgav_packet_buffer_t * b)
  {
  bgav_packet_t * ret;
  if(!b->read_packet->valid)
    return (bgav_packet_t*)0;
  ret = b->read_packet;
  b->read_packet = b->read_packet->next;
  return ret;
  }

bgav_packet_t * bgav_packet_buffer_get_packet_write(bgav_packet_buffer_t * b)
  {
  bgav_packet_t * cur;
  bgav_packet_t * ret;
  if(b->write_packet->valid)
    {
    /* Insert a packet */
    cur = b->packets;
    while(cur->next != b->write_packet)
      cur = cur->next;
    cur->next = bgav_packet_create();
    cur->next->next = b->write_packet;
    b->write_packet = cur->next;
    }
  ret = b->write_packet;
  b->write_packet = b->write_packet->next;
  ret->timestamp = GAVL_TIME_UNDEFINED;
  //  fprintf(stderr, "Get packet write: %p\n", ret);  
  return ret;
  }

int bgav_packet_buffer_get_timestamp(bgav_packet_buffer_t * b,
                                     gavl_time_t * ret)
  {
  if(!b->read_packet->valid)
    return 0;
  *ret = b->read_packet->timestamp;
  return 1;
  }
