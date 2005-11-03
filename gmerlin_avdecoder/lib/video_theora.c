/*****************************************************************
 
  video_theora.c
 
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
#include <theora/theora.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
  {
  theora_info    ti;
  theora_comment tc;
  theora_state   ts;
  gavl_video_frame_t * frame;  

  /* For setting up the video frame */

  int offset_x;
  int offset_y;

  int offset_x_uv;
  int offset_y_uv;
  
  } theora_priv_t;

static uint8_t * ptr_2_op(uint8_t * ptr, ogg_packet * op)
  {
  memcpy(op, ptr, sizeof(*op));
  ptr += sizeof(*op);
  op->packet = ptr;
  ptr += op->bytes;
  return ptr;
  }
  
static int init_theora(bgav_stream_t * s)
  {
  int sub_h, sub_v;
  int i;
  uint8_t * ext_pos;
  ogg_packet op;
  theora_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;

  /* Initialize theora structures */
  theora_info_init(&priv->ti);
  theora_comment_init(&priv->tc);

  /* Get header packets and initialize decoder */
  if(!s->ext_data)
    {
    fprintf(stderr, "Theora codec requires extradata\n");
    return 0;
    }
  ext_pos = s->ext_data;

  for(i = 0; i < 3; i++)
    {
    ext_pos = ptr_2_op(ext_pos, &op);
    theora_decode_header(&priv->ti, &priv->tc, &op);
    }

  theora_decode_init(&priv->ts, &priv->ti);

  /* Get format */

  s->data.video.format.image_width  = priv->ti.frame_width;
  s->data.video.format.image_height = priv->ti.frame_height;

  s->data.video.format.frame_width  = priv->ti.frame_width;
  s->data.video.format.frame_height = priv->ti.frame_height;
  
  if(!priv->ti.aspect_numerator || !priv->ti.aspect_denominator)
    {
    s->data.video.format.pixel_width  = 1;
    s->data.video.format.pixel_height = 1;
    }
  else
    {
    s->data.video.format.pixel_width  = priv->ti.aspect_numerator;
    s->data.video.format.pixel_height = priv->ti.aspect_denominator;
    }

  s->data.video.format.timescale      = priv->ti.fps_numerator;
  s->data.video.format.frame_duration = priv->ti.fps_denominator;

  switch(priv->ti.pixelformat)
    {
    case OC_PF_420:
      s->data.video.format.pixelformat = GAVL_YUV_420_P;
      break;
    case OC_PF_422:
      s->data.video.format.pixelformat = GAVL_YUV_422_P;
      break;
    case OC_PF_444:
      s->data.video.format.pixelformat = GAVL_YUV_444_P;
      break;
    default:
      fprintf(stderr, "Error, unknown pixelformat %d\n",
              priv->ti.pixelformat);
      return 0;
    }

  /* Get offsets */

  gavl_pixelformat_chroma_sub(s->data.video.format.pixelformat,
                              &sub_h, &sub_v);
  
  priv->offset_x = priv->ti.offset_x;
  priv->offset_y = priv->ti.offset_y;

  priv->offset_x_uv = priv->ti.offset_x / sub_h;
  priv->offset_y_uv = priv->ti.offset_y / sub_v;
  
  /* Create frame */
  priv->frame = gavl_video_frame_create((gavl_video_format_t*)0);
  s->description = bgav_sprintf("Theora (Version %d.%d.%d)",
                                priv->ti.version_major, priv->ti.version_minor,
                                priv->ti.version_subminor); 
  
  return 1;
  }

// static int64_t frame_counter = 0;

static int decode_theora(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  bgav_packet_t * p;
  ogg_packet op;
  yuv_buffer yuv;
  theora_priv_t * priv;
  priv = (theora_priv_t*)(s->data.video.decoder->priv);

  while(1)
    {
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!p)
      return 0;
    memcpy(&op, p->data, sizeof(op));
    op.packet = p->data + sizeof(op);
    
    if(!theora_packet_isheader(&op))
      break;
    bgav_demuxer_done_packet_read(s->demuxer, p);
    }
  
  theora_decode_packetin(&priv->ts, &op);

  bgav_demuxer_done_packet_read(s->demuxer, p);
    
  theora_decode_YUVout(&priv->ts, &yuv);

#if 0
  s->data.video.last_frame_time =
    theora_granule_frame(&priv->ts, priv->ts.granulepos) * s->data.video.format.frame_duration;
  //  fprintf(stderr, "Time: %lld\n", s->data.video.last_frame_time);
#endif
  /* Copy the frame */

  if(frame)
    {
    priv->frame->planes[0] = yuv.y + priv->offset_y * yuv.y_stride + priv->offset_x;
    
    priv->frame->planes[1] = yuv.u + priv->offset_y_uv * yuv.uv_stride + priv->offset_x_uv;
    priv->frame->planes[2] = yuv.v + priv->offset_y_uv * yuv.uv_stride + priv->offset_x_uv;

    priv->frame->strides[0] = yuv.y_stride;
    priv->frame->strides[1] = yuv.uv_stride;
    priv->frame->strides[2] = yuv.uv_stride;
    
    gavl_video_frame_copy(&s->data.video.format,
                          frame, priv->frame);

    //    s->time_scaled = (frame_counter++) * s->data.video.format.frame_duration;
    }
  
  return 1;
  }

static void close_theora(bgav_stream_t * s)
  {
  theora_priv_t * priv;
  priv = (theora_priv_t*)(s->data.video.decoder->priv);

  theora_clear(&priv->ts);
  theora_comment_clear(&priv->tc);
  theora_info_clear(&priv->ti);

  gavl_video_frame_null(priv->frame);
  gavl_video_frame_destroy(priv->frame);

  free(priv);
  }

static void resync_theora(bgav_stream_t * s)
  {
  theora_priv_t * priv;
  priv = (theora_priv_t*)(s->data.video.decoder->priv);
  
  }

static bgav_video_decoder_t decoder =
  {
    name:   "Theora decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('T', 'H', 'R', 'A'),
                            0x00  },
    init:   init_theora,
    decode: decode_theora,
    close:  close_theora,
    resync: resync_theora,
  };

void bgav_init_video_decoders_theora()
  {
  bgav_video_decoder_register(&decoder);
  }
