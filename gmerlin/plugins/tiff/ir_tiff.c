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
  uint16_t BitsPerSample;
  uint16 SamplesPerPixel;
  uint16 SampleFormat;
  uint16 Orientation;
  gavl_video_format_t format;
  TIFF * tiff;

  int is_planar;
  
  void (*convert_scanline)(uint8_t * dst, uint8_t * src, int width, int plane);
  

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

static void convert_scanline_RGB_16(uint8_t * dst, uint8_t * src, int width, int plane)
  {
  memcpy(dst, src, width * 3 * 2);
  }

static void convert_scanline_RGB_16_planar(uint8_t * _dst, uint8_t * _src, int width, int plane)
  {
  int i;
  uint16_t * dst = (uint16_t *)_dst;
  uint16_t * src = (uint16_t *)_src;

  dst += plane;
  for(i = 0; i < width; i++)
    {
    *dst = *src;
    dst += 3;
    src++;
    }
  }


static void convert_scanline_RGBA_16(uint8_t * dst, uint8_t * src, int width, int plane)
  {
  memcpy(dst, src, width * 4 * 2);
  }

/* Big/Little endian floating point routines taken from libsndfile */

#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
static float
float32_read (unsigned char *cptr)
{       int             exponent, mantissa, negative ;
        float   fvalue ;

        negative = cptr [3] & 0x80 ;
        exponent = ((cptr [3] & 0x7F) << 1) | ((cptr [2] & 0x80) ? 1 : 0) ;
        mantissa = ((cptr [2] & 0x7F) << 16) | (cptr [1] << 8) | (cptr [0]) ;

        if (! (exponent || mantissa))
                return 0.0 ;

        mantissa |= 0x800000 ;
        exponent = exponent ? exponent - 127 : 0 ;

        fvalue = mantissa ? ((float) mantissa) / ((float) 0x800000) : 0.0 ;

        if (negative)
                fvalue *= -1 ;

        if (exponent > 0)
                fvalue *= (1 << exponent) ;
        else if (exponent < 0)
                fvalue /= (1 << abs (exponent)) ;

        return fvalue ;
} /* float32_le_read */
static double
double64_read (unsigned char *cptr)
{       int             exponent, negative ;
        double  dvalue ;

        negative = (cptr [7] & 0x80) ? 1 : 0 ;
        exponent = ((cptr [7] & 0x7F) << 4) | ((cptr [6] >> 4) & 0xF) ;

        /* Might not have a 64 bit long, so load the mantissa into a double. */
        dvalue = (((cptr [6] & 0xF) << 24) | (cptr [5] << 16) | (cptr [4] << 8)
| cptr [3]) ;
        dvalue += ((cptr [2] << 16) | (cptr [1] << 8) | cptr [0]) / ((double) 0x1000000) ;

        if (exponent == 0 && dvalue == 0.0)
                return 0.0 ;

        dvalue += 0x10000000 ;

        exponent = exponent - 0x3FF ;

        dvalue = dvalue / ((double) 0x10000000) ;

        if (negative)
                dvalue *= -1 ;

        if (exponent > 0)
                dvalue *= (1 << exponent) ;
        else if (exponent < 0)
                dvalue /= (1 << abs (exponent)) ;

        return dvalue ;
} /* double64_le_read */
#else
static float
float32_read (unsigned char *cptr)
{       int             exponent, mantissa, negative ;
        float   fvalue ;

        negative = cptr [0] & 0x80 ;
        exponent = ((cptr [0] & 0x7F) << 1) | ((cptr [1] & 0x80) ? 1 : 0) ;
        mantissa = ((cptr [1] & 0x7F) << 16) | (cptr [2] << 8) | (cptr [3]) ;

        if (! (exponent || mantissa))
                return 0.0 ;

        mantissa |= 0x800000 ;
        exponent = exponent ? exponent - 127 : 0 ;

        fvalue = mantissa ? ((float) mantissa) / ((float) 0x800000) : 0.0 ;

        if (negative)
                fvalue *= -1 ;

        if (exponent > 0)
                fvalue *= (1 << exponent) ;
        else if (exponent < 0)
                fvalue /= (1 << abs (exponent)) ;

        return fvalue ;
} /* float32_be_read */
static double
double64_read (unsigned char *cptr)
{       int             exponent, negative ;
        double  dvalue ;

        negative = (cptr [0] & 0x80) ? 1 : 0 ;
        exponent = ((cptr [0] & 0x7F) << 4) | ((cptr [1] >> 4) & 0xF) ;

        /* Might not have a 64 bit long, so load the mantissa into a double. */
        dvalue = (((cptr [1] & 0xF) << 24) | (cptr [2] << 16) | (cptr [3] << 8)
| cptr [4]) ;
        dvalue += ((cptr [5] << 16) | (cptr [6] << 8) | cptr [7]) / ((double) 0x1000000) ;

        if (exponent == 0 && dvalue == 0.0)
                return 0.0 ;

        dvalue += 0x10000000 ;

        exponent = exponent - 0x3FF ;

        dvalue = dvalue / ((double) 0x10000000) ;

        if (negative)
                dvalue *= -1 ;

        if (exponent > 0)
                dvalue *= (1 << exponent) ;
        else if (exponent < 0)
                dvalue /= (1 << abs (exponent)) ;

        return dvalue ;
} /* double64_be_read */
#endif


