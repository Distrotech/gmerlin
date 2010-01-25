/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <gavl/gavl.h>

int gavl_video_frame_ssim(const gavl_video_frame_t * src1,
                          const gavl_video_frame_t * src2,
                          gavl_video_frame_t * dst,
                          const gavl_video_format_t * format)
  {
  int i, j;

  if(format->pixelformat != GAVL_GRAY_FLOAT)
    return 0;

  for(i = 0; i < format->image_height; i++)
    {
    for(j = 0; j < format->image_width; j++)
      {
      
      }
    
    }
  
  return 1;
  }
