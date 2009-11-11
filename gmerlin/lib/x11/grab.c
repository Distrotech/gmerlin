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
#include <string.h>

#include <gmerlin/translation.h>

#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <X11/extensions/shape.h>
#include <X11/extensions/XShm.h>

#include <sys/shm.h>

#define DRAW_CURSOR (1<<0)
#define GRAB_ROOT   (1<<1)

#define LOG_DOMAIN "x11grab"
#include <gmerlin/log.h>

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
    {
      .name = "x",
      .long_name = "X",
      .type  = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 0 },
    },
    {
      .name = "y",
      .long_name = "Y",
      .type  = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 0 },
    },
    {
      .name = "w",
      .long_name = "W",
      .type  = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 320 },
    },
    {
      .name = "h",
      .long_name = "H",
      .type  = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 240 },
    },
    { /* End */ },
  };

struct bg_x11_grab_window_s
  {
  Display * dpy;
  Window win;
  Window root;
  gavl_rectangle_i_t grab_rect;
  gavl_rectangle_i_t win_rect;
  
  int flags;

  gavl_pixelformat_t pixelformat;

  gavl_timer_t * timer;

  XImage * image;
  gavl_video_frame_t * frame;

  gavl_video_format_t format;

  Visual * visual;
  int depth;
  
  int use_shm;
  XShmSegmentInfo shminfo;
  
  int root_width, root_height;
  int screen;
  
  };

const bg_parameter_info_t *
bg_x11_grab_window_get_parameters(bg_x11_grab_window_t * win)
  {
  return parameters;
  }

void bg_x11_grab_window_set_parameter(void * data, const char * name,
                                      const bg_parameter_value_t * val)
  {
  bg_x11_grab_window_t * win = data;
  if(!name)
    {
    /* Configure window */
    if(win->win != None)
      XMoveResizeWindow(win->dpy, win->win,
                        win->win_rect.x,
                        win->win_rect.y,
                        win->win_rect.w,
                        win->win_rect.h);
    }
  else if(!strcmp(name, "x"))
    {
    fprintf(stderr, "Set parameter x %d\n", val->val_i);
    win->win_rect.x = val->val_i;
    }
  else if(!strcmp(name, "y"))
    {
    fprintf(stderr, "Set parameter y %d\n", val->val_i);
    win->win_rect.y = val->val_i;
    }
  else if(!strcmp(name, "w"))
    {
    fprintf(stderr, "Set parameter w %d\n", val->val_i);
    win->win_rect.w = val->val_i;
    }
  else if(!strcmp(name, "h"))
    {
    fprintf(stderr, "Set parameter h %d\n", val->val_i);
    win->win_rect.h = val->val_i;
    }
  else if(!strcmp(name, "root"))
    {
    if(val->val_i)
      win->flags |= GRAB_ROOT;
    else
      win->flags &= ~GRAB_ROOT;
    }
  }

int bg_x11_grab_window_get_parameter(void * data, const char * name,
                                     bg_parameter_value_t * val)
  {
  bg_x11_grab_window_t * win = data;


  if(!strcmp(name, "x"))
    {
    val->val_i = win->win_rect.x;
    fprintf(stderr, "Get parameter x %d\n", val->val_i);
    return 1;
    }
  else if(!strcmp(name, "y"))
    {
    val->val_i = win->win_rect.y;
    fprintf(stderr, "Get parameter y %d\n", val->val_i);
    return 1;
    }
  else if(!strcmp(name, "w"))
    {
    val->val_i = win->win_rect.w;
    fprintf(stderr, "Get parameter w %d\n", val->val_i);
    return 1;
    }
  else if(!strcmp(name, "h"))
    {
    val->val_i = win->win_rect.h;
    fprintf(stderr, "Get parameter h %d\n", val->val_i);
    return 1;
    }
  return 0;
  }

