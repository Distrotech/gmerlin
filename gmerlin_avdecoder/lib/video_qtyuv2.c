/*****************************************************************
 
  video_qtyuv2.c
 
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
#include <string.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>

typedef struct
  {
  int bytes_per_line;
  } yuv2_priv_t;


static int init_qtyuv2(bgav_stream_t * s)
  {
  yuv2_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;
  s->data.video.format.colorspace = GAVL_YUY2;
  priv->bytes_per_line = ((s->data.video.format.image_width * 2 + 3) / 4) * 4;

  s->description = bgav_sprintf("YUV 4:2:2 packed");
  return 1;
  }

static int decode_qtyuv2(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  int i, j;
  yuv2_priv_t * priv;
  uint8_t * src, *dst;
  uint8_t * src_save, *dst_save;
  bgav_packet_t * p;
  priv = (yuv2_priv_t*)(s->data.video.decoder->priv);
  
  /* We assume one frame per packet */
  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;

  /* Skip frame */
  if(!f)
    {
    bgav_demuxer_done_packet_read(s->demuxer, p);
    return 1;
    }
  src_save = p->data;
  dst_save = f->planes[0];
  
  for(i = 0; i < s->data.video.format.image_height; i++)
    {
    src = src_save;
    dst = dst_save;
    
    for(j = 0; j < s->data.video.format.image_width; j++)
      {
      dst[0] = src[0];
      dst[1] = src[1] ^ 0x80;
      src+=2;
      dst+=2;
      }
    src_save += priv->bytes_per_line;
    dst_save += f->strides[0];
    }
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

static void close_qtyuv2(bgav_stream_t * s)
  {
  yuv2_priv_t * priv;
  priv = (yuv2_priv_t*)(s->data.video.decoder->priv);
  free(priv);
  }

static bgav_video_decoder_t decoder =
  {
    name:   "Quicktime yuv2 video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('y', 'u', 'v', '2'), 0x00  },
    init:   init_qtyuv2,
    decode: decode_qtyuv2,
    close:  close_qtyuv2,
  };

void bgav_init_video_decoders_qtyuv2()
  {
  bgav_video_decoder_register(&decoder);
  }

