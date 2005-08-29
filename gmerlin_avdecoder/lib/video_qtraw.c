/*****************************************************************
 
  video_qtraw.c
 
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
#include <stdio.h>

typedef struct
  {
  int bytes_per_line;
  void (*scanline_func)(uint8_t * src,
                        uint8_t * dst,
                        int num_pixels,
                        bgav_palette_entry_t * pal);
  } raw_priv_t;

static void scanline_raw_1(uint8_t * src,
                           uint8_t * dst,
                           int num_pixels,
                           bgav_palette_entry_t * pal)
  {
  int i;
  int counter = 0;
  for(i = 0; i < num_pixels; i++)
    {
    if(counter == 8)
      {
      counter = 0;
      src++;
      }
    BGAV_PALETTE_2_RGB24(pal[(*src & 0x80) >> 7], dst);
    *src <<= 1;
    dst += 3;
    counter++;
    }
  }

static void scanline_raw_2(uint8_t * src,
                           uint8_t * dst,
                           int num_pixels,
                           bgav_palette_entry_t * pal)
  {
  int i;
  int counter = 0;
  for(i = 0; i < num_pixels; i++)
    {
    if(counter == 4)
      {
      counter = 0;
      src++;
      }
    BGAV_PALETTE_2_RGB24(pal[(*src & 0xc0) >> 6], dst);
    *src <<= 2;
    dst += 3;
    counter++;
    }
  }

static void scanline_raw_4(uint8_t * src,
                           uint8_t * dst,
                           int num_pixels,
                           bgav_palette_entry_t * pal)
  {
  int i;
  int counter = 0;
  for(i = 0; i < num_pixels; i++)
    {
    if(counter == 2)
      {
      counter = 0;
      src++;
      }
    BGAV_PALETTE_2_RGB24(pal[(*src & 0xF0) >> 4], dst);
    *src <<= 4;
    dst += 3;
    counter++;
    }
  }

static void scanline_raw_8(uint8_t * src,
                           uint8_t * dst,
                           int num_pixels,
                           bgav_palette_entry_t * pal)
  {
  int i;
  for(i = 0; i < num_pixels; i++)
    {
    BGAV_PALETTE_2_RGB24(pal[*src], dst);
    src++;
    dst+=3;
    }
  }

static void scanline_raw_16(uint8_t * src,
                            uint8_t * dst,
                            int num_pixels,
                            bgav_palette_entry_t * pal)
  {
  int i;
  uint16_t pixel;
  
  for(i = 0; i < num_pixels; i++)
    {
    pixel = (src[0] << 8) | (src[1]);
    *((uint16_t*)dst) = pixel;
    src += 2;
    dst += 2;
    }
  }

static void scanline_raw_24(uint8_t * src,
                            uint8_t * dst,
                            int num_pixels,
                            bgav_palette_entry_t * pal)
  {
  memcpy(dst, src, num_pixels * 3);
  }

static void scanline_raw_32(uint8_t * src,
                            uint8_t * dst,
                            int num_pixels,
                            bgav_palette_entry_t * pal)
  {
  int i;
  for(i = 0; i < num_pixels; i++)
    {
    dst[0] = src[1];
    dst[1] = src[2];
    dst[2] = src[3];
    dst[3] = src[0];
    dst += 4;
    src += 4;
    }
  }

static void scanline_raw_2_gray(uint8_t * src,
                                uint8_t * dst,
                                int num_pixels,
                                bgav_palette_entry_t * pal)
  {
  int i;
  int counter = 0;
  for(i = 0; i < num_pixels; i++)
    {
    if(counter == 4)
      {
      counter = 0;
      src++;
      }
    BGAV_PALETTE_2_RGB24(pal[(*src & 0xC0) >> 6], dst);
    
    /* Advance */

    *src <<= 2;
    dst += 3;
    counter++;
    }
  }

static void scanline_raw_4_gray(uint8_t * src,
                                uint8_t * dst,
                                int num_pixels,
                                bgav_palette_entry_t * pal)
  {
  int i;

  int counter = 0;
  for(i = 0; i < num_pixels; i++)
    {
    if(counter == 2)
      {
      counter = 0;
      src++;
      }
    BGAV_PALETTE_2_RGB24(pal[(*src & 0xF0) >> 4], dst);
    
    /* Advance */

    *src <<= 4;
    dst += 3;
    counter++;
    }
  }

static void scanline_raw_8_gray(uint8_t * src,
                                uint8_t * dst,
                                int num_pixels,
                                bgav_palette_entry_t * pal)
  {
  int i;
  for(i = 0; i < num_pixels; i++)
    {
    BGAV_PALETTE_2_RGB24(pal[*src], dst);
    dst += 3;
    src++;
    }
  }


