/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

/* Common defintions and routines for creating y4m streams */

typedef struct
  {
  gavl_video_format_t format;
  /* For checking if an incoming frame can be read without memcpying */
  int strides[4]; 
  
  int chroma_mode;  

  y4m_stream_info_t si;
  y4m_frame_info_t fi;
  
  int fd;

  gavl_video_frame_t * frame;
  uint8_t * tmp_planes[4];
  
  bg_encoder_framerate_t fr;
  } bg_y4m_common_t;

/* Set pixelformat and chroma placement from chroma_mode */

void bg_y4m_set_pixelformat(bg_y4m_common_t * com);
int bg_y4m_write_header(bg_y4m_common_t * com);

int bg_y4m_write_frame(bg_y4m_common_t * com, gavl_video_frame_t * frame);

void bg_y4m_cleanup(bg_y4m_common_t * com);
