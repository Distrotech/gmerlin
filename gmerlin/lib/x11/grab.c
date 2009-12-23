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
#include <limits.h> // INT_MIN

#include <gmerlin/translation.h>

#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <X11/extensions/shape.h>
#include <X11/extensions/XShm.h>
#include <X11/Xatom.h>

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include <sys/shm.h>

#define DRAW_CURSOR         (1<<0)
#define GRAB_ROOT           (1<<1)
#define WIN_ONTOP           (1<<2)
#define WIN_STICKY          (1<<3)
#define DISABLE_SCREENSAVER (1<<4)

#define LOG_DOMAIN "x11grab"
#include <gmerlin/log.h>

#define MAX_CURSOR_SIZE 32

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
      .val_default = { .val_i = 1 },
    },
    {
      .name =      "win_ontop",
      .long_name = TRS("Keep grab window on top"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name =      "win_sticky",
      .long_name = TRS("Make grab window sticky"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name =      "disable_screensaver",
      .long_name = TRS("Disable screensaver"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Disable screensaver and energy saving mode"),
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
    {
      .name = "decoration_x",
      .long_name = "DX",
      .type  = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 0 },
    },
    {
      .name = "decoration_y",
      .long_name = "DY",
      .type  = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 0 },
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
  int cfg_flags;

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

  int decoration_x, decoration_y;

#ifdef HAVE_XFIXES
  int use_xfixes;
  int xfixes_eventbase;
  int cursor_changed;
#endif

  gavl_overlay_t      cursor;
  gavl_video_format_t cursor_format;
  
  int cursor_off_x;
  int cursor_off_y;

  int cursor_x;
  int cursor_y;
  
  gavl_rectangle_i_t cursor_rect;

  
  gavl_overlay_blend_context_t * blend;

  bg_x11_screensaver_t scr;
  
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
    return;
    }
  else if(!strcmp(name, "x"))
    {
    win->win_rect.x = val->val_i;
    }
  else if(!strcmp(name, "y"))
    {
    win->win_rect.y = val->val_i;
    }
  else if(!strcmp(name, "w"))
    {
    win->win_rect.w = val->val_i;
    }
  else if(!strcmp(name, "h"))
    {
    win->win_rect.h = val->val_i;
    }
  else if(!strcmp(name, "decoration_x"))
    {
    win->decoration_x = val->val_i;
    }
  else if(!strcmp(name, "decoration_y"))
    {
    win->decoration_y = val->val_i;
    }
  else if(!strcmp(name, "root"))
    {
    if(val->val_i)
      win->cfg_flags |= GRAB_ROOT;
    else
      win->cfg_flags &= ~GRAB_ROOT;
    }
  else if(!strcmp(name, "win_ontop"))
    {
    if(val->val_i)
      win->cfg_flags |= WIN_ONTOP;
    else
      win->cfg_flags &= ~WIN_ONTOP;
    }
  else if(!strcmp(name, "win_sticky"))
    {
    if(val->val_i)
      win->cfg_flags |= WIN_STICKY;
    else
      win->cfg_flags &= ~WIN_STICKY;
    }
  else if(!strcmp(name, "draw_cursor"))
    {
    if(val->val_i)
      win->cfg_flags |= DRAW_CURSOR;
    else
      win->cfg_flags &= ~DRAW_CURSOR;
    }
  else if(!strcmp(name, "disable_screensaver"))
    {
    if(val->val_i)
      win->cfg_flags |= DISABLE_SCREENSAVER;
    else
      win->cfg_flags &= ~DISABLE_SCREENSAVER;
    }
  }

int bg_x11_grab_window_get_parameter(void * data, const char * name,
                                     bg_parameter_value_t * val)
  {
  bg_x11_grab_window_t * win = data;


  if(!strcmp(name, "x"))
    {
    val->val_i = win->win_rect.x;
    return 1;
    }
  else if(!strcmp(name, "y"))
    {
    val->val_i = win->win_rect.y;
    return 1;
    }
  else if(!strcmp(name, "w"))
    {
    val->val_i = win->win_rect.w;
    return 1;
    }
  else if(!strcmp(name, "h"))
    {
    val->val_i = win->win_rect.h;
    return 1;
    }
  else if(!strcmp(name, "decoration_x"))
    {
    val->val_i = win->decoration_x;
    return 1;
    }
  else if(!strcmp(name, "decoration_y"))
    {
    val->val_i = win->decoration_y;
    return 1;
    }
  return 0;
  }

/* Static cursor bitmap taken from ffmpeg/libavdevice/x11grab.c
   Generation routine is native development though */

/* 16x20x1bpp bitmap for the black channel of the mouse pointer */
static const uint16_t const cursor_black[] =

  {
    0x0000, 0x0003, 0x0005, 0x0009, 0x0011,
    0x0021, 0x0041, 0x0081, 0x0101, 0x0201,
    0x03c1, 0x0049, 0x0095, 0x0093, 0x0120,
    0x0120, 0x0240, 0x0240, 0x0380, 0x0000
  };

/* 16x20x1bpp bitmap for the white channel of the mouse pointer */

static const uint16_t const cursor_white[] =
  {
    0x0000, 0x0000, 0x0002, 0x0006, 0x000e,
    0x001e, 0x003e, 0x007e, 0x00fe, 0x01fe,
    0x003e, 0x0036, 0x0062, 0x0060, 0x00c0,
    0x00c0, 0x0180, 0x0180, 0x0000, 0x0000
  };

static const float cursor_transparent[4] = { 0.0, 0.0, 0.0, 0.0 };

#define CURSOR_WIDTH  16
#define CURSOR_HEIGHT 20

static void create_cursor_static(bg_x11_grab_window_t * win)
  {
  int i, j;

  uint16_t black;
  uint16_t white;
  uint8_t * ptr;
  
  gavl_video_frame_fill(win->cursor.frame,
                        &win->cursor_format,
                        cursor_transparent);
  
  for(i = 0; i < CURSOR_HEIGHT; i++)
    {
    black = cursor_black[i];
    white = cursor_white[i];
    ptr = win->cursor.frame->planes[0] +
      i * win->cursor.frame->strides[0];
    
    for(j = 0; j < CURSOR_WIDTH; j++)
      {
      if(white & 0x01)
        {
        ptr[0] = 0xff;
        ptr[1] = 0xff;
        ptr[2] = 0xff;
        ptr[3] = 0xff;
        }
      else if(black & 0x01)
        {
        ptr[0] = 0x00;
        ptr[1] = 0x00;
        ptr[2] = 0x00;
        ptr[3] = 0xff;
        }
      ptr += 4;
      black >>= 1;
      white >>= 1;
      }
    }
  win->cursor.ovl_rect.w = CURSOR_WIDTH;
  win->cursor.ovl_rect.h = CURSOR_HEIGHT;
  }

static void create_window(bg_x11_grab_window_t * ret)
  {
  XSetWindowAttributes attr;
  unsigned long valuemask;
  Atom wm_protocols[1];

  /* Create window */

  attr.background_pixmap = None;

  valuemask = CWBackPixmap; 
  
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

  wm_protocols[0] = XInternAtom(ret->dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(ret->dpy, ret->win, wm_protocols, 1);
  
  XmbSetWMProperties(ret->dpy, ret->win, "X11 grab",
                     "X11 grab", NULL, 0, NULL, NULL, NULL);

  if(!(ret->flags & GRAB_ROOT))
    {
    if(ret->flags & (WIN_ONTOP|WIN_STICKY))
      {
      Atom wm_states[2];
      int num_props = 0;
      Atom wm_state = XInternAtom(ret->dpy, "_NET_WM_STATE", False);

      if(ret->flags & WIN_ONTOP)
        {
        wm_states[num_props++] =
          XInternAtom(ret->dpy, "_NET_WM_STATE_ABOVE", False);
        }
      if(ret->flags & WIN_STICKY)
        {
        wm_states[num_props++] =
          XInternAtom(ret->dpy, "_NET_WM_STATE_STICKY", False);
        }
      
      XChangeProperty(ret->dpy, ret->win, wm_state, XA_ATOM, 32,
                      PropModeReplace,
                      (unsigned char *)wm_states, num_props);
      XSync(ret->dpy, False);
      }
    
    XMapWindow(ret->dpy, ret->win);
    XSync(ret->dpy, False);
    XMoveWindow(ret->dpy, ret->win, ret->decoration_x, ret->decoration_y);
    }
  }

static int realize_window(bg_x11_grab_window_t * ret)
  {
#ifdef HAVE_XFIXES
  int xfixes_errorbase;
#endif
  
  /* Open Display */
  ret->dpy = XOpenDisplay(NULL);
  
  if(!ret->dpy)
    return 0;

  bg_x11_screensaver_init(&ret->scr, ret->dpy);
  
  /* Get X11 stuff */
  ret->screen = DefaultScreen(ret->dpy);
  ret->visual = DefaultVisual(ret->dpy, ret->screen);
  ret->depth = DefaultDepth(ret->dpy, ret->screen);
  
  ret->root = RootWindow(ret->dpy, ret->screen);

  /* Check for XFixes */
#ifdef HAVE_XFIXES
  ret->use_xfixes = XFixesQueryExtension(ret->dpy,
                                         &ret->xfixes_eventbase,
                                         &xfixes_errorbase);

  if(ret->use_xfixes)
    XFixesSelectCursorInput(ret->dpy, ret->root,
                            XFixesDisplayCursorNotifyMask);
  else
#endif
    create_cursor_static(ret);
  
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
  ret->blend = gavl_overlay_blend_context_create();

  ret->cursor_format.image_width  = MAX_CURSOR_SIZE;
  ret->cursor_format.image_height = MAX_CURSOR_SIZE;
  ret->cursor_format.frame_width  = MAX_CURSOR_SIZE;
  ret->cursor_format.frame_height = MAX_CURSOR_SIZE;
  ret->cursor_format.pixel_width = 1;
  ret->cursor_format.pixel_height = 1;
  
  // Fixme: This will break for > 8 bit/channel RGB visuals
  //        and other 8bit / channel RGBA formats
  ret->cursor_format.pixelformat = GAVL_RGBA_32; 

  ret->cursor.frame = gavl_video_frame_create(&ret->cursor_format);
  
  return ret;
  }

void bg_x11_grab_window_destroy(bg_x11_grab_window_t * win)
  {
  if(win->win != None)
    XDestroyWindow(win->dpy, win->win);
  if(win->dpy)
    XCloseDisplay(win->dpy);

  bg_x11_screensaver_cleanup(&win->scr);
  
  gavl_timer_destroy(win->timer);
  gavl_overlay_blend_context_destroy(win->blend);
  gavl_video_frame_destroy(win->cursor.frame);
  free(win);
  }

static void handle_events(bg_x11_grab_window_t * win)
  {
  XEvent evt;

  bg_x11_screensaver_ping(&win->scr);
  
  while(XPending(win->dpy))
    {
    XNextEvent(win->dpy, &evt);

#ifdef HAVE_XFIXES
    if(evt.type == win->xfixes_eventbase + XFixesCursorNotify)
      {
      win->cursor_changed = 1;
      // fprintf(stderr, "Cursor notify\n");
      continue;
      }
#endif

    
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

          win->decoration_x = x_return;
          win->decoration_y = y_return;
          
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
  win->flags = win->cfg_flags;

  if(!win->dpy)
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

  if(win->flags & DRAW_CURSOR)
    {
    gavl_overlay_blend_context_init(win->blend,
                                    &win->format,
                                    &win->cursor_format);
#ifdef HAVE_XFIXES
    if(win->use_xfixes)
      win->cursor_changed = 1;
#endif
    
    win->cursor_x = INT_MIN;
    win->cursor_y = INT_MIN;
    }

  create_window(win);
  handle_events(win);
  
  gavl_timer_set(win->timer, 0);
  gavl_timer_start(win->timer);

  if(win->flags & DISABLE_SCREENSAVER)
    bg_x11_screensaver_disable(&win->scr);
  
  return 1;
  }

void bg_x11_grab_window_close(bg_x11_grab_window_t * win)
  {
  gavl_timer_stop(win->timer);
  bg_x11_screensaver_enable(&win->scr);
  
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

  if(!(win->flags & GRAB_ROOT))
    {
    XUnmapWindow(win->dpy, win->win);
    
    XSync(win->dpy, False);
    }
  //  handle_events(win);
  XDestroyWindow(win->dpy, win->win);
  win->win = None;
  }

#ifdef HAVE_XFIXES
static void get_cursor_xfixes(bg_x11_grab_window_t * win)
  {
  int i, j;
  unsigned long * src;
  uint8_t * dst;
  
  XFixesCursorImage *im;
  im = XFixesGetCursorImage(win->dpy);

  win->cursor.ovl_rect.w = im->width;
  win->cursor.ovl_rect.h = im->height;

  if(win->cursor.ovl_rect.w > MAX_CURSOR_SIZE)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cursor too wide, increase MAX_CURSOR_SIZE in grab.c to %d",
           win->cursor.ovl_rect.w);
    win->cursor.ovl_rect.w = MAX_CURSOR_SIZE;
    }
  if(win->cursor.ovl_rect.h > MAX_CURSOR_SIZE)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cursor too high, increase MAX_CURSOR_SIZE in grab.c to %d",
           win->cursor.ovl_rect.h);
    win->cursor.ovl_rect.h = MAX_CURSOR_SIZE;
    }
  
  win->cursor_off_x = im->xhot;
  win->cursor_off_y = im->yhot;

  for(i = 0; i < win->cursor.ovl_rect.h; i++)
    {
    src = (im->pixels) + i * im->width;
    dst = win->cursor.frame->planes[0] + i * win->cursor.frame->strides[0];

    for(j = 0; j < win->cursor.ovl_rect.w; j++)
      {
      dst[3] = *src >> 24;          // A
      dst[0] = (*src >> 16) & 0xff; // R
      dst[1] = (*src >> 8)  & 0xff; // G
      dst[2] = (*src)       & 0xff; // B

      dst += 4;
      src ++;
      }
    }
  win->cursor_changed = 0;
  XFree(im);
  }
