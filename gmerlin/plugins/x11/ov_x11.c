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
#include <math.h>

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
#include <config.h>


#include "x11_window.h"

#include <X11/extensions/Xvlib.h>
// #undef HAVE_LIBXV

#define XV_ID_YV12  0x32315659 
#define XV_ID_I420  0x30323449
#define XV_ID_YUY2  0x32595559
#define XV_ID_UYVY  0x59565955

#define HAVE_XV
#define HAVE_XSHM

static void
free_frame_x11(void * data, gavl_video_frame_t * frame);


/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase (Display *);

typedef struct 
  {
  XImage * x11_image;
  XShmSegmentInfo shminfo;
#ifdef HAVE_LIBXV
  XvImage * xv_image;
#endif
  } x11_frame_t;

#ifdef HAVE_LIBXV

/* The indices and array MUST match */

#define XV_PARAM_HUE        0
#define XV_PARAM_BRIGHTNESS 1
#define XV_PARAM_SATURATION 2
#define XV_PARAM_CONTRAST   3

#define XV_MODE_NEVER       0
#define XV_MODE_YUV_ONLY    1
#define XV_MODE_ALWAYS      2

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

#endif // HAVE_LIBXV


typedef struct
  {
  x11_window_t win;
  
  gavl_video_format_t video_format;

  int do_sw_scale_cfg;
  int do_sw_scale;

  gavl_scale_mode_t scale_mode;
  
  int can_scale; /* Can scale (hard or software) */

  gavl_video_scaler_t * scaler;
  
  /* Supported modes */

  int have_shm;

  /* X11 Stuff */
  
  Display * dpy;
  
#ifdef HAVE_LIBXV
  XvPortID xv_port;
  int do_xv;
  Atom xv_attr_atoms[NUM_XV_PARAMETERS];
  Atom xv_colorkey_atom;
  int xv_colorkey_settable;
  int have_xv_yv12;
  int have_xv_yuy2;
  int have_xv_i420;
  int have_xv_uyvy;

  int xv_mode;
  
  /* Actual mode */

  int xv_format;

  int have_xv_colorkey;
  int xv_colorkey;  
  int xv_colorkey_orig;

#endif

  int shm_completion_type;

  /* Fullscreen stuff */

  Atom hints_atom;
    
  /* Drawing coords */
  
  gavl_rectangle_t src_rect;
  gavl_rectangle_t dst_rect;
  
  /* Format with updated window size */

  gavl_video_format_t window_format;
  gavl_video_frame_t * window_frame;
  
  /* Configuration flags */

  int auto_resize;
  int remember_size;
  
  /* Callbacks */

  bg_ov_callbacks_t * callbacks;
  bg_parameter_info_t * parameters;

  /* Still image stuff */

  pthread_t       still_thread;
  pthread_mutex_t still_mutex;
  int do_still;
  gavl_video_frame_t * still_frame;
  
  /* Turn off xscreensaver? */

  int disable_xscreensaver_fullscreen;
  int disable_xscreensaver_normal;
  
  int window_visible;

  int squeeze_zoom_active;
  float squeeze;
  float zoom;

  } x11_t;

static void create_parameters(x11_t * x11);	

static int handle_event(x11_t * priv, XEvent * evt);
static void close_x11(void*);

static
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

#ifdef HAVE_LIBXV

static void check_xv(Display * d, Window w,
                     XvPortID * port,
                     int * have_yv12, int * have_yuy2,
                     int * have_i420, int * have_uyvy)
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
  *have_uyvy = 0;
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
          else if(formatValues[k].id == XV_ID_UYVY)
            {
            *have_uyvy = 1;
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
   //  fprintf(stderr, "Check xv: %d %d %d %d\n", (int)(*port), *have_i420, *have_yuy2, *have_yv12);
  return;
  }

#endif

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

#ifdef HAVE_LIBXV

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
                       priv->video_format.image_width,
                       priv->video_format.image_height, &(x11_frame->shminfo));
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
    video_format.image_width  = priv->video_format.image_width;
    video_format.image_height = priv->video_format.image_height;

    video_format.frame_width  = priv->video_format.frame_width;
    video_format.frame_height = priv->video_format.frame_height;

    video_format.colorspace = priv->video_format.colorspace;
    
    ret = gavl_video_frame_create(&video_format);
    x11_frame->xv_image = XvCreateImage(priv->dpy, priv->xv_port,
                                       priv->xv_format, ret->planes[0],
                                       priv->video_format.frame_width, priv->video_format.frame_height);
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
    case XV_ID_UYVY:
      ret->planes[0] =
        x11_frame->xv_image->data + x11_frame->xv_image->offsets[0];
      ret->strides[0] = x11_frame->xv_image->pitches[0];
      break;
    }

  ret->user_data = x11_frame;
  return ret;
  }

