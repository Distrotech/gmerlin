/*****************************************************************
 
  x11_window.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <config.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include <time.h>
#include <sys/time.h>

#include <X11/Xatom.h>

#include <X11/Xutil.h>

#include <translation.h>
#include <utils.h>
#include <log.h>
#define LOG_DOMAIN "x11"

#include <x11/x11.h>
#include <x11/x11_window_private.h>


#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */

// #define FULLSCREEN_MODE_OLD    0
#define FULLSCREEN_MODE_NET_FULLSCREEN (1<<0)
#define FULLSCREEN_MODE_NET_ABOVE      (1<<1)

#define FULLSCREEN_MODE_WIN_LAYER      (1<<2)

#define HAVE_XSHM

static char bm_no_data[] = { 0,0,0,0, 0,0,0,0 };

/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase (Display *);

/* Screensaver detection */

static void check_screensaver(bg_x11_window_t * w)
  {
  char * env;
  
  /* Check for gnome */
  env = getenv("GNOME_DESKTOP_SESSION_ID");
  if(env)
    {
    w->screensaver_mode = SCREENSAVER_MODE_GNOME;
    return;
    }

  /* Check for KDE */
  env = getenv("KDE_FULL_SESSION");
  if(env && !strcmp(env, "true"))
    {
    w->screensaver_mode = SCREENSAVER_MODE_KDE;
    return;
    }

  /* TODO: xfce4 */
  
  }

static void disable_screensaver(bg_x11_window_t * w)
  {
  int interval, prefer_blank, allow_exp;

#if HAVE_XDPMS
  int nothing;
#endif // HAVE_XDPMS
  
  if(w->screensaver_disabled)
    return;

#if HAVE_XDPMS
  if(DPMSQueryExtension(w->dpy, &nothing, &nothing))
    {
    BOOL onoff;
    CARD16 state;
    
    DPMSInfo(w->dpy, &state, &onoff);
    if(onoff)
      {
      w->dpms_disabled = 1;
      DPMSDisable(w->dpy);       // monitor powersave off
      }
    }
#endif // HAVE_XDPMS
    
  switch(w->screensaver_mode)
    {
    case SCREENSAVER_MODE_XLIB:
      XGetScreenSaver(w->dpy, &w->screensaver_saved_timeout,
                      &interval, &prefer_blank,
                      &allow_exp);
      if(w->screensaver_saved_timeout)
        w->screensaver_was_enabled = 1;
      else
        w->screensaver_was_enabled = 0;
      XSetScreenSaver(w->dpy, 0, interval, prefer_blank, allow_exp);
      break;
    case SCREENSAVER_MODE_GNOME:
      break;
    case SCREENSAVER_MODE_KDE:
      w->screensaver_was_enabled =
        (system
             ("dcop kdesktop KScreensaverIface isEnabled 2>/dev/null | sed 's/1/true/g' | grep true 2>/dev/null >/dev/null")
         == 0);
      
      if(w->screensaver_was_enabled)
        system("dcop kdesktop KScreensaverIface enable false > /dev/null");
      break;
    }
  w->screensaver_disabled = 1;
  }

static void enable_screensaver(bg_x11_window_t * w)
  {
  int dummy, interval, prefer_blank, allow_exp;

  if(!w->screensaver_disabled)
    return;

#if HAVE_XDPMS
  if(w->dpms_disabled)
    {
    if(DPMSQueryExtension(w->dpy, &dummy, &dummy))
      {
      if(DPMSEnable(w->dpy))
        {
        // DPMS does not seem to be enabled unless we call DPMSInfo
        BOOL onoff;
        CARD16 state;

        DPMSForceLevel(w->dpy, DPMSModeOn);      
        DPMSInfo(w->dpy, &state, &onoff);
        }

      }
    w->dpms_disabled = 0;
    }
#endif // HAVE_XDPMS
  
  w->screensaver_disabled = 0;
  
  
  if(!w->screensaver_was_enabled)
    return;
  
  switch(w->screensaver_mode)
    {
    case SCREENSAVER_MODE_XLIB:
      XGetScreenSaver(w->dpy, &dummy, &interval, &prefer_blank,
                      &allow_exp);
      XSetScreenSaver(w->dpy, w->screensaver_saved_timeout, interval, prefer_blank,
                      allow_exp);
      break;
    case SCREENSAVER_MODE_GNOME:
      break;
    case SCREENSAVER_MODE_KDE:
      break;
    }
  }