static void convert_scanline_RGB_float_32(uint8_t * dst, uint8_t * src, int width, int plane)
  {
  }

static void convert_scanline_RGB_float_64(uint8_t * _dst, uint8_t * src, int width, int plane)
  {
  int i;
  float * dst = (float*)_dst;

  for(i = 0; i < width*3; i++)
    {
    *dst = double64_read(src);
    dst++;
    src += 8;
    }
  }

static void convert_scanline_RGB_float_64_planar(uint8_t * _dst, uint8_t * src, int width, int plane)
  {
  int i;
  float * dst = (float*)_dst;

  dst += plane;
  
  for(i = 0; i < width; i++)
    {
    *dst = double64_read(src);
    dst+=3;
    src += 8;
    }
  }


static void convert_scanline_RGBA_float_32(uint8_t * dst, uint8_t * src, int width, int plane)
  {
  }

static void convert_scanline_RGBA_float_64(uint8_t * dst, uint8_t * src, int width, int plane)
  {
  }


static int read_header_tiff(void *priv,const char *filename, gavl_video_format_t * format)
  {
  uint16_t tmp_16;
  tiff_t *p = (tiff_t*)priv;
  
  tiff_read_mem(priv, filename);

  if(!(p->tiff = open_tiff_mem("rm", p))) return 0;
  if(!(TIFFGetField(p->tiff, TIFFTAG_IMAGEWIDTH, &p->Width))) return 0;
  if(!(TIFFGetField(p->tiff, TIFFTAG_IMAGELENGTH, &p->Height))) return 0;
  if(!(TIFFGetField(p->tiff, TIFFTAG_SAMPLESPERPIXEL, &p->SamplesPerPixel)))p->SamplesPerPixel = 1;
  if(!(TIFFGetField(p->tiff, TIFFTAG_BITSPERSAMPLE, &p->BitsPerSample)))p->BitsPerSample = 1;
  if(!(TIFFGetField(p->tiff, TIFFTAG_ORIENTATION, &p->Orientation)))
    p->Orientation = ORIENTATION_TOPLEFT;

  if(!(TIFFGetField(p->tiff, TIFFTAG_SAMPLEFORMAT, &p->SampleFormat)))
    p->SampleFormat = SAMPLEFORMAT_UINT;

  if(!(TIFFGetField(p->tiff, TIFFTAG_PLANARCONFIG, &tmp_16)) || (tmp_16 == 1))
    p->is_planar = 0;
  else
    p->is_planar = 1;
  
  format->frame_width  = p->Width;
  format->frame_height = p->Height;

  format->image_width  = format->frame_width;
  format->image_height = format->frame_height;
  format->pixel_width = 1;
  format->pixel_height = 1;

  fprintf(stderr,"Filename:\t %s\n", filename);
  fprintf(stderr,"BitsPerSample:\t %d\n", p->BitsPerSample);
  fprintf(stderr,"SamplePerPixel:\t %d\n", p->SamplesPerPixel);  
  fprintf(stderr,"SampleFormat:\t %d\n", p->SampleFormat);  

  /* Check for format */

  if(p->BitsPerSample <= 8)
    {
    if(p->SamplesPerPixel == 4)
      format->pixelformat = GAVL_RGBA_32;
    else
      format->pixelformat = GAVL_RGB_24;
    }
  else /* High Precision (> 8 bits) */
    {
    switch(p->BitsPerSample)
      {
      case 16:
        switch(p->SampleFormat)
          {
          case SAMPLEFORMAT_UINT:
            /* GAVL_RGB_48 */
            if(p->SamplesPerPixel == 3)
              {
              if(p->is_planar)
                p->convert_scanline = convert_scanline_RGB_16_planar;
              else
                p->convert_scanline = convert_scanline_RGB_16;
              
              format->pixelformat = GAVL_RGB_48;
              }
            /* GAVL_RGB_64 */
            else if(p->SamplesPerPixel == 4)
              {
              p->convert_scanline = convert_scanline_RGBA_16;              
              format->pixelformat = GAVL_RGBA_64;
              }
            else
              {
              fprintf(stderr, "Unsupported samples per pixel\n");
              return 0;
              }
            
            break;
          case SAMPLEFORMAT_INT:
          case SAMPLEFORMAT_IEEEFP:
          case SAMPLEFORMAT_VOID:
          case SAMPLEFORMAT_COMPLEXINT:
          case SAMPLEFORMAT_COMPLEXIEEEFP:
            fprintf(stderr, "Unsupported sampleformat\n");
            return 0;
            break;
          }
        break;
      case 64:
        switch(p->SampleFormat)
          {
          case SAMPLEFORMAT_UINT:
          case SAMPLEFORMAT_INT:
            fprintf(stderr, "Unsupported sampleformat\n");
            return 0;
            break;
          case SAMPLEFORMAT_IEEEFP:
            if(p->is_planar)
              p->convert_scanline = convert_scanline_RGB_float_64_planar;
            else
              p->convert_scanline = convert_scanline_RGB_float_64;

            format->pixelformat = GAVL_RGB_FLOAT;
            break;
            
          case SAMPLEFORMAT_VOID:
          case SAMPLEFORMAT_COMPLEXINT:
          case SAMPLEFORMAT_COMPLEXIEEEFP:
            fprintf(stderr, "Unsupported sampleformat\n");
            return 0;
            break;
          }
        break;
      default:
        fprintf(stderr, "Unsupported bits per sample: %d\n", p->BitsPerSample);
        return 0;
      }
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
  tdata_t buf;
  int num_planes;

  num_planes = p->is_planar ? p->SamplesPerPixel : 1;
  
  p->buffer_position =0;

  if(p->BitsPerSample <= 8)
    {
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
    
    if(p->SamplesPerPixel == 4)
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
    }
  else
    {

    buf = _TIFFmalloc(TIFFScanlineSize(p->tiff));
    
    if(!p->convert_scanline)
      {
      fprintf(stderr, "BUG!!! convert_func == 0x0\n");
      return 0;
      }

    for(j = 0; j < num_planes; j++)
      {
      frame_ptr = frame->planes[0];
      for (i=0;i<p->Height; i++)
        {
        TIFFReadScanline(p->tiff, buf, i, j);
        
        p->convert_scanline(frame_ptr, buf, p->Width, j);
        frame_ptr += frame->strides[0];
        }
      }
    if (buf) _TIFFfree(buf);
    
    }

  
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

