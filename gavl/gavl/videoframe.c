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

void gavl_video_frame_alloc(gavl_video_frame_t * ret,
                            gavl_video_format_t * format)
  {
  switch(format->colorspace)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      ret->pixels_stride = format->frame_width*2;
      ALIGN(ret->pixels_stride);
      ret->pixels = memalign(ALIGNMENT_BYTES,
                             ret->pixels_stride * format->frame_height);
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      ret->pixels_stride = format->frame_width*3;
      ALIGN(ret->pixels_stride);
      ret->pixels = memalign(ALIGNMENT_BYTES,
                             ret->pixels_stride * format->frame_height);
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
      ret->pixels_stride = format->frame_width*4;
      ALIGN(ret->pixels_stride);
      ret->pixels = memalign(ALIGNMENT_BYTES,
                             ret->pixels_stride * format->frame_height);
      break;
    case GAVL_RGBA_32:
      ret->pixels_stride = format->frame_width*4;
      ALIGN(ret->pixels_stride);
      ret->pixels = memalign(ALIGNMENT_BYTES,
                             ret->pixels_stride * format->frame_height);
      break;
    case GAVL_YUY2:
      ret->pixels_stride = format->frame_width*2;
      ALIGN(ret->pixels_stride);
      ret->pixels = memalign(ALIGNMENT_BYTES,
                             ret->pixels_stride * format->frame_height);
      break;
    case GAVL_YUV_420_P:
      ret->y_stride = format->frame_width;
      ret->u_stride = format->frame_width/2;
      ret->v_stride = format->frame_width/2;
      ALIGN(ret->y_stride);
      ALIGN(ret->u_stride);
      ALIGN(ret->v_stride);
      
      ret->pixels = memalign(ALIGNMENT_BYTES,
                             ret->y_stride*format->frame_height+
                             (ret->u_stride*format->frame_height)/2+
                             (ret->v_stride*format->frame_height)/2);
      ret->y = ret->pixels;
      ret->u = ret->y + ret->y_stride*format->frame_height;
      ret->v = ret->u + (ret->u_stride*format->frame_height)/2;
      ret->pixels_stride = ret->y_stride;
      break;
    case GAVL_YUV_422_P:
      ret->y_stride = format->frame_width;
      ret->u_stride = format->frame_width/2;
      ret->v_stride = format->frame_width/2;
      ALIGN(ret->y_stride);
      ALIGN(ret->u_stride);
      ALIGN(ret->v_stride);

      ret->pixels = memalign(ALIGNMENT_BYTES,
                             ret->y_stride*format->frame_height+
                             ret->u_stride*format->frame_height+
                             ret->v_stride*format->frame_height);

      ret->y = ret->pixels;
      ret->u = ret->y + ret->y_stride*format->frame_height;
      ret->v = ret->u + ret->u_stride*format->frame_height;
      ret->pixels_stride = ret->y_stride;
      break;
    case GAVL_COLORSPACE_NONE:
      fprintf(stderr, "Colorspace not specified for video frame\n");
      return;
    }

  }

void gavl_video_frame_free(gavl_video_frame_t * frame)
  {
  if(frame->pixels)
    free(frame->pixels);
  frame->pixels = (uint8_t*)0;
  }

gavl_video_frame_t * gavl_video_frame_create(gavl_video_format_t * format)
  {
  gavl_video_frame_t * ret = calloc(1, sizeof(gavl_video_frame_t));
  if(format)
    gavl_video_frame_alloc(ret, format);
  return ret;
  }

void gavl_video_frame_destroy(gavl_video_frame_t * frame)
  {
  gavl_video_frame_free(frame);
  free(frame);
  }

void gavl_video_frame_null(gavl_video_frame_t* frame)
  {
  frame->pixels = (char*)0;
  }

void gavl_clear_video_frame(gavl_video_frame_t * frame,
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
      memset(frame->pixels, 0x00, format->frame_height * frame->pixels_stride);
      break;
    case GAVL_RGBA_32:
      for(i = 0; i < format->frame_height; i++)
        {
        for(j = 0; j < format->frame_width; j++)
          {
          frame->pixels[4*j + i*frame->pixels_stride]   = 0x00; /* R */
          frame->pixels[4*j + i*frame->pixels_stride+1] = 0x00; /* G */
          frame->pixels[4*j + i*frame->pixels_stride+2] = 0x00; /* B */
          frame->pixels[4*j + i*frame->pixels_stride+3] = 0xFF; /* A */
          }
        }
      
      break;
    case GAVL_YUY2:
      for(i = 0; i < format->frame_height; i++)
        {
        for(j = 0; j < format->frame_width; j++)
          {
          frame->pixels[2*j + i*frame->pixels_stride]   = 0x00; /* Y   */
          frame->pixels[2*j + i*frame->pixels_stride+1] = 0x80; /* U/V */
          }
        }
      break;
    case GAVL_YUV_420_P:
      memset(frame->y, 0x00, format->frame_height * frame->y_stride);
      memset(frame->u, 0x80, (format->frame_height * frame->u_stride)/2);
      memset(frame->v, 0x80, (format->frame_height * frame->v_stride)/2);
      break;
    case GAVL_YUV_422_P:
      memset(frame->y, 0x00, format->frame_height * frame->y_stride);
      memset(frame->u, 0x80, format->frame_height * frame->u_stride);
      memset(frame->v, 0x80, format->frame_height * frame->v_stride);
      break;
    case GAVL_COLORSPACE_NONE:
      break;
      
    }
  }

void gavl_video_frame_copy(gavl_video_format_t * format,
                           gavl_video_frame_t * dst,
                           gavl_video_frame_t * src)
  {
  int i;
  uint8_t * sp, *dp;
  int imax;
  int bytes_per_line;
  
  if(dst->pixels_stride != src->pixels_stride)
    {
    //    fprintf(stderr, "Strides not equal %d %d\n", dst->pixels_stride, src->pixels_stride);
    sp = src->pixels;
    dp = dst->pixels;
    bytes_per_line = (dst->pixels_stride < src->pixels_stride) ?
      dst->pixels_stride : src->pixels_stride;      
    
    for(i = 0; i < format->image_height; i++)
      {
      memcpy(dp, sp, bytes_per_line);
      dp += dst->pixels_stride;
      sp += src->pixels_stride;
      }
    }
  else
    {
    memcpy(dst->pixels, src->pixels, format->image_height * dst->pixels_stride);
    }
  
  if(gavl_colorspace_is_planar(format->colorspace))
    {
    if(format->colorspace == GAVL_YUV_420_P)
      imax = format->image_height / 2;
    else
      imax = format->image_height;

    if(dst->u_stride != src->u_stride)
      {
      sp = src->u;
      dp = dst->u;
      
      bytes_per_line = (dst->u_stride < src->u_stride) ?
        dst->u_stride : src->u_stride;      
      
      for(i = 0; i < imax; i++)
        {
        memcpy(dp, sp, bytes_per_line);
        dp += dst->u_stride;
        sp += src->u_stride;
        }
      }
    else
      {
      memcpy(dst->u, src->u, imax * dst->u_stride);
      }
    
    if(dst->v_stride != src->v_stride)
      {
      sp = src->v;
      dp = dst->v;
      
      bytes_per_line = (dst->v_stride < src->v_stride) ?
        dst->v_stride : src->v_stride;      
      
      for(i = 0; i < imax; i++)
        {
        memcpy(dp, sp, bytes_per_line);
        dp += dst->v_stride;
        sp += src->v_stride;
        }
      }
    else
      memcpy(dst->v, src->v, imax * dst->v_stride);
    }
  }