static int check_disable_screensaver(bg_x11_window_t * w)
  {
  if(w->current_window == w->normal_window)
    {
    if(w->normal_parent != w->root)
      return 0;
    else if(w->disable_screensaver_normal)
      return 1;
    else
      return 0;
    }
  else if(w->current_window == w->fullscreen_window)
    {
    if(w->fullscreen_parent != w->root)
      return 0;
    else if(w->disable_screensaver_fullscreen)
      return 1;
    else
      return 0;
    }
  return 1;
  }

void bg_x11_window_ping_screensaver(bg_x11_window_t * w)
  {
  struct timeval tm;
  gettimeofday(&tm, (struct timezone *)0);

  if(tm.tv_sec - w->screensaver_last_ping_time < 40) // 40 Sec interval
    {
    return;
    }

  w->screensaver_last_ping_time = tm.tv_sec;
  
  switch(w->screensaver_mode)
    {
    case SCREENSAVER_MODE_XLIB:
      break;
    case SCREENSAVER_MODE_GNOME:
      system("gnome-screensaver-command --poke > /dev/null 2> /dev/null");
      break;
    case SCREENSAVER_MODE_KDE:
      break;
    }
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

static void
netwm_set_state(bg_x11_window_t * w, Window win, int action, Atom state)
  {
  /* Setting _NET_WM_STATE by XSendEvent works only, if the window
     is already mapped!! */
  
  XEvent e;
  memset(&e,0,sizeof(e));
  e.xclient.type = ClientMessage;
  e.xclient.message_type = w->_NET_WM_STATE;
  e.xclient.display = w->dpy;
  e.xclient.window = win;
  e.xclient.send_event = True;
  e.xclient.format = 32;
  e.xclient.data.l[0] = action;
  e.xclient.data.l[1] = state;
  
  XSendEvent(w->dpy, w->root, False,
             SubstructureRedirectMask, &e);
  }

#if 0
static void
netwm_set_fullscreen(bg_x11_window_t * w, Window win)
  {
  long                  propvalue[2];
    
  propvalue[0] = w->_NET_WM_STATE_FULLSCREEN;
  propvalue[1] = 0;

  XChangeProperty (w->dpy, win, 
		   w->_NET_WM_STATE, XA_ATOM, 
		       32, PropModeReplace, 
		       (unsigned char *)propvalue, 1);
  XFlush(w->dpy);



  }
#endif

#if 0
static void
netwm_set_layer(bg_x11_window_t * w, Window win, int layer)
  {
  XEvent e;
  
  memset(&e,0,sizeof(e));
  e.xclient.type = ClientMessage;
  e.xclient.message_type = w->_NET_WM_STATE;
  e.xclient.display = w->dpy;
  e.xclient.window = win;
  e.xclient.format = 32;
  e.xclient.data.l[0] = operation;
  e.xclient.data.l[1] = state;
  
  XSendEvent(w->dpy, w->root, False,
             SubstructureRedirectMask, &e);
  
  }
#endif

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

    
    }
  XFree(ldata);
  
  if(wm_check_capability(w->dpy, w->root, w->WIN_PROTOCOLS,
                         w->WIN_LAYER))
    {
    ret |= FULLSCREEN_MODE_WIN_LAYER;
    }
  
  return ret;
  }

