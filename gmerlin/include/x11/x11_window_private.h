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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_LIBXINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#ifdef HAVE_XDPMS
#include <X11/extensions/dpms.h>
#endif

#define SCREENSAVER_MODE_XLIB  0 // MUST be 0 (fallback)
#define SCREENSAVER_MODE_GNOME 1
#define SCREENSAVER_MODE_KDE   2

#ifdef HAVE_GLX
#include <GL/glx.h>
#endif

#include <X11/extensions/XShm.h>

typedef struct video_driver_s video_driver_t;

#define DRIVER_FLAG_BRIGHTNESS (1<<0)
#define DRIVER_FLAG_SATURATION (1<<1)
#define DRIVER_FLAG_CONTRAST   (1<<2)

typedef struct
  {
  int flags;
  const video_driver_t * driver;
  gavl_pixelformat_t * pixelformats;
  void * priv;
  int can_scale;
  bg_x11_window_t * win;
  
  /* Selected pixelformat (used by the core only) */
  gavl_pixelformat_t pixelformat;
  int penalty;
  } driver_data_t;

extern const video_driver_t ximage_driver;

#ifdef HAVE_LIBXV
extern const video_driver_t xv_driver;
#endif

extern const video_driver_t gl_driver;

#define MAX_DRIVERS 3

struct video_driver_s
  {
  int can_scale;
  int (*init)(driver_data_t* data);
  int (*open)(driver_data_t* data);
  
  void (*add_overlay_stream)(driver_data_t* data);
  void (*set_overlay)(driver_data_t* data, int stream, gavl_overlay_t * ovl);
  
  gavl_video_frame_t * (*create_overlay)(driver_data_t* data, int stream);
  void (*destroy_overlay)(driver_data_t* data, int stream, gavl_video_frame_t*);
  
  gavl_video_frame_t * (*create_frame)(driver_data_t* data);
  
  void (*destroy_frame)(driver_data_t* data, gavl_video_frame_t *);
  
  void (*put_frame)(driver_data_t* data,
                    gavl_video_frame_t * frame);

  void (*set_brightness)(driver_data_t* data,float brightness);
  void (*set_saturation)(driver_data_t* data,float saturation);
  void (*set_contrast)(driver_data_t* data,float contrast);
  
  void (*close)(driver_data_t* data);
  void (*cleanup)(driver_data_t* data);
  };

/*
 *  We have 2 windows: One normal window and one
 *  fullscreen window. We optionally talk to both the
 *  children and parents through the XEmbed protocol
 */

typedef struct
  {
  Window win;    /* Actual window */
  Window parent; /* Parent (if non-root we are embedded) */
  Window child;  /* Child window */
  Window toplevel;
  Window focus_child;  /* Focus proxy */
  int parent_xembed;
  int child_xembed;
  int mapped;
  int fullscreen;
  bg_accelerator_map_t * child_accel_map;
  int modality;
  } window_t;

struct bg_x11_window_s
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
  
  unsigned long black;  
  Display * dpy;
  GC gc;

  int is_fullscreen;
  
  window_t normal;
  window_t fullscreen;
  window_t * current;
  
  Window root;
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
  Atom WM_TAKE_FOCUS;
  Atom _NET_SUPPORTED;
  Atom _NET_WM_STATE;
  Atom _NET_WM_STATE_FULLSCREEN;
  Atom _NET_WM_STATE_STAYS_ON_TOP;
  Atom _NET_WM_STATE_ABOVE;
  Atom _NET_MOVERESIZE_WINDOW;
  Atom WIN_PROTOCOLS;
  Atom WM_PROTOCOLS;
  Atom WIN_LAYER;
  Atom _XEMBED_INFO;
  Atom _XEMBED;
  Atom STRING;
  Atom WM_CLASS;
  
  /* For hiding the mouse pointer */
  int idle_counter;
  int pointer_hidden;
  
  /* Screensaver stuff */
  int screensaver_mode;
  int screensaver_disabled;
  int screensaver_was_enabled;
  int screensaver_saved_timeout;

  int disable_screensaver_normal;
  int disable_screensaver_fullscreen;
  int64_t screensaver_last_ping_time;
  
  char * display_string_parent;
  char * display_string_child;

  int auto_resize;
  
  Colormap colormap;

  bg_x11_window_callbacks_t * callbacks;

  XVisualInfo * gl_vi;
  Visual * visual;
  int depth;
  
#ifdef HAVE_GLX
  GLXContext glxcontext;
