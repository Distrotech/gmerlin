/*****************************************************************
 
  ov_x11.c
 
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

#include <pthread.h>

#include <string.h>

// #define DEBUG

#include <stdio.h>

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/keysym.h>

#include <plugin.h>
#include <utils.h>

#include <X11/extensions/Xvlib.h>


#define XV_ID_YV12  0x32315659 
#define XV_ID_I420  0x30323449
#define XV_ID_YUY2  0x32595559

#define HAVE_XV
#define HAVE_XSHM

/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase (Display *);

typedef struct 
  {
  XImage * x11_image;
  XvImage * xv_image;
  XShmSegmentInfo shminfo;
  } x11_frame_t;

/* The indices and array MUST match */

#define XV_PARAM_HUE        0
#define XV_PARAM_BRIGHTNESS 1
#define XV_PARAM_SATURATION 2
#define XV_PARAM_CONTRAST   3

static struct
  {
  char * xv_name;
  char * long_name;
  }
xv_parameters[] =
  {
    { "XV_HUE",        "Hue"        },
    { "XV_BRIGHTNESS", "Brightness" },
    { "XV_SATURATION", "Saturation" },
    { "XV_CONTRAST",   "Contrast"   }
  };

#define NUM_XV_PARAMETERS (sizeof(xv_parameters)/sizeof(xv_parameters[0]))

typedef struct
  {
  gavl_video_format_t format;
  
  unsigned int window_width;
  unsigned int window_height;

  int screen_width;
  int screen_height;
  
  /* Supported modes */

  int have_shm;
  int have_xv_yv12;
  int have_xv_yuy2;
  int have_xv_i420;

  int force_xv;
  
  /* Actual mode */

  int do_xv;
  int xv_format;
  
  /* X11 Stuff */
  
  Display * dpy;

  Window current_window;
  Window normal_window;
  Window fullscreen_window;
    
  XvPortID xv_port;
  Atom xv_attr_atoms[NUM_XV_PARAMETERS];
  
  GC gc;
  Colormap colormap;

  int shm_completion_type;

  /* Fullscreen stuff */

  Pixmap fullscreen_cursor_pixmap;
  Cursor fullscreen_cursor;
  Atom hints_atom;
    
  /* Drawing coords */

  int dst_x;
  int dst_y;
  int dst_w;
  int dst_h;

  /* Configuration flags */

  int auto_resize;
  int remember_size;
  
  /* Callbacks */

  bg_ov_callbacks_t * callbacks;

  /* Delete atom */

  Atom delete_atom;
  Atom protocols_atom;

  bg_parameter_info_t * parameters;

  /* Still image stuff */

  pthread_t       still_thread;
  pthread_mutex_t still_mutex;
  int do_still;
  gavl_video_frame_t * still_frame;
  } x11_t;

void set_callbacks_x11(void * data, bg_ov_callbacks_t * callbacks)
  {
  ((x11_t*)(data))->callbacks = callbacks;
  }

static gavl_colorspace_t get_x11_colorspace(Display * d)
  {
  int screen_number;
  Visual * visual;
  int depth;
  int bpp;
  XPixmapFormatValues * pf;
  int i;
  int num_pf;
  gavl_colorspace_t ret = GAVL_COLORSPACE_NONE;
    
  screen_number = DefaultScreen(d);
  visual = DefaultVisual(d, screen_number);

  if(visual->class != TrueColor)
    {
    //    fprintf(stderr, "No True Color Visual\n");
    return ret;
    }

  depth = DefaultDepth(d, screen_number);
  bpp = 0;
  pf = XListPixmapFormats(d, &num_pf);
  for(i = 0; i < num_pf; i++)
    {
    if(pf[i].depth == depth)
      bpp = pf[i].bits_per_pixel;
    }
  XFree(pf);
  
  ret = GAVL_COLORSPACE_NONE;

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

/*
 *  Stuff copied from some other Software to make a
 *  fullscreen window
 */

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} MotifWmHints, MwmHints;
                                                                               
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)
                                                                               
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)
                                                                               
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)
#define _XA_MOTIF_WM_HINTS              "_MOTIF_WM_HINTS"
                                                                               
static unsigned char bm_no_data[] = { 0,0,0,0, 0,0,0,0 };

static MwmHints fullscreen_hints =
  {
    flags:       MWM_HINTS_DECORATIONS,
    functions:   0,  /* Unused */
    decorations: 0,
    input_mode:  0,  /* Unused */
    status:      0,  /* Unused */
  };

