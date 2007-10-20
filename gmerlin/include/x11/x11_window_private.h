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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_GLX
#include <GL/glx.h>
#endif

#include <X11/extensions/XShm.h>

typedef struct video_driver_s video_driver_t;

typedef struct
  {
  video_driver_t * driver;
  gavl_pixelformat_t * pixelformats;
  void * priv;
  int can_scale;
  bg_x11_window_t * win;
  
  /* Selected pixelformat (used by the core only) */
  gavl_pixelformat_t pixelformat;
  int penalty;
  } driver_data_t;

extern video_driver_t ximage_driver;

#ifdef HAVE_LIBXV
extern video_driver_t xv_driver;
#endif

#define MAX_DRIVERS 2

struct video_driver_s
  {
  int can_scale;
  
  int (*init)(driver_data_t* data);
  
  int (*open)(driver_data_t* data);
  
  gavl_video_frame_t * (*create_frame)(driver_data_t* data);

  void (*destroy_frame)(driver_data_t* data,
                        gavl_video_frame_t *);
  
  void (*put_frame)(driver_data_t* data,
                    gavl_video_frame_t * frame);
  
  void (*close)(driver_data_t* data);
  void (*cleanup)(driver_data_t* data);
  };

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

  XVisualInfo * vi;
  
#ifdef HAVE_GLX
  GLXContext glxcontext;
#endif

  /* XShm */
  
  int have_shm;
  int shm_completion_type;
  int wait_for_completion;
  
  gavl_video_format_t video_format;

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
  
  } x11_window_t;

/* Private functions */

void bg_x11_window_handle_event(bg_x11_window_t * win, XEvent * evt);
void bg_x11_window_ping_screensaver(bg_x11_window_t * w);
void bg_x11_window_get_coords(bg_x11_window_t * w,
                              Window win,
                              int * x, int * y, int * width,
                              int * height);
void bg_x11_window_init(bg_x11_window_t * w);

int bg_x11_window_check_shm(Display * dpy, int * completion_type);

int bg_x11_window_create_shm(bg_x11_window_t * w,
                          XShmSegmentInfo * shminfo, int size);

void bg_x11_window_destroy_shm(bg_x11_window_t * w,
                            XShmSegmentInfo * shminfo);

void bg_x11_window_size_changed(bg_x11_window_t * w);

void bg_x11_window_cleanup_video(bg_x11_window_t * w);
