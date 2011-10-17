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
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include <time.h>
#include <sys/time.h>

#include <X11/Xatom.h>

#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "x11"

#include <x11/x11.h>
#include <x11/x11_window_private.h>

#ifdef HAVE_GLX
#include <GL/glx.h>
#endif

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */

// #define FULLSCREEN_MODE_OLD    0
#define FULLSCREEN_MODE_NET_FULLSCREEN   (1<<0)
#define FULLSCREEN_MODE_NET_ABOVE        (1<<1)
#define FULLSCREEN_MODE_NET_STAYS_ON_TOP (1<<2)

#define FULLSCREEN_MODE_WIN_LAYER      (1<<2)

#define HAVE_XSHM


static char bm_no_data[] = { 0,0,0,0, 0,0,0,0 };

/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase (Display *);

/* Screensaver detection */

static int check_disable_screensaver(bg_x11_window_t * w)
  {
  if(!w->current)
    return 0;
  /* Never change global settings if we are embedded */
  if(w->current->parent != w->root)
    return 0;
  
  if(!w->is_fullscreen)
    return w->disable_screensaver_normal;
  else
    return w->disable_screensaver_fullscreen;
  }

void bg_x11_window_ping_screensaver(bg_x11_window_t * w)
  {
  }



static int
wm_check_capability(Display *dpy, Window root, Atom list, Atom wanted)
  {
  Atom            type;
  int             format;
  unsigned int    i;
  unsigned long   nitems, bytesafter;
  unsigned char   *args;
  unsigned long   *ldata;
  int             retval = 0;
  
  if (Success != XGetWindowProperty
      (dpy, root, list, 0, 16384, False,
       AnyPropertyType, &type, &format, &nitems, &bytesafter, &args))
    return 0;
  if (type != XA_ATOM)
    return 0;
  ldata = (unsigned long*)args;
  for (i = 0; i < nitems; i++)
    {
    if (ldata[i] == wanted)
      retval = 1;

    }
  XFree(ldata);
  return retval;
  }

void
bg_x11_window_set_netwm_state(Display * dpy, Window win, Window root, int action, Atom state)
  {
  /* Setting _NET_WM_STATE by XSendEvent works only, if the window
     is already mapped!! */
  
  XEvent e;
  memset(&e,0,sizeof(e));
  e.xclient.type = ClientMessage;
  e.xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
  e.xclient.window = win;
  e.xclient.send_event = True;
  e.xclient.format = 32;
  e.xclient.data.l[0] = action;
  e.xclient.data.l[1] = state;
  
  XSendEvent(dpy, root, False,
             SubstructureRedirectMask | SubstructureNotifyMask, &e);
  }

static int get_fullscreen_mode(bg_x11_window_t * w)
  {
  int ret = 0;

  Atom            type;
  int             format;
  unsigned int    i;
  unsigned long   nitems, bytesafter;
  unsigned char   *args;
  unsigned long   *ldata;
  if (Success != XGetWindowProperty
      (w->dpy, w->root, w->_NET_SUPPORTED, 0, (65536 / sizeof(long)), False,
       AnyPropertyType, &type, &format, &nitems, &bytesafter, &args))
    return 0;
  if (type != XA_ATOM)
    return 0;
  ldata = (unsigned long*)args;
  for (i = 0; i < nitems; i++)
    {
    if (ldata[i] == w->_NET_WM_STATE_FULLSCREEN)
      {
      ret |= FULLSCREEN_MODE_NET_FULLSCREEN;
      }
    if (ldata[i] == w->_NET_WM_STATE_ABOVE)
      {
      ret |= FULLSCREEN_MODE_NET_ABOVE;
      }
    if (ldata[i] == w->_NET_WM_STATE_STAYS_ON_TOP)
      {
      ret |= FULLSCREEN_MODE_NET_STAYS_ON_TOP;
      }
    }
  XFree(ldata);
  
  if(wm_check_capability(w->dpy, w->root, w->WIN_PROTOCOLS,
                         w->WIN_LAYER))
    {
    ret |= FULLSCREEN_MODE_WIN_LAYER;
    }
  
  return ret;
  }

#define INIT_ATOM(a) w->a = XInternAtom(w->dpy, #a, False);

static void init_atoms(bg_x11_window_t * w)
  {
  INIT_ATOM(WM_DELETE_WINDOW);
  INIT_ATOM(WM_TAKE_FOCUS);
  INIT_ATOM(WIN_PROTOCOLS);
  INIT_ATOM(WM_PROTOCOLS);
  INIT_ATOM(WIN_LAYER);
  INIT_ATOM(_NET_SUPPORTED);
  INIT_ATOM(_NET_WM_STATE);
  INIT_ATOM(_NET_WM_STATE_FULLSCREEN);
  INIT_ATOM(_NET_WM_STATE_ABOVE);
  INIT_ATOM(_NET_WM_STATE_STAYS_ON_TOP);
  INIT_ATOM(_NET_MOVERESIZE_WINDOW);
  INIT_ATOM(_XEMBED_INFO);
  INIT_ATOM(_XEMBED);
  INIT_ATOM(WM_CLASS);
  INIT_ATOM(STRING);
  }

void bg_x11_window_set_fullscreen_mapped(bg_x11_window_t * win,
                                         window_t * w)
  {
  if(win->fullscreen_mode & FULLSCREEN_MODE_NET_ABOVE)
    {
    bg_x11_window_set_netwm_state(win->dpy, w->win, win->root,
                                  _NET_WM_STATE_ADD, win->_NET_WM_STATE_ABOVE);
    }
  else if(win->fullscreen_mode & FULLSCREEN_MODE_NET_STAYS_ON_TOP)
    {
    bg_x11_window_set_netwm_state(win->dpy, w->win, win->root,
                                  _NET_WM_STATE_ADD, win->_NET_WM_STATE_STAYS_ON_TOP);
    }
  if(win->fullscreen_mode & FULLSCREEN_MODE_NET_FULLSCREEN)
    {
    bg_x11_window_set_netwm_state(win->dpy, w->win, win->root,
                                  _NET_WM_STATE_ADD, win->_NET_WM_STATE_FULLSCREEN);
    }
  }

