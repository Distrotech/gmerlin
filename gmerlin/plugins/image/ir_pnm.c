
/*****************************************************************
  ir_pnm.c
 
  Copyright (c) 2001-2005 by Michael Gruenert - one78@web.de
 
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
#include <inttypes.h>
#include <plugin.h>
#include <math.h>
#include <ctype.h>

#define PBMascii      1
#define PGMascii      2
#define PPMascii      3
#define PBMbin        4
#define PGMbin        5
#define PPMbin        6
#define PAM           7

typedef struct
  {
  FILE *pnm_file;
  char *buffer;
  char *buffer_ptr;
  char *buffer_end;
  int is_pnm;
  int width;
  int height;
  int maxval;
  int depht;
  } pnm_t;




static void * create_pnm()
  {
  pnm_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_pnm(void* priv) 
  {
  pnm_t * pnm = (pnm_t*)priv;
  if(pnm)
    free(pnm);
  }

static int read_header_pnm(void *priv,const char *filename, gavl_video_format_t * format)
  {
  int line_lenght;
  char *ptr;
  char *end_ptr;
  pnm_t *p = (pnm_t*)priv;
  unsigned long size;

  p->is_pnm = 0;
 
  /* open file */
  p->pnm_file = fopen(filename,"rb");
    
  if(!p->pnm_file)
    {
    fprintf(stderr,"Can't open File %s\n", filename);
    return 0;
    }
  
  /* read filesize */
  fseek(p->pnm_file, 0, SEEK_END);
  if((size = ftell(p->pnm_file))<0) return 1;
  fseek(p->pnm_file, 0, SEEK_SET);

  fprintf(stderr,"filesize = %ld \n", size);

  p->buffer = calloc(size, sizeof(char));
  
  if(fread(p->buffer, 1, size, p->pnm_file)!=size)
    {
    fprintf(stderr,"Can't read File type\n");
    fclose(p->pnm_file);
    return 0;
    }

  fclose(p->pnm_file);
  
  p->buffer_ptr = p->buffer;
  p->buffer_end = p->buffer + size;

  ptr = p->buffer;
  end_ptr = p->buffer+1;

  if(*ptr != 'P')
    {
    fprintf(stderr,"File isn't a pnm\n");
    if(*p->buffer)
      free(p->buffer);
    return 0;
    }
  
  while(1)
    {
    while(*end_ptr != '\n')
      end_ptr++;
    
    line_lenght = end_ptr - ptr;
    //fprintf(stderr,"line_lenght: \t%d\n", line_lenght);
    
    if(*ptr == 'P')
      {
      ptr ++;
      if((*ptr == '1') || (*ptr == '2') || (*ptr == '3') ||
         (*ptr == '4') || (*ptr == '5') || (*ptr == '6') || (*ptr == '7'))
       
        {
        p->is_pnm = atoi(ptr);
        //fprintf(stderr,"File is a Pnm (%.1s%.1s) is_pnm: %d\n",ptr-1,ptr, p->is_pnm);
        ptr ++;
        }
      else
        break;
   
      if(!isspace(*ptr)) 
        {
        if(*ptr != '#')
          {
          fprintf(stderr,"File must not be a pnm (%.1s%.1s%.1s)\n",ptr-1,ptr, ptr+1);
          p->is_pnm = 0;
          break;
          }
        }
      }
           
    if(*ptr == '#')
      {
      //fprintf(stderr,"ignor line\n");
      ptr = end_ptr;
      }

    if((isdigit(*ptr)) && (p->width == 0))
      {
      if(!isspace(*(ptr-1))) 
        {
        p->is_pnm = 0;
        fprintf(stderr,"Can't read width\n");
        break;
        }
      p->width = atoi(ptr);
      //fprintf(stderr,"width:  %d\n",p->width);
      while(isdigit(*ptr))
        ptr ++;
      }

    if((isdigit(*ptr)) && (p->width != 0) && (p->height == 0))
      {
      if(!isspace(*(ptr-1))) 
        {
        p->is_pnm = 0;
        fprintf(stderr,"Can't read height\n");
        break;
        }
      p->height = atoi(ptr);
      //fprintf(stderr,"height: %d\n",p->height);
      while(isdigit(*ptr))
        ptr ++;
      }
   
    if(((p->is_pnm == 2) || (p->is_pnm == 3) || (p->is_pnm == 5) || (p->is_pnm == 6) || (p->is_pnm == 7)) && (p->width != 0) && (p->height != 0) && isdigit(*ptr))
      {
      if(!isspace(*(ptr-1))) 
        {
        p->is_pnm = 0;
        fprintf(stderr,"Can't read maxval\n");
        break;
        }

      p->maxval = atoi(ptr);
      //fprintf(stderr,"maxval: %d\n",p->maxval);
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
    fprintf(stderr,"Sorry not suported format \n");
    if(*p->buffer)
      free(p->buffer);
    return 0;
    }
  
  if(p->is_pnm == 0)
    {
    fprintf(stderr,"File isn't a pnm (%.1s%.1s%.1s)\n",ptr-1,ptr, ptr+1);
    if(*p->buffer)
      free(p->buffer);
    return 0;
    }

  p->buffer_ptr = ptr;
     
  /* set gavl_video_format */
  format->colorspace = GAVL_RGB_24;
  
  format->frame_width  = p->width;
  format->frame_height = p->height;
  
  format->image_width  = format->frame_width;
  format->image_height = format->frame_height;
  
  format->pixel_width = 1;
  format->pixel_height = 1;
  
  return 1;
  }

