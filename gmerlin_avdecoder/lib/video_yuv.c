/*****************************************************************
 
  video_yuv.c
 
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

/* Various uncompressed YUV decoders */

#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>

#define PADD(size, bytes) ((((size)+bytes-1)/bytes)*bytes)

typedef struct
  {
  gavl_video_frame_t * frame;
  void (*decode_func)(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f);
  } yuv_priv_t;

/* Common initialization */

static void init_common(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;
  
  priv->frame = gavl_video_frame_create(NULL);
  }

/* Decoding functions */

/* yuv2: It's yuyv with signedness swapped and JPEG scaled */

static void decode_yuv2(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f)
  {
  int i, j;
  uint8_t * src, *dst_y, *dst_u, *dst_v;
  yuv_priv_t * priv;
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->planes[0] = p->data;
  
  for(i = 0; i < s->data.video.format.image_height; i++)
    {
    src = priv->frame->planes[0] + i * priv->frame->strides[0];
    dst_y = f->planes[0]         + i * f->strides[0];
    dst_u = f->planes[1]         + i * f->strides[1];
    dst_v = f->planes[2]         + i * f->strides[2];
    
    for(j = 0; j < s->data.video.format.image_width/2; j++)
      {
      dst_y[0] = src[0];        /* Y */
      dst_u[0] = src[1] ^ 0x80; /* U */
      dst_y[1] = src[2];        /* Y */
      dst_v[0] = src[3] ^ 0x80; /* V */
      src+=4;
      dst_y+=2;
      dst_u++;
      dst_v++;
      }
    }
  }

static int init_yuv2(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  
  init_common(s);
  s->description = bgav_sprintf("Full scale YUV 4:2:2 packed (yuv2)");

  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->strides[0] = PADD(s->data.video.format.image_width * 2, 4);
  priv->decode_func = decode_yuv2;
  s->data.video.format.colorspace = GAVL_YUVJ_422_P;
  return 1;
  }

/*
 *   2vuy: Like yuv2 but WITHOUT swapped chroma sign and
 *   packing order uyvy
 */
static void decode_2vuy(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f)
  {
  yuv_priv_t * priv;
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->planes[0] = p->data;
  gavl_video_frame_copy(&(s->data.video.format),
                        f, priv->frame);
  }

static int init_2vuy(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  
  init_common(s);
  s->description = bgav_sprintf("YUV 4:2:2 packed (2vuy)");

  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->strides[0] = PADD(s->data.video.format.image_width * 2, 4);
  priv->decode_func = decode_2vuy;
  s->data.video.format.colorspace = GAVL_UYVY;
  return 1;
  }

/*
 *  VYUY is just YUY2 colorspace!!!
 *  Wow, that was hard to reverse engineer ;-)
 */

static void decode_VYUY(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f)
  {
  yuv_priv_t * priv;
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->planes[0] = p->data;
  gavl_video_frame_copy(&(s->data.video.format),
                        f, priv->frame);
  }

static int init_VYUY(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  
  init_common(s);
  s->description = bgav_sprintf("YUV 4:2:2 packed (VYUY)");

  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->strides[0] = PADD(s->data.video.format.image_width * 2, 4);
  priv->decode_func = decode_VYUY;
  s->data.video.format.colorspace = GAVL_YUY2;
  return 1;
  }

/* The following 2 have just the chroma planed swapped */

static void decode_yv12(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f)
  {
  yuv_priv_t * priv;
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->planes[0] = p->data;
  priv->frame->planes[1] = priv->frame->planes[0] + s->data.video.format.image_height * priv->frame->strides[0];
  priv->frame->planes[2] = priv->frame->planes[1] + s->data.video.format.image_height/2 * priv->frame->strides[1];
  gavl_video_frame_copy(&s->data.video.format, f, priv->frame);
  }

static int init_yv12(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  
  init_common(s);
  s->description = bgav_sprintf("YUV 4:2:0 planar (yv12)");

  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->strides[0] = PADD(s->data.video.format.image_width, 2);
  priv->frame->strides[1] = priv->frame->strides[0]/2;
  priv->frame->strides[2] = priv->frame->strides[1];
  
  priv->decode_func = decode_yv12;
  s->data.video.format.colorspace = GAVL_YUV_420_P;
  return 1;
  }

static void decode_YV12(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f)
  {
  yuv_priv_t * priv;
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->planes[0] = p->data;
  priv->frame->planes[2] = priv->frame->planes[0] + s->data.video.format.image_height * priv->frame->strides[0];
  priv->frame->planes[1] = priv->frame->planes[2] + s->data.video.format.image_height/2 * priv->frame->strides[1];
  gavl_video_frame_copy(&s->data.video.format, f, priv->frame);
  }