static void show_window(bg_x11_window_t * win, window_t * w, int show)
  {
  unsigned long buffer[2];

  if(!w)
    return;
  
  if(!show)
    {
    
    if(w->win == None)
      return;
    
    if(w->win == w->toplevel)
      {
      XUnmapWindow(win->dpy, w->win);
      XWithdrawWindow(win->dpy, w->win,
                      DefaultScreen(win->dpy));
      }
    else if(w->parent_xembed)
      {
      buffer[0] = 0; // Version
      buffer[1] = 0; 
      
      XChangeProperty(win->dpy,
                      w->win,
                      win->_XEMBED_INFO,
                      win->_XEMBED_INFO, 32,
                      PropModeReplace,
                      (unsigned char *)buffer, 2);
      return;
      }
    else
      XUnmapWindow(win->dpy, w->win);
    XSync(win->dpy, False);
    return;
    }
  
  /* Do it the XEMBED way */
  if(w->parent_xembed)
    {
    buffer[0] = 0; // Version
    buffer[1] = XEMBED_MAPPED; 
    
    XChangeProperty(win->dpy,
                    w->win,
                    win->_XEMBED_INFO,
                    win->_XEMBED_INFO, 32,
                    PropModeReplace,
                    (unsigned char *)buffer, 2);
    return;
    }
  
  /* If the window was already mapped, raise it */
  if(w->mapped)
    XRaiseWindow(win->dpy, w->win);
  else
    XMapWindow(win->dpy, w->win);

  if(w->win == w->toplevel)
    {
    if(!w->fullscreen)
      XMoveResizeWindow(win->dpy, w->win,
                        win->window_x, win->window_y,
                        win->window_width, win->window_height);
    }
  }

void bg_x11_window_clear(bg_x11_window_t * win)
  {
//  XSetForeground(win->dpy, win->gc, win->black);
//  XFillRectangle(win->dpy, win->normal_window, win->gc, 0, 0, win->window_width, 
//                 win->window_height);  
  if(win->normal.win != None)
    XClearArea(win->dpy, win->normal.win, 0, 0,
               win->window_width, win->window_height, True);
  
  if(win->fullscreen.win != None)
    XClearArea(win->dpy, win->fullscreen.win, 0, 0,
               win->window_width, win->window_height, True);
  
  //   XSync(win->dpy, False);
  }



/* MWM decorations */

#define MWM_HINTS_DECORATIONS (1L << 1)
#define MWM_HINTS_FUNCTIONS     (1L << 0)

#define MWM_FUNC_ALL                 (1L<<0)
#define PROP_MOTIF_WM_HINTS_ELEMENTS 5
typedef struct
  {
  CARD32 flags;
  CARD32 functions;
  CARD32 decorations;
  INT32 inputMode;
    CARD32 status;
  } PropMotifWmHints;

static
int mwm_set_decorations(bg_x11_window_t * w, Window win, int set)
  {
  PropMotifWmHints motif_hints;
  Atom hintsatom;
  
  /* setup the property */
  motif_hints.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;
  motif_hints.decorations = set;
  motif_hints.functions   = set ? MWM_FUNC_ALL : 0;
  
  /* get the atom for the property */
  hintsatom = XInternAtom(w->dpy, "_MOTIF_WM_HINTS", False);
  
  XChangeProperty(w->dpy, win, hintsatom, hintsatom, 32, PropModeReplace,
                  (unsigned char *) &motif_hints, PROP_MOTIF_WM_HINTS_ELEMENTS);
  return 1;
  }

static void set_fullscreen(bg_x11_window_t * w, Window win)
  {
  if(w->fullscreen_mode & FULLSCREEN_MODE_NET_FULLSCREEN)
    {
    
    }
  else
    mwm_set_decorations(w, win, 0);
  }

static void set_min_size(bg_x11_window_t * w, Window win, int width, int height)
  {
  XSizeHints * h;
  h = XAllocSizeHints();

  h->flags = PMinSize;
  h->min_width = width;
  h->min_height = height;

  XSetWMNormalHints(w->dpy, win, h);
  
  XFree(h);
  }

static int open_display(bg_x11_window_t * w)
  {
  char * normal_id;
  char * fullscreen_id;
#ifdef HAVE_LIBXINERAMA
  int foo,bar;
#endif
  
  /*
   *  Display string is in the form
   *  <XDisplayName(DisplayString(dpy)>:<normal_id>:<fullscreen_id>
   *  It can be NULL. Also, <fullscreen_id> can be missing
   */
  
  if(!w->display_string_parent)
    {
    w->dpy = XOpenDisplay(NULL);
    if(!w->dpy)
      return 0;
    w->normal.parent = None;
    w->fullscreen.parent = None;
    }
  else
    {
    fullscreen_id = strrchr(w->display_string_parent, ':');
    if(!fullscreen_id)
      {
      return 0;
      }
    *fullscreen_id = '\0';
    fullscreen_id++;
    
    normal_id = strrchr(w->display_string_parent, ':');
    if(!normal_id)
      {
      return 0;
      }
    *normal_id = '\0';
    normal_id++;
    
    w->dpy = XOpenDisplay(w->display_string_parent);
    if(!w->dpy)
      return 0;
    
    w->normal.parent = strtoul(normal_id, NULL, 16);
    
    if(!(*fullscreen_id))
      w->fullscreen.parent = None;
    else
      w->fullscreen.parent = strtoul(fullscreen_id, NULL, 16);

    //    fprintf(stderr, "Initialized windows: %ld %ld\n",
    //            w->normal.parent, w->fullscreen.parent);
    }

  w->screen = DefaultScreen(w->dpy);
  w->root =   RootWindow(w->dpy, w->screen);

  if(w->normal.parent == None)
    w->normal.parent = w->root;
  if(w->fullscreen.parent == None)
    w->fullscreen.parent = w->root;
  
  w->normal.child = None;
  w->fullscreen.child = None;
  
  init_atoms(w);
  
  /* Check, which fullscreen modes we have */
  
  w->fullscreen_mode = get_fullscreen_mode(w);

  bg_x11_screensaver_init(&w->scr, w->dpy);
  
  /* Get xinerama screens */

#ifdef HAVE_LIBXINERAMA
  if (XineramaQueryExtension(w->dpy,&foo,&bar) &&
      XineramaIsActive(w->dpy))
    {
    w->xinerama = XineramaQueryScreens(w->dpy,&w->nxinerama);
    }
#endif

  /* Check for XShm */
  w->have_shm = bg_x11_window_check_shm(w->dpy, &w->shm_completion_type);
  
  return 1;
  }