static int init_qtraw(bgav_stream_t * s)
  {
  raw_priv_t * priv;
  int width;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;

  width = s->data.video.format.image_width;
  
  switch(s->data.video.depth)
    {
    case 1:
      /* 1 bpp palette */
      priv->bytes_per_line = width / 8;
      priv->scanline_func = scanline_raw_1;
      if(s->data.video.palette_size < 2)
        {
        fprintf(stderr, "Palette missing or too small %d\n",
                s->data.video.palette_size);
        goto fail;
        }
      s->data.video.format.pixelformat = GAVL_RGB_24;
      s->description = bgav_sprintf("Quicktime Uncompressed 1 bpp palette");
      break;
    case 2:
      /* 2 bpp palette */
      priv->bytes_per_line = width / 4;
      priv->scanline_func = scanline_raw_2;
      if(s->data.video.palette_size < 4)
        {
        fprintf(stderr, "Palette missing or too small\n");
        goto fail;
        }
      s->data.video.format.pixelformat = GAVL_RGB_24;
      s->description = bgav_sprintf("Quicktime Uncompressed 2 bpp palette");
      break;
    case 4:
      /* 4 bpp palette */
      priv->bytes_per_line = width / 2;
      priv->scanline_func = scanline_raw_4;
      if(s->data.video.palette_size < 16)
        {
        fprintf(stderr, "Palette missing or too small\n");
        goto fail;
        }
      s->data.video.format.pixelformat = GAVL_RGB_24;
      s->description = bgav_sprintf("Quicktime Uncompressed 4 bpp palette");
      break;
    case 8:
      /* 8 bpp palette */
      priv->bytes_per_line = width;
      priv->scanline_func = scanline_raw_8;
      if(s->data.video.palette_size < 256)
        {
        fprintf(stderr, "Palette missing or too small\n");
        goto fail;
        }
      s->data.video.format.pixelformat = GAVL_RGB_24;
      s->description = bgav_sprintf("Quicktime Uncompressed 8 bpp palette");
      break;
    case 16:
      /* RGB565 */
      priv->bytes_per_line = width * 2;
      priv->scanline_func = scanline_raw_16;
      s->data.video.format.pixelformat = GAVL_RGB_15;
      s->description = bgav_sprintf("Quicktime Uncompressed 16 bpp RGB");
      break;
    case 24:
      /* 24 RGB */
      priv->bytes_per_line = width * 3;
      priv->scanline_func = scanline_raw_24;
      s->data.video.format.pixelformat = GAVL_RGB_24;
      s->description = bgav_sprintf("Quicktime Uncompressed 24 bpp RGB");
      break;
    case 32:
      /* 32 ARGB */
      priv->bytes_per_line = width * 4;
      priv->scanline_func = scanline_raw_32;
      s->data.video.format.pixelformat = GAVL_RGBA_32;
      s->description = bgav_sprintf("Quicktime Uncompressed 24 bpp RGBA");
      break;
    case 34:
      /* 2 bit gray */
      priv->bytes_per_line = width / 4;
      priv->scanline_func = scanline_raw_2_gray;
      s->data.video.format.pixelformat = GAVL_RGB_24;
      s->description = bgav_sprintf("Quicktime Uncompressed 2 bpp gray");
      break;
    case 36:
      /* 4 bit gray */
      priv->bytes_per_line = width / 2;
      priv->scanline_func = scanline_raw_4_gray;
      s->data.video.format.pixelformat = GAVL_RGB_24;
      s->description = bgav_sprintf("Quicktime Uncompressed 4 bpp gray");
      break;
    case 40:
      /* 8 bit gray */
      priv->bytes_per_line = width;
      priv->scanline_func = scanline_raw_8_gray;
      s->data.video.format.pixelformat = GAVL_RGB_24;
      s->description = bgav_sprintf("Quicktime Uncompressed 8 bpp gray");
      break;
    }
  if(priv->bytes_per_line & 1)
    priv->bytes_per_line++;
  
  return 1;
  
  fail:
  free(priv);
  return 0;
  }

static int decode_qtraw(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  int i;
  raw_priv_t * priv;
  uint8_t * src, *dst;
  bgav_packet_t * p;

  priv = (raw_priv_t*)(s->data.video.decoder->priv);
  
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
  src = p->data;
  dst = f->planes[0];
  
  for(i = 0; i < s->data.video.format.image_height; i++)
    {
    priv->scanline_func(src, dst, s->data.video.format.image_width,
                        s->data.video.palette);
    src += priv->bytes_per_line;
    dst += f->strides[0];
    }
  bgav_demuxer_done_packet_read(s->demuxer, p);
  return 1;
  }

static void close_qtraw(bgav_stream_t * s)
  {
  raw_priv_t * priv;
  priv = (raw_priv_t*)(s->data.video.decoder->priv);
  free(priv);
  }

static bgav_video_decoder_t decoder =
  {
    name:   "Quicktime raw video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('r', 'a', 'w', ' '), 0x00  },
    init:   init_qtraw,
    decode: decode_qtraw,
    close:  close_qtraw,
  };

void bgav_init_video_decoders_qtraw()
  {
  bgav_video_decoder_register(&decoder);
  }