static void make_fullscreen_window(x11_t * x11, Window win)
  {
  MotifWmHints *hints;
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_after;
                                                                               
  /* Stuff for making the cursor */
  XColor black, dummy;
  unsigned char * hints_c;
  
  /*
   *   Set the decorations (via Mwm hints)
   *   This is taken from the sources of gdk-1.2.8
   */
  
  if (!x11->hints_atom)
    x11->hints_atom = XInternAtom (x11->dpy,
                                   _XA_MOTIF_WM_HINTS, False);
  
  XGetWindowProperty(x11->dpy, win,
                     x11->hints_atom, 0, sizeof (MotifWmHints)/sizeof (long),
                     False, AnyPropertyType, &type, &format, &nitems,
                     &bytes_after, &hints_c);

  hints = (MotifWmHints *)hints_c;

  if(type == None)
    hints = &fullscreen_hints;
  else
    {
    if (fullscreen_hints.flags & MWM_HINTS_FUNCTIONS)
      {
      hints->flags |= MWM_HINTS_FUNCTIONS;
      hints->functions = fullscreen_hints.functions;
      }
    if (fullscreen_hints.flags & MWM_HINTS_DECORATIONS)
      {
      hints->flags |= MWM_HINTS_DECORATIONS;
      hints->decorations = fullscreen_hints.decorations;
      }
    }

  XChangeProperty(x11->dpy, win,
                  x11->hints_atom, x11->hints_atom, 32, PropModeReplace,
                  (unsigned char *)hints,
                  sizeof (MotifWmHints)/sizeof (long));

  if (hints != &fullscreen_hints)
    XFree (hints);

  /* Make fullscreen cursor */

  if(x11->fullscreen_cursor_pixmap == None)
    {
    x11->fullscreen_cursor_pixmap =
      XCreateBitmapFromData(x11->dpy, win,
                            bm_no_data, 8, 8);
                                                                                
    XAllocNamedColor(x11->dpy, x11->colormap, "black", &black, &dummy);
    x11->fullscreen_cursor=
      XCreatePixmapCursor(x11->dpy, x11->fullscreen_cursor_pixmap,
                          x11->fullscreen_cursor_pixmap,
                          &black, &black, 0, 0);
    }
  
  XDefineCursor(x11->dpy, win, x11->fullscreen_cursor);
  }


/*
 *  Create the window, if width and height are zero, a fullscreen
 *  window will be created
 */

static Window create_window(x11_t * x11, int width, int height)
  {
  Window ret;
  Visual * visual;
  XSetWindowAttributes attr;
  int screen_number;
  int depth;
  Display * d;
  int fullscreen;
    
  d = x11->dpy;
  
  screen_number = DefaultScreen(d);
  visual = DefaultVisual(d, screen_number);
  depth = DefaultDepth(d, screen_number);

  if(!width || !height)
    {
    width = x11->screen_width;
    height = x11->screen_height;
    fullscreen = 1;
    }
  else
    fullscreen = 0;
    
  attr.background_pixmap = None;
  attr.backing_store = NotUseful;
  attr.border_pixel = 0;
  attr.background_pixel = 0;
  attr.event_mask =
    StructureNotifyMask |
    ButtonPressMask |
    KeyPressMask |
    ExposureMask;
  
  ret = XCreateWindow (d, DefaultRootWindow (d),
                       0 /* x */, 0 /* y */, width, height,
                       0 /* border_width */, depth,
                       InputOutput, visual,
                       (CWBackPixmap | CWBackingStore | CWBorderPixel |
                        CWBackPixel |
                        CWEventMask), &attr);

  XSetWMProtocols(x11->dpy, ret, &x11->delete_atom, 1);
  
  if(fullscreen)
    {
    make_fullscreen_window(x11, ret);
    }

  return ret;
  }

static int check_shm(Display * dpy, int * completion_type)
  {
#ifndef HAVE_XSHM
  return 0;
#else
  int major;
  int minor;
  Bool pixmaps;
  int ret;
  if ((XShmQueryVersion (dpy, &major, &minor,
                         &pixmaps) == 0) ||
      (major < 1) || ((major == 1) && (minor < 1)))
    ret = 0;
  else
    ret = 1;
  if(ret)
    *completion_type = XShmGetEventBase(dpy) + ShmCompletion;
  return ret;
#endif
  }

static void check_xv(Display * d, Window w,
                     XvPortID * port,
                     int * have_yv12, int * have_yuy2,
                     int * have_i420)
  {
  unsigned int version;
  unsigned int release;
  unsigned int dummy;
  unsigned int adaptors;
  unsigned int i;
  unsigned long j;
  XvAdaptorInfo * adaptorInfo;
  XvImageFormatValues * formatValues;
  int formats;
  int k;
  int found = 0;

  *have_yv12 = 0;
  *have_yuy2 = 0;
  *have_i420 = 0;
  if ((XvQueryExtension (d, &version, &release,
                         &dummy, &dummy, &dummy) != Success) ||
      (version < 2) || ((version == 2) && (release < 2)))
    return;

  XvQueryAdaptors (d, w, &adaptors, &adaptorInfo);
  for (i = 0; i < adaptors; i++)
    {
    if (adaptorInfo[i].type & XvImageMask)
      {
      for (j = 0; j < adaptorInfo[i].num_ports; j++)
        {
        formatValues =
          XvListImageFormats(d,
                             adaptorInfo[i].base_id + j,
                             &formats);
        for (k = 0; k < formats; k++)
          {
          if (formatValues[k].id == XV_ID_YV12)
            {
            *have_yv12 = 1;
            found = 1;
            }
          else if(formatValues[k].id == XV_ID_YUY2)
            {
            *have_yuy2 = 1;
            found = 1;
            }
          else if(formatValues[k].id == XV_ID_I420)
            {
            *have_i420 = 1;
            found = 1;
            }
          }
        XFree (formatValues);
        if(found)
          {
          *port = adaptorInfo[i].base_id + j;
          break;
          }
        }
      }
    if(found)
      break;
    }
  XvFreeAdaptorInfo (adaptorInfo);
#ifndef HAVE_XV
  *have_yv12 = 0;
  *have_yuy2 = 0;
  *have_i420 = 0;
#endif
  return;
  }