void bg_x11_window_get_coords(Display * dpy,
                              Window win,
                              int * x, int * y, int * width,
                              int * height)
  {
  Window root_return;
  Window parent_return;
  Window * children_return;
  unsigned int nchildren_return;
  int x_return, y_return;
  unsigned int width_return, height_return;
  unsigned int border_width_return;
  unsigned int depth_return;
  //  Window child_return;

  XGetGeometry(dpy, win, &root_return, &x_return, &y_return,
               &width_return, &height_return,
               &border_width_return, &depth_return);
  
  XQueryTree(dpy, win, &root_return, &parent_return,
             &children_return, &nchildren_return);

  if(nchildren_return)
    XFree(children_return);
  
  if(x) *x = x_return;
  if(y) *y = y_return;
  
  if(width)  *width  = width_return;
  if(height) *height = height_return;
  
  if((x || y) && (parent_return != root_return))
    {
    XGetGeometry(dpy, parent_return, &root_return,
                 &x_return, &y_return,
                 &width_return, &height_return,
                 &border_width_return, &depth_return);
    if(x) *x = x_return;
    if(y) *y = y_return;
    }
  }

static int window_is_viewable(Display * dpy, Window w)
  {
  XWindowAttributes attr;

  if(w == None)
    return 0;
  
  XGetWindowAttributes(dpy, w, &attr);
  if(attr.map_state == IsViewable)
    return 1;
  return 0;
  }


void bg_x11_window_size_changed(bg_x11_window_t * w)
  {
  /* Frame size remains unchanged, to let set_rectangles
     find out, if the image must be reallocated */

  /* Nothing changed actually. */
  if((w->window_format.image_width == w->window_width) &&
     (w->window_format.image_height == w->window_height))
    return;
  
  w->window_format.image_width  = w->window_width;
  w->window_format.image_height = w->window_height;
  
  if(w->callbacks && w->callbacks->size_changed)
    w->callbacks->size_changed(w->callbacks->data,
                               w->window_width, 
                               w->window_height);
  }

void bg_x11_window_init(bg_x11_window_t * w)
  {
  int send_event = -1;
  /* Decide current window */
  if((w->fullscreen.parent != w->root) &&
     window_is_viewable(w->dpy, w->fullscreen.parent))
    {
    //    fprintf(stderr, "Is fullscreen\n");
    
    if(!w->is_fullscreen)
      send_event = 1;
    w->current = &w->fullscreen;
    w->is_fullscreen = 1;
    }
  else
    {
    if(w->is_fullscreen)
      send_event = 0;
    w->current = &w->normal;
    w->is_fullscreen = 0;
    }

#if 1
  if(w->current->parent != w->root)
    {
    //    fprintf(stderr, "bg_x11_window_init %ld %ld\n", w->current->win,
    //            w->current->parent);
    bg_x11_window_get_coords(w->dpy, w->current->parent,
                      NULL, NULL,
                      &w->window_width, &w->window_height);
    XMoveResizeWindow(w->dpy, w->current->win, 0, 0,
                      w->window_width, w->window_height);
    }
  else
    bg_x11_window_get_coords(w->dpy, w->current->win,
                      NULL, NULL,
                      &w->window_width, &w->window_height);
#endif
  //  fprintf(stderr, "Window size: %dx%d\n", w->window_width, w->window_height);
  if((send_event >= 0) && w->callbacks &&
     w->callbacks->set_fullscreen)
    w->callbacks->set_fullscreen(w->callbacks->data, send_event);
  
  bg_x11_window_size_changed(w);

  
  }

void bg_x11_window_embed_parent(bg_x11_window_t * win,
                                window_t * w)
  {
  unsigned long buffer[2];

  
  buffer[0] = 0; // Version
  buffer[1] = 0; 
  
  
  //   XMapWindow(dpy, child);
  XChangeProperty(win->dpy,
                  w->win,
                  win->_XEMBED_INFO,
                  win->_XEMBED_INFO, 32,
                  PropModeReplace,
                  (unsigned char *)buffer, 2);
  w->toplevel = bg_x11_window_get_toplevel(win, w->parent);

  /* We need StructureNotify of both the toplevel and the
     parent */

  XSelectInput(win->dpy, w->parent, StructureNotifyMask);

  if(w->parent != w->toplevel)
    XSelectInput(win->dpy, w->toplevel, StructureNotifyMask);
  
  XSync(win->dpy, False);
  }

void bg_x11_window_embed_child(bg_x11_window_t * win,
                               window_t * w)
  {
  XSelectInput(win->dpy, w->child, FocusChangeMask | ExposureMask |
               PropertyChangeMask);
  
  /* Define back the cursor */
  XDefineCursor(win->dpy, w->win, None);
  win->pointer_hidden = 0;
  
  bg_x11_window_check_embed_property(win, w);
  }

static void create_subwin(bg_x11_window_t * w,
                          window_t * win,
                          int depth, Visual * v, unsigned long attr_mask,
                          XSetWindowAttributes * attr)
  {
  int width, height;
  
  bg_x11_window_get_coords(w->dpy, win->win,
                           NULL, NULL, &width, &height);
  win->subwin = XCreateWindow(w->dpy,
                              win->win, 0, 0, width, height, 0,
                              depth, InputOutput, v, attr_mask, attr);
  XMapWindow(w->dpy, win->subwin);
  }

void bg_x11_window_create_subwins(bg_x11_window_t * w,
                                  int depth, Visual * v)
  {
  unsigned long attr_mask;
  XSetWindowAttributes attr;
  
  w->sub_colormap = XCreateColormap(w->dpy, RootWindow(w->dpy, w->screen),
                                    v, AllocNone);

  memset(&attr, 0, sizeof(attr));

  attr_mask = CWEventMask | CWColormap;
  
  attr.event_mask =
    ExposureMask |
    ButtonPressMask |
    ButtonReleaseMask |
    KeyPressMask |
    PointerMotionMask |
    KeyReleaseMask;
  
  attr.colormap = w->sub_colormap;
  
  create_subwin(w, &w->normal, depth, v, attr_mask, &attr);
  create_subwin(w, &w->fullscreen, depth, v, attr_mask, &attr);
  }