#endif // HAVE_LIBXV

static gavl_video_frame_t *
alloc_frame_ximage(x11_t * priv, gavl_video_format_t * format)
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
                                           format->frame_width,
                                           format->frame_height);
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
    video_format.image_width = format->image_width;
    video_format.image_height = format->image_height;

    video_format.frame_width = format->frame_width;
    video_format.frame_height = format->frame_height;

    video_format.colorspace = format->colorspace;

    ret = gavl_video_frame_create(&video_format);
    
    x11_frame->x11_image = XCreateImage(priv->dpy, visual, depth, ZPixmap,
                                        0, ret->planes[0],
                                        format->frame_width,
                                        format->frame_height,
                                        32,
                                        ret->strides[0]);
    }
  ret->user_data = x11_frame;
  return ret;
  }

static gavl_video_frame_t * alloc_frame_x11(void * data)
  {
  x11_t * priv = (x11_t*)(data);
#ifdef HAVE_LIBXV
  if(priv->do_xv)
    return alloc_frame_xv(priv);
  else
#endif
    if(!priv->do_sw_scale)
      return alloc_frame_ximage(priv, &(priv->video_format));
    else
      return gavl_video_frame_create(&(priv->video_format));
  }

static void
free_frame_x11(void * data, gavl_video_frame_t * frame)
  {
  x11_frame_t * x11_frame;
  x11_t * priv;

  x11_frame = (x11_frame_t*)(frame->user_data);

  if(!x11_frame)
    {
    gavl_video_frame_destroy(frame);
    return;
    }

  priv = (x11_t*)(data);

#ifdef HAVE_LIBXV
  if(x11_frame->xv_image)
    {
    XFree(x11_frame->xv_image);
    x11_frame->xv_image = (XvImage*)0;
    }
  else
#endif
    if(x11_frame->x11_image)
      {
      XFree(x11_frame->x11_image);
      x11_frame->x11_image = (XImage*)0;
      }
  
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
  int dpi_x, dpi_y;
  x11_t * priv;
  int screen;
  
  priv = calloc(1, sizeof(x11_t));

  priv->scaler = gavl_video_scaler_create();
  
  pthread_mutex_init(&(priv->still_mutex), NULL);
    
  /* Open X Display */

  priv->dpy = XOpenDisplay(NULL);

  
  /* Default screen */
  
  screen = DefaultScreen(priv->dpy);

  /* Screen resolution */

  dpi_x = ((((double) DisplayWidth(priv->dpy,screen)) * 25.4) / 
           ((double) DisplayWidthMM(priv->dpy,screen)));
  dpi_y = ((((double) DisplayHeight(priv->dpy,screen)) * 25.4) / 
	    ((double) DisplayHeightMM(priv->dpy,screen)));

  /* We must swap horizontal and vertical here because LARGER resolution means
     SMALLER pixels */
#if 0    
  priv->window_format.pixel_width = dpi_y;
  priv->window_format.pixel_height = dpi_x;
#else
  priv->window_format.pixel_width = 1;
  priv->window_format.pixel_height = 1;
#endif
  /* Check for XShm */

  priv->have_shm = check_shm(priv->dpy, &(priv->shm_completion_type));
  
  /* Create windows */
  
  x11_window_create(&(priv->win),
                    priv->dpy, DefaultVisual(priv->dpy, screen),
                    DefaultDepth(priv->dpy, screen),
                    320, 240, "Video output");
  
  x11_window_select_input(&(priv->win), ButtonPressMask |
                          KeyPressMask | ExposureMask);

  

  
  /* Check for XV Support */
#ifdef HAVE_LIBXV
  check_xv(priv->dpy, priv->win.normal_window,
           &(priv->xv_port),
           &(priv->have_xv_yv12),
           &(priv->have_xv_yuy2),
           &(priv->have_xv_i420),
           &(priv->have_xv_uyvy));

#endif // HAVE_LIBXV
  create_parameters(priv);
  return priv;
  }

#define PADD_IMAGE_SIZE(s) s = ((s + 15) / 16) * 16