static int realize_window(bg_x11_grab_window_t * ret)
  {
  XSetWindowAttributes attr;
  unsigned long valuemask;
  
  /* Open Display */
  ret->dpy = XOpenDisplay(NULL);
  
  if(!ret->dpy)
    return 0;
  
  /* Get X11 stuff */
  ret->screen = DefaultScreen(ret->dpy);
  ret->visual = DefaultVisual(ret->dpy, ret->screen);
  ret->depth = DefaultDepth(ret->dpy, ret->screen);
  
  ret->root = RootWindow(ret->dpy, ret->screen);
  /* Create atoms */
  
  /* Create window */

  attr.background_pixmap = None;

  valuemask = CWBackPixmap; 

  gavl_rectangle_i_dump(&ret->win_rect);
  
  ret->win = XCreateWindow(ret->dpy, ret->root,
                           ret->win_rect.x, // int x,
                           ret->win_rect.y, // int y,
                           ret->win_rect.w, // unsigned int width,
                           ret->win_rect.h, // unsigned int height,
                           0, // unsigned int border_width,
                           ret->depth,
                           InputOutput,
                           ret->visual,
                           valuemask,
                           &attr);

  XSelectInput(ret->dpy, ret->win, StructureNotifyMask);
  
  XShapeCombineRectangles(ret->dpy,
                          ret->win,
                          ShapeBounding,
                          0, 0,
                          (XRectangle *)0,
                          0,
                          ShapeSet,
                          YXBanded); 

  XmbSetWMProperties(ret->dpy, ret->win, "X11 grab",
                     "X11 grab", NULL, 0, NULL, NULL, NULL);


  bg_x11_window_get_coords(ret->dpy, ret->root,
                           (int*)0, (int*)0,
                           &ret->root_width, &ret->root_height);
  
  ret->pixelformat =
    bg_x11_window_get_pixelformat(ret->dpy, ret->visual, ret->depth);

  ret->use_shm = XShmQueryExtension(ret->dpy);
  return 1;
  }

bg_x11_grab_window_t * bg_x11_grab_window_create()
  {
  
  bg_x11_grab_window_t * ret = calloc(1, sizeof(*ret));

  /* Initialize members */  
  ret->win = None;

  ret->timer = gavl_timer_create();
  
  return ret;
  }

void bg_x11_grab_window_destroy(bg_x11_grab_window_t * win)
  {
  if(win->win != None)
    XDestroyWindow(win->dpy, win->win);
  if(win->dpy)
    XCloseDisplay(win->dpy);
  
  gavl_timer_destroy(win->timer);
  
  free(win);
  }

static void handle_events(bg_x11_grab_window_t * win)
  {
  XEvent evt;
  while(XPending(win->dpy))
    {
    XNextEvent(win->dpy, &evt);

    switch(evt.type)
      {
      case ConfigureNotify:
        {
        Window * children_return;
        Window root_return;
        Window parent_return;
        unsigned int nchildren_return;
        int x_return, y_return;
        unsigned int width_return, height_return;
        unsigned int border_width_return;
        unsigned int depth_return;
        
        win->win_rect.w = evt.xconfigure.width;
        win->win_rect.h = evt.xconfigure.height;
        
        XGetGeometry(win->dpy, win->win, &root_return, &x_return, &y_return,
                     &width_return, &height_return,
                     &border_width_return, &depth_return);

        win->win_rect.x = x_return;
        win->win_rect.y = y_return;
        
        XQueryTree(win->dpy, win->win, &root_return, &parent_return,
                   &children_return, &nchildren_return);

        if(nchildren_return)
          XFree(children_return);

        /* Get offset of the window frame */
        if(parent_return != root_return)
          {
          XGetGeometry(win->dpy, parent_return, &root_return, &x_return, &y_return,
                       &width_return, &height_return,
                       &border_width_return, &depth_return);
          
          win->win_rect.x += x_return;
          win->win_rect.y += y_return;
          }
        
        if(!(win->flags & GRAB_ROOT))
          {
          win->grab_rect.x = win->win_rect.x;
          win->grab_rect.y = win->win_rect.y;
          }
        
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Window geometry: %dx%d+%d+%d",
               win->win_rect.w, win->win_rect.h,
               win->win_rect.x, win->win_rect.y);
        }
        break;
      }
    }
  }

