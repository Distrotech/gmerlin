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

#include <stdlib.h>

#include <config.h>

#include <gavl/gavl.h>
#include "convert.h"
#include "v4l2_common.h"
#include <libv4lconvert.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "v4lconvert"

struct bg_v4l2_convert_s
  {
  struct v4lconvert_data * cnv;
  gavl_video_frame_t * frame;
  int dst_size;
  gavl_video_format_t fmt;

  struct v4l2_format src_format;
  struct v4l2_format dst_format;

  int strides[GAVL_MAX_PLANES];
  int num_strides;
  };

#if 0
static void dump_format(struct v4l2_format * format)
  {
  fprintf(stderr, "src_format: %dx%d, %c%c%c%c %d\n",
          format->fmt.pix.width,
          format->fmt.pix.height,
          format->fmt.pix.pixelformat & 0xff,
          (format->fmt.pix.pixelformat >> 8) & 0xff,
          (format->fmt.pix.pixelformat >> 16) & 0xff,
          (format->fmt.pix.pixelformat >> 24) & 0xff,
          format->fmt.pix.sizeimage);
  }
#endif

bg_v4l2_convert_t * bg_v4l2_convert_create(int fd, uint32_t * v4l_fmt,
                                           gavl_pixelformat_t * gavl_fmt,
                                           int width, int height)
  {
  bg_v4l2_convert_t * ret;

  //  fprintf(stderr, "bg_v4l2_convert_create %d %d\n", width, height);
  
  ret = calloc(1, sizeof(*ret));

  ret->cnv = v4lconvert_create(fd);

  /* Set up formats */
  ret->dst_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ret->dst_format.fmt.pix.width = width;
  ret->dst_format.fmt.pix.height = width;
  ret->dst_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
  
  if(v4lconvert_try_format(ret->cnv,
                           &ret->dst_format, /* in / out */
                           &ret->src_format))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Initializing libv4lconvert failed");
    v4lconvert_destroy(ret->cnv);
    free(ret);
    return NULL;
    }

  if(ret->dst_format.fmt.pix.width != width ||
     ret->dst_format.fmt.pix.height != height)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Initializing libv4lconvert failed (resolution not directly supported)");
    v4lconvert_destroy(ret->cnv);
    free(ret);
    return NULL;
    }
  
  //  fprintf(stderr, "Initialized libv4lconvert\n");
  //  dump_format(&ret->src_format);
  //  dump_format(&ret->dst_format);
  
  ret->fmt.pixel_width = 1;
  ret->fmt.pixel_height = 1;
  ret->fmt.image_width = ret->dst_format.fmt.pix.width;
  ret->fmt.image_height = ret->dst_format.fmt.pix.height;
  ret->fmt.frame_width = ret->dst_format.fmt.pix.width;
  ret->fmt.frame_height = ret->dst_format.fmt.pix.height;
  ret->fmt.pixelformat =
    bgv4l2_pixelformat_v4l2_2_gavl(ret->dst_format.fmt.pix.pixelformat);
  ret->frame = bgv4l2_create_frame(NULL, // Can be NULL
                                   &ret->fmt, &ret->dst_format);
  if(gavl_fmt)
    *gavl_fmt = ret->fmt.pixelformat;
  ret->dst_size = ret->dst_format.fmt.pix.sizeimage;

  ret->num_strides = bgv4l2_set_strides(&ret->fmt,
                                        &ret->dst_format, ret->strides);
  return ret;
  }

void bg_v4l2_convert_convert(bg_v4l2_convert_t * cnv, uint8_t * data, int size,
                             gavl_video_frame_t * frame)
  {
  int result;
  //  fprintf(stderr, "bg_v4l2_convert_convert %d\n", size);

  if(bgv4l2_strides_match(frame, cnv->strides, cnv->num_strides))
    {
    result = v4lconvert_convert(cnv->cnv,
                                &cnv->src_format,  /* in */
                                &cnv->dst_format, /* in */
                                data, size, frame->planes[0], cnv->dst_size);
    return;
    }
  
  if(!cnv->frame)
    cnv->frame = bgv4l2_create_frame(NULL, // Can be NULL
                                     &cnv->fmt, &cnv->dst_format);
  
  result = v4lconvert_convert(cnv->cnv,
                              &cnv->src_format,  /* in */
                              &cnv->dst_format, /* in */
                              data, size, cnv->frame->planes[0], cnv->dst_size);
  
  gavl_video_frame_copy(&cnv->fmt, frame, cnv->frame);
  }

void bg_v4l2_convert_destroy(bg_v4l2_convert_t * cnv)
  {
  if(cnv->cnv)
    v4lconvert_destroy(cnv->cnv);
  if(cnv->frame)
    gavl_video_frame_destroy(cnv->frame);
  free(cnv);
  }