static void destroy_subwin(bg_x11_window_t * w,
                           window_t * win)
  {
  if(win->subwin != None)
    {
    XDestroyWindow(w->dpy, win->subwin);
    win->subwin = None;
    }
  }
                          

void bg_x11_window_destroy_subwins(bg_x11_window_t * w)
  {
  destroy_subwin(w, &w->normal);
  destroy_subwin(w, &w->fullscreen);
  
  }

static int create_window(bg_x11_window_t * w,
                         int width, int height)
  {
  const char * display_name;
  unsigned long event_mask;
  XColor black;
  Atom wm_protocols[2];
  XWMHints * wmhints;
  
  //  int i;
  /* Stuff for making the cursor */
  
  XSetWindowAttributes attr;
  unsigned long attr_flags;
  
  if((!w->dpy) && !open_display(w))
    return 0;

  wmhints = XAllocWMHints();
  
  wmhints->input = True;
  wmhints->initial_state = NormalState;
  wmhints->flags |= InputHint|StateHint;
  
  /* Creating windows without colormap results in a BadMatch error */
  w->colormap = XCreateColormap(w->dpy, RootWindow(w->dpy, w->screen),
                                w->visual,
                                AllocNone);
  
  /* Setup event mask */

  event_mask =
    StructureNotifyMask |
    PointerMotionMask |
    ExposureMask |
    ButtonPressMask |
    ButtonReleaseMask |
    PropertyChangeMask |
    KeyPressMask |
    KeyReleaseMask |
    FocusChangeMask;
  
  /* Create normal window */

  memset(&attr, 0, sizeof(attr));
  
  attr.backing_store = NotUseful;
  attr.border_pixel = 0;
  attr.background_pixel = 0;
  attr.event_mask = event_mask;
  attr.colormap = w->colormap;
  attr_flags = (CWBackingStore | CWEventMask | CWBorderPixel |
                CWBackPixel | CWColormap);
  
  wm_protocols[0] = w->WM_DELETE_WINDOW;
  wm_protocols[1] = w->WM_TAKE_FOCUS;
  
  /* Create normal window */
  
  w->normal.win = XCreateWindow(w->dpy, w->normal.parent,
                                0 /* x */,
                                0 /* y */,
                                width, height,
                                0 /* border_width */, w->depth,
                                InputOutput,
                                w->visual,
                                attr_flags,
                                &attr);
  
  if(w->normal.parent == w->root)
    {
    //    set_decorations(w, w->normal.win, 1);
    XSetWMProtocols(w->dpy, w->normal.win, wm_protocols, 2);
    
    if(w->min_width && w->min_height)
      set_min_size(w, w->normal.win, w->min_width, w->min_height);

    w->normal.toplevel = w->normal.win;

    w->normal.focus_child = XCreateSimpleWindow(w->dpy, w->normal.win,
                                                  -1, -1, 1, 1, 0,
                                                  0, 0);
    XSelectInput(w->dpy, w->normal.focus_child,
                 KeyPressMask | KeyReleaseMask | FocusChangeMask);
    
    XSetWMHints(w->dpy, w->normal.win, wmhints);
    XMapWindow(w->dpy, w->normal.focus_child);
    w->normal.child_accel_map = bg_accelerator_map_create();
    }
  else
    bg_x11_window_embed_parent(w, &w->normal);
  
  /* The fullscreen window will be created with the same size for now */
  
  w->fullscreen.win = XCreateWindow (w->dpy, w->fullscreen.parent,
                                     0 /* x */,
                                     0 /* y */,
                                     width, height,
                                     0 /* border_width */, w->depth,
                                     InputOutput, w->visual,
                                     attr_flags,
                                     &attr);

  w->fullscreen.fullscreen = 1;
  
  if(w->fullscreen.parent == w->root)
    {
    /* Setup protocols */
    
    set_fullscreen(w, w->fullscreen.win);
    XSetWMProtocols(w->dpy, w->fullscreen.win, wm_protocols, 2);
    w->fullscreen.toplevel = w->fullscreen.win;

#if 0
    w->fullscreen.focus_child =
      XCreateWindow(w->dpy, w->fullscreen.win,
                    0, 0,
                    width, height,
                    0,
                    0,
                    InputOnly,
                    vi->visual,
                    attr_flags_input_only,
                    &attr);
#endif
    w->fullscreen.focus_child = XCreateSimpleWindow(w->dpy,
                                                    w->fullscreen.win,
                                                    -1, -1, 1, 1, 0,
                                                    0, 0);
    XSelectInput(w->dpy, w->fullscreen.focus_child,
                 KeyPressMask | KeyReleaseMask | FocusChangeMask);
    
    XSetWMHints(w->dpy, w->fullscreen.win, wmhints);
    XMapWindow(w->dpy, w->fullscreen.focus_child);
    w->fullscreen.child_accel_map = bg_accelerator_map_create();
    }
  else
    {
    bg_x11_window_embed_parent(w, &w->fullscreen);
    }

  /* Set the final event masks now. We blocked SubstructureNotifyMask
   * before because we don't want our focus children as
   * embeded clients..
   */
  
  XSync(w->dpy, False);
  XSelectInput(w->dpy, w->normal.win,
               event_mask | SubstructureNotifyMask);
  XSelectInput(w->dpy, w->fullscreen.win,
               event_mask | SubstructureNotifyMask);
  
  /* Create GC */
  
  w->gc = XCreateGC(w->dpy, w->normal.win, 0, NULL);
  
  /* Create colormap and fullscreen cursor */
  
  w->fullscreen_cursor_pixmap =
    XCreateBitmapFromData(w->dpy, w->fullscreen.win,
                          bm_no_data, 8, 8);

  black.pixel = BlackPixel(w->dpy, w->screen);
  XQueryColor(w->dpy, DefaultColormap(w->dpy, w->screen), &black);
  
  w->fullscreen_cursor=
    XCreatePixmapCursor(w->dpy, w->fullscreen_cursor_pixmap,
                        w->fullscreen_cursor_pixmap,
                        &black, &black, 0, 0);
  
  w->black = BlackPixel(w->dpy, w->screen);
  
  
  display_name = XDisplayName(DisplayString(w->dpy));

  XFree(wmhints);
  
  /* Determine if we are in fullscreen mode */
  bg_x11_window_init(w);


  return 1;
  }