static void set_drawing_coords(x11_t * priv)
  {
  float zoom_factor;
  float squeeze_factor;
  gavl_video_options_t * opt;
  //  

  zoom_factor = (priv->squeeze_zoom_active) ? priv->zoom * 0.01 : 1.0;
  squeeze_factor = (priv->squeeze_zoom_active) ? priv->squeeze : 0.0;

  //  fprintf(stderr, "Zoom: %f\n", zoom_factor);
  
  priv->window_format.image_width = priv->win.window_width;
  priv->window_format.image_height = priv->win.window_height;
  
  if(priv->can_scale)
    {
    gavl_rectangle_fit_aspect(&priv->dst_rect,
                              &priv->video_format,
                              &priv->src_rect,
                              &priv->window_format,
                              zoom_factor, squeeze_factor);
#ifdef HAVE_LIBXV
    if(priv->have_xv_colorkey)
      {
      XSetForeground(priv->dpy, priv->win.gc, priv->xv_colorkey);
      XFillRectangle(priv->dpy, priv->win.current_window, priv->win.gc,
                     priv->dst_rect.x, priv->dst_rect.y, priv->dst_rect.w, priv->dst_rect.h);
      }
#endif
    }
  else
    {
    priv->dst_rect.x = (priv->win.window_width - priv->video_format.image_width) / 2;
    priv->dst_rect.y = (priv->win.window_height - priv->video_format.image_height) / 2;
    priv->dst_rect.w = priv->video_format.image_width;
    priv->dst_rect.h = priv->video_format.image_height;
    }

  if(!priv->do_sw_scale)
    return;

  /* Allocate larger image if nesessary */
  gavl_rectangle_set_all(&priv->src_rect,
                         &priv->video_format);
  
  if((priv->window_format.image_width > priv->window_format.frame_width) ||
     (priv->window_format.image_height > priv->window_format.frame_height))
    {
    priv->window_format.frame_width = priv->window_format.image_width;
    priv->window_format.frame_height = priv->window_format.image_height;
    
    PADD_IMAGE_SIZE(priv->window_format.frame_width);
    PADD_IMAGE_SIZE(priv->window_format.frame_height);

    if(priv->window_frame)
      {
      free_frame_x11(priv, priv->window_frame);
      }
    priv->window_frame = alloc_frame_ximage(priv, &(priv->window_format));
    }

  /* Reinitialize scaler */
  
#if 0
  fprintf(stderr, "src_rect 1: ");
  gavl_rectangle_dump(&(priv->src_rect));
  fprintf(stderr, "\n");

  
  fprintf(stderr, "dst_rect 1: ");
  gavl_rectangle_dump(&(priv->dst_rect));
  fprintf(stderr, "\n");
  
#endif

  opt = gavl_video_scaler_get_options(priv->scaler);
  gavl_video_options_set_scale_mode(opt, priv->scale_mode);
  gavl_video_options_set_quality(opt, 2);
  
  gavl_video_scaler_init(priv->scaler,
                         priv->video_format.colorspace,
                         &(priv->src_rect),
                         &(priv->dst_rect),
                         &(priv->video_format),
                         &(priv->window_format)); 

  
#if 0
  fprintf(stderr, "src_rect 2: ");
  gavl_rectangle_dump(&(priv->src_rect));
  fprintf(stderr, "\n");

  
  fprintf(stderr, "dst_rect 2: ");
  gavl_rectangle_dump(&(priv->dst_rect));
  fprintf(stderr, "\n");
  
#endif
  }

