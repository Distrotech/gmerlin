/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

/* Modelled roughly after the code found in imlib2 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <config.h>
#include <translation.h>

#include <plugin.h>
#include <log.h>
#define LOG_DOMAIN "ir_bmp"


#define BI_RGB       0
#define BI_RLE8      1
#define BI_RLE4      2
#define BI_BITFIELDS 3

typedef struct tagRGBQUAD
  {
  unsigned char       rgbBlue;
  unsigned char       rgbGreen;
  unsigned char       rgbRed;
  unsigned char       rgbReserved;
  } RGBQUAD;

typedef struct
  {
  FILE *bmp_file;
  unsigned long offset, comp, imgsize;
  unsigned short bitcount;
  unsigned long width, height;
  unsigned long y_per_meter, x_per_meter;
  RGBQUAD rgbQuads[256];
  } bmp_t;


static int ReadleShort(FILE * file, unsigned short *ret)
  {
  unsigned char       b[2];
  
  if (fread(b, sizeof(unsigned char), 2, file) != 2)
    return 0;
  
  *ret = (b[1] << 8) | b[0];
  return 1;
  }

static int ReadleLong(FILE * file, unsigned long *ret)
  {
  unsigned char       b[4];
  
  if (fread(b, sizeof(unsigned char), 4, file) != 4)
    return 0;
  
  *ret = (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
  return 1;
  }

static void * create_bmp()
  {
  bmp_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_bmp(void* priv) 
  {
  bmp_t * bmp = (bmp_t*)priv;
  if(bmp)
    free(bmp);
  }

static struct 
  {
  int bits;
  uint32_t r_mask;
  uint32_t g_mask;
  uint32_t b_mask;
  gavl_pixelformat_t csp;
  }
  pixelformats[] = 
    {
      {
        1,
        0x00FF,
        0x00FF,
        0x00FF,
        GAVL_RGB_32
      },
      {
        4,
        0x00FF,
        0x00FF,
        0x00FF,
        GAVL_RGB_32
      },
      {
        8,
        0x00FF,
        0x00FF,
        0x00FF,
        GAVL_RGB_32
      },
      {
        16,
        0x001F, 
        0x07E0, 
        0xF800, 
        GAVL_RGB_16
      },
      {
        16,
        0xF800, 
        0x07E0, 
        0x001F, 
        GAVL_BGR_16
      },
      {
        16,
        0x001F, 
        0x03E0, 
        0x7C00, 
        GAVL_RGB_15
      },
      {
        16,
        0x7C00, 
        0x03E0, 
        0x001F, 
        GAVL_BGR_15
      },
      {
        24,
        0x000000FF,
        0x0000FF00,
        0x00FF0000,
        GAVL_BGR_24
      },
      {
        24,
        0x00FF0000,
        0x0000FF00,
        0x000000FF,
        GAVL_RGB_24
      },
      {
        32,
        0x00FF0000,
        0x0000FF00,
        0x000000FF,
        GAVL_RGB_32
      },
      {
        32,
        0x000000FF,
        0x0000FF00,
        0x00FF0000,
        GAVL_BGR_32
      },
      { 
        0, 0, 0, 0, GAVL_PIXELFORMAT_NONE
      } 
    
    };


static gavl_pixelformat_t get_csp(int bits, uint32_t r_mask,
                          uint32_t g_mask, uint32_t b_mask) 
  {
  int i = 0;
  while(pixelformats[i].csp != GAVL_PIXELFORMAT_NONE)
    { 
    if((bits == pixelformats[i].bits) && 
       (r_mask == pixelformats[i].r_mask) && 
       (g_mask == pixelformats[i].g_mask) && 
       (b_mask == pixelformats[i].b_mask)) 
      return pixelformats[i].csp; 
    i++;
    } 
  return GAVL_PIXELFORMAT_NONE;
  }

static int read_header_bmp(void *priv,const char *filename, gavl_video_format_t * format)
  {
  int i;
  bmp_t *p = (bmp_t*)priv;
  char type[2];
  unsigned short ncols, tmpShort = 0, planes;
  unsigned long headSize = 0, size;
  unsigned long rmask, gmask, bmask;
    
  rmask = 0xff;
  gmask = 0xff;
  bmask = 0xff;

  p->bmp_file = fopen(filename,"rb");
    
  if(!p->bmp_file)
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Can't open File %s", filename);

  /* read filesize */
  
  fseek(p->bmp_file, 0, SEEK_END);
  if((size = ftell(p->bmp_file))<0) return 1;
  fseek(p->bmp_file, 0, SEEK_SET);

  if(fread(type, 1, 2, p->bmp_file)!=2)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,"Can't read File type");
    fclose(p->bmp_file);
    return 0;
    }
  if(strncmp(type, "BM", 2))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,"File isn't a BMP");
    fclose(p->bmp_file);
    return 0;
    }

  /* read header */
  
  fseek(p->bmp_file, 8, SEEK_CUR);
  ReadleLong(p->bmp_file, &p->offset);
  ReadleLong(p->bmp_file, &headSize);
  
  if (headSize == 12)
    {
    ReadleShort(p->bmp_file, &tmpShort);
    p->width = tmpShort;
    ReadleShort(p->bmp_file, &tmpShort);
    p->height = tmpShort;
    ReadleShort(p->bmp_file, &planes);
    ReadleShort(p->bmp_file, &p->bitcount);
    p->imgsize = size - p->offset;
    p->comp = BI_RGB;
    }
  else if (headSize == 40)
    {
    ReadleLong(p->bmp_file, &p->width);
    ReadleLong(p->bmp_file, &p->height);
    ReadleShort(p->bmp_file, &planes);
    ReadleShort(p->bmp_file, &p->bitcount);
    ReadleLong(p->bmp_file, &p->comp);
    ReadleLong(p->bmp_file, &p->imgsize);
    p->imgsize = size - p->offset;
    fseek(p->bmp_file, 16, SEEK_CUR);
    }
  else
    {
    fclose(p->bmp_file);
    return 0;
    }

  /* check img- width / height */
  
  if ((p->width < 1) || (p->height < 1) || (p->width > 8192) || (p->height > 8192))
    {
    fclose(p->bmp_file);
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,"Can't detect width or height");
    return 0;
    }

  /* check bitcount */
  
  if ((p->bitcount != 1) && (p->bitcount != 4) && (p->bitcount != 8) && (p->bitcount != 16) && (p->bitcount != 24) && (p->bitcount != 32))
    {
    fclose(p->bmp_file);
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,"Bitcount not suported (bitcount: %d)", p->bitcount);
    return 0;
    }
  
  if(p->bitcount < 16)
    {
    ncols = (p->offset - headSize - 14);
    if (headSize == 12)
      {
      ncols /= 3;
      if (ncols > 256)ncols = 256;
      for (i = 0; i < ncols; i++)
        fread(&p->rgbQuads[i], 3, 1, p->bmp_file);
      }
    else
      {
      ncols /= 4;
      if (ncols > 256)ncols = 256;
      fread(p->rgbQuads, 4, ncols, p->bmp_file);
      }
    }
  else
    {
    if (p->comp == BI_BITFIELDS)
      {
      ReadleLong(p->bmp_file, &bmask);
      ReadleLong(p->bmp_file, &gmask);
      ReadleLong(p->bmp_file, &rmask);
      }
    else if (p->bitcount == 16)
      {
      rmask = 0x7C00;
      gmask = 0x03E0;
      bmask = 0x001F;
      }
    else if ((p->bitcount == 32) || (p->bitcount == 24))
      {
      rmask = 0x000000FF;
      gmask = 0x0000FF00;
      bmask = 0x00FF0000;
      }
    }

  /* set gavl format */
  
  format->pixelformat =  get_csp(p->bitcount, rmask, gmask, bmask);
  
  format->frame_width  = p->width;
  format->frame_height = p->height;
  
  format->image_width  = format->frame_width;
  format->image_height = format->frame_height;
  format->pixel_width = 1;
  format->pixel_height = 1;
  
  return 1;
  }