static int init_YV12(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  
  init_common(s);
  s->description = bgav_sprintf("YUV 4:2:0 planar (YV12)");

  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->strides[0] = PADD(s->data.video.format.image_width, 2);
  priv->frame->strides[1] = priv->frame->strides[0]/2;
  priv->frame->strides[2] = priv->frame->strides[1];
  
  priv->decode_func = decode_YV12;
  s->data.video.format.colorspace = GAVL_YUV_420_P;
  return 1;
  }

/*
 *  YVU9 is yuv410 with swapped chroma planes
 */

static void decode_YVU9(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f)
  {
  yuv_priv_t * priv;
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->planes[0] = p->data;
  priv->frame->planes[2] = priv->frame->planes[0] + s->data.video.format.image_height * priv->frame->strides[0];
  priv->frame->planes[1] = priv->frame->planes[2] + (s->data.video.format.image_height)/4 * priv->frame->strides[1];

  gavl_video_frame_copy(&(s->data.video.format), f, priv->frame);
  }



static int init_YVU9(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  
  init_common(s);
  s->description = bgav_sprintf("YVU9");

  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->strides[0] = PADD(s->data.video.format.image_width, 8);
  priv->frame->strides[1] = priv->frame->strides[0]/4;
  priv->frame->strides[2] = priv->frame->strides[1];
  
  priv->decode_func = decode_YVU9;
  s->data.video.format.colorspace = GAVL_YUV_410_P;
  return 1;
  }


/* v308: Packed YUV 4:4:4, we make this planar */

static void decode_v308(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f)
  {
  int i, j;
  uint8_t * src, *dst_y, *dst_u, *dst_v;
  yuv_priv_t * priv;
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->planes[0] = p->data;
  
  for(i = 0; i < s->data.video.format.image_height; i++)
    {
    src = priv->frame->planes[0] + i * priv->frame->strides[0];

    dst_y = f->planes[0] + i * f->strides[0];
    dst_u = f->planes[1] + i * f->strides[1];
    dst_v = f->planes[2] + i * f->strides[2];
    
    for(j = 0; j < s->data.video.format.image_width; j++)
      {
      *dst_y = src[1];
      *dst_u = src[2];
      *dst_v = src[0];
      
      src+=3;
      dst_y++;
      dst_u++;
      dst_v++;
      
      }
    }
  }

static int init_v308(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  
  init_common(s);
  s->description = bgav_sprintf("YUV 4:4:4 packed (v308)");

  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->strides[0] = s->data.video.format.image_width * 3;
  priv->decode_func = decode_v308;
  s->data.video.format.colorspace = GAVL_YUV_444_P;
  return 1;
  }

/*
 *  v410: Packed YUV 4:4:4, (10 bit per component)
 *  we make this planar
 */

static void decode_v410(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f)
  {
  int i, j;
  uint8_t * src, *dst_y, *dst_u, *dst_v;
  uint32_t src_i;
  yuv_priv_t * priv;
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->planes[0] = p->data;
  
  for(i = 0; i < s->data.video.format.image_height; i++)
    {
    src = priv->frame->planes[0] + i * priv->frame->strides[0];

    dst_y = f->planes[0] + i * f->strides[0];
    dst_u = f->planes[1] + i * f->strides[1];
    dst_v = f->planes[2] + i * f->strides[2];
    
    for(j = 0; j < s->data.video.format.image_width; j++)
      {
      src_i = BGAV_PTR_2_32LE(src);
      
      *dst_v = (src_i >> 4) & 0xff;
      *dst_u = (src_i >> 14)  & 0xff;
      *dst_y = (src_i >> 24) & 0xff;
      
      src+=4;
      dst_y++;
      dst_u++;
      dst_v++;
      
      }
    }
  }

static int init_v410(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  
  init_common(s);
  s->description = bgav_sprintf("YUV 4:4:4 packed (v410)");

  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->strides[0] = s->data.video.format.image_width * 4;
  priv->decode_func = decode_v410;
  s->data.video.format.colorspace = GAVL_YUV_444_P;
  return 1;
  }

/*
 *  yuv4: packed YUV 4:2:0
 *  Pretty weird thing existent exclusively in the
 *  qt4l/lqt universe :-)
 */