static int _open_x11(void * data,
                     gavl_video_format_t * format,
                     const char * window_title, int still)
  {
  int still_running;
  x11_t * priv;
  gavl_colorspace_t x11_colorspace;
  priv = (x11_t*)data;

  /* Stop still thread if necessary */
  
  pthread_mutex_lock(&(priv->still_mutex));
  still_running = priv->do_still;
  if(priv->do_still)
    priv->do_still = 0;
  pthread_mutex_unlock(&(priv->still_mutex));

  if(still_running)
    {
    pthread_join(priv->still_thread, NULL);
    close_x11(priv);
    }

  /* Set screensaver options, but not in still image mode */

  if(still)
    {
    priv->win.disable_xscreensaver_fullscreen = 0;
    priv->win.disable_xscreensaver_normal     = 0;
    }
  else
    {
    priv->win.disable_xscreensaver_fullscreen = priv->disable_xscreensaver_fullscreen;
    priv->win.disable_xscreensaver_normal     = priv->disable_xscreensaver_normal;
    }
  
  x11_window_set_title(&(priv->win), window_title);
  
  /* Decide colorspace */

  x11_colorspace = get_x11_colorspace(priv->dpy);

#ifdef DEBUG
  fprintf(stderr, "Display colorspace %s\n",
          gavl_colorspace_to_string(x11_colorspace));
#endif
  
#ifdef HAVE_LIBXV
  priv->do_xv = 0;

  if(priv->xv_mode != XV_MODE_NEVER)
    {
    
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
        else if(priv->have_xv_uyvy)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_UYVY;
          format->colorspace = GAVL_UYVY;
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
        if(priv->have_xv_uyvy)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_UYVY;
          format->colorspace = GAVL_UYVY;
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
           priv->xv_mode == XV_MODE_ALWAYS)
          {
          if(priv->have_xv_uyvy)
            {
            priv->do_xv = 1;
            priv->xv_format = XV_ID_UYVY;
            format->colorspace = GAVL_UYVY;
            }
          else if(priv->have_xv_yuy2)
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

    /* Try to grab port, switch to XImage if unsuccessful */

    if(priv->do_xv)
      {
      if(Success != XvGrabPort(priv->dpy, priv->xv_port, CurrentTime))
        {
        priv->do_xv = 0;
        format->colorspace = x11_colorspace;
        }
      else if(priv->have_xv_colorkey)
        {
        XvGetPortAttribute(priv->dpy, priv->xv_port, priv->xv_colorkey_atom, 
                           &(priv->xv_colorkey_orig));
        if(priv->xv_colorkey_settable)
          {
          priv->xv_colorkey = 0x00010100;
          XvSetPortAttribute(priv->dpy, priv->xv_port, priv->xv_colorkey_atom, 
                             priv->xv_colorkey);
          }
        else
          priv->xv_colorkey = priv->xv_colorkey_orig;
        }
      }

    }
  if(priv->do_xv)
    {
    priv->do_sw_scale = 0;
    priv->can_scale = 1;
    }
  else
    {
    priv->do_sw_scale = priv->do_sw_scale_cfg;
    priv->can_scale = priv->do_sw_scale;
    format->colorspace = x11_colorspace;
    }
  
#else
  format->colorspace = x11_colorspace;
  priv->do_sw_scale = priv->do_sw_scale_cfg;
  priv->can_scale = priv->do_sw_scale;
  
#endif // !HAVE_LIBXV
  priv->window_format.colorspace = format->colorspace;
  gavl_video_format_copy(&(priv->video_format), format);

  gavl_rectangle_set_all(&priv->src_rect, &priv->video_format);
    
  if(priv->auto_resize)
    {
    if(priv->can_scale)
      {
      gavl_video_format_fit_to_source(&(priv->window_format),
                                      &(priv->video_format));
      }
    else
      {
      priv->window_format.image_width =  priv->video_format.image_width;
      priv->window_format.image_height = priv->video_format.image_height;
      }
    x11_window_resize(&(priv->win), priv->window_format.image_width,
                      priv->window_format.image_height);
    set_drawing_coords(priv);
    }
  else
    {
    x11_window_clear(&priv->win);
    set_drawing_coords(priv);
    }
  
  return 1;
  }


static int open_x11(void * data,
                    gavl_video_format_t * format,
                    const char * window_title)
  {
  return _open_x11(data, format, window_title, 0);
  }

static void close_x11(void * data)
  {
#ifdef HAVE_LIBXV
  x11_t * priv = (x11_t*)data;
  if(priv->do_xv)
    {
    XvUngrabPort(priv->dpy, priv->xv_port, CurrentTime);
    XvStopVideo(priv->dpy, priv->xv_port, priv->win.current_window);
    if(priv->xv_colorkey_settable)
      XvSetPortAttribute(priv->dpy, priv->xv_port, priv->xv_colorkey_atom, 
                         priv->xv_colorkey_orig);
    
    }
#endif
  x11_window_clear(&priv->win);
  }