static int shmerror = 0;

static int handle_error (Display * display, XErrorEvent * error)
  {
  shmerror = 1;
  return 0;
  }

static int create_shm(x11_t * priv, x11_frame_t * frame, int size)
  {
  shmerror = 0;
  frame->shminfo.shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);
  if (frame->shminfo.shmid == -1)
    goto error;

  frame->shminfo.shmaddr = (char *) shmat (frame->shminfo.shmid, 0, 0);
  if (frame->shminfo.shmaddr == (char *)-1)
    goto error;

  XSync (priv->dpy, False);
  XSetErrorHandler (handle_error);

  frame->shminfo.readOnly = True;
  if(!(XShmAttach (priv->dpy, &(frame->shminfo))))
    shmerror = 1;
  
  XSync (priv->dpy, False);
  XSetErrorHandler (NULL);
  
  if(shmerror)
    {
    error:
#ifdef DEBUG
    fprintf (stderr, "cannot create shared memory\n");
#endif
    return 0;
    }

  return 1;
  }

static void destroy_shm(x11_t * priv, x11_frame_t * frame)
  {
  XShmDetach (priv->dpy, &(frame->shminfo));
  shmdt (frame->shminfo.shmaddr);
  shmctl (frame->shminfo.shmid, IPC_RMID, 0);
  }

static gavl_video_frame_t *
alloc_frame_xv(x11_t * priv)
  {
  gavl_video_frame_t * ret;
  gavl_video_format_t video_format;
  x11_frame_t * x11_frame;
  x11_frame = calloc(1, sizeof(*x11_frame));
  
  if(priv->have_shm)
    {
    x11_frame->xv_image =
      XvShmCreateImage(priv->dpy, priv->xv_port, priv->xv_format, NULL,
                       priv->format.image_width,
                       priv->format.image_height, &(x11_frame->shminfo));
    if(!x11_frame->xv_image)
      priv->have_shm = 0;
    else
      {
      if(!create_shm(priv, x11_frame,
                     x11_frame->xv_image->data_size))
        {
        XFree(x11_frame->xv_image);
        x11_frame->xv_image = NULL;
        priv->have_shm = 0;
        }
      else
        {
        x11_frame->xv_image->data = x11_frame->shminfo.shmaddr;
        }
      }
    }
  
  if(!priv->have_shm)
    {
    video_format.image_width  = priv->format.image_width;
    video_format.image_height = priv->format.image_height;

    video_format.frame_width  = priv->format.frame_width;
    video_format.frame_height = priv->format.frame_height;

    video_format.colorspace = priv->format.colorspace;
    
    ret = gavl_video_frame_create(&video_format);
    x11_frame->xv_image = XvCreateImage(priv->dpy, priv->xv_port,
                                       priv->xv_format, ret->planes[0],
                                       priv->format.frame_width, priv->format.frame_height);
    }
  else
    {
    ret = calloc(1, sizeof(*ret));
    }
  
  switch(priv->xv_format)
    {
    case XV_ID_YV12:
      ret->planes[0] =
        x11_frame->xv_image->data + x11_frame->xv_image->offsets[0];
      ret->planes[1] =
        x11_frame->xv_image->data + x11_frame->xv_image->offsets[2];
      ret->planes[2] =
        x11_frame->xv_image->data + x11_frame->xv_image->offsets[1];
      
      ret->strides[0]      = x11_frame->xv_image->pitches[0];
      ret->strides[1]      = x11_frame->xv_image->pitches[2];
      ret->strides[2]      = x11_frame->xv_image->pitches[1];
      break;
    case XV_ID_I420:
      ret->planes[0] =
        x11_frame->xv_image->data + x11_frame->xv_image->offsets[0];
      ret->planes[1] =
        x11_frame->xv_image->data + x11_frame->xv_image->offsets[1];
      ret->planes[2] =
        x11_frame->xv_image->data + x11_frame->xv_image->offsets[2];
      
      ret->strides[0] = x11_frame->xv_image->pitches[0];
      ret->strides[1] = x11_frame->xv_image->pitches[1];
      ret->strides[2] = x11_frame->xv_image->pitches[2];
      break;
    case XV_ID_YUY2:
      ret->planes[0] =
        x11_frame->xv_image->data + x11_frame->xv_image->offsets[0];
      ret->strides[0] = x11_frame->xv_image->pitches[0];
      break;
    }

  ret->user_data = x11_frame;
  return ret;
  }