static void init_atoms(bg_x11_window_t * w)
  {
  w->WM_DELETE_WINDOW         = XInternAtom(w->dpy, "WM_DELETE_WINDOW", False);

  w->WIN_PROTOCOLS             = XInternAtom(w->dpy, "WIN_PROTOCOLS", False);
  w->WM_PROTOCOLS             = XInternAtom(w->dpy, "WM_PROTOCOLS", False);
  w->WIN_LAYER              = XInternAtom(w->dpy, "WIN_LAYER", False);

  w->_NET_SUPPORTED           = XInternAtom(w->dpy, "_NET_SUPPORTED", False);
  w->_NET_WM_STATE            = XInternAtom(w->dpy, "_NET_WM_STATE", False);
  w->_NET_WM_STATE_FULLSCREEN = XInternAtom(w->dpy, "_NET_WM_STATE_FULLSCREEN",
                                            False);
  
  w->_NET_WM_STATE_ABOVE = XInternAtom(w->dpy, "_NET_WM_STATE_ABOVE",
                                       False);

  w->_NET_MOVERESIZE_WINDOW   = XInternAtom(w->dpy, "_NET_MOVERESIZE_WINDOW",
                                            False);

  }

void bg_x11_window_clear(bg_x11_window_t * win)
  {
//  XSetForeground(win->dpy, win->gc, win->black);
//  XFillRectangle(win->dpy, win->normal_window, win->gc, 0, 0, win->window_width, 
//                 win->window_height);  
  if(win->normal_window != None)
    XClearArea(win->dpy, win->normal_window, 0, 0,
               win->window_width, win->window_height, True);

  if(win->fullscreen_window != None)
    XClearArea(win->dpy, win->fullscreen_window, 0, 0,
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

static void set_decorations(bg_x11_window_t * w, Window win, int decorations)
  {
  mwm_set_decorations(w, win, decorations);
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
    w->normal_parent = None;
    w->fullscreen_parent = None;
    }
  else
    {
    fullscreen_id = strrchr(w->display_string_parent, ':');
    if(!fullscreen_id)
      {
      fprintf(stderr, "Invalid display string: %s\n",
              w->display_string_parent);
      return 0;
      }
    *fullscreen_id = '\0';
    fullscreen_id++;
    
    normal_id = strrchr(w->display_string_parent, ':');
    if(!normal_id)
      {
      fprintf(stderr, "Invalid display string: %s\n",
              w->display_string_parent);
      return 0;
      }
    *normal_id = '\0';
    normal_id++;
    
    w->dpy = XOpenDisplay(w->display_string_parent);
    if(!w->dpy)
      {
      fprintf(stderr, "Opening display %s failed\n", w->display_string_parent);
      return 0;
      }
    
    w->normal_parent = strtoul(normal_id, (char **)0, 16);
    
    if(!(*fullscreen_id))
      w->fullscreen_parent = None;
    else
      w->fullscreen_parent = strtoul(fullscreen_id, (char **)0, 16);
    
    }

  w->screen = DefaultScreen(w->dpy);
  w->root =   RootWindow(w->dpy, w->screen);

  if(w->normal_parent == None)
    w->normal_parent = w->root;
  if(w->fullscreen_parent == None)
    w->fullscreen_parent = w->root;
  
  w->normal_child = None;
  w->fullscreen_child = None;
  
  init_atoms(w);
  
  check_screensaver(w);
  
  /* Get xinerama screens */

#ifdef HAVE_LIBXINERAMA
  if (XineramaQueryExtension(w->dpy,&foo,&bar) &&
      XineramaIsActive(w->dpy))
    {
    w->xinerama = XineramaQueryScreens(w->dpy,&(w->nxinerama));
    }
#endif

  /* Check for XShm */
  w->have_shm = bg_x11_window_check_shm(w->dpy, &w->shm_completion_type);
  
  return 1;
  }



void bg_x11_window_get_coords(bg_x11_window_t * w,
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
  
  XGetGeometry(w->dpy, win, &root_return, &x_return, &y_return,
               &width_return, &height_return,
               &border_width_return, &depth_return);
  
  XQueryTree(w->dpy, win, &root_return, &parent_return,
             &children_return, &nchildren_return);

  if(nchildren_return)
    {
    XFree(children_return);
    }

  if(x) *x = x_return;
  if(y) *y = y_return;
  
  if(width)  *width  = width_return;
  if(height) *height = height_return;
  
  if((x || y) && (parent_return != root_return))
    {
    XGetGeometry(w->dpy, parent_return, &root_return,
                 &x_return, &y_return,
                 &width_return, &height_return,
                 &border_width_return, &depth_return);
    if(x) *x = x_return;
    if(y) *y = y_return;
    }
  }

static int window_is_mapped(Display * dpy, Window w)
  {
  XWindowAttributes attr;
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
  if(window_is_mapped(w->dpy, w->fullscreen_window))
    {
    if(w->current_window != w->fullscreen_window)
      send_event = 1;
    w->current_window = w->fullscreen_window;
    w->current_parent = w->fullscreen_parent;
    }
  else
    {
    if(w->current_window != w->normal_window)
      send_event = 0;
    w->current_window = w->normal_window;
    w->current_parent = w->normal_parent;
    }
#if 1
  if((w->current_window == w->fullscreen_window) &&
     (w->fullscreen_parent != w->root))
    {
    bg_x11_window_get_coords(w, w->fullscreen_parent,
                      (int*)0, (int*)0,
                      &w->window_width, &w->window_height);
    XMoveResizeWindow(w->dpy, w->fullscreen_window, 0, 0,
                      w->window_width, w->window_height);
    XSetInputFocus(w->dpy, w->fullscreen_window, RevertToNone, CurrentTime);
    }
  else if((w->current_window == w->normal_window) &&
          (w->normal_parent != w->root))
    {
    bg_x11_window_get_coords(w, w->normal_parent,
                      (int*)0, (int*)0,
                      &w->window_width, &w->window_height);
    XMoveResizeWindow(w->dpy, w->normal_window, 0, 0,
                      w->window_width, w->window_height);

    if(window_is_mapped(w->dpy, w->normal_window))
      XSetInputFocus(w->dpy, w->normal_window, RevertToNone, CurrentTime);
    }
  else
    bg_x11_window_get_coords(w, w->current_window,
                      (int*)0, (int*)0,
                      &w->window_width, &w->window_height);
#endif

  if((send_event >= 0) && w->callbacks &&
     w->callbacks->set_fullscreen)
    w->callbacks->set_fullscreen(w->callbacks->data, send_event);
  
  bg_x11_window_size_changed(w);
  
  }

static int create_window(bg_x11_window_t * w,
                         int width, int height)
  {
  const char * display_name;
  long event_mask;

//  int i;
  /* Stuff for making the cursor */
  XColor black;
  Atom wm_protocols[1];
  
  XSetWindowAttributes attr;
  unsigned long attr_flags;

  if((!w->dpy) && !open_display(w))
    return 0;
  
  if(w->normal_parent != w->root)
    {
    // bg_x11_window_get_coords(w, w->normal_parent,
    //                  (int*)0, (int*)0, &width,
    //                  &height);
    
    /* We need size changes of the parent window */
    XSelectInput(w->dpy, w->normal_parent, StructureNotifyMask);
    }
  if(w->fullscreen_parent != w->root)
    {
    XSelectInput(w->dpy, w->fullscreen_parent, StructureNotifyMask);
    }
  /* Not clear why this is needed. Not creating the colormap
     results in a BadMatch error */
  w->colormap = XCreateColormap(w->dpy, RootWindow(w->dpy, w->screen),
                                w->vi->visual,
                                AllocNone);
  
  /* Setup event mask */

  event_mask =
    SubstructureNotifyMask |
    StructureNotifyMask |
    PointerMotionMask |
    ExposureMask |
    ButtonPressMask | KeyPressMask;
  
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

  /* Create normal window */
  
  w->normal_window = XCreateWindow(w->dpy, w->normal_parent,
                                   0 /* x */,
                                   0 /* y */,
                                   width, height,
                                   0 /* border_width */, w->vi->depth,
                                   InputOutput,
                                   w->vi->visual,
                                   attr_flags,
                                   &attr);
  
  if(w->normal_parent == w->root)
    {
    set_decorations(w, w->normal_window, 1);
    XSetWMProtocols(w->dpy, w->normal_window, wm_protocols, 1);
    
    if(w->min_width && w->min_height)
      {
      set_min_size(w, w->normal_window, w->min_width, w->min_height);
      }
    }
  else
    XMapWindow(w->dpy, w->normal_window);
  
  /* The fullscreen window will be created with the same size for now */
  
  w->fullscreen_window = XCreateWindow (w->dpy, w->fullscreen_parent,
                                        0 /* x */,
                                        0 /* y */,
                                        width, height,
                                        0 /* border_width */, w->vi->depth,
                                        InputOutput, w->vi->visual,
                                        attr_flags,
                                        &attr);
  
  if(w->fullscreen_parent == w->root)
    {
    /* Setup protocols */
    
    set_decorations(w, w->fullscreen_window, 0);
    XSetWMProtocols(w->dpy, w->fullscreen_window, wm_protocols, 1);
    }
  else
    XMapWindow(w->dpy, w->fullscreen_window);
  
  /* Create GC */
  
  w->gc = XCreateGC(w->dpy, w->normal_window, 0, NULL);
  
  /* Create colormap and fullscreen cursor */
  
  w->fullscreen_cursor_pixmap =
    XCreateBitmapFromData(w->dpy, w->fullscreen_window,
                          bm_no_data, 8, 8);

  black.pixel = BlackPixel(w->dpy, w->screen);
  XQueryColor(w->dpy, DefaultColormap(w->dpy, w->screen), &black);
  
  w->fullscreen_cursor=
    XCreatePixmapCursor(w->dpy, w->fullscreen_cursor_pixmap,
                        w->fullscreen_cursor_pixmap,
                        &black, &black, 0, 0);
  
  w->black = BlackPixel(w->dpy, w->screen);
  
  /* Check, which fullscreen modes we have */
  
  w->fullscreen_mode = get_fullscreen_mode(w);
  
  display_name = XDisplayName(DisplayString(w->dpy));
  
  /* Determine if we are in fullscreen mode */
  bg_x11_window_init(w);
  return 1;
  }

#if 0
void x11_window_select_input(bg_x11_window_t * w, long event_mask)
  {
  XSelectInput(w->dpy, w->normal_window, event_mask | w->event_mask);
  XSelectInput(w->dpy, w->fullscreen_window, event_mask | w->event_mask);
  }
#endif

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
    XTranslateCoordinates(w->dpy, w->normal_window, w->root, 0, 0,
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
  XEvent evt;
  Window fullscreen_toplevel;
  Window normal_toplevel;
  Window focus_window;
  
  /* Normal->fullscreen */
  
  if(w->fullscreen_parent == w->root)
    fullscreen_toplevel = w->fullscreen_window;
  else
    fullscreen_toplevel = w->fullscreen_parent;
  
  if(w->normal_parent == w->root)
    normal_toplevel = w->normal_window;
  else
    normal_toplevel = w->normal_parent;
  
  if(fullscreen && (w->current_window == w->normal_window))
    {
    get_fullscreen_coords(w, &x, &y, &width, &height);

    w->normal_width = w->window_width;
    w->normal_height = w->window_height;
    
    w->window_width = width;
    w->window_height = height;
    
    w->current_window = w->fullscreen_window;
    w->current_parent = w->fullscreen_parent;
    
    XMapRaised(w->dpy, fullscreen_toplevel);
    
    XSetTransientForHint(w->dpy, fullscreen_toplevel, None);
    
    XMoveResizeWindow(w->dpy, fullscreen_toplevel, x, y, width, height);

    if(w->fullscreen_parent != w->root)
      XMoveResizeWindow(w->dpy, w->fullscreen_window, x, y, width, height);
    
#if 1
    if(w->fullscreen_mode & FULLSCREEN_MODE_NET_ABOVE)
      {
      netwm_set_state(w, fullscreen_toplevel,
                      _NET_WM_STATE_ADD, w->_NET_WM_STATE_ABOVE);
      }
    if(w->fullscreen_mode & FULLSCREEN_MODE_NET_FULLSCREEN)
      {
      netwm_set_state(w, fullscreen_toplevel,
                      _NET_WM_STATE_ADD, w->_NET_WM_STATE_FULLSCREEN);
      }
#endif

    XWithdrawWindow(w->dpy, normal_toplevel, w->screen);
    
    /* Wait until the window is mapped */ 
    
    if(w->fullscreen_child != None)
      focus_window = w->fullscreen_child;
    else
      focus_window = w->fullscreen_window;
    
    do
      {
      XMaskEvent(w->dpy, ExposureMask, &evt);
      bg_x11_window_handle_event(w, &evt);
      } while((evt.type != Expose) || (evt.xexpose.window != focus_window));
    
    XSetInputFocus(w->dpy, focus_window, RevertToNone, CurrentTime);
    if(w->callbacks && w->callbacks->set_fullscreen)
      w->callbacks->set_fullscreen(w->callbacks->data, 1);
    bg_x11_window_clear(w);
    XFlush(w->dpy);
    ret = 1;
    }
  
  else if(!fullscreen && (w->current_window == w->fullscreen_window))
    {
    /* Unmap fullscreen window */
    netwm_set_state(w, fullscreen_toplevel,
                    _NET_WM_STATE_REMOVE, w->_NET_WM_STATE_FULLSCREEN);
    netwm_set_state(w, fullscreen_toplevel,
                    _NET_WM_STATE_REMOVE, w->_NET_WM_STATE_ABOVE);
    XWithdrawWindow(w->dpy, fullscreen_toplevel, w->screen);
    XUnmapWindow(w->dpy, fullscreen_toplevel);
    
    /* Map normal window */
    w->current_window = w->normal_window;
    w->current_parent = w->normal_parent;
    
    XMapWindow(w->dpy, normal_toplevel);
    XSync(w->dpy, False);
    
    w->window_width  = w->normal_width;
    w->window_height = w->normal_height;

    if(w->normal_child != None)
      focus_window = w->normal_child;
    else
      focus_window = w->normal_window;
    
    do
      {
      XMaskEvent(w->dpy, ExposureMask, &evt);
      bg_x11_window_handle_event(w, &evt);
      }while((evt.type != Expose) || (evt.xexpose.window != focus_window));
    
    if(w->normal_parent == w->root)
      XMoveResizeWindow(w->dpy, w->normal_window,
                        w->window_x, w->window_y,
                        w->window_width, w->window_height);
    
    XSetInputFocus(w->dpy, focus_window, RevertToNone, CurrentTime);

    if(w->callbacks && w->callbacks->set_fullscreen)
      w->callbacks->set_fullscreen(w->callbacks->data, 0);
    
    bg_x11_window_clear(w);
    XFlush(w->dpy);
    ret = 1;
    }

  if(ret)
    {
    if(check_disable_screensaver(w))
      disable_screensaver(w);
    else
      enable_screensaver(w);
    }
  return ret;
  }

void bg_x11_window_resize(bg_x11_window_t * win,
                          int width, int height)
  {
  win->normal_width = width;
  win->normal_height = height;
  if(win->current_window == win->normal_window)
    {
    win->window_width = width;
    win->window_height = height;
    XResizeWindow(win->dpy, win->normal_window, width, height);
    }
  }


/* Public methods */

bg_x11_window_t * bg_x11_window_create(const char * display_string)
  {
  bg_x11_window_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->display_string_parent = bg_strdup(ret->display_string_parent, display_string);
  ret->scaler = gavl_video_scaler_create();
  return ret;
  }

const char * bg_x11_window_get_display_string(bg_x11_window_t * w)
  {
  if(w->normal_window == None)
    create_window(w, w->window_width, w->window_height);
  
  if(!w->display_string_child)
    w->display_string_child = bg_sprintf("%s:%08lx:%08lx",
                                         XDisplayName(DisplayString(w->dpy)),
                                         w->normal_window, w->fullscreen_window);
  return w->display_string_child;
  }


void bg_x11_window_destroy(bg_x11_window_t * w)
  {
  bg_x11_window_cleanup_video(w);
  
  if(w->normal_window != None)
    XDestroyWindow(w->dpy, w->normal_window);
  
  if(w->fullscreen_window != None)
    XDestroyWindow(w->dpy, w->fullscreen_window);
  
  if(w->fullscreen_cursor != None)
    XFreeCursor(w->dpy, w->fullscreen_cursor);
  
  if(w->fullscreen_cursor_pixmap != None)
    XFreePixmap(w->dpy, w->fullscreen_cursor_pixmap);

  if(w->gc != None)
    XFreeGC(w->dpy, w->gc);
  
#ifdef HAVE_LIBXINERAMA
  if(w->xinerama)
    XFree(w->xinerama);
#endif
  if(w->vi)
    XFree(w->vi);
  
  if(w->dpy)
    XCloseDisplay(w->dpy);
  
  if(w->display_string_parent)
    free(w->display_string_parent);
  if(w->display_string_child)
    free(w->display_string_child);

  if(w->scaler)
    gavl_video_scaler_destroy(w->scaler);
  
  free(w);
  }

bg_parameter_info_t common_parameters[] =
  {
    {
      BG_LOCALE,
      name:        "window",
      long_name:   TRS("General"),
    },
    {
      name:        "auto_resize",
      long_name:   TRS("Auto resize window"),
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "window_width",
      long_name:   "Window width",
      type:        BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 320 }
    },
    {
      name:        "window_height",
      long_name:   "Window height",
      type:        BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 240 }
    },
    {
      name:        "window_x",
      long_name:   "Window x",
      type:        BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 100 }
    },
    {
      name:        "window_y",
      long_name:   "Window y",
      type:        BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 100 }
    },
