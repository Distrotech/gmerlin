/*****************************************************************
 
  video_tiff.c
 
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

/* Tiff decoder for quicktime. Based on gmerlin tiff reader plugin by
   one78 */

#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#include <avdec_private.h>
#include <codecs.h>

typedef struct
  {
  TIFF * tiff;
  uint8_t *buffer;
  uint64_t buffer_size;
  uint32_t buffer_position;
  uint32_t buffer_alloc;
  uint32_t Width;
  uint32_t Height;
  uint16 SampleSperPixel;
  uint16 Orientation;
  uint32_t * raster;
  bgav_packet_t * packet;
  } tiff_t;

/* libtiff read callbacks */

static tsize_t read_function(thandle_t fd, tdata_t data, tsize_t length)
  {
  uint32_t bytes_read;
  tiff_t *p = (tiff_t*)(fd);

  bytes_read = length;
  if(length > p->buffer_size - p->buffer_position)
    bytes_read = p->buffer_size - p->buffer_position;

  memcpy(data, p->buffer + p->buffer_position, bytes_read);
  p->buffer_position += bytes_read;
  return bytes_read;
  }

static toff_t seek_function(thandle_t fd, toff_t off, int whence)
  {
  tiff_t *p = (tiff_t*)(fd);

  if (whence == SEEK_SET) p->buffer_position = off;
  else if (whence == SEEK_CUR) p->buffer_position += off;
  else if (whence == SEEK_END) p->buffer_size += off;

  if (p->buffer_position > p->buffer_size) {
  fprintf(stderr, "codec_tiff: warning, seek overshot buffer.\n");
  return -1;
  }

  if (p->buffer_position > p->buffer_size)
    p->buffer_position = p->buffer_size;

  return (p->buffer_position);
  }

static toff_t size_function(thandle_t fd)
  {
  tiff_t *p = (tiff_t*)(fd);
  return (p->buffer_size);
  }

static int close_function(thandle_t fd)
  {
  return (0);
  }

static tsize_t write_function(thandle_t fd, tdata_t data, tsize_t length)
  {
  return 0;
  }

static int map_file_proc(thandle_t a, tdata_t* b, toff_t* c)
  {
  return 0;
  }

static void unmap_file_proc(thandle_t a, tdata_t b, toff_t c)
  {
  }

static TIFF* open_tiff_mem(char *mode, tiff_t* p)
  {
  return TIFFClientOpen("gmerlin_avdecoder", mode, (thandle_t)p,
                        read_function,write_function ,
                        seek_function, close_function,
                        size_function, map_file_proc ,unmap_file_proc);
  }



static int read_header_tiff(bgav_stream_t * s,
                            gavl_video_format_t * format)
  {
  tiff_t *p = (tiff_t*)(s->data.video.decoder->priv);

  p->packet = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!p->packet)
    return 0;
  
  //  tiff_read_mem(priv, filename);
  
  p->buffer = p->packet->data;
  p->buffer_size = p->packet->data_size;
  p->buffer_position = 0;
  
  if(!(p->tiff = open_tiff_mem("rm", p))) return 0;
  
  if(format)
    {
    if(!(TIFFGetField(p->tiff, TIFFTAG_IMAGEWIDTH, &p->Width))) return 0;
    if(!(TIFFGetField(p->tiff, TIFFTAG_IMAGELENGTH, &p->Height))) return 0;
    if(!(TIFFGetField(p->tiff, TIFFTAG_SAMPLESPERPIXEL, &p->SampleSperPixel)))return 0;
    
    if(!(TIFFGetField(p->tiff, TIFFTAG_ORIENTATION, &p->Orientation)))
      p->Orientation = ORIENTATION_TOPLEFT;
    
    format->frame_width  = p->Width;
    format->frame_height = p->Height;
    
    format->image_width  = format->frame_width;
    format->image_height = format->frame_height;
    format->pixel_width = 1;
    format->pixel_height = 1;
    
    if(p->SampleSperPixel ==4)
      {
      format->pixelformat = GAVL_RGBA_32;
      }
    else
      {
      format->pixelformat = GAVL_RGB_24;
      }
    }
  return 1;
  }

