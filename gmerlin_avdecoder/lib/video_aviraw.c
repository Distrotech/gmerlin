/*****************************************************************
 
  video_aviraw.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

#include <avdec_private.h>
#include <codecs.h>
#include <stdio.h>

/* Palette */

#if 0

static void scanline_1(uint8_t * src, uint8_t * dst,
                       int num_pixels, bgav_palette_entry_t * pal)
  {
  
  }

static void scanline_4(uint8_t * src, uint8_t * dst,
                       int num_pixels, bgav_palette_entry_t * pal)
  {
  
  }

#endif

static void scanline_8(uint8_t * src, uint8_t * dst,
                       int num_pixels, bgav_palette_entry_t * pal)
  {
  int i;
  for(i = 0; i < num_pixels; i++)
    {
    BGAV_PALETTE_2_RGB24(pal[*src], dst);
    src++;
    dst+=3;
    }
  }

/* Non palette */

static void scanline_16(uint8_t * src, uint8_t * dst,
                        int num_pixels, bgav_palette_entry_t * pal)
  {
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
  memcpy(dst, src, num_pixels * 2);
#else
  int i;
  for(i = 0; i < num_pixels; i++)
    {
    dst[2*i]   = src[2*i+1];
    dst[2*i+1] = src[2*i];
    }
#endif
  }

static void scanline_16_swap(uint8_t * src, uint8_t * dst,
                        int num_pixels, bgav_palette_entry_t * pal)
  {
#ifdef GAVL_PROCESSOR_BIG_ENDIAN
  memcpy(dst, src, num_pixels * 2);
#else
  int i;
  for(i = 0; i < num_pixels; i++)
    {
    dst[2*i]   = src[2*i+1];
    dst[2*i+1] = src[2*i];
    }
#endif
  }



static void scanline_24(uint8_t * src, uint8_t * dst,
                        int num_pixels, bgav_palette_entry_t * pal)
  {
  memcpy(dst, src, num_pixels * 3);
  }

static void scanline_32(uint8_t * src, uint8_t * dst,
                        int num_pixels, bgav_palette_entry_t * pal)
  {
  memcpy(dst, src, num_pixels * 4);
  }

typedef struct
  {
  void (*scanline_func)(uint8_t * src, uint8_t * dst,
                        int num_pixels, bgav_palette_entry_t * pal);
  int in_stride;
  } aviraw_t;

static void close_aviraw(bgav_stream_t * s)
  {
  aviraw_t * priv;
  priv = (aviraw_t*)(s->data.video.decoder->priv);
  free(priv);
  }

static int decode_aviraw(bgav_stream_t * s, gavl_video_frame_t * f)
  {
  int i;
  bgav_packet_t * p;
  aviraw_t * priv;
  

  uint8_t * src;
  uint8_t * dst;

  priv = (aviraw_t*)(s->data.video.decoder->priv);

  while(1)
    {
    p = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!p)
      return 0;
    if(!p->data_size)
      {
      bgav_demuxer_done_packet_read(s->demuxer, p);
      s->position++;
      }
    else
      break;
    }
  

  if(f)
    {
    /* RGB AVIs are upside down */
    src = p->data;
    dst = f->planes[0] + (s->data.video.format.image_height-1) * f->strides[0];
    for(i = 0; i < s->data.video.format.image_height; i++)
      {
      priv->scanline_func(src, dst, s->data.video.format.image_width,
                          s->data.video.palette);
      src += priv->in_stride;
      dst -= f->strides[0];
      }
    }

  bgav_demuxer_done_packet_read(s->demuxer, p);
  
  return 1;
  }

static int init_aviraw(bgav_stream_t * s)
  {
  aviraw_t * priv;

  priv = calloc(1, sizeof(*priv));

  s->data.video.decoder->priv = priv;
    
  switch(s->data.video.depth)
    {
#if 0
    case 1:
      if(s->data.video.palette_size < 2)
        fprintf(stderr, "Warning: Palette too small %d < 2\n", s->data.video.palette_size);
      priv->scanline_func = scanline_1;
      break;
    case 4:
      if(s->data.video.palette_size < 16)
        fprintf(stderr, "Warning: Palette too small %d < 16\n", s->data.video.palette_size);
      priv->scanline_func = scanline_4;
      break;
#endif
    case 8:
      if(s->data.video.palette_size < 256)
        fprintf(stderr, "Warning: Palette too small %d < 256\n", s->data.video.palette_size);
      priv->scanline_func = scanline_8;
      break;
    case 16:
      if(s->fourcc == BGAV_MK_FOURCC('M','T','V',' '))
        {
        s->data.video.format.pixelformat = GAVL_RGB_16;
        priv->scanline_func = scanline_16_swap;
        }
      else
        {
        s->data.video.format.pixelformat = GAVL_RGB_15;
        priv->scanline_func = scanline_16;
        }
      break;
    case 24:
      priv->scanline_func = scanline_24;
      s->data.video.format.pixelformat = GAVL_BGR_24;
      break;
    case 32:
      priv->scanline_func = scanline_32;
      s->data.video.format.pixelformat = GAVL_BGR_32;
      break;
    default:
      fprintf(stderr, "Unsupported depth: %d\n", s->data.video.depth);
      return 0;
      break;
    }

  priv->in_stride = (s->data.video.format.image_width * s->data.video.depth + 7) / 8;

  /* Padd to 4 byte */
  if(priv->in_stride % 4)
    {
    priv->in_stride += (4 - (priv->in_stride % 4));
    }
  s->description = bgav_sprintf("RGB uncompressed");
  return 1;
  }

static bgav_video_decoder_t decoder =
  {
    name:   "Raw video decoder for AVI",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('R', 'G', 'B', ' '),
                            BGAV_MK_FOURCC('M', 'T', 'V', ' '),
                            /* RGB3 is used by NSV, but seems to be the same as 24 bpp AVI */
                            BGAV_MK_FOURCC('R', 'G', 'B', '3'),
                            0x00  },
    init:   init_aviraw,
    decode: decode_aviraw,
    close:  close_aviraw,
    resync: NULL,
  };

void bgav_init_video_decoders_aviraw()
  {
  bgav_video_decoder_register(&decoder);
  }
