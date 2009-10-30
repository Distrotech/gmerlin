/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <stdlib.h> // NULL


#include <gavl/gavl.h>

#if 0

typedef (*extract_func)(const gavl_video_frame_t * src,
                        gavl_video_frame_t * dst,
                        int plane, int offset, int advance,
                        int width, int height);

static void extract_rgb15_r(const gavl_video_frame_t * src,
                            gavl_video_frame_t * dst,
                            int plane, int offset, int advance,
                            int width, int height)
  {
  
  }

static void extract_rgb15_g(const gavl_video_frame_t * src,
                            gavl_video_frame_t * dst,
                            int plane, int offset, int advance,
                            int width, int height)
  {
  
  }

static void extract_rgb15_b(const gavl_video_frame_t * src,
                            gavl_video_frame_t * dst,
                            int plane, int offset, int advance,
                            int width, int height)
  {
  
  }

static void extract_rgb16_r(const gavl_video_frame_t * src,
                            gavl_video_frame_t * dst,
                            int plane, int offset, int advance,
                            int width, int height)
  {
  
  }

static void extract_rgb16_g(const gavl_video_frame_t * src,
                            gavl_video_frame_t * dst,
                            int plane, int offset, int advance,
                            int width, int height)
  {
  
  }

static void extract_rgb16_b(const gavl_video_frame_t * src,
                            gavl_video_frame_t * dst,
                            int plane, int offset, int advance,
                            int width, int height)
  {
  
  }

static void extract_8(const gavl_video_frame_t * src,
                      gavl_video_frame_t * dst,
                      int plane, int offset, int advance,
                      int width, int height)
  {
  
  }

static void extract_8_y(const gavl_video_frame_t * src,
                        gavl_video_frame_t * dst,
                        int plane, int offset, int advance,
                        int width, int height)
  {
  
  }

static void extract_8_uv(const gavl_video_frame_t * src,
                         gavl_video_frame_t * dst,
                         int plane, int offset, int advance,
                         int width, int height)
  {
  
  }

static void extract_16(const gavl_video_frame_t * src,
                      gavl_video_frame_t * dst,
                      int plane, int offset, int advance,
                      int width, int height)
  {
  
  }

static void extract_16_y(const gavl_video_frame_t * src,
                         gavl_video_frame_t * dst,
                         int plane, int offset, int advance,
                         int width, int height)
  {
  
  }

static void extract_16_uv(const gavl_video_frame_t * src,
                          gavl_video_frame_t * dst,
                          int plane, int offset, int advance,
                          int width, int height)
  {
  
  }

static void extract_float(const gavl_video_frame_t * src,
                          gavl_video_frame_t * dst,
                          int plane, int offset, int advance,
                          int width, int height)
  {
  
  }

static void extract_float_uv(const gavl_video_frame_t * src,
                             gavl_video_frame_t * dst,
                             int plane, int offset, int advance,
                             int width, int height)
  {
  
  }

#endif