static void get_fullscreen_coords(bg_x11_window_t * w,
                                  int * x, int * y, int * width, int * height)
  {
#ifdef HAVE_LIBXINERAMA
  int x_return, y_return;
  int i;
  Window child;
  /* Get the coordinates of the normal window */

  *x = 0;
  *y = 0;
  *width  = DisplayWidth(w->dpy, w->screen);
  *height = DisplayHeight(w->dpy, w->screen);
  
  if(w->nxinerama)
    {
    XTranslateCoordinates(w->dpy, w->normal.win, w->root, 0, 0,
                          &x_return,
                          &y_return,
                          &child);
    
    /* Get the xinerama screen we are on */
    
    for(i = 0; i < w->nxinerama; i++)
      {
      if((x_return >= w->xinerama[i].x_org) &&
         (y_return >= w->xinerama[i].y_org) &&
         (x_return < w->xinerama[i].x_org + w->xinerama[i].width) &&
         (y_return < w->xinerama[i].y_org + w->xinerama[i].height))
        {
        *x = w->xinerama[i].x_org;
        *y = w->xinerama[i].y_org;
        *width = w->xinerama[i].width;
        *height = w->xinerama[i].height;
        break;
        }
      }
    }
#else
  *x = 0;
  *y = 0;
  *width  = DisplayWidth(w->dpy, w->screen);
  *height = DisplayHeight(w->dpy, w->screen);
#endif
  }

int bg_x11_window_set_fullscreen(bg_x11_window_t * w,int fullscreen)
  {
  int ret = 0;
  int width;
  int height;
  int x;
  int y;
  
  /* Return early if there is nothing to do */
  if(!!fullscreen == !!w->is_fullscreen)
    return 0;
  
  /* Normal->fullscreen */
  
  if(fullscreen)
    {
    if(w->normal.parent != w->root)
      return 0;
    get_fullscreen_coords(w, &x, &y, &width, &height);

    w->normal_width = w->window_width;
    w->normal_height = w->window_height;
    
    w->window_width = width;
    w->window_height = height;
    
    w->current = &w->fullscreen;
    w->is_fullscreen = 1;
    w->need_fullscreen = 1;
    bg_x11_window_show(w, 1);
    
    if(w->callbacks && w->callbacks->set_fullscreen)
      w->callbacks->set_fullscreen(w->callbacks->data, 1);
    
    bg_x11_window_clear(w);
    
    /* Hide old window */
    if(w->normal.win == w->normal.toplevel)
      XWithdrawWindow(w->dpy, w->normal.toplevel, w->screen);
    
    XFlush(w->dpy);
    ret = 1;
    }
  else
    {
    if(w->fullscreen.parent != w->root)
      return 0;
    /* Unmap fullscreen window */
#if 1
    bg_x11_window_set_netwm_state(w->dpy, w->fullscreen.win, w->root,
                                  _NET_WM_STATE_REMOVE, w->_NET_WM_STATE_FULLSCREEN);

    bg_x11_window_set_netwm_state(w->dpy, w->fullscreen.win, w->root,
                                  _NET_WM_STATE_REMOVE, w->_NET_WM_STATE_ABOVE);
    XUnmapWindow(w->dpy, w->fullscreen.win);
#endif
    XWithdrawWindow(w->dpy, w->fullscreen.win, w->screen);
    
    /* Map normal window */
    w->current = &w->normal;
    w->is_fullscreen = 0;
    
    w->window_width  = w->normal_width;
    w->window_height = w->normal_height;
    
    bg_x11_window_show(w, 1);
    
    if(w->callbacks && w->callbacks->set_fullscreen)
      w->callbacks->set_fullscreen(w->callbacks->data, 0);
    
    bg_x11_window_clear(w);
    XFlush(w->dpy);
    ret = 1;
    }

  if(ret)
    {
    if(check_disable_screensaver(w))
      bg_x11_screensaver_disable(&w->scr);
    else
      bg_x11_screensaver_enable(&w->scr);
    }
  return ret;
  }

void bg_x11_window_resize(bg_x11_window_t * win,
                          int width, int height)
  {
  win->normal_width = width;
  win->normal_height = height;
  if(!win->is_fullscreen && (win->normal.parent == win->root))
    {
    win->window_width = width;
    win->window_height = height;
    XResizeWindow(win->dpy, win->normal.win, width, height);
    }
  }

void bg_x11_window_get_size(bg_x11_window_t * win, int * width, int * height)
  {
  *width = win->window_width;
  *height = win->window_height;
  }

/* Public methods */

bg_x11_window_t * bg_x11_window_create(const char * display_string)
  {
  bg_x11_window_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->display_string_parent = bg_strdup(ret->display_string_parent, display_string);
  ret->scaler = gavl_video_scaler_create();
  
  /* Set default OpenGL attributes */
  
  bg_x11_window_set_gl_attribute(ret, BG_GL_ATTRIBUTE_RGBA, 1);
  bg_x11_window_set_gl_attribute(ret, BG_GL_ATTRIBUTE_RED_SIZE, 8);
  bg_x11_window_set_gl_attribute(ret, BG_GL_ATTRIBUTE_GREEN_SIZE, 8);
  bg_x11_window_set_gl_attribute(ret, BG_GL_ATTRIBUTE_BLUE_SIZE, 8);
  bg_x11_window_set_gl_attribute(ret, BG_GL_ATTRIBUTE_DOUBLEBUFFER, 1);
  
  ret->icon      = None;
  ret->icon_mask = None;
  
  return ret;
  }

const char * bg_x11_window_get_display_string(bg_x11_window_t * w)
  {
  if(w->normal.win == None)
    create_window(w, w->window_width, w->window_height);
  
  if(!w->display_string_child)
    w->display_string_child = bg_sprintf("%s:%08lx:%08lx",
                                         XDisplayName(DisplayString(w->dpy)),
                                         w->normal.win, w->fullscreen.win);
  return w->display_string_child;
  }

