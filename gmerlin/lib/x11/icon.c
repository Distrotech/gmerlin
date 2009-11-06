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

/* Warning: These functions are not optimized and not intended to
   be used very often */

static Pixmap make_icon(bg_x11_window_t * win,
                        const gavl_video_frame_t * icon,
                        const gavl_video_format_t * format)
  {
  XImage * im;
  gavl_video_format_t out_format;
  gavl_video_converter_t * cnv;
  gavl_video_options_t * opt;
  int do_convert;
  const gavl_video_frame_t * image_frame;
  gavl_video_frame_t * out_frame;
  
  Pixmap ret;
  
  /* Create converter */
  cnv = gavl_video_converter_create();
  opt = gavl_video_converter_get_options(cnv);
  gavl_video_options_set_alpha_mode(opt, GAVL_ALPHA_IGNORE);

  /* Create pixmap */
  ret = XCreatePixmap(win->dpy, win->root, format->image_width,
                      format->image_height, win->depth);

  /* Set up format and converter */
  gavl_video_format_copy(&out_format, format);
  out_format.pixelformat = bg_x11_window_get_pixelformat(win->dpy, win->visual, win->depth);

  do_convert = gavl_video_converter_init(cnv, format, &out_format);
  if(do_convert)
    {
    out_frame = gavl_video_frame_create(&out_format);
    image_frame = out_frame;
    gavl_video_convert(cnv, icon, out_frame);
    }
  else
    {
    image_frame = icon;
    out_frame = NULL;
    }
  
  /* Make image */
  
  im = XCreateImage(win->dpy, win->visual, win->depth,
                    ZPixmap,
                    0, (char*)(image_frame->planes[0]),
                    format->image_width,
                    format->image_height,
                    32,
                    image_frame->strides[0]);
  
  XPutImage(win->dpy,            /* dpy        */
            ret, /* d          */
            win->gc,             /* gc         */
            im, /* image      */
            0,    /* src_x      */
            0,    /* src_y      */
            0,          /* dst_x      */
            0,          /* dst_y      */
            format->image_width,    /* src_width  */
            format->image_height);  /* src_height */
  
  /* Cleanup */
  gavl_video_converter_destroy(cnv);
  if(out_frame)
    gavl_video_frame_destroy(out_frame);
  
  im->data = NULL;
  XDestroyImage(im);
  
  /* Return */
  return ret;
  
  }

static void create_mask_8(gavl_video_format_t * format,
                          gavl_video_frame_t * frame,
                          char * im, int bytes_per_line)
  {
  int i, j;
  uint8_t * ptr;
  uint8_t * im_ptr;
  int shift;
  
  for(i = 0; i < format->image_height; i++)
    {
    ptr = frame->planes[0] + i * frame->strides[0];    
    im_ptr = (uint8_t*)(im + i * bytes_per_line);
    shift = 0;
    
    for(j = 0; j < format->image_width; j++)
      {
      if(*ptr >= 0x80)
        *im_ptr |= 1 << shift;
      if(shift == 7)
        {
        im_ptr++;
        shift = 0;
        }
      else
        shift++;
      ptr++;
      }
    }
  
  }

static void create_mask_16(gavl_video_format_t * format,
                           gavl_video_frame_t * frame,
                           char * im, int bytes_per_line)
  {
  int i, j;
  uint16_t * ptr;
  uint8_t * im_ptr;
  int shift;

  for(i = 0; i < format->image_height; i++)
    {
    ptr = (uint16_t*)(frame->planes[0] + i * frame->strides[0]);
    im_ptr = (uint8_t*)(im + i * bytes_per_line);
    shift = 0;

    for(j = 0; j < format->image_width; j++)
      {
      if(*ptr >= 0x8000)
        *im_ptr |= 1 << shift;
      if(shift == 7)
        {
        im_ptr++;
        shift = 0;
        }
      else
        shift++;
      ptr++;
      }
    }
  
  }

static void create_mask_float(gavl_video_format_t * format,
                              gavl_video_frame_t * frame,
                              char * im, int bytes_per_line)
  {
  int i, j;
  float * ptr;
  uint8_t * im_ptr;
  int shift;

  for(i = 0; i < format->image_height; i++)
    {
    ptr = (float*)(frame->planes[0] + i * frame->strides[0]);
    im_ptr = (uint8_t*)(im + i * bytes_per_line);
    shift = 0;

    for(j = 0; j < format->image_width; j++)
      {
      if(*ptr >= 0.5)
        *im_ptr |= 1 << shift;
      if(shift == 7)
        {
        im_ptr++;
        shift = 0;
        }
      else
        shift++;
      ptr++;
      }
    }

  }
                          
static Pixmap make_mask(bg_x11_window_t * win,
                        const gavl_video_frame_t * icon,
                        const gavl_video_format_t * format)
  {
  gavl_video_frame_t * alpha_frame;
  gavl_video_format_t alpha_format;
  char * image_data;
  
  Pixmap ret;
  int bytes_per_line;
  
  /* Extract alpha */
  if(!gavl_get_color_channel_format(format,
                                    &alpha_format,
                                    GAVL_CCH_ALPHA))
    return None; /* No alpha */

  alpha_frame = gavl_video_frame_create(&alpha_format);

  gavl_video_frame_extract_channel(format,
                                   GAVL_CCH_ALPHA,
                                   icon,
                                   alpha_frame);
  
  /* Create image */
  
  bytes_per_line = (format->image_width + 7) / 8;
  image_data = calloc(1, bytes_per_line * format->image_height);
  
  switch(alpha_format.pixelformat)
    {
    case GAVL_GRAY_8:
      create_mask_8(&alpha_format, alpha_frame, image_data, bytes_per_line);
      break;
    case GAVL_GRAY_16:
      create_mask_16(&alpha_format, alpha_frame, image_data, bytes_per_line);
      break;
    case GAVL_GRAY_FLOAT:
      create_mask_float(&alpha_format, alpha_frame, image_data, bytes_per_line);
      break;
    default:
      break;
    }
  ret = XCreateBitmapFromData(win->dpy, win->root,
                              image_data,
                              format->image_width,
                              format->image_height);
  
  gavl_video_frame_destroy(alpha_frame);
  free(image_data);
  return ret;
  }

void bg_x11_window_make_icon(bg_x11_window_t * win,
                             const gavl_video_frame_t * icon,
                             const gavl_video_format_t * format,
                             Pixmap * icon_ret, Pixmap * mask_ret)
  {
  if(icon_ret)
    *icon_ret = make_icon(win, icon, format);
  
  if(mask_ret)
    *mask_ret = make_mask(win, icon, format);
  }

