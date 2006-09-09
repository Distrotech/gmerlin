/*****************************************************************
 
  qt_esds.c
 
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
#include <qt.h>
#include <stdio.h>

// #define ENABLE_DUMP

void bgav_qt_esds_dump(int indent, qt_esds_t * e)
  {
  bgav_diprintf(indent, "esds:\n");
  bgav_qt_atom_dump_header(indent+2, &(e->h));
  bgav_diprintf(indent+2, "Version:          %d\n", e->version);
  bgav_diprintf(indent+2, "Flags:            0x%0x06x\n", e->flags);
  bgav_diprintf(indent+2, "objectTypeId:     %d\n", e->objectTypeId);
  bgav_diprintf(indent+2, "streamType:       0x%02x\n", e->streamType);
  bgav_diprintf(indent+2, "bufferSizeDB:     %d\n", e->bufferSizeDB);

  bgav_diprintf(indent+2, "maxBitrate:       %d\n", e->maxBitrate);
  bgav_diprintf(indent+2, "avgBitrate:       %d\n", e->avgBitrate);
  bgav_diprintf(indent+2, "decoderConfigLen: %d\n", e->decoderConfigLen);
  bgav_diprintf(indent+2, "decoderConfig:\n");
  bgav_hexdump(e->decoderConfig, e->decoderConfigLen, 16);
  }

static int read_mp4_descr_length(bgav_input_context_t * input)
  {
  uint8_t b;
  int num_bytes = 0;
  unsigned int length = 0;
  
  do
    {
    if(!bgav_input_read_8(input, &b))
      return -1;
    num_bytes++;
    length = (length << 7) | (b & 0x7F);
    } while ((b & 0x80) && (num_bytes < 4));
  return length;
  }

int bgav_qt_esds_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_esds_t * ret)
  {
  uint8_t tag;
  
  READ_VERSION_AND_FLAGS;
  memcpy(&(ret->h), h, sizeof(*h));
  
  if(!bgav_input_read_8(input, &tag))
    return 0;

  if(tag == 0x03)
    {
    if(read_mp4_descr_length(input) < 20)
      return 0;
    bgav_input_skip(input, 3);
    }
  else
    bgav_input_skip(input, 2);

  if(!bgav_input_read_8(input, &tag))
    return 0;

  if(tag != 0x04)
    return 0;
  
  if(read_mp4_descr_length(input) < 15)
    return 0;

  if(!bgav_input_read_8(input, &(ret->objectTypeId)) ||
     !bgav_input_read_8(input, &(ret->streamType)) ||
     !bgav_input_read_24_be(input, &(ret->bufferSizeDB)) ||
     !bgav_input_read_32_be(input, &(ret->maxBitrate)) ||
     !bgav_input_read_32_be(input, &(ret->avgBitrate)))
    return 0;

  if(!bgav_input_read_8(input, &tag))
    return 0;

  if(tag != 0x05)
    return 0;

  ret->decoderConfigLen = read_mp4_descr_length(input);

  ret->decoderConfig = calloc(ret->decoderConfigLen+16, 1);
  if(bgav_input_read_data(input, ret->decoderConfig,
                          ret->decoderConfigLen) < ret->decoderConfigLen)
    return 0;
  bgav_qt_atom_skip(input, h);
#ifdef ENABLE_DUMP
  bgav_qt_esds_dump(ret);
#endif
  return 1;
  }

void bgav_qt_esds_free(qt_esds_t * esds)
  {
  if(esds->decoderConfig)
    free(esds->decoderConfig);
  }