static gavl_video_frame_t *
alloc_frame_ximage(x11_t * priv)
  {
  Visual * visual;
  int depth;
  gavl_video_format_t video_format;
  x11_frame_t * x11_frame;
  gavl_video_frame_t * ret = (gavl_video_frame_t*)0;

  x11_frame = calloc(1, sizeof(*x11_frame));
  
  visual = DefaultVisual(priv->dpy, DefaultScreen(priv->dpy));
  depth = DefaultDepth(priv->dpy, DefaultScreen(priv->dpy));

  if(priv->have_shm)
    {
    /* Create Shm Image */
    x11_frame->x11_image = XShmCreateImage(priv->dpy, visual, depth, ZPixmap,
                                           NULL, &(x11_frame->shminfo),
                                           priv->format.frame_width,
                                           priv->format.frame_height);
    if(!x11_frame->x11_image)
      priv->have_shm = 0;
    else
      {
      if(!create_shm(priv, x11_frame,
                     x11_frame->x11_image->height *
                     x11_frame->x11_image->bytes_per_line))
        {
        XDestroyImage(x11_frame->x11_image);

        priv->have_shm = 0;
        }
      x11_frame->x11_image->data = x11_frame->shminfo.shmaddr;

      ret = calloc(1, sizeof(*ret));

      ret->planes[0] = x11_frame->x11_image->data;
      ret->strides[0] = x11_frame->x11_image->bytes_per_line;
      }
    }
  if(!priv->have_shm)
    {
    /* Use gavl to allocate memory aligned scanlines */
    video_format.image_width = priv->format.image_width;
    video_format.image_height = priv->format.image_height;

    video_format.frame_width = priv->format.frame_width;
    video_format.frame_height = priv->format.frame_height;

    video_format.colorspace = priv->format.colorspace;

    ret = gavl_video_frame_create(&video_format);
    
    x11_frame->x11_image = XCreateImage(priv->dpy, visual, depth, ZPixmap,
                                        0, ret->planes[0],
                                        priv->format.frame_width,
                                        priv->format.frame_height,
                                        32,
                                        ret->strides[0]);
    }
  ret->user_data = x11_frame;
  return ret;
  }

static gavl_video_frame_t * alloc_frame_x11(void * data)
  {
  x11_t * priv = (x11_t*)(data);
  if(priv->do_xv)
    return alloc_frame_xv(priv);
  else
    return alloc_frame_ximage(priv);
  }

static void
free_frame_x11(void * data, gavl_video_frame_t * frame)
  {
  x11_frame_t * x11_frame;
  x11_t * priv;

  x11_frame = (x11_frame_t*)(frame->user_data);
  priv = (x11_t*)(data);
  
  if(priv->do_xv)
    XFree(x11_frame->xv_image);
  else
    XFree(x11_frame->x11_image);
  
  if(priv->have_shm)
    {
    destroy_shm(priv, x11_frame);
    free(x11_frame);
    free(frame);
    }
  else
    {
    free(x11_frame);
    gavl_video_frame_destroy(frame);
    }
  }

static void * create_x11()
  {
  x11_t * priv;
  int screen;
  
  priv = calloc(1, sizeof(x11_t));

  pthread_mutex_init(&(priv->still_mutex), NULL);
    
  /* Open X Display */

  priv->dpy = XOpenDisplay(NULL);

  /* Get delete atom */

  priv->delete_atom = XInternAtom(priv->dpy, "WM_DELETE_WINDOW", 0);
  priv->protocols_atom = XInternAtom(priv->dpy, "WM_PROTOCOLS", 0);
  
  /* Default screen */
  
  screen = DefaultScreen(priv->dpy);
  
  /* Check for XShm */

  priv->have_shm = check_shm(priv->dpy, &(priv->shm_completion_type));

  /* Create colormap */
  
  priv->colormap = XCreateColormap(priv->dpy, RootWindow(priv->dpy, screen),
                                   DefaultVisual(priv->dpy, screen),
                                   AllocNone);
  
  /* Create windows */
  
  priv->screen_width  = DisplayWidth(priv->dpy, screen);
  priv->screen_height = DisplayHeight(priv->dpy, screen);
  
  priv->normal_window     = create_window(priv, 320, 240);
  priv->fullscreen_window = create_window(priv, 0, 0);
  
  /* Create GC */
  
  priv->gc = XCreateGC(priv->dpy, priv->normal_window, 0, NULL);
  
  /* Check for XV Support */
  
  check_xv(priv->dpy, priv->normal_window,
           &(priv->xv_port),
           &(priv->have_xv_yv12),
           &(priv->have_xv_yuy2),
           &(priv->have_xv_i420));

  priv->current_window = priv->normal_window;

  //  fprintf(stderr, "XMapWindow: %p\n", priv);
  
  return priv;
  }

