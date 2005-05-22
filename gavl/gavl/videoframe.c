/*****************************************************************

  videoframe.c

  Copyright (c) 2001 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include "gavl.h"

#include "video.h"
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Taken from a52dec (thanks guys) */

#ifdef HAVE_MEMALIGN
/* some systems have memalign() but no declaration for it */
void * memalign (size_t align, size_t size);
#else
/* assume malloc alignment is sufficient */
#define memalign(align,size) malloc (size)
#endif

#define ALIGNMENT_BYTES 8

#define ALIGN(a) a=((a+ALIGNMENT_BYTES-1)/ALIGNMENT_BYTES)*ALIGNMENT_BYTES

static void video_frame_alloc(gavl_video_frame_t * ret,
                              const gavl_video_format_t * format)
  {
  switch(format->colorspace)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      ret->strides[0] = format->frame_width*2;
      ALIGN(ret->strides[0]);
      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                             ret->strides[0] * format->frame_height);
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      ret->strides[0] = format->frame_width*3;
      ALIGN(ret->strides[0]);
      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                             ret->strides[0] * format->frame_height);
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
      ret->strides[0] = format->frame_width*4;
      ALIGN(ret->strides[0]);
      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                             ret->strides[0] * format->frame_height);
      break;
    case GAVL_RGBA_32:
      ret->strides[0] = format->frame_width*4;
      ALIGN(ret->strides[0]);
      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                             ret->strides[0] * format->frame_height);
      break;
    case GAVL_YUY2:
    case GAVL_UYVY:
      ret->strides[0] = format->frame_width*2;
      ALIGN(ret->strides[0]);
      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                             ret->strides[0] * format->frame_height);
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUVJ_420_P:
      ret->strides[0] = format->frame_width;
      ret->strides[1] = format->frame_width/2;
      ret->strides[2] = format->frame_width/2;
      ALIGN(ret->strides[0]);
      ALIGN(ret->strides[1]);
      ALIGN(ret->strides[2]);
      
      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                             ret->strides[0]*format->frame_height+
                             (ret->strides[1]*format->frame_height)/2+
                             (ret->strides[2]*format->frame_height)/2);
      ret->planes[1] = ret->planes[0] + ret->strides[0]*format->frame_height;
      ret->planes[2] = ret->planes[1] + (ret->strides[1]*format->frame_height)/2;
      break;
    case GAVL_YUV_410_P:
      ret->strides[0] = format->frame_width;
      ret->strides[1] = format->frame_width/4;
      ret->strides[2] = format->frame_width/4;
      ALIGN(ret->strides[0]);
      ALIGN(ret->strides[1]);
      ALIGN(ret->strides[2]);
      
      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                             ret->strides[0]*format->frame_height+
                             (ret->strides[1]*format->frame_height)/4+
                             (ret->strides[2]*format->frame_height)/4);
      ret->planes[1] = ret->planes[0] + ret->strides[0]*format->frame_height;
      ret->planes[2] = ret->planes[1] + (ret->strides[1]*format->frame_height)/4;
      break;
    case GAVL_YUV_422_P:
    case GAVL_YUVJ_422_P:
      ret->strides[0] = format->frame_width;
      ret->strides[1] = format->frame_width/2;
      ret->strides[2] = format->frame_width/2;
      ALIGN(ret->strides[0]);
      ALIGN(ret->strides[1]);
      ALIGN(ret->strides[2]);

      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                             ret->strides[0]*format->frame_height+
                             ret->strides[1]*format->frame_height+
                             ret->strides[2]*format->frame_height);

      ret->planes[1] = ret->planes[0] + ret->strides[0]*format->frame_height;
      ret->planes[2] = ret->planes[1] + ret->strides[1]*format->frame_height;
      break;
    case GAVL_YUV_411_P:
      ret->strides[0] = format->frame_width;
      ret->strides[1] = format->frame_width/4;
      ret->strides[2] = format->frame_width/4;
      ALIGN(ret->strides[0]);
      ALIGN(ret->strides[1]);
      ALIGN(ret->strides[2]);

      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                             ret->strides[0]*format->frame_height+
                             ret->strides[1]*format->frame_height+
                             ret->strides[2]*format->frame_height);

      ret->planes[1] = ret->planes[0] + ret->strides[0]*format->frame_height;
      ret->planes[2] = ret->planes[1] + ret->strides[1]*format->frame_height;
      break;
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_444_P:
      ret->strides[0] = format->frame_width;
      ret->strides[1] = format->frame_width;
      ret->strides[2] = format->frame_width;
      ALIGN(ret->strides[0]);
      ALIGN(ret->strides[1]);
      ALIGN(ret->strides[2]);

      ret->planes[0] = memalign(ALIGNMENT_BYTES,
                                ret->strides[0]*format->frame_height+
                                ret->strides[1]*format->frame_height+
                                ret->strides[2]*format->frame_height);

      ret->planes[1] = ret->planes[0] + ret->strides[0]*format->frame_height;
      ret->planes[2] = ret->planes[1] + ret->strides[1]*format->frame_height;
      break;
    case GAVL_COLORSPACE_NONE:
      fprintf(stderr, "Colorspace not specified for video frame\n");
      return;
    }

  }