#if 0
    // #ifdef HAVE_LIBXV
    {
      name:        "xv_mode",
      long_name:   TRS("Try XVideo"),
      type:        BG_PARAMETER_STRINGLIST,

      multi_names: (char*[]){ "never", "yuv_only", "always", (char*)0},
      multi_labels: (char*[]){ TRS("Never"),
                               TRS("For YCbCr formats only"),
                               TRS("Always"), (char*)0},
      val_default: { val_str: "yuv_only" },
      help_string: TRS("Choose when to try XVideo (with hardware scaling). Note that your graphics card/driver must support this."),
    },
#endif
    {
      name:        "disable_xscreensaver_normal",
      long_name:   TRS("Disable Screensaver for normal playback"),
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 }
    },
    {
      name:        "disable_xscreensaver_fullscreen",
      long_name:   TRS("Disable Screensaver for fullscreen playback"),
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "sw_scaler",
      long_name:   TRS("Software scaler"),
      type:        BG_PARAMETER_SECTION,
    },
    {
      name:        "scale_mode",
      long_name:   TRS("Scale mode"),
      type:        BG_PARAMETER_STRINGLIST,
      multi_names:  (char*[]){ "auto",
                               "nearest",
                               "bilinear",
                               "quadratic",
                               "cubic_bspline",
                               "cubic_mitchell",
                               "cubic_catmull",
                               "sinc_lanczos",
                               (char*)0 },
      multi_labels: (char*[]){ TRS("Auto"),
                               TRS("Nearest"),
                               TRS("Bilinear"),
                               TRS("Quadratic"),
                               TRS("Cubic B-Spline"),
                               TRS("Cubic Mitchell-Netravali"),
                               TRS("Cubic Catmull-Rom"),
                               TRS("Sinc with Lanczos window"),
                               (char*)0 },
      val_default: { val_str: "auto" },
      help_string: TRS("Choose scaling method. Auto means to choose based on the conversion quality. Nearest is fastest, Sinc with Lanczos window is slowest"),
    },
    {
      name:        "scale_order",
      long_name:   TRS("Scale order"),
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 4 },
      val_max:     { val_i: 1000 },
      val_default: { val_i: 4 },
      help_string: TRS("Order for sinc scaling"),
    },
    {
      name:        "scale_quality",
      long_name:   TRS("Scale quality"),
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: GAVL_QUALITY_FASTEST },
      val_max:     { val_i: GAVL_QUALITY_BEST },
      val_default: { val_i: GAVL_QUALITY_DEFAULT },
      help_string: TRS("Scale quality"),
    },
  };


