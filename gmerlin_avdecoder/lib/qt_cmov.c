/*****************************************************************
 
  qt_cmov.c
 
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

#include <zlib.h>
#include <stdio.h>

int bgav_qt_cmov_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_moov_t * ret)
  {
  qt_atom_header_t ch;
  uint32_t compression_id;
  int have_dcom = 0;
  uint32_t size_compressed;
  uint32_t size_uncompressed;
  uLongf size_uncompressed_ret; /* use zlib type to keep gcc happy */
  int result;
  uint8_t * buf_compressed;
  uint8_t * buf_uncompressed;
  
  bgav_input_context_t * input_mem;
  
  while(input->position < h->start_position + h->size)
    {
    if(!bgav_qt_atom_read_header(input, &ch))
      return 0;

    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('d', 'c', 'o', 'm'):
        if(!bgav_input_read_fourcc(input, &compression_id))
          return 0;
        switch(compression_id)
          {
          case BGAV_MK_FOURCC('z', 'l', 'i', 'b'):
            have_dcom = 1;
            break;
          default:
            fprintf(stderr, "Unknown compression method: ");
            bgav_dump_fourcc(compression_id);
            return 0;
          }
        break;
      case BGAV_MK_FOURCC('c', 'm', 'v', 'd'):
        if(!bgav_input_read_32_be(input, &size_uncompressed))
          return 0;
        size_compressed = h->size - (input->position - h->start_position);
        //        fprintf(stderr, "Compressed size: %d Uncompressed size: %d\n",
        //                size_compressed, size_uncompressed);

        /* Decompress header and parse it */

        buf_compressed   = malloc(size_compressed);
        buf_uncompressed = malloc(size_uncompressed);
        
        if(bgav_input_read_data(input, buf_compressed, size_compressed) <
           size_compressed)
          return 0;
        size_uncompressed_ret = size_uncompressed;
        if(uncompress(buf_uncompressed, &size_uncompressed_ret,
                      buf_compressed, size_compressed) != Z_OK)
          {
          fprintf(stderr, "Uncompression failed\n");
          return 0;
          }
        else
          {
          //          fprintf(stderr, "Uncompressed %d bytes\n",
          //                  (int)size_uncompressed_ret);
          //          outfile = fopen("header.dat", "w");
          //          fwrite(buf_uncompressed, size_uncompressed_ret, 1, outfile);
          //          fclose(outfile);
          }
        input_mem = bgav_input_open_memory(buf_uncompressed,
                                           size_uncompressed_ret);

        if(!bgav_qt_atom_read_header(input_mem, &ch) ||
           ch.fourcc != BGAV_MK_FOURCC('m', 'o', 'o', 'v'))
          {
          bgav_input_destroy(input_mem);
          return 0;
          }
        result = bgav_qt_moov_read(&ch, input_mem, ret);
        bgav_input_destroy(input_mem);
        free(buf_compressed);
        free(buf_uncompressed);
        
        return result;
        break;
      default:
        bgav_qt_atom_skip(input, &ch);
      }
    }
  return 1;
  }
