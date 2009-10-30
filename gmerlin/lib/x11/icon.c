/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <x11/x11.h>
#include <x11/x11_window_private.h>

void bg_x11_window_make_icon(bg_x11_window_t * win,
                             const gavl_video_frame_t * icon,
                             const gavl_video_format_t * icon_format,
                             Pixmap * icon_ret, Pixmap * mask_ret)
  {
  XImage * im;
  
  if(icon_ret)
    {
    gavl_video_format_t out_format;
    gavl_video_converter_t * cnv;
    gavl_video_options_t * opt;
    cnv = gavl_video_converter_create();
    opt = gavl_video_converter_get_options(cnv);
    gavl_video_options_set_alpha_mode(opt, GAVL_ALPHA_IGNORE);
     
    gavl_video_converter_destroy(cnv);
    }

  }