void bg_x11_window_destroy(bg_x11_window_t * w)
  {
  bg_x11_window_cleanup_video(w);
  bg_x11_window_cleanup_gl(w);
  bg_x11_window_destroy_subwins(w);
  
  if(w->colormap != None)
    XFreeColormap(w->dpy, w->colormap);
  
  if(w->normal.win != None)
    XDestroyWindow(w->dpy, w->normal.win);
  
  if(w->fullscreen.win != None)
    XDestroyWindow(w->dpy, w->fullscreen.win);
  
  if(w->fullscreen_cursor != None)
    XFreeCursor(w->dpy, w->fullscreen_cursor);
  
  if(w->fullscreen_cursor_pixmap != None)
    XFreePixmap(w->dpy, w->fullscreen_cursor_pixmap);

  if(w->gc != None)
    XFreeGC(w->dpy, w->gc);

  if(w->normal.child_accel_map)     bg_accelerator_map_destroy(w->normal.child_accel_map);
  if(w->fullscreen.child_accel_map) bg_accelerator_map_destroy(w->fullscreen.child_accel_map);
  
#ifdef HAVE_LIBXINERAMA
  if(w->xinerama)
    XFree(w->xinerama);
#endif
  
#ifdef GAVE_GLX
  if(w->gl_fbconfigs)
    XFree(w->gl_fbconfigs);
#endif
  
  if(w->icon != None)
    XFreePixmap(w->dpy, w->icon);
  
  if(w->icon_mask != None)
    XFreePixmap(w->dpy, w->icon_mask);
  
  if(w->dpy)
    {
    XCloseDisplay(w->dpy);
    bg_x11_screensaver_cleanup(&w->scr);
    }
  if(w->display_string_parent)
    free(w->display_string_parent);
  if(w->display_string_child)
    free(w->display_string_child);

  if(w->scaler)
    gavl_video_scaler_destroy(w->scaler);
  
  free(w);
  }

static const bg_parameter_info_t common_parameters[] =
  {
    {
      BG_LOCALE,
      .name =        "window",
      .long_name =   TRS("General"),
    },
    {
      .name =        "auto_resize",
      .long_name =   TRS("Auto resize window"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 }
    },
    {
      .name =        "window_width",
      .long_name =   "Window width",
      .type =        BG_PARAMETER_INT,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 320 }
    },
    {
      .name =        "window_height",
      .long_name =   "Window height",
      .type =        BG_PARAMETER_INT,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 240 }
    },
    {
      .name =        "window_x",
      .long_name =   "Window x",
      .type =        BG_PARAMETER_INT,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 100 }
    },
    {
      .name =        "window_y",
      .long_name =   "Window y",
      .type =        BG_PARAMETER_INT,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 100 }
    },
    {
      .name =        "disable_xscreensaver_normal",
      .long_name =   TRS("Disable Screensaver for normal playback"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 }
    },
    {
      .name =        "disable_xscreensaver_fullscreen",
      .long_name =   TRS("Disable Screensaver for fullscreen playback"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 }
    },
    {
      .name =        "force_hw_scale",
      .long_name =   TRS("Force hardware scaling"),
      .type =        BG_PARAMETER_CHECKBUTTON,      .val_default = { .val_i = 1 },
      .help_string = TRS("Use hardware scaling even if it involves more CPU intensive pixelformat conversions"),

    },
#ifdef HAVE_GLX
    {
      .name =        "background_color",
      .long_name =   TRS("Background color"),
      .type =        BG_PARAMETER_COLOR_RGB,
      .flags =       BG_PARAMETER_SYNC,
      .help_string = TRS("Specify the background color for videos with alpha channel. This is only used by the OpenGL driver."),

    },
