/*****************************************************************
 
  video_tga.c
 
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

/* TGA decoder for quicktime */

#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>

#include "targa.h"

typedef struct
  {
  tga_image tga;
  gavl_video_frame_t * frame;
  int bytes_per_pixel;

  /* Read the first frame during initialization to get the format */
  int do_init;
  int have_frame;
  
  } tga_priv_t;


static gavl_colorspace_t get_colorspace(int depth, int * bytes_per_pixel)
  {
  switch(depth)
    {
    case 16:
      *bytes_per_pixel = 2;
      return GAVL_BGR_15;
      break;
    case 24:
      *bytes_per_pixel = 3;
      return GAVL_BGR_24;
      break;
    case 32:
      *bytes_per_pixel = 4;
      return GAVL_RGBA_32;
      break;
    default:
      return GAVL_COLORSPACE_NONE;
    }
  }

static int decode_tga(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  bgav_packet_t * p;
  tga_priv_t * priv;
  
  priv = (tga_priv_t*)(s->data.video.decoder->priv);

  if(!priv->have_frame)
    {
    /* Decode a frame */
    
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!p)
      return 0;

    if(tga_read_from_memory(&(priv->tga), p->data, p->data_size) != TGA_NOERR)
      return 0;
    
    bgav_demuxer_done_packet_read(s->demuxer, p);
    }
  if(priv->do_init)
    {
    priv->have_frame = 1;

    /* Figure out the format */

    s->data.video.format.frame_width = priv->tga.width;
    s->data.video.format.frame_height = priv->tga.height;
    
    s->data.video.format.image_width = priv->tga.width;
    s->data.video.format.image_height = priv->tga.height;
    
    s->data.video.format.pixel_width = 1;
    s->data.video.format.pixel_height = 1;
    switch(priv->tga.image_type)
      {
      case TGA_IMAGE_TYPE_COLORMAP:
      case TGA_IMAGE_TYPE_COLORMAP_RLE:
        s->data.video.format.colorspace = get_colorspace(priv->tga.color_map_depth,
                                                         &(priv->bytes_per_pixel));
        break;
      default:
        s->data.video.format.colorspace = get_colorspace(priv->tga.pixel_depth,
                                                         &(priv->bytes_per_pixel));
        break;
      }
    if(s->data.video.format.colorspace == GAVL_COLORSPACE_NONE)
      return 0;
    return 1;
    }
  /* Copy it */
  
  if(frame)
    {
    switch(priv->tga.image_type)
      {
      case TGA_IMAGE_TYPE_COLORMAP:
      case TGA_IMAGE_TYPE_COLORMAP_RLE:
        if(tga_color_unmap(&priv->tga) != TGA_NOERR)
          {
          tga_free_buffers(&priv->tga);
          memset(&priv->tga, 0, sizeof(priv->tga));
          return 0;
          }
        break;
      default:
        break;
      }
    if(s->data.video.format.colorspace == GAVL_RGBA_32)
      tga_swap_red_blue(&(priv->tga));
    
    priv->frame->strides[0] = priv->bytes_per_pixel * s->data.video.format.image_width;
    priv->frame->planes[0] = priv->tga.image_data;
    
    /* Figure out the copy function */
    
    if(priv->tga.image_descriptor & TGA_R_TO_L_BIT)
      {
      if(priv->tga.image_descriptor & TGA_T_TO_B_BIT)
        {
        gavl_video_frame_copy_flip_x(&(s->data.video.format), frame, priv->frame);
        }
      else
        {
        gavl_video_frame_copy_flip_xy(&(s->data.video.format), frame, priv->frame);
        }
      }
    else
      {
      if(priv->tga.image_descriptor & TGA_T_TO_B_BIT)
        {
        gavl_video_frame_copy(&(s->data.video.format), frame, priv->frame);
        }
      else
        {
        gavl_video_frame_copy_flip_y(&(s->data.video.format), frame, priv->frame);
        }
      }
    }
  /* Free anything */

  tga_free_buffers(&priv->tga);
  memset(&priv->tga, 0, sizeof(priv->tga));
  
  priv->have_frame = 0;
  return 1;
  }

static int init_tga(bgav_stream_t * s)
  {
  tga_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  
  s->description = bgav_sprintf("TGA Video (%s)",
                                ((s->data.video.format.colorspace ==
                                  GAVL_RGBA_32) ? "RGBA" : "RGB")); 
  
  s->data.video.decoder->priv = priv;

  /* Get format by decoding first frame */

  priv->do_init = 1;
  priv->frame = gavl_video_frame_create(NULL);
  
  if(!decode_tga(s, (gavl_video_frame_t *)0))
    return 0;
  priv->do_init = 0;
  return 1;
  }

static void close_tga(bgav_stream_t * s)
  {
  tga_priv_t * priv;
  priv = (tga_priv_t*)(s->data.video.decoder->priv);
  gavl_video_frame_null(priv->frame);
  gavl_video_frame_destroy(priv->frame);
  free(priv);
  
  }

static bgav_video_decoder_t decoder =
  {
    name:   "TGA video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('t', 'g', 'a', ' '),
                            0x00  },
    init:   init_tga,
    decode: decode_tga,
    close:  close_tga,
    resync: NULL,
  };

void bgav_init_video_decoders_tga()
  {
  bgav_video_decoder_register(&decoder);
  }

