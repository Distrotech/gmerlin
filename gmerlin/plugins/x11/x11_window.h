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


/*
 *  Stuff for making a nice normal/fullscreen window pair
 *  (This code is shared with lemnuria)
 * 
 *  We open an own X11-connection here, as this is, what all
 *  animation/video playback programs should do.
 */

#include <time.h>

#include <config.h>

#ifdef HAVE_LIBXINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#ifdef HAVE_XDPMS
#include <X11/extensions/dpms.h>
#endif

#define SCREENSAVER_MODE_XLIB  0 // MUST be 0 (fallback)
#define SCREENSAVER_MODE_GNOME 1
#define SCREENSAVER_MODE_KDE   2

typedef struct
  {
  /* User settable stuff (initialized before x11_window_create) */

  int min_width;
  int min_height;
  
#ifdef HAVE_LIBXINERAMA
  XineramaScreenInfo *xinerama;
  int                nxinerama;
#endif

#ifdef HAVE_XDPMS
  int dpms_disabled;
#endif
  
  long event_mask;
  int mapped;
  unsigned long black;  
  Display * dpy;
  GC gc;  
  Window normal_window;
  Window fullscreen_window;
  Window current_window;
  Window current_parent;
  Window root;

  Window normal_parent;
  Window fullscreen_parent;
  
  Window normal_child;
  Window fullscreen_child;
  
  int window_width, window_height;

  int normal_width, normal_height;
  
  int screen;
  int window_x, window_y;

  /* Event types (set by x11_window_handle_event()) */

  int do_delete;
    
  /* Fullscreen stuff */

  int fullscreen_mode;
  Pixmap fullscreen_cursor_pixmap;
  Cursor fullscreen_cursor;

  Atom WM_DELETE_WINDOW;
  Atom _NET_SUPPORTED;
  Atom _NET_WM_STATE;
  Atom _NET_WM_STATE_FULLSCREEN;
  Atom _NET_WM_STATE_ABOVE;
  Atom _NET_MOVERESIZE_WINDOW;
  Atom WIN_PROTOCOLS;
  Atom WM_PROTOCOLS;
  Atom WIN_LAYER;

  /* For hiding the mouse pointer */
  int idle_counter;
  int pointer_hidden;
  
  XEvent evt;

  /* Screensaver stuff */
  int screensaver_mode;
  int screensaver_disabled;
  int screensaver_was_enabled;
  int screensaver_saved_timeout;

  int disable_screensaver_normal;
  int disable_screensaver_fullscreen;
  
  int64_t screensaver_last_ping_time;
  
  char * display_string;

  Colormap colormap;
  int size_changed;
  } x11_window_t;

int x11_window_open_display(x11_window_t * w, const char * display_string);

int x11_window_create(x11_window_t * w, Visual * visual, int depth,
                      int width, int height, const char * title);

const char *
x11_window_get_display_string(x11_window_t * w);

void x11_window_handle_event(x11_window_t*, XEvent*evt);

void x11_window_select_input(x11_window_t*, long event_mask);

void x11_window_set_fullscreen(x11_window_t*,int);

void x11_window_destroy(x11_window_t*);

void x11_window_set_title(x11_window_t*, const char * title);

void x11_window_show(x11_window_t*, int show);

void x11_window_set_class_hint(x11_window_t*,
                                const char * name,
                                const char * klass);

void x11_window_clear(x11_window_t*);


/* Timeout is in milliseconds, -1 means block forever */

XEvent * x11_window_next_event(x11_window_t*,
                               int timeout);

void x11_window_resize(x11_window_t * win,
                       int width, int height);

void x11_window_init(x11_window_t * win);