int bg_x11_grab_window_init(bg_x11_grab_window_t * win,
                            gavl_video_format_t * format)
  {
  if(win->win == None)
    {
    if(!realize_window(win))
      return 0;
    }
  
  if(win->flags & GRAB_ROOT)
    {
    /* TODO */

    format->image_width = win->root_width;
    format->image_height = win->root_height;

    win->grab_rect.x = 0;
    win->grab_rect.y = 0;
    
    win->grab_rect.w = win->root_width;
    win->grab_rect.h = win->root_height;
    
    }
  else
    {
    format->image_width = win->win_rect.w;
    format->image_height = win->win_rect.h;
    gavl_rectangle_i_copy(&win->grab_rect, &win->win_rect);
    }
  
  /* Common format parameters */
  format->pixel_width = 1;
  format->pixel_height = 1;
  format->pixelformat = win->pixelformat;
  format->framerate_mode = GAVL_FRAMERATE_VARIABLE;
  format->timescale = 1000;
  format->frame_duration = 0;

  format->frame_width = format->image_width;
  format->frame_height = format->image_height;
  
  gavl_video_format_copy(&win->format, format);

  /* Create image */
  if(win->use_shm)
    {
    win->frame = gavl_video_frame_create(NULL);
    win->image = XShmCreateImage(win->dpy,
                                 win->visual,
                                 win->depth,
                                 ZPixmap,
                                 NULL,
                                 &win->shminfo,
                                 format->frame_width,
                                 format->frame_height);
    win->shminfo.shmid = shmget(IPC_PRIVATE,
                                    win->image->bytes_per_line * win->image->height,
                                    IPC_CREAT|0777);
    if(win->shminfo.shmid == -1)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't get shared memory segment");
      return 0;
      }
    win->shminfo.shmaddr = shmat(win->shminfo.shmid, 0, 0);
    win->image->data = win->shminfo.shmaddr;
    
    if(!XShmAttach(win->dpy, &win->shminfo))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't attach shared memory segment");
      return 0;
      }
    win->frame->planes[0] = (uint8_t*)win->image->data;
    win->frame->strides[0] = win->image->bytes_per_line;

    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using shared memory for grabbing");
    }
  else
    {
    win->frame = gavl_video_frame_create(format);
    win->image = XCreateImage(win->dpy, win->visual, win->depth,
                              ZPixmap,
                              0, (char*)(win->frame->planes[0]),
                              format->frame_width,
                              format->frame_height,
                              32,
                              win->frame->strides[0]);
    }
  
  if(!(win->flags & GRAB_ROOT))
    XMapWindow(win->dpy, win->win);
  
  handle_events(win);
  
  gavl_timer_set(win->timer, 0);
  gavl_timer_start(win->timer);
  
  return 1;
  }

void bg_x11_grab_window_close(bg_x11_grab_window_t * win)
  {
  gavl_timer_stop(win->timer);

  if(win->use_shm)
    {
    gavl_video_frame_null(win->frame);
    gavl_video_frame_destroy(win->frame);

    XShmDetach(win->dpy, &win->shminfo);
    shmdt(win->shminfo.shmaddr);
    shmctl(win->shminfo.shmid, IPC_RMID, NULL);
    XDestroyImage(win->image);
    }
  else
    {
    gavl_video_frame_destroy(win->frame);
    win->image->data = NULL;
    XDestroyImage(win->image);
    }
  
  XUnmapWindow(win->dpy, win->win);
  }

int bg_x11_grab_window_grab(bg_x11_grab_window_t * win,
                            gavl_video_frame_t  * frame)
  {
  int crop_left = 0;
  int crop_right = 0;
  int crop_top = 0;
  int crop_bottom = 0;

  gavl_rectangle_i_t rect;
  
  handle_events(win);

  /* Crop */
  if(win->grab_rect.x < 0)
    crop_left = -win->grab_rect.x;
  if(win->grab_rect.y < 0)
    crop_top = -win->grab_rect.y;

  if(win->grab_rect.x + win->grab_rect.w > win->root_width)
    crop_right = win->grab_rect.x + win->grab_rect.w - win->root_width;

  if(win->grab_rect.y + win->grab_rect.h > win->root_height)
    crop_bottom = win->grab_rect.y + win->grab_rect.h - win->root_height;
  
  if(crop_left || crop_right || crop_top || crop_bottom)
    {
    gavl_video_frame_clear(win->frame, &win->format);
    }

  gavl_rectangle_i_copy(&rect, &win->grab_rect);

  rect.x += crop_left;
  rect.y += crop_top;
  rect.w -= (crop_left + crop_right);
  rect.h -= (crop_top + crop_bottom);
  
  if(win->use_shm)
    {
    if(!XShmGetImage(win->dpy, win->root, win->image, rect.x, rect.y, AllPlanes))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "XShmGetImage failed");
      }
    }
  else
    {
    XGetSubImage(win->dpy, win->root,
                 rect.x, rect.y, rect.w, rect.h,
                 AllPlanes, ZPixmap, win->image,
                 crop_left, crop_top);
    }
    
  gavl_video_frame_copy(&win->format, frame, win->frame);
  
  frame->timestamp = gavl_time_scale(gavl_timer_get(win->timer),
                                     win->format.timescale);
  
  // fprintf(stderr, "Timestamp: %"PRId64"\n", frame->timestamp);
  
  return 1;
  }