static int get_channel_properties(gavl_pixelformat_t src_format,
                                  gavl_pixelformat_t * dst_format_ret,
                                  gavl_color_channel_t ch,
                                  int * plane_ret,
                                  int * offset_ret,
                                  int * advance_ret,
                                  int * sub_h_ret, int * sub_v_ret)
  {
  int plane = 0, offset = 0, advance = 1, sub_h = 1, sub_v = 1;
  gavl_pixelformat_t dst_format;
  
  switch(src_format)
    {
    case GAVL_PIXELFORMAT_NONE:
      return 0;
    case GAVL_GRAY_8:
      dst_format = GAVL_GRAY_8;
      switch(ch)
        {
        case GAVL_CCH_Y:
          break;
        default:
          return 0;
        }
      break;
    case GAVL_GRAY_16:
      dst_format = GAVL_GRAY_16;
      switch(ch)
        {
        case GAVL_CCH_Y:
          break;
        default:
          return 0;
        }
      break;
    case GAVL_GRAY_FLOAT:
      dst_format = GAVL_GRAY_FLOAT;
      switch(ch)
        {
        case GAVL_CCH_Y:
          break;
        default:
          return 0;
        }
      break;
    case GAVL_GRAYA_16:
      dst_format = GAVL_GRAY_8;
      advance = 2;
      switch(ch)
        {
        case GAVL_CCH_Y:
          break;
        case GAVL_CCH_ALPHA:
          offset  = 1;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_GRAYA_32:
      dst_format = GAVL_GRAY_16;
      advance = 2;
      switch(ch)
        {
        case GAVL_CCH_Y:
          break;
        case GAVL_CCH_ALPHA:
          offset  = 1;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_GRAYA_FLOAT:
      dst_format = GAVL_GRAY_FLOAT;
      advance = 2;
      switch(ch)
        {
        case GAVL_CCH_Y:
          break;
        case GAVL_CCH_ALPHA:
          offset  = 1;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_RGB_15:
      dst_format = GAVL_GRAY_8;
      switch(ch)
        {
        case GAVL_CCH_RED:
          break;
        case GAVL_CCH_GREEN:
          break;
        case GAVL_CCH_BLUE:
          break;
        default:
          return 0;
        }
      break;
    case GAVL_BGR_15:
      dst_format = GAVL_GRAY_8;
      switch(ch)
        {
        case GAVL_CCH_RED:
          break;
        case GAVL_CCH_GREEN:
          break;
        case GAVL_CCH_BLUE:
          break;
        default:
          return 0;
        }
      break;
    case GAVL_RGB_16:
      dst_format = GAVL_GRAY_8;
      switch(ch)
        {
        case GAVL_CCH_RED:
          break;
        case GAVL_CCH_GREEN:
          break;
        case GAVL_CCH_BLUE:
          break;
        default:
          return 0;
        }
      break;
    case GAVL_BGR_16:
      dst_format = GAVL_GRAY_8;
      switch(ch)
        {
        case GAVL_CCH_RED:
          break;
        case GAVL_CCH_GREEN:
          break;
        case GAVL_CCH_BLUE:
          break;
        default:
          return 0;
        }
      break;
    case GAVL_RGB_24:
      dst_format = GAVL_GRAY_8;
      advance = 3;
      switch(ch)
        {
        case GAVL_CCH_RED:
          offset  = 0;
          break;
        case GAVL_CCH_GREEN:
          offset  = 1;
          break;
        case GAVL_CCH_BLUE:
          offset  = 2;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_BGR_24:
      dst_format = GAVL_GRAY_8;
      advance = 3;
      switch(ch)
        {
        case GAVL_CCH_RED:
          offset  = 2;
          break;
        case GAVL_CCH_GREEN:
          offset  = 1;
          break;
        case GAVL_CCH_BLUE:
          offset  = 0;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_RGB_32:
      dst_format = GAVL_GRAY_8;
      advance = 4;
      switch(ch)
        {
        case GAVL_CCH_RED:
          offset  = 0;
          break;
        case GAVL_CCH_GREEN:
          offset  = 1;
          break;
        case GAVL_CCH_BLUE:
          offset  = 2;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_BGR_32:
      dst_format = GAVL_GRAY_8;
      advance = 4;
      switch(ch)
        {
        case GAVL_CCH_RED:
          offset  = 2;
          break;
        case GAVL_CCH_GREEN:
          offset  = 1;
          break;
        case GAVL_CCH_BLUE:
          offset  = 0;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_RGBA_32:
      dst_format = GAVL_GRAY_8;
      advance = 4;
      switch(ch)
        {
        case GAVL_CCH_RED:
          offset  = 0;
          break;
        case GAVL_CCH_GREEN:
          offset  = 1;
          break;
        case GAVL_CCH_BLUE:
          offset  = 2;
          break;
        case GAVL_CCH_ALPHA:
          offset  = 3;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_RGB_48:
      dst_format = GAVL_GRAY_16;
      advance = 3;
      switch(ch)
        {
        case GAVL_CCH_RED:
          offset  = 0;
          break;
        case GAVL_CCH_GREEN:
          offset  = 1;
          break;
        case GAVL_CCH_BLUE:
          offset  = 2;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_RGBA_64:
      dst_format = GAVL_GRAY_16;
      advance = 4;
      switch(ch)
        {
        case GAVL_CCH_RED:
          offset  = 0;
          break;
        case GAVL_CCH_GREEN:
          offset  = 1;
          break;
        case GAVL_CCH_BLUE:
          offset  = 2;
          break;
        case GAVL_CCH_ALPHA:
          offset  = 3;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_RGB_FLOAT:
      dst_format = GAVL_GRAY_FLOAT;
      advance = 3;
      switch(ch)
        {
        case GAVL_CCH_RED:
          offset  = 0;
          break;
        case GAVL_CCH_GREEN:
          offset  = 1;
          break;
        case GAVL_CCH_BLUE:
          offset  = 2;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_RGBA_FLOAT:
      dst_format = GAVL_GRAY_FLOAT;
      advance = 4;
      switch(ch)
        {
        case GAVL_CCH_RED:
          offset  = 0;
          break;
        case GAVL_CCH_GREEN:
          offset  = 1;
          break;
        case GAVL_CCH_BLUE:
          offset  = 2;
          break;
        case GAVL_CCH_ALPHA:
          offset  = 3;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_YUY2:
      dst_format = GAVL_GRAY_8;
      switch(ch)
        {
        case GAVL_CCH_Y:
          offset  = 0;
          advance = 2;
          break;
        case GAVL_CCH_CB:
          offset  = 1;
          advance = 4;
          break;
        case GAVL_CCH_CR:
          offset  = 3;
          advance = 4;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_UYVY:
      dst_format = GAVL_GRAY_8;
      switch(ch)
        {
        case GAVL_CCH_Y:
          offset  = 1;
          advance = 2;
          break;
        case GAVL_CCH_CB:
          offset  = 0;
          advance = 4;
          break;
        case GAVL_CCH_CR:
          offset  = 2;
          advance = 4;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_YUVA_32:
      dst_format = GAVL_GRAY_8;
      advance = 4;
      switch(ch)
        {
        case GAVL_CCH_Y:
          offset  = 0;
          break;
        case GAVL_CCH_CB:
          offset  = 1;
          break;
        case GAVL_CCH_CR:
          offset  = 2;
          break;
        case GAVL_CCH_ALPHA:
          offset  = 3;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_YUVA_64:
      dst_format = GAVL_GRAY_16;
      advance = 4;
      switch(ch)
        {
        case GAVL_CCH_Y:
          offset  = 0;
          break;
        case GAVL_CCH_CB:
          offset  = 1;
          break;
        case GAVL_CCH_CR:
          offset  = 2;
          break;
        case GAVL_CCH_ALPHA:
          offset  = 3;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_YUV_FLOAT:
      dst_format = GAVL_GRAY_FLOAT;
      advance = 3;
      switch(ch)
        {
        case GAVL_CCH_Y:
          offset  = 0;
          break;
        case GAVL_CCH_CB:
          offset  = 1;
          break;
        case GAVL_CCH_CR:
          offset  = 2;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_YUVA_FLOAT:
      dst_format = GAVL_GRAY_FLOAT;
      advance = 4;
      switch(ch)
        {
        case GAVL_CCH_Y:
          offset  = 0;
          break;
        case GAVL_CCH_CB:
          offset  = 1;
          break;
        case GAVL_CCH_CR:
          offset  = 2;
          break;
        case GAVL_CCH_ALPHA:
          offset  = 3;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUV_422_P:
    case GAVL_YUV_444_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_410_P:
      dst_format = GAVL_GRAY_8;
      switch(ch)
        {
        case GAVL_CCH_Y:
          plane  = 0;
          break;
        case GAVL_CCH_CB:
          plane  = 1;
          break;
        case GAVL_CCH_CR:
          plane  = 2;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      dst_format = GAVL_GRAY_8;
      switch(ch)
        {
        case GAVL_CCH_Y:
          plane  = 0;
          break;
        case GAVL_CCH_CB:
          plane  = 1;
          break;
        case GAVL_CCH_CR:
          plane  = 2;
          break;
        default:
          return 0;
        }
      break;
    case GAVL_YUV_444_P_16:
    case GAVL_YUV_422_P_16:
      dst_format = GAVL_GRAY_16;
      switch(ch)
        {
        case GAVL_CCH_Y:
          plane  = 0;
          break;
        case GAVL_CCH_CB:
          plane  = 1;
          break;
        case GAVL_CCH_CR:
          plane  = 2;
          break;
        default:
          return 0;
        }
      break;
    }

  /* Get chroma subsampling */
  
  if(plane)
    gavl_pixelformat_chroma_sub(src_format, &sub_h, &sub_v);
  
  /* Return stuff */
  if(dst_format_ret)
    *dst_format_ret = dst_format;

  if(plane_ret)
    *plane_ret = plane;
  if(offset_ret)
    *offset_ret = offset;
  
  if(advance_ret)
    *advance_ret = advance;

  if(sub_h_ret)
    *sub_h_ret = sub_h;

  if(sub_v_ret)
    *sub_v_ret = sub_v;
  
  return 1;
  }

int gavl_get_color_channel_format(const gavl_video_format_t * frame_format,
                                  gavl_video_format_t * channel_format,
                                  gavl_color_channel_t ch)
  {
  int sub_h, sub_v;

  gavl_video_format_copy(channel_format, frame_format);

  if(!get_channel_properties(frame_format->pixelformat,
                             &channel_format->pixelformat,
                             ch,
                             NULL, // int * plane_ret,
                             NULL, // int * offset_ret,
                             NULL, // int * advance_ret,
                             &sub_h, &sub_v))
    return 0;

  channel_format->image_width /= sub_h;
  channel_format->frame_width /= sub_h;

  channel_format->image_height /= sub_v;
  channel_format->frame_height /= sub_v;
  return 1;
  }

int
gavl_video_frame_extract_channel(const gavl_video_format_t * format,
                                 gavl_color_channel_t ch,
                                 const gavl_video_frame_t * src,
                                 gavl_video_frame_t * dst)
  {
  return 1;
  }

int
gavl_video_frame_merge_channel(const gavl_video_format_t * format,
                               gavl_color_channel_t ch,
                               const gavl_video_frame_t * src,
                               gavl_video_frame_t * dst)
  {
  return 1;
  }
