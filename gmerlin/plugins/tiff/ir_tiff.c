/*****************************************************************
 
  ir_tiff.c
 
  Copyright (c) 2003-2004 by Michael Gruenert - one78@web.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>
#include <inttypes.h>
#include <plugin.h>

typedef struct
  {
  uint8_t *buffer;
  uint64_t buffer_size;
  uint32_t buffer_position;
  uint32_t buffer_alloc;
  uint32_t Width;
  uint32_t Height;
  uint16 SampleSperPixel;
  uint16 Orientation;
  gavl_video_format_t format;
  TIFF * tiff;
  } tiff_t;

static void * create_tiff()
  {
  tiff_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_tiff(void* priv)
  {
  tiff_t * tiff = (tiff_t*)priv;
  if(tiff->buffer) free(tiff->buffer);
  free(tiff);
  }

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
  fprintf(stderr,"write_funktion\n");
  return 0;
  }

static int map_file_proc(thandle_t a, tdata_t* b, toff_t* c)
  {
  fprintf(stderr,"write_funktion\n");
  return 0;
  }

static void unmap_file_proc(thandle_t a, tdata_t b, toff_t c)
  {
  fprintf(stderr,"write_funktion\n");
  }

static TIFF* open_tiff_mem(char *mode, tiff_t* p)
  {
  return TIFFClientOpen("Gmerlin tiff plugin", mode, (thandle_t)p,
                        read_function,write_function ,
                        seek_function, close_function,
                        size_function, map_file_proc ,unmap_file_proc);
  }

static int tiff_read_mem(tiff_t *tiff, const char *filename)
  {
  FILE * file;
  
  if(!(file = fopen(filename,"r")))return 0;
  
  fseek(file, 0, SEEK_END);

  if((tiff->buffer_size = ftell(file))<0) return 0;

  fseek(file, 0, SEEK_SET);

  if(tiff->buffer_size > tiff->buffer_alloc)
    {
    tiff->buffer_alloc = tiff->buffer_size + 128;
    tiff->buffer = realloc(tiff->buffer, tiff->buffer_alloc);
    }
  
  if((fread(tiff->buffer, 1, tiff->buffer_size, file)) < tiff->buffer_size)
    return 0;

  tiff->buffer_position = 0;
  
  fclose(file);
  return 1;
  }


static int read_header_tiff(void *priv,const char *filename, gavl_video_format_t * format)
  {
  tiff_t *p = (tiff_t*)priv;
  
  tiff_read_mem(priv, filename);

  if(!(p->tiff = open_tiff_mem("rm", p))) return 0;
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
    format->colorspace = GAVL_RGBA_32;
    }
  else
    {
    format->colorspace = GAVL_RGB_24;
    }
  return 1;
  }

#if 0
static uint32_t * transpose(uint32_t * raster, int width, int height)
  {
  int i, j;
  uint32_t * ret;

  ret = malloc(width * height * sizeof(*ret));

  for(i = 0; i < height; i++)
    {
    for(j = 0; j < width; j++)
      {
      ret[i * width + j] = raster[j * height + i];
      }
    }
  free(raster);
  return ret;
  }
#endif

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

static int read_image_tiff(void *priv, gavl_video_frame_t *frame)
  {
  int i, j;
  uint32_t *raster;
  tiff_t *p = (tiff_t*)priv;
  uint32_t * raster_ptr;
  uint8_t * frame_ptr;
  uint8_t * frame_ptr_start;

  p->buffer_position =0;
    
  raster = (uint32_t*)_TIFFmalloc(p->Height * p->Width * sizeof(uint32_t));

  if(!TIFFReadRGBAImage(p->tiff, p->Width, p->Height, (uint32*)raster, 0))
    return 0;

#if 0
  if((p->Orientation == ORIENTATION_LEFTTOP) ||
     (p->Orientation == ORIENTATION_RIGHTTOP) ||
     (p->Orientation == ORIENTATION_LEFTBOT) ||
     (p->Orientation == ORIENTATION_RIGHTBOT))
    raster = transpose(raster, p->Width, p->Height);
#endif
  
  //  fprintf(stderr, "Orientation: %d\n", p->Orientation);
  
  if(p->SampleSperPixel ==4)
    {
    frame_ptr_start = frame->planes[0];
    
    for (i=0;i<p->Height; i++)
      {
      frame_ptr = frame_ptr_start;
      
      raster_ptr = raster + (p->Height - 1 - i) * p->Width;
      
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
      
      raster_ptr = raster + (p->Height - 1 - i) * p->Width;
      
      for(j=0;j<p->Width; j++)
        {
        GET_RGB(frame_ptr, raster_ptr);
        raster_ptr++;
        }
      frame_ptr_start += frame->strides[0];
      }
    }

  if (raster) _TIFFfree(raster);
  TIFFClose( p->tiff );
  return 1;
  }

bg_image_reader_plugin_t the_plugin =
  {
    common:
    {
      name:          "ir_tiff",
      long_name:     "TIFF loader ",
      mimetypes:     (char*)0,
      extensions:    "tif tiff",
      type:          BG_PLUGIN_IMAGE_READER,
      flags:         BG_PLUGIN_FILE,
      priority:      BG_PLUGIN_PRIORITY_MAX,
      create:        create_tiff,
      destroy:       destroy_tiff,
    },
    read_header: read_header_tiff,
    read_image:  read_image_tiff,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;

