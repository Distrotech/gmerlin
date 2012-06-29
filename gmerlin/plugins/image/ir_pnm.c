/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <inttypes.h>
#include <gmerlin/plugin.h>
#include <math.h>
#include <ctype.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gavl/metatags.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "ir_pnm"

#define Bits_16  (1<<8)

#define PBMascii      1
#define PGMascii      2
#define PPMascii      3
#define PBMbin        4
#define PGMbin        5
#define PPMbin        6
#define PAM           7

#define PGMascii_16   (PGMascii | Bits_16) 
#define PPMascii_16   (PPMascii | Bits_16)
#define PGMbin_16     (PGMbin   | Bits_16)
#define PPMbin_16     (PPMbin   | Bits_16)

typedef struct
  {
  uint8_t *buffer;
  int buffer_alloc;
  
  uint8_t *buffer_ptr;
  uint8_t *buffer_end;
  int is_pnm;
  int width;
  int height;
  int maxval;

  gavl_metadata_t m;
  } pnm_t;




static void * create_pnm()
  {
  pnm_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_pnm(void* priv) 
  {
  pnm_t * pnm = priv;
  if(pnm->buffer)
    free(pnm->buffer);
  gavl_metadata_free(&pnm-> m);
  free(pnm);
  
  }

static int read_header_pnm(void *priv,const char *filename, gavl_video_format_t * format)
  {
  FILE *pnm_file;
  char *ptr;
  char *end_ptr;
  pnm_t *p = priv;
  unsigned long size;

  p->is_pnm = 0;
  p->width = 0;
  p->height = 0;
  p->maxval = 0;
  
  /* open file */
  pnm_file = fopen(filename,"rb");
    
  if(!pnm_file)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,"Cannot open file %s: %s",
           filename, strerror(errno));
    return 0;
    }
  
  /* read filesize */
  fseek(pnm_file, 0, SEEK_END);
  if((size = ftell(pnm_file))<0) return 1;
  fseek(pnm_file, 0, SEEK_SET);


  if(p->buffer_alloc < size)
    {
    p->buffer_alloc = size + 1024;
    p->buffer = realloc(p->buffer, size);
    }
  
  if(fread(p->buffer, 1, size, pnm_file)!=size)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,"Can't read File type");
    fclose(pnm_file);
    return 0;
    }

  fclose(pnm_file);
  
  p->buffer_ptr = p->buffer;
  p->buffer_end = p->buffer + size;

  ptr = (char*)p->buffer;
  end_ptr = (char*)(p->buffer+1);

  if(*ptr != 'P')
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "File is no pnm");
    return 0;
    }
  
  while(1)
    {
    while(*end_ptr != '\n')
      end_ptr++;
    
    if(*ptr == 'P')
      {
      ptr++;
      if((*ptr == '1') || (*ptr == '2') || (*ptr == '3') ||
         (*ptr == '4') || (*ptr == '5') || (*ptr == '6') || (*ptr == '7'))
       
        {
        p->is_pnm = atoi(ptr);
        ptr ++;
        }
      else
        break;
   
      if(!isspace(*ptr)) 
        {
        if(*ptr != '#')
          {
          bg_log(BG_LOG_ERROR, LOG_DOMAIN,"File is no pnm (%.1s%.1s%.1s)",ptr-1,ptr, ptr+1);
          p->is_pnm = 0;
          break;
          }
        }
      }
           
    if(*ptr == '#')
      {
      ptr = end_ptr;
      }

    if((isdigit(*ptr)) && (p->width == 0))
      {
      if(!isspace(*(ptr-1))) 
        {
        p->is_pnm = 0;
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,"Cannot read width");
        break;
        }
      p->width = atoi(ptr);
      while(isdigit(*ptr))
        ptr ++;
      }

    if((isdigit(*ptr)) && (p->width != 0) && (p->height == 0))
      {
      if(!isspace(*(ptr-1))) 
        {
        p->is_pnm = 0;
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,"Cannot read height");
        break;
        }
      p->height = atoi(ptr);
      while(isdigit(*ptr))
        ptr ++;
      }
   
    if(((p->is_pnm == 2) || (p->is_pnm == 3) || (p->is_pnm == 5) || (p->is_pnm == 6) || (p->is_pnm == 7)) && (p->width != 0) && (p->height != 0) && isdigit(*ptr))
      {
      if(!isspace(*(ptr-1))) 
        {
        p->is_pnm = 0;
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,"Cannot read maxval");
        break;
        }

      p->maxval = atoi(ptr);
      while(ptr != end_ptr+1)
        ptr ++;
      break;
      }
    
    if(((p->is_pnm == 1) || (p->is_pnm == 4)) && ((p->width != 0) && (p->height != 0)))
      {
      ptr ++;
      break;
      }
      
    if(end_ptr <= ptr)
      end_ptr++;

    ptr++;
    }

  if(p->is_pnm == 7)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,"PAM format not suported");
    return 0;
    }
  
  if(p->is_pnm == 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,"File is no pnm (%.1s%.1s%.1s)",ptr-1,ptr, ptr+1);
    return 0;
    }

  p->buffer_ptr = (uint8_t*)ptr;
     
  /* set gavl_video_format */

  if((p->is_pnm == PGMbin) || (p->is_pnm == PGMascii))
    {
    if(p->maxval > 255)
      {
      format->pixelformat = GAVL_GRAY_16;
      p->is_pnm |= Bits_16;
      }
    else
      {
      format->pixelformat = GAVL_GRAY_8;
      }
    }
  else
    {
    if(p->maxval > 255)
      {
      format->pixelformat = GAVL_RGB_48;
      p->is_pnm |= Bits_16;
      }
    else
      {
      format->pixelformat = GAVL_RGB_24;
      }
    }
  format->frame_width  = p->width;
  format->frame_height = p->height;
  
  format->image_width  = format->frame_width;
  format->image_height = format->frame_height;
  
  format->pixel_width = 1;
  format->pixel_height = 1;

  if((p->is_pnm == PGMbin) || (p->is_pnm == PGMascii))
    gavl_metadata_set(&p->m, GAVL_META_FORMAT, "PGM");
  else if((p->is_pnm == PBMbin) || (p->is_pnm == PBMascii))
    gavl_metadata_set(&p->m, GAVL_META_FORMAT, "PBM");
  else
    gavl_metadata_set(&p->m, GAVL_META_FORMAT, "PPM");
  return 1;
  }

