/*****************************************************************
 
  qt_esds.c
 
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

#include <string.h>
#include <stdlib.h>

#include <avdec_private.h>
#include <qt.h>

#if 0
typedef struct
  {
  qt_atom_header_t * h;
  int      version;
  uint32_t flags;

  uint8_t  objectTypeId;
  uint8_t  streamType;
  int      bufferSizeDB;
  int32_t  maxBitrate;
  int32_t  avgBitrate;
  int	   decoderConfigLen;
  uint8_t* decoderConfig;
  } qt_esds_t;
#endif

static void bgav_qt_esds_dump(qt_esds_t * ret)
  {
  fprintf(stderr, "esds:\n");
  bgav_qt_atom_dump_header(&(ret->h));
  fprintf(stderr, "Version:          %d\n", ret->version);
  fprintf(stderr, "Flags:            0x%0x06x\n", ret->flags);
  fprintf(stderr, "objectTypeId:     %d\n", ret->objectTypeId);
  fprintf(stderr, "streamType:       0x%02x\n", ret->streamType);
  fprintf(stderr, "bufferSizeDB:     %d\n", ret->bufferSizeDB);

  fprintf(stderr, "maxBitrate:       %d\n", ret->maxBitrate);
  fprintf(stderr, "avgBitrate:       %d\n", ret->avgBitrate);
  fprintf(stderr, "decoderConfigLen: %d\n", ret->decoderConfigLen);
  fprintf(stderr, "decoderConfig:\n");
  bgav_hexdump(ret->decoderConfig, ret->decoderConfigLen, 16);
  
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

  //  fprintf(stderr, "tag: 0x%02x\n", tag);
  
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

  //  fprintf(stderr, "tag: 0x%02x\n", tag);

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

  //  fprintf(stderr, "tag: 0x%02x\n", tag);
  if(tag != 0x05)
    return 0;

  ret->decoderConfigLen = read_mp4_descr_length(input);
  ret->decoderConfig = malloc(ret->decoderConfigLen);
  if(bgav_input_read_data(input, ret->decoderConfig,
                          ret->decoderConfigLen) < ret->decoderConfigLen)
    return 0;
  //  fprintf(stderr, "decoderConfigLen: %d\n", ret->decoderConfigLen);
  //  fprintf(stderr, "Skipping %lld bytes\n", h->size - (input->position - h->start_position));
  bgav_qt_atom_skip(input, h);

  bgav_qt_esds_dump(ret);

  return 1;
  }

void bgav_qt_esds_free(qt_esds_t * esds)
  {
  if(esds->decoderConfig)
    free(esds->decoderConfig);
  }