static void video_frame_free(gavl_video_frame_t * frame)
  {
  if(frame->planes[0])
    free(frame->planes[0]);
  frame->planes[0] = (uint8_t*)0;
  }

gavl_video_frame_t * gavl_video_frame_create(const gavl_video_format_t * format)
  {
  gavl_video_frame_t * ret = calloc(1, sizeof(gavl_video_frame_t));
  if(format)
    video_frame_alloc(ret, format);
  return ret;
  }

void gavl_video_frame_destroy(gavl_video_frame_t * frame)
  {
  video_frame_free(frame);
  free(frame);
  }

void gavl_video_frame_null(gavl_video_frame_t* frame)
  {
  frame->planes[0] = (uint8_t*)0;
  }

void gavl_video_frame_clear(gavl_video_frame_t * frame,
                            gavl_video_format_t * format)
  {
  int i, j;
  switch(format->colorspace)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_RGB_24:
    case GAVL_BGR_24:
    case GAVL_RGB_32:
    case GAVL_BGR_32:
      memset(frame->planes[0], 0x00, format->frame_height * frame->strides[0]);
      break;
    case GAVL_RGBA_32:
      for(i = 0; i < format->frame_height; i++)
        {
        for(j = 0; j < format->frame_width; j++)
          {
          frame->planes[0][4*j + i*frame->strides[0]]   = 0x00; /* R */
          frame->planes[0][4*j + i*frame->strides[0]+1] = 0x00; /* G */
          frame->planes[0][4*j + i*frame->strides[0]+2] = 0x00; /* B */
          frame->planes[0][4*j + i*frame->strides[0]+3] = 0xFF; /* A */
          }
        }
      
      break;
    case GAVL_YUY2:
      for(i = 0; i < format->frame_height; i++)
        {
        for(j = 0; j < format->frame_width; j++)
          {
          frame->planes[0][2*j + i*frame->strides[0]]   = 0x00; /* Y   */
          frame->planes[0][2*j + i*frame->strides[0]+1] = 0x80; /* U/V */
          }
        }
      break;
    case GAVL_UYVY:
      for(i = 0; i < format->frame_height; i++)
        {
        for(j = 0; j < format->frame_width; j++)
          {
          frame->planes[0][2*j + i*frame->strides[0]+1] = 0x00; /* Y */
          frame->planes[0][2*j + i*frame->strides[0]]   = 0x80; /* U/V   */
          }
        }
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUVJ_420_P:
      memset(frame->planes[0], 0x00, format->frame_height * frame->strides[0]);
      memset(frame->planes[1], 0x80, (format->frame_height * frame->strides[1])/2);
      memset(frame->planes[2], 0x80, (format->frame_height * frame->strides[2])/2);
      break;
    case GAVL_YUV_410_P:
      memset(frame->planes[0], 0x00, format->frame_height * frame->strides[0]);
      memset(frame->planes[1], 0x80, (format->frame_height * frame->strides[1])/4);
      memset(frame->planes[2], 0x80, (format->frame_height * frame->strides[2])/4);
      break;
    case GAVL_YUV_422_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      memset(frame->planes[0], 0x00, format->frame_height * frame->strides[0]);
      memset(frame->planes[1], 0x80, format->frame_height * frame->strides[1]);
      memset(frame->planes[2], 0x80, format->frame_height * frame->strides[2]);
      break;
    case GAVL_COLORSPACE_NONE:
      break;
      
    }
  }