static void destroy_x11(void * data)
  {
  int still_running;
  x11_t * priv = (x11_t*)data;

  /* Stop still thread if necessary */
  
  pthread_mutex_lock(&(priv->still_mutex));
  still_running = priv->do_still;
  if(priv->do_still)
    priv->do_still = 0;
  pthread_mutex_unlock(&(priv->still_mutex));

  if(still_running)
    {
    //    fprintf(stderr, "Stopping still thread...");
    pthread_join(priv->still_thread, NULL);
    close_x11(priv);
    //    fprintf(stderr, "done\n");
    }

  
  if(priv->parameters)
    {
    bg_parameter_info_destroy_array(priv->parameters);
    }
  

  x11_window_destroy(&(priv->win));
    
  XCloseDisplay(priv->dpy);

  gavl_video_scaler_destroy(priv->scaler);
  
  free(priv);
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
  
  x11_window_handle_event(&(priv->win), evt);

  //  fprintf(stderr, "Idle: %d, hidden: %d, evt: %p\n", priv->win.idle_counter, 
  //          priv->win.pointer_hidden, evt);
  if(priv->win.do_delete)
    {
    pthread_mutex_lock(&(priv->still_mutex));
    if(priv->do_still)
      {
      x11_window_show(&(priv->win), 0);
      priv->do_still = 0; /* Exit still thread *when window is closed */
      }
    pthread_mutex_unlock(&(priv->still_mutex));
    }

  if(!evt)
    return 0;
  
  if(evt->type == priv->shm_completion_type)
    {
    return 1;
    }
  switch(evt->type)
    {
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
          ((evt->xbutton.x-priv->dst_rect.x)*priv->video_format.image_width)/(priv->dst_rect.w);
        y_image =
          ((evt->xbutton.y-priv->dst_rect.y)*priv->video_format.image_height)/(priv->dst_rect.h);
        
        if((x_image >= 0) && (x_image < priv->video_format.image_width) &&
           (y_image >= 0) && (y_image < priv->video_format.image_height))
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
          
          if(priv->win.current_window == priv->win.normal_window)
            x11_window_set_fullscreen(&(priv->win), 1);
          /* Fullscreen -> Non Fullscreen */
          else
            x11_window_set_fullscreen(&(priv->win), 0);
          set_drawing_coords(priv);
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
      //      if((priv->window_format.image_width != priv->win.window_width) ||
      //         (priv->window_format.image_height != priv->win.window_height))
        {
        x11_window_clear(&(priv->win));
        set_drawing_coords(priv);
        }
      break;
    }
  return 0;
  }

static void handle_events(x11_t * priv)
  {
  XEvent * event;
  
  if(priv->have_shm)
    {
    while(1)
      {
      event = x11_window_next_event(&(priv->win), -1);
      if(handle_event(priv, event))
        break;
      }
    }
  else
    {
    while(1)
      {
      event = x11_window_next_event(&(priv->win), -1);
      handle_event(priv, event);
      if(!event)
        break;
      }
    }
  }

static void write_frame_x11(void * data, gavl_video_frame_t * frame)
  {
  XImage * image;
  x11_t * priv = (x11_t*)data;
  x11_frame_t * x11_frame = (x11_frame_t*)(frame->user_data);

#ifdef HAVE_LIBXV
  
  if(priv->do_xv)
    {
    if(priv->have_shm)
      {
      XvShmPutImage(priv->dpy,
                    priv->xv_port,
                    priv->win.current_window,
                    priv->win.gc,
                    x11_frame->xv_image,
                    priv->src_rect.x,  /* src_x  */
                    priv->src_rect.y,  /* src_y  */
                    priv->src_rect.w,  /* src_w  */
                    priv->src_rect.h,  /* src_h  */
                    priv->dst_rect.x,  /* dest_x */
                    priv->dst_rect.y,  /* dest_y */
                    priv->dst_rect.w,  /* dest_w */
                    priv->dst_rect.h,  /* dest_h */
                    True);
      }
    else
      {
      XvPutImage(priv->dpy,
                 priv->xv_port,
                 priv->win.current_window,
                 priv->win.gc,
                 x11_frame->xv_image,
                 priv->src_rect.x,  /* src_x  */
                 priv->src_rect.y,  /* src_y  */
                 priv->src_rect.w,  /* src_w  */
                 priv->src_rect.h,  /* src_h  */
                 priv->dst_rect.x,          /* dest_x  */
                 priv->dst_rect.y,          /* dest_y  */
                 priv->dst_rect.w,          /* dest_w  */
                 priv->dst_rect.h);         /* dest_h  */
      }
    }
  else
    {
#endif // HAVE_LIBXV

    if(priv->do_sw_scale)
      {
      gavl_video_scaler_scale(priv->scaler, frame, priv->window_frame);
      x11_frame = (x11_frame_t*)(priv->window_frame->user_data);
      image = x11_frame->x11_image;
      }
    else
      {
      x11_frame = (x11_frame_t*)(frame->user_data);
      image = x11_frame->x11_image;
      }
#if 0
    fprintf(stderr, "dst_rect: ");
    gavl_rectangle_dump(&(priv->dst_rect));
    fprintf(stderr, "\n");
#endif
    if(priv->have_shm)
      {
      
      XShmPutImage(priv->dpy,            /* dpy        */
                   priv->win.current_window, /* d          */
                   priv->win.gc,             /* gc         */
                   image, /* image      */
                   priv->dst_rect.x,    /* src_x      */
                   priv->dst_rect.y,    /* src_y      */
                   priv->dst_rect.x,          /* dst_x      */
                   priv->dst_rect.y,          /* dst_y      */
                   priv->dst_rect.w,    /* src_width  */
                   priv->dst_rect.h,   /* src_height */
                   True                  /* send_event */);
      }
    else
      {
      XPutImage(priv->dpy,            /* display */
                priv->win.current_window, /* d       */
                priv->win.gc,             /* gc      */
                image, /* image   */
                priv->dst_rect.x,                    /* src_x   */
                priv->dst_rect.y,                    /* src_y   */
                priv->dst_rect.x,          /* dest_x  */
                priv->dst_rect.y,          /* dest_y  */
                priv->dst_rect.w,    /* src_width  */
                priv->dst_rect.h   /* src_height */
                );
      }
#ifdef HAVE_LIBXV
    }
#endif // HAVE_LIBXV

  XSync(priv->dpy, False);
  handle_events(priv);
  }