bg_parameter_info_t * bg_x11_window_get_parameters(bg_x11_window_t * win)
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
    if(win->normal_parent == win->root)
      win->window_x = val->val_i;
    }
  else if(!strcmp(name, "window_y"))
    {
    if(win->normal_parent == win->root)
      win->window_y = val->val_i;
    }
  else if(!strcmp(name, "window_width"))
    {
    if(win->normal_parent == win->root)
      win->window_width = val->val_i;
    }
  else if(!strcmp(name, "window_height"))
    {
    if(win->normal_parent == win->root)
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
  
  }

int bg_x11_window_create_window(bg_x11_window_t * win)
  {
  int num;
  XVisualInfo vi_template;

  if(!win->dpy && !open_display(win))
    return 0;
  
  memset(&vi_template, 0, sizeof(vi_template));
  vi_template.class = TrueColor;
  vi_template.depth = DefaultDepth(win->dpy, win->screen);
  
  win->vi = XGetVisualInfo(win->dpy, VisualClassMask | VisualDepthMask,
                           &vi_template, &num);
  return create_window(win, win->window_width, win->window_height);
  }

int bg_x11_window_create_window_gl(bg_x11_window_t * win,
                                   int * attr_list )
  {
#ifdef HAVE_GLX
  if(!win->dpy && !open_display(win))
    return 0;
  win->vi = glXChooseVisual(win->dpy, win->screen, attr_list);
  return create_window(win, win->window_width, win->window_height);
#else
  return 0;
#endif
  }