void gavl_video_frame_copy(gavl_video_format_t * format,
                           gavl_video_frame_t * dst,
                           gavl_video_frame_t * src)
  {
  int i, j;
  uint8_t * sp, *dp;
  int jmax;
  int bytes_per_line;
  int sub_h, sub_v;

  int planes = gavl_colorspace_num_planes(format->colorspace);
  sub_h = 1;
  sub_v = 1;

  
  for(i = 0; i < planes; i++)
    {
    if(i)
      gavl_colorspace_chroma_sub(format->colorspace, &sub_h, &sub_v);
    if(dst->strides[i] == src->strides[i])
      {
      memcpy(dst->planes[i], src->planes[i],
             format->image_height / sub_v * dst->strides[i]);

      }
    else
      {
      bytes_per_line =
        dst->strides[i] < src->strides[i] ?
        dst->strides[i] : src->strides[i];
      sp = src->planes[i];
      dp = dst->planes[i];
      jmax = format->image_height / sub_v;
      for(j = 0; j < jmax; j++)
        {
        memcpy(dp, sp, bytes_per_line);
        sp += src->strides[i];
        dp += dst->strides[i];
        }
      }
    }
  
  }

static inline void flip_scanline_1(uint8_t * dst, uint8_t * src, int len)
  {
  int i;
  dst += (len-1);
  
  for(i = 0; i < len; i++)
    {
    *dst = *src;
    dst--;
    src++;
    }
  }

static void flip_scanline_2(uint8_t * dst, uint8_t * src, int len)
  {
  int i;
  dst += 2*(len-1);
  
  for(i = 0; i < len; i++)
    {
    dst[0] = src[0];
    dst[1] = src[1];
    
    dst-=2;
    src+=2;
    }
  
  }

static void flip_scanline_3(uint8_t * dst, uint8_t * src, int len)
  {
  int i;
  dst += 3*(len-1);
  
  for(i = 0; i < len; i++)
    {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    
    dst-=3;
    src+=3;
    }
  
  }

static void flip_scanline_4(uint8_t * dst, uint8_t * src, int len)
  {
  int i;
  dst += 4*(len-1);
  
  for(i = 0; i < len; i++)
    {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[4] = src[4];
    
    dst-=4;
    src+=4;
    }
  
  }

typedef void (*flip_scanline_func)(uint8_t * dst, uint8_t * src, int len);

static flip_scanline_func find_flip_scanline_func(gavl_colorspace_t csp)
  {
  switch(csp)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_YUY2:
    case GAVL_UYVY:
      return flip_scanline_2;
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      return flip_scanline_3;
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGBA_32:
      return flip_scanline_4;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_444_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      return flip_scanline_1;
      break;
    case GAVL_COLORSPACE_NONE:
      break;
    }
  return (flip_scanline_func)0;
  }

void gavl_video_frame_copy_flip_x(gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  gavl_video_frame_t * src)
  {
  uint8_t * src_ptr;
  uint8_t * dst_ptr;
  
  int i, j;
  int sub_h, sub_v;
  int planes;
  flip_scanline_func func;
  
  planes = gavl_colorspace_num_planes(format->colorspace);
  func = find_flip_scanline_func(format->colorspace);
  
  sub_h = 1;
  sub_v = 1;
  
  for(i = 0; i < planes; i++)
    {
    if(i)
      gavl_colorspace_chroma_sub(format->colorspace, &sub_h, &sub_v);

    src_ptr = src->planes[i];
    dst_ptr = dst->planes[i];
            
    for(j = 0; j < format->image_height / sub_v; j++)
      {
      func(dst_ptr, src_ptr, format->image_width / sub_h);

      src_ptr += src->strides[i];
      dst_ptr += dst->strides[i];
      }
    }
  
  }