static int read_image_bmp(void *priv, gavl_video_frame_t *frame)
  {
  int keep_going;
  bmp_t *p = (bmp_t*)priv;
  uint8_t * frame_ptr;
  uint8_t * frame_ptr_start;
  unsigned char byte = 0;
  unsigned char color_byte = 0;
  unsigned char zaehl_byte = 0;
  unsigned char *buffer_ptr, *buffer, *buffer_end;
  unsigned char t1 = 0x00, t2 = 0x00;
  unsigned long k;
  unsigned short x, y, i, width, skip;

  /* read file to header */
  
  fseek(p->bmp_file, p->offset, SEEK_SET);

  buffer = malloc(p->imgsize);
  
  if (!buffer)
    {
    fclose(p->bmp_file);
    return 0;
    }
  
  fread(buffer, 1, p->imgsize, p->bmp_file);
  fclose(p->bmp_file);
  
  buffer_ptr = buffer;
  buffer_end = buffer + p->imgsize;

  /**** Bitcount 1 ****/

  if(p->bitcount == 1)
    {
    if (p->comp == BI_RGB)
      {
      frame_ptr_start = frame->planes[0] + (p->height - 1) * frame->strides[0]; 
      skip = ((((p->width + 31) / 32) * 32) - p->width) / 8;
  
      for (y = 0; y < p->height; y++)
        {
        frame_ptr = frame_ptr_start; 
        
        for (x = 0; x < p->width; x++)
          {
          if ((x & 7) == 0)
            byte = *(buffer_ptr++);
          k = (byte >> 7) & 1;

          frame_ptr[0] = p->rgbQuads[k].rgbRed;
          frame_ptr[1] = p->rgbQuads[k].rgbGreen;
          frame_ptr[2] = p->rgbQuads[k].rgbBlue;
          frame_ptr[3] = 0xff;
          byte <<= 1;
          frame_ptr += 4;
          }
        buffer_ptr +=  skip;
        frame_ptr_start -= frame->strides[0];
        }
      }
    }

  /**** Bitcount 4 ****/

  if(p->bitcount == 4)
    {
    if (p->comp == BI_RGB)
      {
      frame_ptr_start = frame->planes[0] + (p->height - 1) * frame->strides[0]; 
      skip = ((((p->width + 7) / 8) * 8) - p->width) / 2;
      for (y = 0; y < p->height; y++)
        {
        frame_ptr = frame_ptr_start; 
        for (x = 0; x < p->width ; x++)
          {
          if ((x & 1) == 0)
            byte = *(buffer_ptr++);
          k = (byte & 0xF0) >> 4;
          frame_ptr[0] = p->rgbQuads[k].rgbRed;
          frame_ptr[1] = p->rgbQuads[k].rgbGreen;
          frame_ptr[2] = p->rgbQuads[k].rgbBlue;
          frame_ptr[3] = 0xff;
          byte <<= 4;
          frame_ptr += 4;
          }
        buffer_ptr +=  skip;
        frame_ptr_start -= frame->strides[0];
        }
      }
    else if (p->comp == BI_RLE4)
      {
      keep_going = 1;
      frame_ptr_start = frame->planes[0] + (p->height - 1) * frame->strides[0]; 
      frame_ptr = frame_ptr_start;
      x = 0;
      y = 0;
      
      while((buffer_ptr < buffer_end) && keep_going)
        {
        if((int)(buffer_ptr - buffer) % 2)
          buffer_ptr++;

        zaehl_byte = *(buffer_ptr++);
        color_byte = *(buffer_ptr++);
       
        if(zaehl_byte == 0)
          {
          switch(color_byte)
            {
            case 0:
              /* End of line */
              y++;
              x = 0;
              frame_ptr_start -= frame->strides[0];
              frame_ptr = frame_ptr_start;
              break;
            case 1:
              /* End of RLE */
              keep_going = 0;
              break;
            case 2:
              /* Switch Cursor */
              x += *(buffer_ptr++);
              y -= *(buffer_ptr++);
              break;
            default:
              /* Single pixels with different colors */
              width = color_byte;
             
              for (i = 0; i < width; i++)
                {
                if ((i & 1) == 0)
                  {
                  color_byte = *(buffer_ptr++);
                  t1 = color_byte & 0xF;
                  t2 = (color_byte >> 4) & 0xF;
                  }

                color_byte = (i & 1) ? t1 : t2;
                frame_ptr[0] = p->rgbQuads[color_byte].rgbRed;
                frame_ptr[1] = p->rgbQuads[color_byte].rgbGreen;
                frame_ptr[2] = p->rgbQuads[color_byte].rgbBlue;
                frame_ptr[3] = 0xff;
                frame_ptr += 4;
                x ++;
                if(x >= p->width)
                  {
                  buffer_ptr += ((width - i)/2);
                  break;
                  }
                }
              break;
            }
          }
        else
          {
          /* Pixel with the same color */
          width = zaehl_byte;

          t1 = color_byte & 0xF;
          t2 = (color_byte >> 4) & 0xF;

          for (i = 0; i < width; i++)
            {
            color_byte = (i & 1) ? t1 : t2;
            frame_ptr[0] = p->rgbQuads[color_byte].rgbRed;
            frame_ptr[1] = p->rgbQuads[color_byte].rgbGreen;
            frame_ptr[2] = p->rgbQuads[color_byte].rgbBlue;
            frame_ptr[3] = 0xff;
            frame_ptr += 4;
            x ++;
            if(x >= p->width)
              {
              break;
              }
            }
          }
        }
      }
    }
 
  /**** Bitcount 8 ****/

  if(p->bitcount == 8)
    {
    if (p->comp == BI_RGB)
      {
      frame_ptr_start = frame->planes[0] + (p->height - 1) * frame->strides[0]; 
      skip = (((p->width + 3) / 4) * 4) - p->width;

      for (y = 0; y < p->height; y++)
        {

        frame_ptr = frame_ptr_start; 
        for (x = 0; x < p->width; x++)
          {
          byte = *(buffer_ptr++);
          frame_ptr[0] = p->rgbQuads[byte].rgbRed;
          frame_ptr[1] = p->rgbQuads[byte].rgbGreen;
          frame_ptr[2] = p->rgbQuads[byte].rgbBlue;
          frame_ptr[3] = 0xff;
          frame_ptr += 4;
          }
        buffer_ptr +=  skip;
        frame_ptr_start -= frame->strides[0];
        }
      }
    else if (p->comp == BI_RLE8)
      {
      keep_going = 1;
      frame_ptr_start = frame->planes[0] + (p->height - 1) * frame->strides[0]; 
      frame_ptr = frame_ptr_start;
      x = 0;
      y = 0;
      
      while((buffer_ptr < buffer_end) && keep_going)
        {
        if((int)(buffer_ptr - buffer) % 2)
          buffer_ptr++;

        zaehl_byte = *(buffer_ptr++);
        color_byte = *(buffer_ptr++);
       
        if(zaehl_byte == 0)
          {
          switch(color_byte)
            {
            case 0:
              /* End of line */
              y++;
              x = 0;
              frame_ptr_start -= frame->strides[0];
              frame_ptr = frame_ptr_start;
              break;
            case 1:
              /* End of RLE */
              keep_going = 0;
              break;
            case 2:
              /* Switch Cursor */
              x += *(buffer_ptr++);
              y -= *(buffer_ptr++);
              break;
            default:
              /* Single pixel with different colors */
              width = color_byte;
              
              for (i = 0; i < width; i++)
                {
                color_byte = *(buffer_ptr++);
                frame_ptr[0] = p->rgbQuads[color_byte].rgbRed;
                frame_ptr[1] = p->rgbQuads[color_byte].rgbGreen;
                frame_ptr[2] = p->rgbQuads[color_byte].rgbBlue;
                frame_ptr[3] = 0xff;
                frame_ptr += 4;
                x ++;
                if(x >= p->width)
                  {
                  buffer_ptr += (width - i - 1);
                  break;
                  }
                }
              break;
            }
          }
        else
          {
          /* Pixel with the same color */
          width = zaehl_byte;
          
          for (i = 0; i < width; i++)
            {
            frame_ptr[0] = p->rgbQuads[color_byte].rgbRed;
            frame_ptr[1] = p->rgbQuads[color_byte].rgbGreen;
            frame_ptr[2] = p->rgbQuads[color_byte].rgbBlue;
            frame_ptr[3] = 0xff;
            frame_ptr += 4;
            x ++;
            if(x >= p->width)
              {
              break;
              }
            }
          }
        }
      }
    }
     
  /**** Bitcount 16 ****/
    
  if (p->bitcount == 16)
    { 
    frame_ptr_start = frame->planes[0] + (p->height - 1) * frame->strides[0]; 
    skip = (((p->width * 16 + 31) / 32) * 4) - (p->width * 2); 
    
    for (y = 0; y < p->height; y++) 
      {
      frame_ptr = frame_ptr_start; 
      
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
      memcpy(frame_ptr, buffer_ptr, p->width * 2);
      buffer_ptr += p->width * 2;
#else
      
      for (x = 0; x < p->width ; x++)
        {
        frame_ptr[1]= *(buffer_ptr++);
        frame_ptr[0]= *(buffer_ptr++);
        frame_ptr += 2;
        }
#endif
      
      buffer_ptr +=  skip;
      frame_ptr_start -= frame->strides[0];
      }
    }

  /**** Bitcount 24 ****/

  if (p->bitcount == 24)
    {
    frame_ptr_start = frame->planes[0] + (p->height - 1) * frame->strides[0];
    skip = (4 - ((p->width * 3) % 4)) & 3;
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start ;
#if 0
      for (x = 0; x < p->width ; x++)
        {
        frame_ptr[0]= *(buffer_ptr++);
        frame_ptr[1]= *(buffer_ptr++);
        frame_ptr[2]= *(buffer_ptr++);
        frame_ptr += 3;
        }
      buffer_ptr += skip;
#else
      memcpy(frame_ptr, buffer_ptr, p->width * 3);
      buffer_ptr += (p->width * 3 + skip);
#endif
      frame_ptr_start -= frame->strides[0];
      }
    }
  
  /**** Bitcount 32 ****/
  
  else if (p->bitcount == 32)
    {
    frame_ptr_start = frame->planes[0] + (p->height - 1) * frame->strides[0];
    skip = (((p->width * 32 + 31) / 32) * 4) - (p->width * 4);
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start ;
#if 0
      for (x = 0; x < p->width; x++)
        {
        frame_ptr[0]= *(buffer_ptr++);
        frame_ptr[1]= *(buffer_ptr++);
        frame_ptr[2]= *(buffer_ptr++);
        buffer_ptr++;
        frame_ptr += 4;
        }
      buffer_ptr += skip;
#else
      memcpy(frame_ptr, buffer_ptr, p->width * 4);
      buffer_ptr += (p->width * 4 + skip);
#endif
      frame_ptr_start -= frame->strides[0];
      }
    }

  if(buffer)
    free(buffer);
  
  return 1;
  }

bg_image_reader_plugin_t the_plugin =
  {
    common:
    {
      BG_LOCALE,
      name:          "ir_bmp",
      long_name:     TRS("BMP reader"),
      description:    TRS("Reader for BMP images"),
      mimetypes:     (char*)0,
      extensions:    "bmp",
      type:          BG_PLUGIN_IMAGE_READER,
      flags:         BG_PLUGIN_FILE,
      priority:      BG_PLUGIN_PRIORITY_MAX,
      create:        create_bmp,
      destroy:       destroy_bmp,
    },
    read_header: read_header_bmp,
    read_image:  read_image_bmp,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;