#endif
    {
      .name =        "sw_scaler",
      .long_name =   TRS("Software scaler"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name =        "scale_mode",
      .long_name =   TRS("Scale mode"),
      .type =        BG_PARAMETER_STRINGLIST,
      .multi_names =  (char const*[]){ "auto",
                               "nearest",
                               "bilinear",
                               "quadratic",
                               "cubic_bspline",
                               "cubic_mitchell",
                               "cubic_catmull",
                               "sinc_lanczos",
                               NULL },
      .multi_labels = (char const*[]){ TRS("Auto"),
                               TRS("Nearest"),
                               TRS("Bilinear"),
                               TRS("Quadratic"),
                               TRS("Cubic B-Spline"),
                               TRS("Cubic Mitchell-Netravali"),
                               TRS("Cubic Catmull-Rom"),
                               TRS("Sinc with Lanczos window"),
                               NULL },
      .val_default = { .val_str = "auto" },
      .help_string = TRS("Choose scaling method. Auto means to choose based on the conversion quality. Nearest is fastest, Sinc with Lanczos window is slowest."),
    },
    {
      .name =        "scale_order",
      .long_name =   TRS("Scale order"),
      .type =        BG_PARAMETER_INT,
      .val_min =     { .val_i = 4 },
      .val_max =     { .val_i = 1000 },
      .val_default = { .val_i = 4 },
      .help_string = TRS("Order for sinc scaling"),
    },
    {
      .name =        "scale_quality",
      .long_name =   TRS("Scale quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST },
      .val_default = { .val_i = GAVL_QUALITY_DEFAULT },
      .help_string = TRS("Scale quality"),
    },
    { /* End of parameters */ },
  };


const bg_parameter_info_t * bg_x11_window_get_parameters(bg_x11_window_t * win)
  {
  return common_parameters;
  }

void
bg_x11_window_set_parameter(void * data, const char * name,
                            const bg_parameter_value_t * val)
  {
  gavl_scale_mode_t scale_mode = GAVL_SCALE_AUTO;
  gavl_video_options_t * opt;
  bg_x11_window_t * win;
  if(!name)
    return;
  win = (bg_x11_window_t *)data;

  if(!strcmp(name, "auto_resize"))
    {
    win->auto_resize = val->val_i;
    }
  else if(!strcmp(name, "window_x"))
    {
    if(win->normal.parent == win->root)
      win->window_x = val->val_i;
    }
  else if(!strcmp(name, "window_y"))
    {
    if(win->normal.parent == win->root)
      win->window_y = val->val_i;
    }
  else if(!strcmp(name, "window_width"))
    {
    if(win->normal.parent == win->root)
      win->window_width = val->val_i;
    }
  else if(!strcmp(name, "window_height"))
    {
    if(win->normal.parent == win->root)
      win->window_height = val->val_i;
    }
  else if(!strcmp(name, "disable_xscreensaver_normal"))
    {
    win->disable_screensaver_normal = val->val_i;
    }
  else if(!strcmp(name, "disable_xscreensaver_fullscreen"))
    {
    win->disable_screensaver_fullscreen = val->val_i;
    }
#ifdef HAVE_GLX
  else if(!strcmp(name, "background_color"))
    {
    memcpy(win->background_color, val->val_color, 3 * sizeof(float));
    }
#endif
  
  else if(!strcmp(name, "force_hw_scale"))
    {
    win->force_hw_scale = val->val_i;
    }
  else if(!strcmp(name, "scale_mode"))
    {
    if(!strcmp(val->val_str, "auto"))
      scale_mode = GAVL_SCALE_AUTO;
    else if(!strcmp(val->val_str, "nearest"))
      scale_mode = GAVL_SCALE_NEAREST;
    else if(!strcmp(val->val_str, "bilinear"))
      scale_mode = GAVL_SCALE_BILINEAR;
    else if(!strcmp(val->val_str, "quadratic"))
      scale_mode = GAVL_SCALE_QUADRATIC;
    else if(!strcmp(val->val_str, "cubic_bspline"))
      scale_mode = GAVL_SCALE_CUBIC_BSPLINE;
    else if(!strcmp(val->val_str, "cubic_mitchell"))
      scale_mode = GAVL_SCALE_CUBIC_MITCHELL;
    else if(!strcmp(val->val_str, "cubic_catmull"))
      scale_mode = GAVL_SCALE_CUBIC_CATMULL;
    else if(!strcmp(val->val_str, "sinc_lanczos"))
      scale_mode = GAVL_SCALE_SINC_LANCZOS;

    opt = gavl_video_scaler_get_options(win->scaler);
    if(scale_mode != gavl_video_options_get_scale_mode(opt))
      {
      win->scaler_options_changed = 1;
      gavl_video_options_set_scale_mode(opt, scale_mode);
      }
    }
  else if(!strcmp(name, "scale_order"))
    {
    opt = gavl_video_scaler_get_options(win->scaler);
    if(val->val_i != gavl_video_options_get_scale_order(opt))
      {
      win->scaler_options_changed = 1;
      gavl_video_options_set_scale_order(opt, val->val_i);
      }
    }
  else if(!strcmp(name, "scale_quality"))
    {
    opt = gavl_video_scaler_get_options(win->scaler);
    if(val->val_i != gavl_video_options_get_quality(opt))
      {
      win->scaler_options_changed = 1;
      gavl_video_options_set_quality(opt, val->val_i);
      }
    }
  
  
  }

int
bg_x11_window_get_parameter(void * data, const char * name,
                            bg_parameter_value_t * val)
  {
  bg_x11_window_t * win;
  if(!name)
    return 0;
  win = (bg_x11_window_t *)data;
  
  if(!strcmp(name, "window_x"))
    {
    val->val_i = win->window_x;
    return 1;
    }
  else if(!strcmp(name, "window_y"))
    {
    val->val_i = win->window_y;
    return 1;
    }
  else if(!strcmp(name, "window_width"))
    {
    val->val_i = win->window_width;
    return 1;
    }
  else if(!strcmp(name, "window_height"))
    {
    val->val_i = win->window_height;
    return 1;
    }
  return 0;
  }


void bg_x11_window_set_size(bg_x11_window_t * win, int width, int height)
  {
  win->window_width = width;
  win->window_height = height;
  }


int bg_x11_window_realize(bg_x11_window_t * win)
  {
  int ret;
  
  if(!win->dpy && !open_display(win))
    return 0;
  
// #ifdef HAVE_GLX
#if 0
  win->gl_vi = glXChooseVisual(win->dpy, win->screen, attr_list);
  
  if(!win->gl_vi)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Could not get GL Visual");
    win->visual = DefaultVisual(win->dpy, win->screen);
    win->depth = DefaultDepth(win->dpy, win->screen);
    }
  else
    {
    win->visual = win->gl_vi->visual;
    win->depth  = win->gl_vi->depth;
    }

#endif

  win->visual = DefaultVisual(win->dpy, win->screen);
  win->depth = DefaultDepth(win->dpy, win->screen);
  
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Got Visual 0x%lx depth %d",
         win->visual->visualid, win->depth);

  ret = create_window(win, win->window_width, win->window_height);
  bg_x11_window_init_gl(win);
  return ret;
  }

/* Handle X11 events, callbacks are called from here */
void bg_x11_window_set_callbacks(bg_x11_window_t * win,
                                 bg_x11_window_callbacks_t * callbacks)
  {
  win->callbacks = callbacks;
  }

void bg_x11_window_set_title(bg_x11_window_t * w, const char * title)
  {
  if(w->normal.parent == w->root)
    XmbSetWMProperties(w->dpy, w->normal.win, title,
                       title, NULL, 0, NULL, NULL, NULL);
  if(w->fullscreen.parent == w->root)
    XmbSetWMProperties(w->dpy, w->fullscreen.win, title,
                       title, NULL, 0, NULL, NULL, NULL);
  }