void gavl_video_frame_copy_flip_y(gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  gavl_video_frame_t * src)
  {
  uint8_t * src_ptr;
  uint8_t * dst_ptr;
  
  int i, j;
  int sub_h, sub_v;
  int planes;
  int bytes_per_line;
    
  planes = gavl_colorspace_num_planes(format->colorspace);
  
  sub_h = 1;
  sub_v = 1;
  
  for(i = 0; i < planes; i++)
    {
    if(i)
      gavl_colorspace_chroma_sub(format->colorspace, &sub_h, &sub_v);
    
    src_ptr = src->planes[i] +
      ((format->image_height / sub_v) - 1) * src->strides[i];

    dst_ptr = dst->planes[i];

    bytes_per_line =
      dst->strides[i] < src->strides[i] ?
      dst->strides[i] : src->strides[i];
    
    for(j = 0; j < format->image_height / sub_v; j++)
      {
      memcpy(dst_ptr, src_ptr, bytes_per_line);
      
      src_ptr -= src->strides[i];
      dst_ptr += dst->strides[i];
      }
    }

  }

void gavl_video_frame_copy_flip_xy(gavl_video_format_t * format,
                                  gavl_video_frame_t * dst,
                                  gavl_video_frame_t * src)
  {
  uint8_t * src_ptr;
  uint8_t * dst_ptr;
  
  int i, j;
  int sub_h, sub_v;
  int planes;
  flip_scanline_func func;
  
  planes = gavl_colorspace_num_planes(format->colorspace);
  func = find_flip_scanline_func(format->colorspace);
  
  sub_h = 1;
  sub_v = 1;
  
  for(i = 0; i < planes; i++)
    {
    if(i)
      gavl_colorspace_chroma_sub(format->colorspace, &sub_h, &sub_v);

    src_ptr = src->planes[i] +
      ((format->image_height / sub_v) - 1) * src->strides[i];

    dst_ptr = dst->planes[i];
            
    for(j = 0; j < format->image_height / sub_v; j++)
      {
      func(dst_ptr, src_ptr, format->image_width / sub_h);

      src_ptr -= src->strides[i];
      dst_ptr += dst->strides[i];
      }
    }
  
  }



void gavl_video_frame_dump(gavl_video_frame_t * frame,
                           gavl_video_format_t * format,
                           const char * namebase)
  {
  char * filename;
  int baselen;
  int sub_h, sub_v;
  int planes;
  int i, j;
  
  FILE * output;

  planes = gavl_colorspace_num_planes(format->colorspace);
  
  baselen = strlen(namebase);
  
  filename = malloc(baselen + 4);
  strcpy(filename, namebase);

  sub_h = 1;
  sub_v = 1;
  
  for(i = 0; i < planes; i++)
    {
    filename[baselen]   = '.';
    filename[baselen+1] = 'p';
    filename[baselen+2] = i + 1 + '0';
    filename[baselen+3] = '\0';

    output = fopen(filename, "wb");

    if(i == 1)
      gavl_colorspace_chroma_sub(format->colorspace,
                          &sub_h, &sub_v);
    
    for(j = 0; j < format->image_height / sub_v; j++)
      {
      fwrite(frame->planes[i] + j*frame->strides[i], 1,  format->image_width / sub_h,
             output);
      }
    fclose(output);
    }
  free(filename);
  }

void gavl_video_frame_get_subframe(gavl_colorspace_t colorspace,
                                   gavl_video_frame_t * src,
                                   gavl_video_frame_t * dst,
                                   gavl_rectangle_t * src_rect)
  {
  int uv_sub_h, uv_sub_v;
  int i;
  int bytes;
  int num_planes = gavl_colorspace_num_planes(colorspace);

  dst->strides[0] = src->strides[0];
  
  if(num_planes > 1)
    {
    gavl_colorspace_chroma_sub(colorspace, &uv_sub_h,
                               &uv_sub_v);
    bytes = gavl_colorspace_bytes_per_component(colorspace);
    dst->planes[0] = src->planes[0] + src_rect->y * src->strides[0] + src_rect->x * bytes;

    for(i = 1; i < num_planes; i++)
      {
      dst->planes[i] = src->planes[i] + (src_rect->y/uv_sub_v) * src->strides[i] + (src_rect->x/uv_sub_h) * bytes;
      dst->strides[i] = src->strides[i];
      }
    }
  else
    {
    if(((colorspace == GAVL_YUY2) || (colorspace == GAVL_UYVY)) &&
       (src_rect->x & 1))
      src_rect->x--;
    bytes = gavl_colorspace_bytes_per_pixel(colorspace);
    dst->planes[0] = src->planes[0] + src_rect->y * src->strides[0] + src_rect->x * bytes;
    }
  }