static void set_drawing_coords(x11_t * priv)
  {
  float aspect_window;
  float aspect_video;
  if(priv->do_xv)
    {
    /* Check for apsect ratio */
    aspect_window = (float)(priv->window_width)/(float)(priv->window_height);
    
    aspect_video  =
      (float)(priv->format.image_width  * priv->format.pixel_width)/
      (float)(priv->format.image_height * priv->format.pixel_height);

    if(aspect_window > aspect_video) /* Bars left and right */
      {
      priv->dst_w = priv->window_height * aspect_video;
      priv->dst_h = priv->window_height;
      priv->dst_x = (priv->window_width - priv->dst_w)/2;
      priv->dst_y = 0;
      }
    else                             /* Bars top and bottom */
      {
      priv->dst_w = priv->window_width;
      priv->dst_h = priv->window_width / aspect_video;
      priv->dst_x = 0;
      priv->dst_y = (priv->window_height - priv->dst_h)/2;
      }
    }
  else
    {
    priv->dst_x = (priv->window_width - priv->format.image_width) / 2;
    priv->dst_y = (priv->window_height - priv->format.image_height) / 2;
    priv->dst_w = priv->format.image_width;
    priv->dst_h = priv->format.image_height;
    
    }
  }

static int open_x11(void * data,
                    gavl_video_format_t * format,
                    const char * window_title)
  {
  int still_running;
  x11_t * priv;
  gavl_colorspace_t x11_colorspace;
  int window_width;
  int window_height;
  
  priv = (x11_t*)data;

  /* Stop still thread if necessary */
  
  pthread_mutex_lock(&(priv->still_mutex));
  still_running = priv->do_still;
  if(priv->do_still)
    priv->do_still = 0;
  pthread_mutex_unlock(&(priv->still_mutex));

  if(still_running)
    pthread_join(priv->still_thread, NULL);
    
  XmbSetWMProperties(priv->dpy, priv->normal_window, window_title,
                     window_title, NULL, 0, NULL, NULL, NULL);
    
  /* Decide colorspace */

  x11_colorspace = get_x11_colorspace(priv->dpy);

#ifdef DEBUG
  fprintf(stderr, "Display colorspace %s\n",
          gavl_colorspace_to_string(x11_colorspace));
#endif
  
  priv->do_xv = 0;
  
  switch(format->colorspace)
    {
    case GAVL_YUV_420_P:
      if(priv->have_xv_yv12)
        {
        priv->do_xv = 1;
        priv->xv_format = XV_ID_YV12;
        }
      else if(priv->have_xv_i420)
        {
        priv->do_xv = 1;
        priv->xv_format = XV_ID_I420;
        }
      /* yuv420 -> yuy2 is cheaper than yuv->rgb */
      else if(priv->have_xv_yuy2)
        {
        priv->do_xv = 1;
        priv->xv_format = XV_ID_YUY2;
        format->colorspace = GAVL_YUY2;
        }
      else
        format->colorspace = x11_colorspace;
      break;
    case GAVL_YUY2:
      if(priv->have_xv_yuy2)
        {
        priv->do_xv = 1;
        priv->xv_format = XV_ID_YUY2;
        }
      else if(priv->have_xv_yv12)
        {
        priv->do_xv = 1;
        priv->xv_format = XV_ID_YV12;
        format->colorspace = GAVL_YUV_420_P;
        }
      else if(priv->have_xv_i420)
        {
        priv->do_xv = 1;
        priv->xv_format = XV_ID_I420;
        format->colorspace = GAVL_YUV_420_P;
        }
      else
        format->colorspace = x11_colorspace;
      break;
    default:
      if(gavl_colorspace_is_yuv(format->colorspace) ||
         priv->force_xv)
        {
        if(priv->have_xv_yuy2)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_YUY2;
          format->colorspace = GAVL_YUY2;
          }
        else if(priv->have_xv_yv12)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_YV12;
          format->colorspace = GAVL_YUV_420_P;
          }
        else if(priv->have_xv_i420)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_I420;
          format->colorspace = GAVL_YUV_420_P;
          }
        else
          format->colorspace = x11_colorspace;
        }
      else
        format->colorspace = x11_colorspace;
    }

  gavl_video_format_copy(&(priv->format), format);

#ifdef DEBUG
  if(priv->do_xv)
    fprintf(stderr, "Using XVideo output\n");
  else
    fprintf(stderr, "Using XImage output\n");
#endif

  if(priv->auto_resize)
    {
    if(priv->do_xv)
      {
      if(priv->format.pixel_width > priv->format.pixel_height)
        {
        window_width =
          (priv->format.image_width * priv->format.pixel_width) /
          (priv->format.pixel_height);
        window_height = priv->format.image_height;
        }
      else if(priv->format.pixel_height > priv->format.pixel_width)
        {
        window_height =
          (priv->format.image_height * priv->format.pixel_height) /
          priv->format.pixel_width;
        window_width = priv->format.image_width;
        }
      else
        {
        window_width = priv->format.image_width;
        window_height = priv->format.image_height;
        }
      }
    else
      {
      window_width =  priv->format.image_width;
      window_height = priv->format.image_height;
      }
    XResizeWindow(priv->dpy, priv->normal_window, window_width,
                  window_height);
    }
  else
    set_drawing_coords(priv);

  XMapWindow(priv->dpy, priv->current_window);
  
  XClearArea(priv->dpy, priv->current_window, 0, 0,
             priv->window_width, priv->window_height, True);
  
  return 1;
  }