void bg_x11_window_set_options(bg_x11_window_t * w,
                               const char * name, 
                               const char * klass,
                               const gavl_video_frame_t * icon,
                               const gavl_video_format_t * icon_format)
  {
  /* Set Class hints */
  if(name && klass)
    {
    XClassHint xclasshint;

    /* const madness */
    xclasshint.res_name = bg_strdup(NULL, name);
    xclasshint.res_class = bg_strdup(NULL, klass);

    if(w->normal.parent == w->root)
      XSetClassHint(w->dpy, w->normal.win, &xclasshint);

    if(w->fullscreen.parent == w->root)
      XSetClassHint(w->dpy, w->fullscreen.win, &xclasshint);

    free(xclasshint.res_name);
    free(xclasshint.res_class);
    }

  /* Set Icon (TODO) */
  if(icon && icon_format)
    {
    XWMHints xwmhints;
    memset(&xwmhints, 0, sizeof(xwmhints));

    if((w->normal.parent == w->root) ||
       (w->fullscreen.parent == w->root))
      {
      if(w->icon != None)
        {
        XFreePixmap(w->dpy, w->icon);
        w->icon = None; 
        }
      if(w->icon_mask != None)
        {
        XFreePixmap(w->dpy, w->icon_mask);
        w->icon_mask = None; 
        }
      
      bg_x11_window_make_icon(w,
                              icon,
                              icon_format,
                              &w->icon,
                              &w->icon_mask);
      
      xwmhints.icon_pixmap = w->icon;
      xwmhints.icon_mask   = w->icon_mask;
      
      if(xwmhints.icon_pixmap != None)
        xwmhints.flags |= IconPixmapHint;
      
      if(xwmhints.icon_mask != None)
        xwmhints.flags |= IconMaskHint;
      
      if(w->normal.parent == w->root)
        XSetWMHints(w->dpy, w->normal.win, &xwmhints);
      if(w->fullscreen.parent == w->root)
        XSetWMHints(w->dpy, w->fullscreen.win, &xwmhints);
      }
    }
  }

void bg_x11_window_show(bg_x11_window_t * win, int show)
  {
  show_window(win, win->current, show);
  
  if(show && check_disable_screensaver(win))
    bg_x11_screensaver_disable(&win->scr);
  else
    bg_x11_screensaver_enable(&win->scr);
  
  if(!show)
    {
    XSync(win->dpy, False);
    bg_x11_window_handle_events(win, 0);
    }
  }

Window bg_x11_window_get_toplevel(bg_x11_window_t * w, Window win)
  {
  Window *children_return;
  Window root_return;
  Window parent_return;
  Atom     type_ret;
  int      format_ret;
  unsigned long   nitems_ret;
  unsigned long   bytes_after_ret;
  unsigned char  *prop_ret;
  
  unsigned int nchildren_return;
  while(1)
    {
    XGetWindowProperty(w->dpy, win,
                       w->WM_CLASS, 0L, 0L, 0,
                       w->STRING,
                       &type_ret,&format_ret,&nitems_ret,
                       &bytes_after_ret,&prop_ret);
    if(type_ret!=None)
      {
      XFree(prop_ret);
      return win;
      }
    XQueryTree(w->dpy, win, &root_return, &parent_return,
               &children_return, &nchildren_return);
    if(nchildren_return)
      XFree(children_return);
    if(parent_return == root_return)
      break;
    win = parent_return;
    }
  return win;
  }

void 
bg_x11_window_send_xembed_message(bg_x11_window_t * w, Window win, 
                                  long time, int message, int detail, 
                                  int data1, int data2)
  {
  XClientMessageEvent xclient;

  xclient.window = win;
  xclient.type = ClientMessage;
  xclient.message_type = w->_XEMBED;
  xclient.format = 32;
  xclient.data.l[0] = time;
  xclient.data.l[1] = message;
  xclient.data.l[2] = detail;
  xclient.data.l[3] = data1;
  xclient.data.l[4] = data2;
  
  XSendEvent(w->dpy, win,
             False, NoEventMask, (XEvent *)&xclient);
  XSync(w->dpy, False);
  }

int bg_x11_window_check_embed_property(bg_x11_window_t * win,
                                       window_t * w)
  {
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *data;
  unsigned long *data_long;
  int flags;
  if(XGetWindowProperty(win->dpy, w->child,
                        win->_XEMBED_INFO,
                        0, 2, False,
                        win->_XEMBED_INFO, &type, &format,
                        &nitems, &bytes_after, &data) != Success)
    return 0;
  
  if (type == None)             /* No info property */
    return 0;
  if (type != win->_XEMBED_INFO)
    return 0;
  
  data_long = (unsigned long *)data;
  flags = data_long[1];
  XFree(data);
  
  if(flags & XEMBED_MAPPED)
    {
    XMapWindow(win->dpy, w->child);
    XRaiseWindow(win->dpy, w->focus_child);
    }
  else
    {
    /* Window should not be mapped right now */
    //    XUnmapWindow(win->dpy, w->child);
    }
  
  if(!w->child_xembed)
    {
    w->child_xembed = 1;
    bg_x11_window_send_xembed_message(win,
                                      w->child,
                                      CurrentTime,
                                      XEMBED_EMBEDDED_NOTIFY,
                                      0,
                                      w->win, 0);
    XFlush(win->dpy);
    }
  return 1;
  }

gavl_pixelformat_t 
bg_x11_window_get_pixelformat(Display * dpy, Visual * visual, int depth)
  {
  int bpp;
  XPixmapFormatValues * pf;
  int i;
  int num_pf;
  gavl_pixelformat_t ret = GAVL_PIXELFORMAT_NONE;
  
  bpp = 0;
  pf = XListPixmapFormats(dpy, &num_pf);
  for(i = 0; i < num_pf; i++)
    {
    if(pf[i].depth == depth)
      bpp = pf[i].bits_per_pixel;
    }
  XFree(pf);
  
  ret = GAVL_PIXELFORMAT_NONE;
  switch(bpp)
    {
    case 16:
      if((visual->red_mask == 63488) &&
         (visual->green_mask == 2016) &&
         (visual->blue_mask == 31))
        ret = GAVL_RGB_16;
      else if((visual->blue_mask == 63488) &&
              (visual->green_mask == 2016) &&
              (visual->red_mask == 31))
        ret = GAVL_BGR_16;
      break;
    case 24:
      if((visual->red_mask == 0xff) && 
         (visual->green_mask == 0xff00) &&
         (visual->blue_mask == 0xff0000))
        ret = GAVL_RGB_24;
      else if((visual->red_mask == 0xff0000) && 
         (visual->green_mask == 0xff00) &&
         (visual->blue_mask == 0xff))
        ret = GAVL_BGR_24;
      break;
    case 32:
      if((visual->red_mask == 0xff) && 
         (visual->green_mask == 0xff00) &&
         (visual->blue_mask == 0xff0000))
        ret = GAVL_RGB_32;
      else if((visual->red_mask == 0xff0000) && 
         (visual->green_mask == 0xff00) &&
         (visual->blue_mask == 0xff))
        ret = GAVL_BGR_32;
      break;
    }
  return ret;
  }

