/*****************************************************************
 
  pes_packet.c
 
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

#include <avdec_private.h>
#include <pes_packet.h>

#define PADDING_STREAM   0xbe
#define PRIVATE_STREAM_2 0xbf

int bgav_pes_header_read(bgav_input_context_t * input,
                         bgav_pes_header_t * ret)
  {
  uint8_t c;
  uint16_t len;
  int64_t pos;
  uint16_t tmp_16;
  
  uint8_t header_flags;
  uint8_t header_size;

  uint32_t header;

  memset(ret, 0, sizeof(*ret));
  
  if(!bgav_input_read_32_be(input, &header))
    return 0;
  ret->stream_id = header & 0x000000ff;

  if(!bgav_input_read_16_be(input, &len))
    return 0;

  if((ret->stream_id == PADDING_STREAM)
     (ret->stream_id == PRIVATE_STREAM_2))
    {
    ret->payload_size = len;
    return 1;
    }

  pos = input->position;

  if(!bgav_input_read_8(input, &c))
    return 0;

  if((c & 0xC0) == 0x80) /* MPEG-2 */
    {
    if(!bgav_input_read_8(input, &header_flags))
      return 0;
    
    if(header_flags)
      {
      if(!bgav_input_read_8(input, &header_size))
        return 0;
      
      /* Read stuff */
      if(header_flags & 0x80) /* PTS present */
        {
        bgav_input_read_8(input, &c);
        ret->pts = (c >> 1) & 7;
        ret->pts <<= 15;
        bgav_input_read_16_be(input, &tmp_16);
        ret->pts |= (tmp_16 >> 1);
        ret->pts <<= 15;
        bgav_input_read_16_be(input, &tmp_16);
        ret->pts |= (tmp_16 >> 1);
        header_size -= 5;
        }
      bgav_input_skip(input, header_size);
      }
    }
  else /* MPEG-1 */
    {
    while(input->position < pos + len)
      {
      if((c & 0x80) != 0x80)
        break;
      bgav_input_read_8(input, &c);
      }
    /* Skip STD Buffer scale */
    
    if((c & 0xC0) == 0x40)
      {
      bgav_input_skip(input, 1);
      bgav_input_read_8(input, &c);
      /*      fprintf(stderr, "STD buffer scale\n"); */
      }

    if((c & 0xf0) == 0x20)
      {
      ret->pts = ((c >> 1) & 7) << 30;
      bgav_input_read_16_be(input, &tmp_16);
      ret->pts |= ((tmp_16 >> 1) << 15);
      bgav_input_read_16_be(input, &tmp_16);
      ret->pts |= (tmp_16 >> 1);
      /*
        fprintf(stderr, "PTS: %f\n", (float)demuxer->pes_packet.pts / 90000.0);
      */
      }
    else if((c & 0xf0) == 0x30)
      {
      ret->pts = ((c >> 1) & 7) << 30;
      bgav_input_read_16_be(input, &tmp_16);
      ret->pts |= ((tmp_16 >> 1) << 15);
      bgav_input_read_16_be(input, &tmp_16);
      ret->pts |= (tmp_16 >> 1);
      /* SKIP DTS */
      bgav_input_skip(input, 5);
      /*
        fprintf(stderr, "PTS: %f, DTS skipped\n",
        (float)demuxer->pes_packet.pts / 90000.0);
      */
      }
    else
      {
      bgav_input_skip(input, 1);
      }
    }
  ret->payload_size = len - (input->position - pos);
  return 1;
  }

void bgav_pes_header_dump(bgav_pes_header_t * p)
  {
  fprintf(stderr, "PES Header: PTS: %f, Stream ID: %02x, payload_size: %d\n",
          (float)p->pts / 90000.0, p->stream_id, p->payload_size);
  
  }