static void close_x11(void * data)
  {
  //  x11_t * priv = (x11_t*)data;
  }

static void destroy_x11(void * data)
  {
  x11_t * priv = (x11_t*)data;

  XDestroyWindow(priv->dpy, priv->normal_window);
  XDestroyWindow(priv->dpy, priv->fullscreen_window);
  XFreeGC(priv->dpy, priv->gc);

  if(priv->fullscreen_cursor)
    XFreeCursor(priv->dpy, priv->fullscreen_cursor);

  if(priv->fullscreen_cursor_pixmap)
    XFreePixmap(priv->dpy, priv->fullscreen_cursor_pixmap);

  XFreeColormap(priv->dpy, priv->colormap);
    
  XCloseDisplay(priv->dpy);

  free(priv);
  }

static void get_window_size(Display * dpy, Window win,
                            unsigned int * width, unsigned int * height)
  {
  Window root;
  int x, y;
  unsigned int border_width, depth;
  
  XGetGeometry(dpy, win, &root, &x, &y,
               width, height, &border_width, &depth);
  }

static int handle_event(x11_t * priv, XEvent * evt)
  {
  KeySym keysym;
  char key_char;
  int  have_key_char;
  int  done;
  int  x_image;
  int  y_image;
  int  button_number = 0;
    
  if(evt->type == priv->shm_completion_type)
    {
    return 1;
    }
  switch(evt->type)
    {
    case ClientMessage:
      if((evt->xclient.message_type == priv->protocols_atom) &&
         (evt->xclient.data.l[0] == priv->delete_atom))
        {
        //        fprintf(stderr, "Delete window\n");

        pthread_mutex_lock(&(priv->still_mutex));
        if(priv->do_still)
          XUnmapWindow(priv->dpy, priv->normal_window);
        pthread_mutex_unlock(&(priv->still_mutex));
        
        }
      break;
    case ButtonPress:
      
      if(priv->callbacks &&
         priv->callbacks->button_callback)
        {
        switch(evt->xbutton.button)
          {
          case Button1:
            button_number = 1;
            break;
          case Button2:
            button_number = 2;
            break;
          case Button3:
            button_number = 3;
            break;
          case Button4:
            button_number = 4;
            break;
          case Button5:
            button_number = 5;
            break;
          }

        /* Transform coordinates */
        
        x_image =
          ((evt->xbutton.x-priv->dst_x)*priv->format.image_width)/(priv->dst_w);
        y_image =
          ((evt->xbutton.y-priv->dst_y)*priv->format.image_height)/(priv->dst_h);
        
        if((x_image >= 0) && (x_image < priv->format.image_width) &&
           (y_image >= 0) && (y_image < priv->format.image_height))
          priv->callbacks->button_callback(priv->callbacks->data,
                                           x_image, y_image, button_number);
        }


      break;
    case KeyPress:
      have_key_char =
        XLookupString(&(evt->xkey), &key_char, 1, &keysym, NULL);
      
      done = 1;
      
      /* Handle some keys here */

      switch(keysym)
        {
        case XK_Tab:
          /* Non Fullscreen -> Fullscreen */
          if(priv->current_window == priv->normal_window)
            {
            priv->current_window = priv->fullscreen_window;
            priv->window_width = priv->screen_width;
            priv->window_height = priv->screen_height;
            set_drawing_coords(priv);
            XMapWindow(priv->dpy, priv->fullscreen_window);
            XMoveWindow(priv->dpy,
                        priv->fullscreen_window, 0, 0);
            }
          /* Fullscreen -> Non Fullscreen */
          else
            {
            XUnmapWindow(priv->dpy, priv->fullscreen_window);
            get_window_size(priv->dpy,
                            priv->normal_window,
                            &(priv->window_width),
                            &(priv->window_height));
            set_drawing_coords(priv);
            XRaiseWindow(priv->dpy, priv->normal_window);
            priv->current_window = priv->normal_window;
            }
          break;
        default:
          done = 0;
        }

      if(done)
        return 0;
            
      /* Pass the rest to the player */

      if(have_key_char &&
         priv->callbacks &&
         priv->callbacks->key_callback)
        {
        priv->callbacks->key_callback(priv->callbacks->data,
                                      (int)(key_char));
        }
      break;
    case ConfigureNotify:
      if(priv->current_window != priv->normal_window)
        return 0;

      if((priv->window_width != evt->xconfigure.width) ||
         (priv->window_height != evt->xconfigure.height))
        {
        priv->window_width  = evt->xconfigure.width;
        priv->window_height = evt->xconfigure.height;
        }
      set_drawing_coords(priv);
      //      XClearArea(priv->dpy, evt->xconfigure.window, 0, 0,
      //                 priv->window_width, priv->window_height, True);
      break;
    }
  return 0;
  }