/* Handle X11 events, callbacks are called from here */
void bg_x11_window_set_callbacks(bg_x11_window_t * win,
                                 bg_x11_window_callbacks_t * callbacks)
  {
  win->callbacks = callbacks;
  }



void bg_x11_window_set_title(bg_x11_window_t * w, const char * title)
  {
  if(w->normal_parent == w->root)
    XmbSetWMProperties(w->dpy, w->normal_window, title,
                       title, NULL, 0, NULL, NULL, NULL);
  }

void bg_x11_window_set_class_hint(bg_x11_window_t * w,
                                  char * name, char * klass)
  {
  XClassHint xclasshint={name,klass};

  if(w->normal_parent == w->root)
    XSetClassHint(w->dpy, w->normal_window, &xclasshint);

  if(w->fullscreen_parent == w->root)
    XSetClassHint(w->dpy, w->fullscreen_window, &xclasshint);
  }

void bg_x11_window_show(bg_x11_window_t * win, int show)
  {
  if(!show)
    {
    if(win->current_window == None)
      return;
    
    XWithdrawWindow(win->dpy, win->current_window,
                    DefaultScreen(win->dpy));
    win->mapped = 0;
    XSync(win->dpy, False);
    enable_screensaver(win);
    return;
    }

  if(check_disable_screensaver(win))
    disable_screensaver(win);
  
  /* If the window was already mapped, raise it */
  
  if(win->mapped)
    {
    XRaiseWindow(win->dpy, win->current_window);
    }
  else
    {
    XMapWindow(win->dpy, win->current_window);

    if((win->current_window == win->normal_window) &&
       (win->normal_parent == win->root))
      {
      XMoveResizeWindow(win->dpy, win->normal_window,
                        win->window_x, win->window_y,
                        win->window_width, win->window_height);
      }
    else if(win->current_window == win->fullscreen_window)
      {
      if(win->fullscreen_mode & FULLSCREEN_MODE_NET_ABOVE)
        {
        netwm_set_state(win, win->fullscreen_window,
                        _NET_WM_STATE_ADD, win->_NET_WM_STATE_ABOVE);
        }
      if(win->fullscreen_mode & FULLSCREEN_MODE_NET_FULLSCREEN)
        {
        netwm_set_state(win, win->fullscreen_window,
                        _NET_WM_STATE_ADD, win->_NET_WM_STATE_FULLSCREEN);
        }
      }
    }
  }