#define INC_PTR { p->buffer_ptr++; if(p->buffer_ptr >= p->buffer_end) return 1; }

static int read_image_pnm(void *priv, gavl_video_frame_t *frame)
  {
  int x, y;
  uint32_t byte;
  uint8_t * frame_ptr;
  uint16_t * frame_ptr_16;
  uint8_t * frame_ptr_start;
  pnm_t *p = priv;
  uint8_t pixels_8, mask;

  if(!frame)
    return 1;

  /* PBM ascii */
  if(p->is_pnm == PBMascii)
    {
#ifdef DEBUG
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "PBMascii");
#endif
    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        while(!isdigit(*p->buffer_ptr))
           INC_PTR;
        byte = atoi((char*)p->buffer_ptr);
        if(byte == 0)
          byte = 0xff;
        if(byte == 1)
          byte = 0x00;
        frame_ptr[0]= byte;
        frame_ptr[1]= byte;
        frame_ptr[2]= byte;
        frame_ptr += 3;
        INC_PTR;
        while(isdigit(*p->buffer_ptr))
           INC_PTR;
        }
      frame_ptr_start += frame->strides[0];
      }
    }  


  /* PBM binaer */
  if(p->is_pnm == PBMbin)
    {
#ifdef DEBUG
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "PBMbin");
#endif
    frame_ptr_start = frame->planes[0];
        
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;

      pixels_8 = *(p->buffer_ptr);
      INC_PTR;
      mask = 0x80;
            
      for (x = 0; x < p->width; x++)
        {
        if(!mask)
          {
          pixels_8 = *(p->buffer_ptr);
          INC_PTR;
          mask = 0x80;
          }
        byte = pixels_8 & mask ? 0x00 : 0xff;
        frame_ptr[0]= byte;
        frame_ptr[1]= byte;
        frame_ptr[2]= byte;
        frame_ptr += 3;

        mask >>= 1;
        }
      
      frame_ptr_start += frame->strides[0];
      }
    }

  
  /* PGM ascii */
  if(p->is_pnm == PGMascii)
    {
#ifdef DEBUG
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "PGMascii");
#endif
    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi((char*)p->buffer_ptr);
        byte = (byte * 255)/p->maxval;
        frame_ptr[0]= byte;
        frame_ptr++;
        INC_PTR;
        while(isdigit(*p->buffer_ptr))
          INC_PTR;
        }
      frame_ptr_start += frame->strides[0];
      }
    }

  if(p->is_pnm == PGMascii_16)
    {
#ifdef DEBUG
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "PGMascii_16");
#endif
    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr_16 = (uint16_t*)frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi((char*)p->buffer_ptr);
        byte = (byte * 65535)/p->maxval;
        frame_ptr_16[0]= byte;
        frame_ptr_16++;
        INC_PTR;
        while(isdigit(*p->buffer_ptr))
          INC_PTR;
        }
      frame_ptr_start += frame->strides[0];
      }
    }


  /* PGM binaer */
  if(p->is_pnm == PGMbin)
    {
#ifdef DEBUG
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "PGMbin");
#endif

    frame_ptr_start = frame->planes[0];
        
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;
      for (x = 0; x < p->width ; x++)
        {
        byte = *(p->buffer_ptr);
        byte = (byte * 255)/p->maxval;
        frame_ptr[0]= byte;
        frame_ptr++;
        INC_PTR;
        }
      frame_ptr_start += frame->strides[0];
      }
    }

  if(p->is_pnm == PGMbin_16)
    {
#ifdef DEBUG
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "PGMbin_16");
#endif
    frame_ptr_start = frame->planes[0];
        
    for (y = 0; y < p->height; y++)
      {
      frame_ptr_16 = (uint16_t*)frame_ptr_start;
      for (x = 0; x < p->width ; x++)
        {
        byte = (p->buffer_ptr[0] << 8) | p->buffer_ptr[1];
        byte = (byte * 65535)/p->maxval;
        frame_ptr_16[0]= byte;
        frame_ptr_16++;
        INC_PTR;
        INC_PTR;
        }
      frame_ptr_start += frame->strides[0];
      }
    }


  /* PPM ascii */
  if(p->is_pnm == PPMascii)
    {
#ifdef DEBUG
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "PPMascii");
#endif
    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi((char*)p->buffer_ptr);
        byte = (byte * 255)/p->maxval;
        frame_ptr[0]= byte;
        INC_PTR;
        while(!isspace(*p->buffer_ptr))
          INC_PTR;
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi((char*)p->buffer_ptr);
        byte = (byte * 255)/p->maxval;
        frame_ptr[1]= byte;
        INC_PTR;
        while(!isspace(*p->buffer_ptr))
          INC_PTR;
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi((char*)p->buffer_ptr);
        byte = (byte * 255)/p->maxval;
        frame_ptr[2]= byte;
        INC_PTR;
        while(!isspace(*p->buffer_ptr))
          INC_PTR;
        frame_ptr += 3;
        }
      frame_ptr_start += frame->strides[0];
      }
    }

  if(p->is_pnm == PPMascii_16)
    {
#ifdef DEBUG
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "PPMascii_16");
#endif
    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr_16 = (uint16_t*)frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi((char*)p->buffer_ptr);
        byte = (byte * 65535)/p->maxval;
        frame_ptr_16[0]= byte;
        INC_PTR;
        while(!isspace(*p->buffer_ptr))
          INC_PTR;
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi((char*)p->buffer_ptr);
        byte = (byte * 65535)/p->maxval;
        frame_ptr_16[1]= byte;
        INC_PTR;
        while(!isspace(*p->buffer_ptr))
          INC_PTR;
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi((char*)p->buffer_ptr);
        byte = (byte * 65535)/p->maxval;
        frame_ptr_16[2]= byte;
        INC_PTR;
        while(!isspace(*p->buffer_ptr))
          INC_PTR;
        frame_ptr_16 += 3;
        }
      frame_ptr_start += frame->strides[0];
      }
    }

  /* PPM binaer */
  if(p->is_pnm == PPMbin)
    {
    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        byte = *(p->buffer_ptr);
        byte = (byte * 255)/p->maxval;


        frame_ptr[0]= byte;
        INC_PTR;
        byte = *(p->buffer_ptr);
        byte = (byte * 255)/p->maxval;


        frame_ptr[1]= byte;
        INC_PTR;
        byte = *(p->buffer_ptr);
        byte = (byte * 255)/p->maxval;

                
        frame_ptr[2]= byte;
        INC_PTR;
        frame_ptr += 3;
        }
      frame_ptr_start += frame->strides[0];
      }
    }
  
  if(p->is_pnm == PPMbin_16)
    {
#ifdef DEBUG
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "PPMbin_16");
#endif

    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr_16 = (uint16_t*)frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        byte = (p->buffer_ptr[0] << 8) | p->buffer_ptr[1];
        byte = (byte * 65535)/p->maxval;
        frame_ptr_16[0]= byte;
        INC_PTR;
        INC_PTR;
        byte = (p->buffer_ptr[0] << 8) | p->buffer_ptr[1];
        byte = (byte * 65535)/p->maxval;
        frame_ptr_16[1]= byte;
        INC_PTR;
        INC_PTR;
        byte = (p->buffer_ptr[0] << 8) | p->buffer_ptr[1];
        byte = (byte * 65535)/p->maxval;
        frame_ptr_16[2]= byte;
        INC_PTR;
        INC_PTR;
        frame_ptr_16 += 3;
        }
      frame_ptr_start += frame->strides[0];
      }
    }
  
  return 1;
  }

static const gavl_metadata_t * get_metadata_pnm(void * priv)
  {
  pnm_t *p = priv;
  return &p->m;
  }


const bg_image_reader_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "ir_pnm",
      .long_name =     TRS("PNM reader"),
      .description =    TRS("Reader for PBM/PGM/PPM images"),
      .type =          BG_PLUGIN_IMAGE_READER,
      .flags =         BG_PLUGIN_FILE,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        create_pnm,
      .destroy =       destroy_pnm,
    },
    .extensions =    "pnm ppm pbm pgm",
    .read_header = read_header_pnm,
    .get_metadata = get_metadata_pnm,
    .read_image =  read_image_pnm,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;