static void handle_events(x11_t * priv)
  {
  XEvent event;
  
  if(priv->have_shm)
    {
    while(1)
      {
      XNextEvent(priv->dpy, &event);
      if(handle_event(priv, &event))
        break;
      }
    }
  else
    {
    while(XPending(priv->dpy))
      {
      XNextEvent(priv->dpy, &event);
      handle_event(priv, &event);
      }
    }
  }

static void write_frame_x11(void * data, gavl_video_frame_t * frame)
  {
  x11_t * priv = (x11_t*)data;
  x11_frame_t * x11_frame = (x11_frame_t*)(frame->user_data);

  
  if(priv->do_xv)
    {
    if(priv->have_shm)
      {
      //      fprintf(stderr, "Write frame x11 %d %d\n", priv->dst_w, priv->dst_h);
      XvShmPutImage(priv->dpy,
                    priv->xv_port,
                    priv->current_window,
                    priv->gc,
                    x11_frame->xv_image,
                    0,                   /* src_x  */
                    0,                   /* src_y  */
                    priv->format.image_width,   /* src_w  */
                    priv->format.image_height,  /* src_h  */
                    priv->dst_x,         /* dest_x */
                    priv->dst_y,         /* dest_y */
                    priv->dst_w,         /* dest_w */
                    priv->dst_h,         /* dest_h */
                    True);
      }
    else
      {
      XvPutImage(priv->dpy,
                 priv->xv_port,
                 priv->current_window,
                 priv->gc,
                 x11_frame->xv_image,
                 0,                    /* src_x   */
                 0,                    /* src_y   */
                 priv->format.image_width,    /* src_w   */
                 priv->format.image_height,   /* src_h   */
                 priv->dst_x,          /* dest_x  */
                 priv->dst_y,          /* dest_y  */
                 priv->dst_w,          /* dest_w  */
                 priv->dst_h);         /* dest_h  */
      }
    }
  else
    {
    if(priv->have_shm)
      {
      
      XShmPutImage(priv->dpy,            /* dpy        */
                   priv->current_window, /* d          */
                   priv->gc,             /* gc         */
                   x11_frame->x11_image, /* image      */
                   0,                    /* src_x      */
                   0,                    /* src_y      */
                   priv->dst_x,          /* dst_x      */
                   priv->dst_y,          /* dst_y      */
                   priv->format.image_width,    /* src_width  */
                   priv->format.image_height,   /* src_height */
                   True                  /* send_event */);
      }
    else
      {
      XPutImage(priv->dpy,            /* display */
                priv->current_window, /* d       */
                priv->gc,             /* gc      */
                x11_frame->x11_image, /* image   */
                0,                    /* src_x   */
                0,                    /* src_y   */
                priv->dst_x,          /* dest_x  */
                priv->dst_y,          /* dest_y  */
                priv->format.image_width,          /* width   */
                priv->format.image_height          /* height  */);
      }
    }

  handle_events(priv);
  }

static void * thread_func(void * data)
  {
  gavl_time_t time;
  XEvent event;

  x11_t * priv = (x11_t*)data;
  time = gavl_seconds_to_time(0.05);

  while(1)
    {
    pthread_mutex_lock(&(priv->still_mutex));
    if(!priv->do_still)
      {
      pthread_mutex_unlock(&(priv->still_mutex));
      break;
      }
    pthread_mutex_unlock(&(priv->still_mutex));
        
    while(XPending(priv->dpy))
      {
      XNextEvent(priv->dpy, &event);
      if(event.type == Expose)
        {
        write_frame_x11(data, priv->still_frame);
        }
      else
        handle_event(priv, &event);
      }
    gavl_time_delay(&time);
    }
  
  free_frame_x11(data, priv->still_frame);
  return NULL;
  }

static void put_still_x11(void * data, gavl_video_format_t * format,
                          gavl_video_frame_t * frame)
  {
  gavl_video_converter_t * cnv;
  gavl_video_format_t tmp_format;
  gavl_video_options_t opt;
  
  x11_t * priv = (x11_t*)data;
  /* Initialize as if we displayed video */

  gavl_video_format_copy(&tmp_format, format);
  open_x11(data, &tmp_format, "Video output");

  /* Create the output frame for the format */

  priv->still_frame = alloc_frame_x11(data);
    
  /* Now, we have the proper format, let's invoke the converter */

  cnv = gavl_video_converter_create();
  gavl_video_default_options(&opt);
  
  gavl_video_init(cnv, &opt, format, &tmp_format);
  gavl_video_convert(cnv, frame, priv->still_frame);

  gavl_video_converter_destroy(cnv);

  priv->do_still = 1;
  pthread_create(&(priv->still_thread),
                 (pthread_attr_t*)0,
                 thread_func, priv);
  
  }