#define INC_PTR { p->buffer_ptr++; if(p->buffer_ptr >= p->buffer_end) return 1; }

static int read_image_pnm(void *priv, gavl_video_frame_t *frame)
  {
  int x, y;
  int byte;
  uint8_t * frame_ptr;
  uint8_t * frame_ptr_start;
  pnm_t *p = (pnm_t*)priv;
  uint8_t pixels_8, mask;
  

  /* PBM ascii */
  if(p->is_pnm == PBMascii)
    {
    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        while(!isdigit(*p->buffer_ptr))
           INC_PTR;
        byte = atoi(p->buffer_ptr);
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
    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi(p->buffer_ptr);
        byte = (byte * 255)/p->maxval;
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


  /* PGM binaer */
  if(p->is_pnm == PGMbin)
    {
    frame_ptr_start = frame->planes[0];
        
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;
      for (x = 0; x < p->width ; x++)
        {
        byte = *(p->buffer_ptr);
        byte = (byte * 255)/p->maxval;
        frame_ptr[0]= byte;
        frame_ptr[1]= byte;
        frame_ptr[2]= byte;
        frame_ptr += 3;
        INC_PTR;
        }
      frame_ptr_start += frame->strides[0];
      }
    }


  /* PPM ascii */
  if(p->is_pnm == PPMascii)
    {
    frame_ptr_start = frame->planes[0];
    
    for (y = 0; y < p->height; y++)
      {
      frame_ptr = frame_ptr_start;

      for (x = 0; x < p->width; x++)
        {
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi(p->buffer_ptr);
        byte = (byte * 255)/p->maxval;
        frame_ptr[0]= byte;
        INC_PTR;
        while(!isspace(*p->buffer_ptr))
          INC_PTR;
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi(p->buffer_ptr);
        byte = (byte * 255)/p->maxval;
        frame_ptr[1]= byte;
        INC_PTR;
        while(!isspace(*p->buffer_ptr))
          INC_PTR;
        while(!isdigit(*p->buffer_ptr))
          INC_PTR;
        byte = atoi(p->buffer_ptr);
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

  /* when ignor the maxval */
#if  0
  /* PPM binaer */
  if(p->is_pnm == PPMbin)
      {
      frame_ptr_start = frame->planes[0];
      
      for (y = 0; y < p->height; y++)
        {
        frame_ptr = frame_ptr_start ;
        memcpy(frame_ptr, p->buffer_ptr, p->width * 3);
        p->buffer_ptr += (p->width * 3);
        frame_ptr_start += frame->strides[0];
        }
      }
#endif

  if(*p->buffer)
    free(p->buffer);
  
  return 1;
  }

bg_image_reader_plugin_t the_plugin =
  {
    common:
    {
      name:          "ir_pnm",
      long_name:     "PNM loader ",
      mimetypes:     (char*)0,
      extensions:    "pnm ppm pbm pgm pam",
      type:          BG_PLUGIN_IMAGE_READER,
      flags:         BG_PLUGIN_FILE,
      priority:      BG_PLUGIN_PRIORITY_MAX,
      create:        create_pnm,
      destroy:       destroy_pnm,
    },
    read_header: read_header_pnm,
    read_image:  read_image_pnm,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;