static void * thread_func(void * data)
  {
  Window win;
  XEvent * event;

  x11_t * priv = (x11_t*)data;

  while(1)
    {
    pthread_mutex_lock(&(priv->still_mutex));
    if(!priv->do_still)
      {
      pthread_mutex_unlock(&(priv->still_mutex));
      break;
      }
    pthread_mutex_unlock(&(priv->still_mutex));

    win = priv->win.current_window;
    event = x11_window_next_event(&(priv->win), 50);
    handle_event(priv, event);
    if(win != priv->win.current_window)
      write_frame_x11(data, priv->still_frame);

    if(event && (event->type == Expose))
      write_frame_x11(data, priv->still_frame);
    }
  
  free_frame_x11(data, priv->still_frame);
  return NULL;
  }

static void put_still_x11(void * data, gavl_video_format_t * format,
                          gavl_video_frame_t * frame)
  {
  gavl_video_converter_t * cnv;
  gavl_video_format_t tmp_format;
  
  x11_t * priv = (x11_t*)data;

  /* If window isn't visible, return */

  if(!priv->window_visible)
    return;
  
  /* Initialize as if we displayed video */
  
  gavl_video_format_copy(&tmp_format, format);
  _open_x11(data, &tmp_format, "Video output", 1);
  
  /* Create the output frame for the format */

  priv->still_frame = alloc_frame_x11(data);
    
  /* Now, we have the proper format, let's invoke the converter */

  cnv = gavl_video_converter_create();
  
  gavl_video_converter_init(cnv, format, &tmp_format);
  gavl_video_convert(cnv, frame, priv->still_frame);

  gavl_video_converter_destroy(cnv);
  
  write_frame_x11(data, priv->still_frame);
  
  priv->do_still = 1;
  pthread_create(&(priv->still_thread),
                 (pthread_attr_t*)0,
                 thread_func, priv);
  
  }

static void show_window_x11(void * data, int show)
  {
  x11_t * priv = (x11_t*)data;
  XEvent * evt;
  
  x11_window_show(&(priv->win), show);

  priv->window_visible = show;
  
  /* Clear the area and wait for the first ExposeEvent to come */
  x11_window_clear(&(priv->win));
//  XClearArea(priv->win.dpy, priv->win.current_window, 0, 0,
//             priv->win.window_width, priv->win.window_height, True);
  XSync(priv->dpy, False);
  
  /* Get the expose event before we draw the first frame */
  
  while((evt = x11_window_next_event(&(priv->win), 0)))
    {
    handle_event(priv, evt);
    }
  
  //  fprintf(stderr, "Show window done\n");
  }

#ifdef HAVE_LIBXV