static int get_num_xv_parameters(Display * dpy, XvPortID xv_port)
  {
  XvAttribute *attr;
  int         nattr;
  int i, j;
  int ret = 0;
  attr = XvQueryPortAttributes(dpy, xv_port, &nattr);
  //  fprintf(stderr, "nattr: %d xv_port: %d\n", nattr, xv_port);

  for(i = 0; i < nattr; i++)
    {
    if((attr[i].flags & XvSettable) && (attr[i].flags & XvGettable))
      {
      for(j = 0; j < NUM_XV_PARAMETERS; j++)
        {
        if(!strcmp(attr[i].name, xv_parameters[j].xv_name))
          ret++;
        }
      }
    }
  XFree(attr);
  if(ret) /* One for the section */
    ret++;
  return ret;
  }

static void get_xv_parameters(x11_t * x11,
                              bg_parameter_info_t * info)
  {
  XvAttribute *attr;
  int         nattr;
  int i, j;
  int index = 0;

  /* Make an own section */

  info[index].name      = bg_strdup((char*)0, "xv_parameter");
  info[index].long_name = bg_strdup((char*)0, "XVideo Settings");
  info[index].type      = BG_PARAMETER_SECTION;

  index++;
    
  attr = XvQueryPortAttributes(x11->dpy, x11->xv_port, &nattr);

  for(i = 0; i < nattr; i++)
    {
    if((attr[i].flags & XvSettable) && (attr[i].flags & XvGettable))
      {
      for(j = 0; j < NUM_XV_PARAMETERS; j++)
        {
        if(!strcmp(attr[i].name, xv_parameters[j].xv_name))
          {
          /* Create atom */
          x11->xv_attr_atoms[j] = XInternAtom(x11->dpy, xv_parameters[j].xv_name,
                                              False);
          
          info[index].name      = bg_strdup((char*)0, xv_parameters[j].xv_name);
          info[index].long_name = bg_strdup((char*)0, xv_parameters[j].long_name);
          info[index].type      = BG_PARAMETER_SLIDER_INT;
          info[index].val_min.val_i = attr[i].min_value;
          info[index].val_max.val_i = attr[i].max_value;

          /* Get the default value from the X Server */

          XvGetPortAttribute(x11->dpy, x11->xv_port,
                             x11->xv_attr_atoms[j],
                             &(info[index].val_default.val_i));
          index++;
          }
        }
      }
    }
  XFree(attr);
  }

bg_parameter_info_t common_parameters[] =
  {
    {
      name:        "window",
      long_name:   "Window",
    },
    {
      name:        "auto_resize",
      long_name:   "Auto Resize",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "remember_size",
      long_name:   "Remember Size",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "force_xv",
      long_name:   "Force XVideo",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
  };

#define NUM_COMMON_PARAMETERS sizeof(common_parameters)/sizeof(common_parameters[0])

bg_parameter_info_t *
get_parameters_x11(void * priv)
  {
  int i;
  int index;
  int num_xv_parameters;
  int num_parameters;
    
  x11_t * x11 = (x11_t*)priv;
  if(!x11->parameters)
    {
    /* Count parameters */
    num_parameters = NUM_COMMON_PARAMETERS;
    
    num_xv_parameters = get_num_xv_parameters(x11->dpy, x11->xv_port);

    num_parameters += num_xv_parameters;

    /* Create parameter array */

    x11->parameters = calloc(num_parameters+1, sizeof(bg_parameter_info_t));

    index = 0;
    for(i = 0; i < NUM_COMMON_PARAMETERS; i++)
      {
      bg_parameter_info_copy(&(x11->parameters[index]),
                             &(common_parameters[i]));
      index++;
      }
    //    fprintf(stderr, "num_xv_parameters: %d\n", num_xv_parameters);
    if(num_xv_parameters)
      {
      get_xv_parameters(x11, &(x11->parameters[index]));
      index += num_xv_parameters;
      }
    }
    
  return x11->parameters;
  }

/* Set parameter */

void
set_parameter_x11(void * priv, char * name, bg_parameter_value_t * val)
  {
  x11_t * p = (x11_t*)priv;
  
  if(!name)
    return;
  if(!strcmp(name, "auto_resize"))
    {
    p->auto_resize = val->val_i;
    }
  if(!strcmp(name, "remember_size"))
    {
    p->remember_size = val->val_i;
    }
  if(!strcmp(name, "force_xv"))
    {
    p->force_xv = val->val_i;
    }
  }

bg_ov_plugin_t the_plugin =
  {
    common:
    {
      name:          "ov_x11",
      long_name:     "X11 display driver",
      type:          BG_PLUGIN_OUTPUT_VIDEO,
      flags:         BG_PLUGIN_PLAYBACK,
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      create:        create_x11,
      destroy:       destroy_x11,

      get_parameters: get_parameters_x11,
      set_parameter:  set_parameter_x11
    },

    open:          open_x11,
    put_video:     write_frame_x11,
    alloc_frame:   alloc_frame_x11,
    free_frame:    free_frame_x11,
    close:         close_x11,
    put_still:     put_still_x11,
    set_callbacks: set_callbacks_x11,
  };