#endif

static void draw_cursor(bg_x11_grab_window_t * win, gavl_rectangle_i_t * rect,
                        gavl_video_frame_t * frame)
  {
  Window root;
  Window child;
  int root_x;
  int root_y;
  int win_x;
  int win_y;
  unsigned int mask;
  int init_blend = 0;
  
  if(!XQueryPointer(win->dpy, win->root, &root,
                    &child, &root_x,
                    &root_y, &win_x, &win_y,
                    &mask))
    return;
  
  /* Bounding box check */
  if(root_x >= rect->x + rect->w + MAX_CURSOR_SIZE)
    return;

  if(root_x + MAX_CURSOR_SIZE < rect->x)
    return;

  if(root_y >= rect->y + rect->h + MAX_CURSOR_SIZE)
    return;

  if(root_y + MAX_CURSOR_SIZE < rect->y)
    return;

  win->cursor.dst_x = root_x - rect->x - win->cursor_off_x;
  win->cursor.dst_y = root_y - rect->y - win->cursor_off_y;

  if((win->cursor.dst_x != win->cursor_x) ||
     (win->cursor.dst_y != win->cursor_y))
    init_blend = 1;

#ifdef HAVE_XFIXES
  if(win->cursor_changed)
    {
    init_blend = 1;
    get_cursor_xfixes(win);
    }
#endif
  
  if(init_blend)
    {
    gavl_overlay_blend_context_set_overlay(win->blend,
                                           &win->cursor);
    }
  
  gavl_overlay_blend(win->blend, frame);
  // fprintf(stderr, "Cursor 2: %d %d\n", win->cursor.dst_x, win->cursor.dst_y);

  win->cursor_x = win->cursor.dst_x;
  win->cursor_x = win->cursor.dst_y;
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
  
  if(win->use_shm)
    {
    gavl_rectangle_i_copy(&rect, &win->grab_rect);

    if(rect.x < 0)
      rect.x = 0;
    if(rect.y < 0)
      rect.y = 0;

    if(rect.x + rect.w > win->root_width)
      rect.x = win->root_width - rect.w;

    if(rect.y + rect.h > win->root_height)
      rect.y = win->root_height - rect.h;
    
    //    fprintf(stderr, "XShmGetImage %d %d\n", rect.x, rect.y);
    if(!XShmGetImage(win->dpy, win->root, win->image, rect.x, rect.y, AllPlanes))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "XShmGetImage failed");
      }
    }
  else
    {
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
    
    XGetSubImage(win->dpy, win->root,
                 rect.x, rect.y, rect.w, rect.h,
                 AllPlanes, ZPixmap, win->image,
                 crop_left, crop_top);
    }
    
  gavl_video_frame_copy(&win->format, frame, win->frame);

  if(win->flags & DRAW_CURSOR)
    draw_cursor(win, &rect, frame);
  
  frame->timestamp = gavl_time_scale(win->format.timescale, gavl_timer_get(win->timer));
  return 1;
  }