static int get_num_xv_parameters(Display * dpy, XvPortID xv_port)
  {
  XvAttribute *attr;
  int         nattr;
  int i, j;
  int ret = 0;
  attr = XvQueryPortAttributes(dpy, xv_port, &nattr);
  //  fprintf(stderr, "nattr: %d xv_port: %d\n", nattr, (int)xv_port);

  for(i = 0; i < nattr; i++)
    {
    //    fprintf(stderr, "attr[%d].name: %s\n", i, attr[i].name);
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
    if((attr[i].flags & XvGettable) && !strcmp(attr[i].name, "XV_COLORKEY"))
      {
      x11->have_xv_colorkey = 1;
      x11->xv_colorkey_atom = XInternAtom(x11->dpy, attr[i].name,
                                           False);
      if(attr[i].flags & XvSettable)
        x11->xv_colorkey_settable = 1;
      }
    else if((attr[i].flags & XvSettable) && (attr[i].flags & XvGettable))
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
          info[index].flags = BG_PARAMETER_SYNC;
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
#endif // HAVE_LIBXV

bg_parameter_info_t common_parameters[] =
  {
    {
      name:        "window",
      long_name:   "General",
    },
    {
      name:        "auto_resize",
      long_name:   "Auto resize window",
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
#ifdef HAVE_LIBXV
    {
      name:        "xv_mode",
      long_name:   "Try XVideo",
      type:        BG_PARAMETER_STRINGLIST,

      multi_names: (char*[]){ "never", "yuv_only", "always", (char*)0},
      multi_labels: (char*[]){ "Never", "For YUV formats only", "Always", (char*)0},
      val_default: { val_str: "yuv_only" },
      help_string: "Choose when to try XVideo (with hardware scaling)",
    },
#endif
    {
      name:        "disable_xscreensaver_normal",
      long_name:   "Disable XScreensaver for normal playback",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 }
    },
    {
      name:        "disable_xscreensaver_fullscreen",
      long_name:   "Disable XScreensaver for fullscreen playback",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "squeeze_zoom_active",
      long_name:   "Enable Squeeze/Zoom",
      type:        BG_PARAMETER_CHECKBUTTON,
      flags:       BG_PARAMETER_SYNC,
      val_default: { val_i: 0 }
    },
    {
      name:        "squeeze",
      long_name:   "Squeeze",
      type:        BG_PARAMETER_SLIDER_FLOAT,
      flags:       BG_PARAMETER_SYNC,
      val_default: { val_f: 0.0 },
      val_min:     { val_f: -1.0 },
      val_max:     { val_f:  1.0 },
      num_digits:  3
    },
    {
      name:        "zoom",
      long_name:   "Zoom",
      type:        BG_PARAMETER_SLIDER_FLOAT,
      flags:       BG_PARAMETER_SYNC,
      val_default: { val_f: 100.0 },
      val_min:     { val_f: 20.0 },
      val_max:     { val_f: 180.0 },
    },
    {
      name:        "sw_scaler",
      long_name:   "Software scaler",
      type:        BG_PARAMETER_SECTION,
    },
    {
      name:        "do_sw_scale",
      long_name:   "Enable software scaler",
      type:        BG_PARAMETER_CHECKBUTTON,
      help_string: "This enables software scaling for the case that no hardware scaling is available"
    },
    {
      name:        "scale_mode",
      long_name:   "Scale mode",
      type:        BG_PARAMETER_STRINGLIST,
      multi_names:  (char*[]){ "nearest", "bilinear", (char*)0 },
      multi_labels: (char*[]){ "Nearest", "Bilinear", (char*)0 },
      val_default: { val_str: "nearest" },
    }
  };

#define NUM_COMMON_PARAMETERS sizeof(common_parameters)/sizeof(common_parameters[0])

static void create_parameters(x11_t * x11)
  {
  int i;
  int index;
#ifdef HAVE_LIBXV
  int num_xv_parameters;
#endif
  int num_parameters;
    

//  fprintf(stderr, "get_parameters_x11 %p\n", x11->parameters);

  if(!x11->parameters)
    {
    /* Count parameters */
    num_parameters = NUM_COMMON_PARAMETERS;
#ifdef HAVE_LIBXV
    num_xv_parameters = get_num_xv_parameters(x11->dpy, x11->xv_port);
    num_parameters += num_xv_parameters;
#endif // HAVE_LIBXV

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
#ifdef HAVE_LIBXV
    if(num_xv_parameters)
      {
      get_xv_parameters(x11, &(x11->parameters[index]));
      index += num_xv_parameters;
      }
#endif
    }


  }

static bg_parameter_info_t *
get_parameters_x11(void * priv)
  {
  x11_t * x11 = (x11_t*)priv;
    
  return x11->parameters;
  }

/* Set xv parameter */

#ifdef HAVE_LIBXV
static int set_xv_parameter(x11_t * p, const char * name, 
                             int value)
  {
  int j;
 
  for(j = 0; j < NUM_XV_PARAMETERS; j++)
    {
    if(!strcmp(name, xv_parameters[j].xv_name))
      {
//      fprintf(stderr, "Set xv parameter %s\n", name);
      XvSetPortAttribute (p->dpy, p->xv_port,
                        p->xv_attr_atoms[j], value);
      return 1;
      }
    }
  return 0;
  }
#endif

/* Set parameter */

static void
set_parameter_x11(void * priv, char * name, bg_parameter_value_t * val)
  {
  x11_t * p = (x11_t*)priv;
  
  if(!name)
    {
    return;
    }

#ifdef HAVE_LIBXV
  else if(set_xv_parameter(p, name, val->val_i))
    return;
#endif
  else if(!strcmp(name, "auto_resize"))
    {
    p->auto_resize = val->val_i;
    }
  else if(!strcmp(name, "remember_size"))
    {
    p->remember_size = val->val_i;
    }
#ifdef HAVE_LIBXV
  else if(!strcmp(name, "xv_mode"))
    {
    if(!strcmp(val->val_str, "never"))
      p->xv_mode = XV_MODE_NEVER;
    else if(!strcmp(val->val_str, "yuv_only"))
      p->xv_mode = XV_MODE_YUV_ONLY;
    else if(!strcmp(val->val_str, "always"))
      p->xv_mode = XV_MODE_ALWAYS;
    }
#endif
  else if(!strcmp(name, "window_x"))
    {
    p->win.window_x = val->val_i;
    }
  else if(!strcmp(name, "window_y"))
    {
    p->win.window_y = val->val_i;
    }
  else if(!strcmp(name, "window_width"))
    {
    p->win.window_width = val->val_i;
    }
  else if(!strcmp(name, "window_height"))
    {
    p->win.window_height = val->val_i;
    }
  else if(!strcmp(name, "disable_xscreensaver_normal"))
    {
    p->disable_xscreensaver_normal = val->val_i;
    }
  else if(!strcmp(name, "disable_xscreensaver_fullscreen"))
    {
    p->disable_xscreensaver_fullscreen = val->val_i;
    }
  else if(!strcmp(name, "squeeze_zoom_active"))
    {
    p->squeeze_zoom_active = val->val_i;
    x11_window_clear(&p->win);
    set_drawing_coords(p);
    }
  else if(!strcmp(name, "squeeze"))
    {
    p->squeeze = val->val_f;
    if(p->squeeze_zoom_active)
      {
      x11_window_clear(&p->win);
      set_drawing_coords(p);
      }
    }
  else if(!strcmp(name, "zoom"))
    {
    p->zoom = val->val_f;
    if(p->squeeze_zoom_active)
      {
      x11_window_clear(&p->win);
      set_drawing_coords(p);
      }
    }
  else if(!strcmp(name, "do_sw_scale"))
    {
    p->do_sw_scale_cfg = val->val_i;
    }
  else if(!strcmp(name, "scale_mode"))
    {
    if(!strcmp(val->val_str, "nearest"))
      p->scale_mode = GAVL_SCALE_NEAREST;
    else if(!strcmp(val->val_str, "bilinear"))
      p->scale_mode = GAVL_SCALE_BILINEAR;
    }
  }

static int
get_parameter_x11(void * priv, char * name, bg_parameter_value_t * val)
  {
  x11_t * p = (x11_t*)priv;
  
  if(!name)
    return 0;
  if(!strcmp(name, "window_x"))
    {
    val->val_i = p->win.window_x;
    return 1;
    }
  if(!strcmp(name, "window_y"))
    {
    val->val_i = p->win.window_y;
    return 1;
    }
  if(!strcmp(name, "window_width"))
    {
    val->val_i = p->win.window_width;
    return 1;
    }
  if(!strcmp(name, "window_height"))
    {
    val->val_i = p->win.window_height;
    return 1;
    }
  return 0;
  }


bg_ov_plugin_t the_plugin =
  {
    common:
    {
      name:          "ov_x11",
      long_name:     "X11 display driver",
      type:          BG_PLUGIN_OUTPUT_VIDEO,
      flags:         BG_PLUGIN_PLAYBACK,
      priority:      BG_PLUGIN_PRIORITY_MAX,
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      create:        create_x11,
      destroy:       destroy_x11,

      get_parameters: get_parameters_x11,
      set_parameter:  set_parameter_x11,
      get_parameter:  get_parameter_x11
    },

    open:          open_x11,
    put_video:     write_frame_x11,
    alloc_frame:   alloc_frame_x11,
    free_frame:    free_frame_x11,
    close:         close_x11,
    put_still:     put_still_x11,
    set_callbacks: set_callbacks_x11,
    show_window:   show_window_x11
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
