/*****************************************************************
 
  video_rtjpeg.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/* Ported from libquicktime */

#include <RTjpeg.h>
#include <avdec_private.h>

#define BLOCK_SIZE 16

typedef struct
  {
  gavl_video_frame_t * frame;
  RTjpeg_t * rtjpeg;
  } rtjpeg_t;

static int init_rtjpeg(bgav_stream_t * s)
  {
  return 0;
  }
  
static int decode_rtjpeg(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  return 0;
  }


static void close_rtjpeg(bgav_stream_t * s)
  {

  }

static bgav_video_decoder_t rtjpeg_decoder =
  {
    name:   "rtjpeg video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('R', 'T', 'J', '0'), 0x00  },
    init:   init_rtjpeg,
    decode: decode_rtjpeg,
    close:  close_rtjpeg,
  };

void bgav_init_video_decoders_rtjpeg()
  {
  bgav_video_decoder_register(&rtjpeg_decoder);
  }
