/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <string.h>
#include <errno.h>

#include <config.h>

#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "y4m"

#include <yuv4mpeg.h>
#include "y4m_common.h"

void bg_y4m_set_pixelformat(bg_y4m_common_t * com)
  {
  switch(com->chroma_mode)
    {
    case Y4M_CHROMA_420JPEG:
      com->format.pixelformat = GAVL_YUV_420_P;
      com->format.chroma_placement = GAVL_CHROMA_PLACEMENT_DEFAULT;
      break;
    case Y4M_CHROMA_420MPEG2:
      com->format.pixelformat = GAVL_YUV_420_P;
      com->format.chroma_placement = GAVL_CHROMA_PLACEMENT_MPEG2;
      break;
    case Y4M_CHROMA_420PALDV:
      com->format.pixelformat = GAVL_YUV_420_P;
      com->format.chroma_placement = GAVL_CHROMA_PLACEMENT_DVPAL;
      break;
    case Y4M_CHROMA_444:
      com->format.pixelformat = GAVL_YUV_444_P;
      break;
    case Y4M_CHROMA_422:
      com->format.pixelformat = GAVL_YUV_422_P;
      break;
    case Y4M_CHROMA_411:
      com->format.pixelformat = GAVL_YUV_411_P;
      break;
    case Y4M_CHROMA_MONO:
      /* Monochrome isn't supported by gavl, we choose the format with the
         smallest chroma planes to save memory */
      com->format.pixelformat = GAVL_YUV_410_P;
      break;
    case Y4M_CHROMA_444ALPHA:
      /* Must be converted to packed */
      com->format.pixelformat = GAVL_YUVA_32;
      com->tmp_planes[0] = malloc(com->format.image_width *
                                    com->format.image_height * 4);
        
      com->tmp_planes[1] = com->tmp_planes[0] +
        com->format.image_width * com->format.image_height;
      com->tmp_planes[2] = com->tmp_planes[1] +
        com->format.image_width * com->format.image_height;
      com->tmp_planes[3] = com->tmp_planes[2] +
        com->format.image_width * com->format.image_height;
      break;
    }
  
  }

int bg_y4m_write_header(bg_y4m_common_t * com)
  {
  int i, err;
  y4m_ratio_t r;

  y4m_accept_extensions(1);
  
  /* Set up the stream- and frame header */
  y4m_init_stream_info(&(com->si));
  y4m_init_frame_info(&(com->fi));

  y4m_si_set_width(&(com->si), com->format.image_width);
  y4m_si_set_height(&(com->si), com->format.image_height);

  switch(com->format.interlace_mode)
    {
    case GAVL_INTERLACE_TOP_FIRST:
      i = Y4M_ILACE_TOP_FIRST;
      break;
    case GAVL_INTERLACE_BOTTOM_FIRST:
      i = Y4M_ILACE_BOTTOM_FIRST;
      break;
    case GAVL_INTERLACE_MIXED:
      com->format.interlace_mode = GAVL_INTERLACE_NONE;
    default: // Fall through
      i = Y4M_ILACE_NONE;
      break;
    }
  y4m_si_set_interlace(&(com->si), i);

  r.n = com->format.timescale;
  r.d = com->format.frame_duration;
  
  y4m_si_set_framerate(&(com->si), r);

  r.n = com->format.pixel_width;
  r.d = com->format.pixel_height;

  y4m_si_set_sampleaspect(&(com->si), r);
  y4m_si_set_chroma(&(com->si), com->chroma_mode);

  /* Now, it's time to write the stream header */

  err = y4m_write_stream_header(com->fd, &(com->si));
  
  if(err != Y4M_OK)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing stream header failed: %s",
           ((err == Y4M_ERR_SYSTEM) ? strerror(errno) : y4m_strerr(err)));
    return 0;
    }
  return 1;

  }