#endif

  struct
    {
    int value;
    int changed;
    } gl_attributes[BG_GL_ATTRIBUTE_NUM];

  /* XShm */
  
  int have_shm;
  int shm_completion_type;
  int wait_for_completion;
  
  gavl_video_format_t video_format;
  int video_open;
  
  /* Scaling stuff */
  gavl_video_format_t window_format;
  gavl_video_frame_t * window_frame;
  gavl_video_scaler_t * scaler;
  int do_sw_scale;
  int scaler_options_changed;
  
  gavl_rectangle_f_t src_rect;
  gavl_rectangle_i_t dst_rect;
  
  driver_data_t drivers[MAX_DRIVERS];
  
  driver_data_t * current_driver;
  
  int drivers_initialized;

  /* For asynchronous focus grabbing */
  int need_focus;
  Time focus_time;

  int need_fullscreen;
  
  int force_hw_scale;
  
  /* Overlay stuff */
  int num_overlay_streams;
  
  int has_overlay; /* 1 if there are overlays to blend, 0 else */
  
  struct
    {
    gavl_overlay_blend_context_t * ctx;
    gavl_overlay_t * ovl;
    unsigned long texture; /* For OpenGL only */
    gavl_video_format_t format;
    } * overlay_streams;

  float brightness;
  float saturation;
  float contrast;
  
  gavl_video_frame_t * still_frame;
  gavl_video_frame_t * out_frame;
  int still_mode;

  Pixmap icon;
  Pixmap icon_mask;
  };

void bg_x11_window_put_frame_internal(bg_x11_window_t * win,
                                      gavl_video_frame_t * frame);

/* Private functions */

void bg_x11_window_handle_event(bg_x11_window_t * win, XEvent * evt);
void bg_x11_window_ping_screensaver(bg_x11_window_t * w);
void bg_x11_window_get_coords(bg_x11_window_t * w,
                              Window win,
                              int * x, int * y, int * width,
                              int * height);
void bg_x11_window_init(bg_x11_window_t * w);

gavl_pixelformat_t 
bg_x11_window_get_pixelformat(bg_x11_window_t * win);

void bg_x11_window_make_icon(bg_x11_window_t * win,
                             const gavl_video_frame_t * icon,
                             const gavl_video_format_t * icon_format,
                             Pixmap * icon_ret, Pixmap * mask_ret);


int bg_x11_window_check_shm(Display * dpy, int * completion_type);

int bg_x11_window_create_shm(bg_x11_window_t * w,
                          XShmSegmentInfo * shminfo, int size);

void bg_x11_window_destroy_shm(bg_x11_window_t * w,
                            XShmSegmentInfo * shminfo);

void bg_x11_window_size_changed(bg_x11_window_t * w);

void bg_x11_window_cleanup_video(bg_x11_window_t * w);

Window bg_x11_window_get_toplevel(bg_x11_window_t * w, Window win);

void bg_x11_window_send_xembed_message(bg_x11_window_t * w, Window win, long time,
                                       int type, int detail, int data1, int data2);

void bg_x11_window_embed_parent(bg_x11_window_t * win,
                                window_t * w);
void bg_x11_window_embed_child(bg_x11_window_t * win,
                               window_t * w);

int bg_x11_window_check_embed_property(bg_x11_window_t * win,
                                       window_t * w);

void bg_x11_window_set_fullscreen_mapped(bg_x11_window_t * win,
                                         window_t * w);

/* For OpenGL support */

int bg_x11_window_init_gl(bg_x11_window_t *);

#define XEMBED_MAPPED                   (1 << 0)

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY		0
#define XEMBED_WINDOW_ACTIVATE  	1
#define XEMBED_WINDOW_DEACTIVATE  	2
#define XEMBED_REQUEST_FOCUS	 	3
#define XEMBED_FOCUS_IN 	 	4
#define XEMBED_FOCUS_OUT  		5
#define XEMBED_FOCUS_NEXT 		6
#define XEMBED_FOCUS_PREV 		7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON 		10
#define XEMBED_MODALITY_OFF 		11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

/* Modifiers field for XEMBED_REGISTER_ACCELERATOR */
#define XEMBED_MODIFIER_SHIFT    (1 << 0)
#define XEMBED_MODIFIER_CONTROL  (1 << 1)
#define XEMBED_MODIFIER_ALT      (1 << 2)
#define XEMBED_MODIFIER_SUPER    (1 << 3)
#define XEMBED_MODIFIER_HYPER    (1 << 4)

