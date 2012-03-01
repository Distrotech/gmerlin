/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>
#include <gavl/gavl.h>
#include <gmerlin/parameter.h>

#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>


gavl_pixelformat_t bgv4l2_pixelformat_v4l2_2_gavl(int csp);

uint32_t bgv4l2_pixelformat_gavl_2_v4l2(gavl_pixelformat_t scp);

/* uictl */
int bgv4l2_ioctl(int fd, int request, void * arg);

// Parameter stuff

void bgv4l2_create_device_selector(bg_parameter_info_t * info,
                                   int capability);

int bgv4l2_set_device_parameter(int fd,
                                struct v4l2_queryctrl * controls,
                                int num_controls,
                                const char * name,
                                const bg_parameter_value_t * val);

int bgv4l2_get_device_parameter(int fd,
                                struct v4l2_queryctrl * controls,
                                int num_controls,
                                const char * name,
                                bg_parameter_value_t * val);


struct v4l2_queryctrl * bgv4l2_create_device_controls(int fd, int * num);
                          
// Device handling stuff

int bgv4l2_open_device(const char * device, int capability,
                       struct v4l2_capability * cap);

gavl_video_frame_t * bgv4l2_create_frame(uint8_t * data, // Can be NULL
                                         const gavl_video_format_t * gavl,
                                         const struct v4l2_format * v4l2);

int bgv4l2_set_strides(const gavl_video_format_t * gavl,
                       const struct v4l2_format * v4l2, int * ret);

int bgv4l2_strides_match(const gavl_video_frame_t * f, int * strides, int num_strides);



typedef enum
  {
    BGV4L2_IO_METHOD_RW = 0,
    BGV4L2_IO_METHOD_MMAP = 1,
    //    BGV4L2_IO_METHOD_USERPTR = 2,
  } bgv4l2_io_method_t;

#define CLEAR(x) memset (&(x), 0, sizeof (x))