static uint8_t yj_8_to_y_8[256] = 
{
  0x10, 0x11, 0x12, 0x13, 0x13, 0x14, 0x15, 0x16, 
  0x17, 0x18, 0x19, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 
  0x1e, 0x1f, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 
  0x25, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 
  0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x31, 
  0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x38, 
  0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3e, 0x3f, 
  0x40, 0x41, 0x42, 0x43, 0x44, 0x44, 0x45, 0x46, 
  0x47, 0x48, 0x49, 0x4a, 0x4a, 0x4b, 0x4c, 0x4d, 
  0x4e, 0x4f, 0x50, 0x50, 0x51, 0x52, 0x53, 0x54, 
  0x55, 0x56, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 
  0x5c, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 
  0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x68, 
  0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6e, 0x6f, 
  0x70, 0x71, 0x72, 0x73, 0x74, 0x74, 0x75, 0x76, 
  0x77, 0x78, 0x79, 0x7a, 0x7a, 0x7b, 0x7c, 0x7d, 
  0x7e, 0x7f, 0x80, 0x81, 0x81, 0x82, 0x83, 0x84, 
  0x85, 0x86, 0x87, 0x87, 0x88, 0x89, 0x8a, 0x8b, 
  0x8c, 0x8d, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 
  0x93, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 
  0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0x9f, 
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa5, 0xa6, 
  0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xab, 0xac, 0xad, 
  0xae, 0xaf, 0xb0, 0xb1, 0xb1, 0xb2, 0xb3, 0xb4, 
  0xb5, 0xb6, 0xb7, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 
  0xbc, 0xbd, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 
  0xc3, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 
  0xca, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd6, 
  0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdc, 0xdd, 
  0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe2, 0xe3, 0xe4, 
  0xe5, 0xe6, 0xe7, 0xe8, 0xe8, 0xe9, 0xea, 0xeb, 
};


static void convert_yuva4444(uint8_t ** dst, uint8_t ** src,
                             int width, int height, int stride)
  {
  int i, j;
  uint8_t *y, *u, *v, *a, *s;
  
  y = dst[0];
  u = dst[1];
  v = dst[2];
  a = dst[3];

  s = src[0];
  
  for(i = 0; i < height; i++)
    {
    s = src[0] + i * stride;
    for(j = 0; j < width; j++)
      {
      *(y++) = *(s++);
      *(u++) = *(s++);
      *(v++) = *(s++);
      *(a++) = yj_8_to_y_8[*(s++)];
      }
    }
  }


int bg_y4m_write_frame(bg_y4m_common_t * com, gavl_video_frame_t * frame)
  {
  int result;
  /* Check for YUVA4444 */
  if(com->format.pixelformat == GAVL_YUVA_32)
    {
    convert_yuva4444(com->tmp_planes, frame->planes,
                     com->format.image_width,
                     com->format.image_height,
                     frame->strides[0]);
    result = y4m_write_frame(com->fd, &(com->si), &(com->fi), com->tmp_planes);
    }
  else
    {
    if((frame->strides[0] == com->strides[0]) &&
       (frame->strides[1] == com->strides[1]) &&
       (frame->strides[2] == com->strides[2]) &&
       (frame->strides[3] == com->strides[3]))
      result = y4m_write_frame(com->fd, &(com->si), &(com->fi), frame->planes);
    else
      {
      if(!com->frame)
        com->frame = gavl_video_frame_create_nopad(&(com->format));
      gavl_video_frame_copy(&(com->format), com->frame, frame);
      result = y4m_write_frame(com->fd, &(com->si), &(com->fi), com->frame->planes);
      }
    }
  if(result != Y4M_OK)
    return 0;
  return 1;
  }

void bg_y4m_cleanup(bg_y4m_common_t * com)
  {
  y4m_fini_stream_info(&(com->si));
  y4m_fini_frame_info(&(com->fi));
  
  if(com->tmp_planes[0])
    free(com->tmp_planes[0]);
  if(com->frame)
    gavl_video_frame_destroy(com->frame);
  }