static void decode_yuv4(bgav_stream_t * s, bgav_packet_t * p, gavl_video_frame_t * f)
  {
  int i, j;
  uint8_t * src, *dst_y, *dst_u, *dst_v;
  yuv_priv_t * priv;
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->planes[0] = p->data;

  /* Packing order for one macropixel is U0V0Y0Y1Y2Y3 */

  for(i = 0; i < s->data.video.format.image_height/2; i++)
    {
    src = priv->frame->planes[0] + i * priv->frame->strides[0];
    dst_y = f->planes[0] + 2 * i * f->strides[0];
    dst_u = f->planes[1] + i * f->strides[1];
    dst_v = f->planes[2] + i * f->strides[2];
    
    for(j = 0; j < s->data.video.format.image_width/2; j++)
      {
      dst_u[0]               = src[0] ^ 0x80;
      dst_v[0]               = src[1] ^ 0x80;
      
      dst_y[0]               = src[2]; /* Top left  */
      dst_y[1]               = src[3]; /* Top right */
      
      dst_y[f->strides[0]]   = src[4]; /* Bottom left  */
      dst_y[f->strides[0]+1] = src[5]; /* Bottom right */
      
      src+=6;
      dst_y+=2;
      dst_u++;
      dst_v++;
      }
    }
  }

static int init_yuv4(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  
  init_common(s);
  s->description = bgav_sprintf("YUV 4:2:0 packed (yuv4)");
  
  priv = (yuv_priv_t *)(s->data.video.decoder->priv);

  priv->frame->strides[0] = PADD(s->data.video.format.image_width, 2) * 3;
  priv->decode_func = decode_yuv4;
  s->data.video.format.colorspace = GAVL_YUV_420_P;
  return 1;
  }


/* Generic decode function */

static int decode(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  yuv_priv_t * priv;
  bgav_packet_t * p;
  priv = (yuv_priv_t*)(s->data.video.decoder->priv);
  
  /* We assume one frame per packet */
  
  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;

  if(!p->data_size)
    return 1; /* Libquicktime/qt4l bug */
  
  /* Skip frame */
  if(!f)
    {
    bgav_demuxer_done_packet_read(s->demuxer, p);
    return 1;
    }

  priv->decode_func(s, p, f);
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

/* Generic close function */

static void close(bgav_stream_t * s)
  {
  yuv_priv_t * priv;
  priv = (yuv_priv_t*)(s->data.video.decoder->priv);

  gavl_video_frame_null(priv->frame);
  gavl_video_frame_destroy(priv->frame);
  
  free(priv);
  }

/* Decoders */

static bgav_video_decoder_t yuv2_decoder =
  {
    name:   "yuv2 video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('y', 'u', 'v', '2'), 0x00  },
    init:   init_yuv2,
    decode: decode,
    close:  close,
  };

#if 1
static bgav_video_decoder_t twovuy_decoder =
  {
    name:   "2vuy video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('2', 'v', 'u', 'y'), 0x00  },
    init:   init_2vuy,
    decode: decode,
    close:  close,
  };
#endif

static bgav_video_decoder_t yv12_decoder =
  {
    name:   "yv12 video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('y', 'v', '1', '2'), 0x00  },
    init:   init_yv12,
    decode: decode,
    close:  close,
  };

static bgav_video_decoder_t YV12_decoder =
  {
    name:   "YV12 video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('Y', 'V', '1', '2'), 0x00  },
    init:   init_YV12,
    decode: decode,
    close:  close,
  };

static bgav_video_decoder_t VYUY_decoder =
  {
    name:   "VYUY video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('V', 'Y', 'U', 'Y'),
                            BGAV_MK_FOURCC('2', 'v', 'u', 'y'),
                            0x00 },
    init:   init_VYUY,
    decode: decode,
    close:  close,
  };

static bgav_video_decoder_t YVU9_decoder =
  {
    name:   "YVU9 video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('Y', 'V', 'U', '9'), 0x00  },
    init:   init_YVU9,
    decode: decode,
    close:  close,
  };

static bgav_video_decoder_t v308_decoder =
  {
    name:   "v308 video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('v', '3', '0', '8'), 0x00  },
    init:   init_v308,
    decode: decode,
    close:  close,
  };

static bgav_video_decoder_t v410_decoder =
  {
    name:   "v410 video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('v', '4', '1', '0'), 0x00  },
    init:   init_v410,
    decode: decode,
    close:  close,
  };

static bgav_video_decoder_t yuv4_decoder =
  {
    name:   "yuv4 video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('y', 'u', 'v', '4'), 0x00  },
    init:   init_yuv4,
    decode: decode,
    close:  close,
  };

void bgav_init_video_decoders_yuv()
  {
  bgav_video_decoder_register(&yuv2_decoder);
  bgav_video_decoder_register(&twovuy_decoder);
  bgav_video_decoder_register(&yv12_decoder);
  bgav_video_decoder_register(&YV12_decoder);
  bgav_video_decoder_register(&VYUY_decoder);
  bgav_video_decoder_register(&YVU9_decoder);
  bgav_video_decoder_register(&v308_decoder);
  bgav_video_decoder_register(&v410_decoder);
  bgav_video_decoder_register(&yuv4_decoder);
  }

