/*****************************************************************
 
  video_png.c
 
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

/* PNG decoder for quicktime and avi */

#include <stdlib.h>
#include <string.h>
#include <png.h>

#include <config.h>
#include <codecs.h>
#include <avdec_private.h>

typedef struct
  {
  uint8_t ** rows;
  int buffer_position;
  int buffer_size;
  uint8_t * buffer;
  } png_priv_t;

/* This function will be passed to libpng for reading data */

static void read_function(png_structp png_ptr,
                          png_bytep data,
                          png_uint_32 length)
  {
  png_priv_t * priv = (png_priv_t*)png_get_io_ptr(png_ptr);
  
  if((long)(length + priv->buffer_position) <= priv->buffer_size)
    {
    memcpy(data, priv->buffer + priv->buffer_position, length);
    priv->buffer_position += length;
    }
  }

static int init_png(bgav_stream_t * s)
  {
  png_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  priv->rows = malloc(s->data.video.format.image_height * sizeof(uint8_t*));

  /* We support RGBA for streams with a depth of 32 */

  if(s->data.video.depth == 32)
    s->data.video.format.colorspace = GAVL_RGBA_32;
  else
    s->data.video.format.colorspace = GAVL_RGB_24;
  
  s->description = bgav_sprintf("PNG Video (%s)",
                                ((s->data.video.format.colorspace ==
                                  GAVL_RGBA_32) ? "RGBA" : "RGB")); 
  
  s->data.video.decoder->priv = priv;

  return 1;
  }

static int decode_png(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  bgav_packet_t * p;
  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_info = 0;
  png_priv_t * priv;
  int i;
  int png_bit_depth;
  int png_color_type;
  
  priv = (png_priv_t*)(s->data.video.decoder->priv);

  p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p)
    return 0;
  
  /* We decode only if we have a frame */

  if(frame)
    {
    priv->buffer = p->data;
    priv->buffer_size = p->data_size;
    priv->buffer_position = 0;
    
    for(i = 0; i < s->data.video.format.image_height; i++)
      priv->rows[i] = frame->planes[0] + i * frame->strides[0];

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    info_ptr = png_create_info_struct(png_ptr);
    png_set_read_fn(png_ptr, priv, (png_rw_ptr)read_function);
    png_read_info(png_ptr, info_ptr);

    png_bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    png_color_type = png_get_color_type(png_ptr, info_ptr);

    switch(png_color_type)
      {
      case PNG_COLOR_TYPE_GRAY:       //  (bit depths 1, 2, 4, 8, 16)
        if(png_bit_depth < 8)
          png_set_gray_1_2_4_to_8(png_ptr);
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
          {
          png_set_tRNS_to_alpha(png_ptr);
          }
        png_set_gray_to_rgb(png_ptr);
        break;
      case PNG_COLOR_TYPE_GRAY_ALPHA: //  (bit depths 8, 16)
        png_set_gray_to_rgb(png_ptr);
        break;
      case PNG_COLOR_TYPE_PALETTE:    //  (bit depths 1, 2, 4, 8)
        png_set_palette_to_rgb(png_ptr);
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
          png_set_tRNS_to_alpha(png_ptr);
        break;
      case PNG_COLOR_TYPE_RGB:        //  (bit_depths 8, 16)
        if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
          png_set_tRNS_to_alpha(png_ptr);
        if(png_bit_depth == 16)
          png_set_strip_16(png_ptr);
        break;
      case PNG_COLOR_TYPE_RGB_ALPHA:  //  (bit_depths 8, 16)
        if(png_bit_depth == 16)
          png_set_strip_16(png_ptr);
        break;
      }
    
    if((png_color_type & PNG_COLOR_MASK_ALPHA) &&
       (s->data.video.format.colorspace != GAVL_RGBA_32))
      png_set_strip_alpha(png_ptr);

    /* Read the image */

    png_read_image(png_ptr, priv->rows);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    }
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

static void close_png(bgav_stream_t * s)
  {
  png_priv_t * priv;
  priv = (png_priv_t*)(s->data.video.decoder->priv);

  free(priv->rows);
  free(priv);
  }

static bgav_video_decoder_t decoder =
  {
    name:   "PNG video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('p', 'n', 'g', ' '),
                            BGAV_MK_FOURCC('M', 'P', 'N', 'G'),
                            0x00  },
    init:   init_png,
    decode: decode_png,
    close:  close_png,
    resync: NULL,
  };

void bgav_init_video_decoders_png()
  {
  bgav_video_decoder_register(&decoder);
  }

