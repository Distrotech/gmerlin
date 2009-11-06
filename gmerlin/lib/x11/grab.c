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

#include <config.h>
#include <stdlib.h>

#include <gmerlin/translation.h>


#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <X11/extensions/shape.h>

static const bg_parameter_info_t parameters[] = 
  {
    {
      .name =      "root",
      .long_name = TRS("Grab from root window"),
      .type = BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name =      "draw_cursor",
      .long_name = TRS("Draw cursor"),
      .type = BG_PARAMETER_CHECKBUTTON,
    },
    
    { /* End */ },
  };

struct bg_x11_grab_window_s
  {
  Display * dpy;
  Window win;
  Window root;
  gavl_rectangle_i_t grab_rect;
  
  };

const bg_parameter_info_t * bg_x11_grab_window_get_parameters(bg_x11_grab_window_t * win)
  {
  return parameters;
  }

void bg_x11_grab_window_set_parameter(void * data, const char * name,
                                      const bg_parameter_value_t * val)
  {

  }

static int realize_window(bg_x11_grab_window_t * ret)
  {
  XSetWindowAttributes attr;
  unsigned long valuemask;
  int screen;
  /* Open Display */
  ret->dpy = XOpenDisplay(NULL);
  
  if(!ret->dpy)
    return 0;
  
  /* Get X11 stuff */
  screen = DefaultScreen(ret->dpy);

  ret->root = RootWindow(ret->dpy, screen);
  /* Create atoms */
  
  /* Create window */

  attr.background_pixmap = None;

  valuemask = CWBackPixmap; 
  
  ret->win = XCreateWindow(ret->dpy, ret->root,
                           0, // int x,
                           0, // int y,
                           100, // unsigned int width,
                           100, // unsigned int height,
                           3, // unsigned int border_width,
                           DefaultDepth(ret->dpy, screen),
                           InputOutput,
                           DefaultVisual(ret->dpy, screen),
                           valuemask,
                           &attr);

  XShapeCombineRectangles(ret->dpy,
                          ret->win,
                          ShapeBounding,
                          0, 0,
                          (XRectangle *)0,
                          0,
                          ShapeSet,
                          YXBanded); 
  
  return 1;
  }

bg_x11_grab_window_t * bg_x11_grab_window_create()
  {
  
  bg_x11_grab_window_t * ret = calloc(1, sizeof(*ret));

  /* Initialize members */  
  ret->win = None;

  if(!realize_window(ret))
    goto fail;
  
  return ret;

  fail:
  bg_x11_grab_window_destroy(ret);
  return NULL;
  }

void bg_x11_grab_window_destroy(bg_x11_grab_window_t * win)
  {
  if(win->win != None)
    XDestroyWindow(win->dpy, win->win);
  if(win->dpy)
    XCloseDisplay(win->dpy);
  free(win);
  }

static void handle_events(bg_x11_grab_window_t * win)
  {
  XEvent evt;
  while(XPending(win->dpy))
    XNextEvent(win->dpy, &evt);
  
  }

int bg_x11_grab_window_init(bg_x11_grab_window_t * win, gavl_video_format_t * format)
  {
  // XSetWMSizeHints();

  format->image_width = 100;
  format->image_height = 100;
  format->frame_width = 100;
  format->frame_height = 100;
  format->pixel_width = 1;
  format->pixel_height = 1;
  format->pixelformat = GAVL_YUV_420_P;
  format->framerate_mode = GAVL_FRAMERATE_VARIABLE;
  format->timescale = 1000;
  format->frame_duration = 0;

  XMapWindow(win->dpy, win->win);

  handle_events(win);
  
  return 1;
  }

void bg_x11_grab_window_close(bg_x11_grab_window_t * win)
  {
  XUnmapWindow(win->dpy, win->win);
  }

int bg_x11_grab_window_grab(bg_x11_grab_window_t * win, gavl_video_frame_t  * frame)
  {
  handle_events(win);
  return 1;
  }