#define GET_RGBA(fp, rp)  \
  fp[0]=TIFFGetR(*rp); \
  fp[1]=TIFFGetG(*rp); \
  fp[2]=TIFFGetB(*rp); \
  fp[3]=TIFFGetA(*rp); \
  fp += 4;

#define GET_RGB(fp, rp)  \
  fp[0]=TIFFGetR(*rp); \
  fp[1]=TIFFGetG(*rp); \
  fp[2]=TIFFGetB(*rp); \
  fp += 3;


static int read_image_tiff(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  uint32_t * raster_ptr;
  uint8_t * frame_ptr;
  uint8_t * frame_ptr_start;
  int i, j;
  
  tiff_t *p = (tiff_t*)(s->data.video.decoder->priv);

  if(!p->raster)
    p->raster =
      (uint32_t*)_TIFFmalloc(p->Height * p->Width * sizeof(uint32_t));
  
  if(!TIFFReadRGBAImage(p->tiff, p->Width, p->Height, (uint32*)p->raster, 0))
    return 0;

    if(p->SampleSperPixel ==4)
    {
    frame_ptr_start = frame->planes[0];

    for (i=0;i<p->Height; i++)
      {
      frame_ptr = frame_ptr_start;

      raster_ptr = p->raster + (p->Height - 1 - i) * p->Width;

      for(j=0;j<p->Width; j++)
        {
        GET_RGBA(frame_ptr, raster_ptr);
        raster_ptr++;
        }
      frame_ptr_start += frame->strides[0];
      }
    }
  else
    {
    frame_ptr_start = frame->planes[0];

    for (i=0;i<p->Height; i++)
      {
      frame_ptr = frame_ptr_start;

      raster_ptr = p->raster + (p->Height - 1 - i) * p->Width;

      for(j=0;j<p->Width; j++)
        {
        GET_RGB(frame_ptr, raster_ptr);
        raster_ptr++;
        }
      frame_ptr_start += frame->strides[0];
      }
    }

  TIFFClose( p->tiff );
  p->tiff = (TIFF*)0;
  bgav_demuxer_done_packet_read(s->demuxer, p->packet);
  p->packet = (bgav_packet_t*)0;
  return 1;
  
  }

static int init_tiff(bgav_stream_t * s)
  {
  tiff_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;

  /* We support RGBA for streams with a depth of 32 */

  if(!read_header_tiff(s, &(s->data.video.format)))
    return 0;
    
  if(s->data.video.depth == 32)
    s->data.video.format.pixelformat = GAVL_RGBA_32;
  else
    s->data.video.format.pixelformat = GAVL_RGB_24;
  
  s->description = bgav_sprintf("TIFF Video (%s)",
                                ((s->data.video.format.pixelformat ==
                                  GAVL_RGBA_32) ? "RGBA" : "RGB")); 
  

  return 1;
  }

static int decode_tiff(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  tiff_t * priv;
  priv = (tiff_t*)(s->data.video.decoder->priv);

  //  fprintf(stderr, "Decode tiff...");
  
  /* We decode only if we have a frame */

  if(frame)
    {
    if(!priv->packet && !read_header_tiff(s, (gavl_video_format_t*)0))
      return 0;
    read_image_tiff(s, frame);
    }
  else
    {
    priv->packet = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!priv->packet)
      return 0;
    bgav_demuxer_done_packet_read(s->demuxer, priv->packet);
    priv->packet = (bgav_packet_t*)0;
    }
  //  fprintf(stderr, "done\n");
  return 1;
  }

static void close_tiff(bgav_stream_t * s)
  {
  tiff_t * priv;
  priv = (tiff_t*)(s->data.video.decoder->priv);

  if (priv->raster) _TIFFfree(priv->raster);

  free(priv);
  }


static bgav_video_decoder_t decoder =
  {
    name:   "TIFF video decoder",
    fourccs:  (uint32_t[]){ BGAV_MK_FOURCC('t', 'i', 'f', 'f'),
                            0x00  },
    init:   init_tiff,
    decode: decode_tiff,
    close:  close_tiff,
    resync: NULL,
  };

void bgav_init_video_decoders_tiff()
  {
  bgav_video_decoder_register(&decoder);
  }
