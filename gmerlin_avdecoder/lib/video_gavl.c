/*****************************************************************
 
  video_gavl.c
 
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

#include <avdec_private.h>
#include <codecs.h>

static int init_gavl(bgav_stream_t * s)
  {
  return 1;
  }

static int decode_gavl(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  bgav_packet_t * p;

  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p || !(p->video_frame))
    return 0;
  
  if(frame)
    gavl_video_frame_copy(&(s->data.video.format), frame, p->video_frame);

  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

static void close_gavl(bgav_stream_t * s)
  {
  
  }

static bgav_video_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('g', 'a', 'v', 'l'),
                           0x00 },
    name: "gavl video decoder",
    init: init_gavl,
    close: close_gavl,
    decode: decode_gavl
  };

void bgav_init_video_decoders_gavl()
  {
  bgav_video_decoder_register(&decoder);
  }
