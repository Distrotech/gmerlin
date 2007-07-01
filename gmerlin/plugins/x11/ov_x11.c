/*****************************************************************
 
  ov_x11.c
 
  Copyright (c) 2003-2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

// #include <pthread.h>

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
#include <config.h>
#include <translation.h>

#include <keycodes.h>
#include <utils.h>
#include <config.h>
#include <log.h>

#define LOG_DOMAIN "x11"

#include "x11_window.h"

#ifdef HAVE_LIBXV
#include <X11/extensions/Xvlib.h>
#endif

static int ignore_mask = Mod2Mask;

// #undef HAVE_LIBXV

#define SQUEEZE_MIN -1.0
#define SQUEEZE_MAX 1.0
#define SQUEEZE_DELTA 0.04

#define ZOOM_MIN 20.0
#define ZOOM_MAX 180.0
#define ZOOM_DELTA 2.0

#define BCS_DELTA 0.025

#define XV_ID_YV12  0x32315659 
#define XV_ID_I420  0x30323449
#define XV_ID_YUY2  0x32595559
#define XV_ID_UYVY  0x59565955

#define HAVE_XV
#define HAVE_XSHM

static struct
  {
  KeySym x11;
  int bg;
  }
keycodes[] = 
  {
    { XK_0, BG_KEY_0 },
    { XK_1, BG_KEY_1 },
    { XK_2, BG_KEY_2 },
    { XK_3, BG_KEY_3 },
    { XK_4, BG_KEY_4 },
    { XK_5, BG_KEY_5 },
    { XK_6, BG_KEY_6 },
    { XK_7, BG_KEY_7 },
    { XK_8, BG_KEY_8 },
    { XK_9, BG_KEY_9 },
    { XK_space,  BG_KEY_SPACE },
    { XK_Return, BG_KEY_RETURN },
    { XK_Left,   BG_KEY_LEFT },
    { XK_Right,  BG_KEY_RIGHT },
    { XK_Up,     BG_KEY_UP },
    { XK_Down,   BG_KEY_DOWN },
    { XK_Page_Up,  BG_KEY_PAGE_UP },
    { XK_Page_Down, BG_KEY_PAGE_DOWN },
    { XK_plus,  BG_KEY_PLUS },
    { XK_minus, BG_KEY_MINUS },

    { XK_a,        BG_KEY_A },
    { XK_b,        BG_KEY_B },
    { XK_c,        BG_KEY_C },
    { XK_d,        BG_KEY_D },
    { XK_e,        BG_KEY_E },
    { XK_f,        BG_KEY_F },
    { XK_g,        BG_KEY_G },
    { XK_h,        BG_KEY_H },
    { XK_i,        BG_KEY_I },
    { XK_j,        BG_KEY_J },
    { XK_k,        BG_KEY_K },
    { XK_l,        BG_KEY_L },
    { XK_m,        BG_KEY_M },
    { XK_n,        BG_KEY_N },
    { XK_o,        BG_KEY_O },
    { XK_p,        BG_KEY_P },
    { XK_q,        BG_KEY_Q },
    { XK_r,        BG_KEY_R },
    { XK_s,        BG_KEY_S },
    { XK_t,        BG_KEY_T },
    { XK_u,        BG_KEY_U },
    { XK_v,        BG_KEY_V },
    { XK_W,        BG_KEY_W },
    { XK_x,        BG_KEY_X },
    { XK_y,        BG_KEY_Y },
    { XK_z,        BG_KEY_Z },
    
    { XK_A,        BG_KEY_A },
    { XK_B,        BG_KEY_B },
    { XK_C,        BG_KEY_C },
    { XK_D,        BG_KEY_D },
    { XK_E,        BG_KEY_E },
    { XK_F,        BG_KEY_F },
    { XK_G,        BG_KEY_G },
    { XK_H,        BG_KEY_H },
    { XK_I,        BG_KEY_I },
    { XK_J,        BG_KEY_J },
    { XK_K,        BG_KEY_K },
    { XK_L,        BG_KEY_L },
    { XK_M,        BG_KEY_M },
    { XK_N,        BG_KEY_N },
    { XK_O,        BG_KEY_O },
    { XK_P,        BG_KEY_P },
    { XK_Q,        BG_KEY_Q },
    { XK_R,        BG_KEY_R },
    { XK_S,        BG_KEY_S },
    { XK_T,        BG_KEY_T },
    { XK_U,        BG_KEY_U },
    { XK_V,        BG_KEY_V },
    { XK_W,        BG_KEY_W },
    { XK_X,        BG_KEY_X },
    { XK_Y,        BG_KEY_Y },
    { XK_Z,        BG_KEY_Z },

    { XK_F1,       BG_KEY_F1 },
    { XK_F2,       BG_KEY_F2 },
    { XK_F3,       BG_KEY_F3 },
    { XK_F4,       BG_KEY_F4 },
    { XK_F5,       BG_KEY_F5 },
    { XK_F6,       BG_KEY_F6 },
    { XK_F7,       BG_KEY_F7 },
    { XK_F8,       BG_KEY_F8 },
    { XK_F9,       BG_KEY_F9 },
    { XK_F10,      BG_KEY_F10 },
    { XK_F11,      BG_KEY_F11 },
    { XK_F12,      BG_KEY_F12 },
  };

static int get_key_code(KeySym x11)
  {
  int i;

  for(i = 0; i < sizeof(keycodes)/sizeof(keycodes[0]); i++)
    {
    if(x11 == keycodes[i].x11)
      return keycodes[i].bg;
    }
  return BG_KEY_NONE;
  }

static int get_key_mask(int x11_mask)
  {
  int ret = 0;

  if(x11_mask & ShiftMask)
    ret |= BG_KEY_SHIFT_MASK;
  if(x11_mask & ControlMask)
    ret |= BG_KEY_CONTROL_MASK;
  if(x11_mask & Mod1Mask)
    ret |= BG_KEY_ALT_MASK;
  return ret;
  }

static void
free_frame_x11(void * data, gavl_video_frame_t * frame);

static void write_frame_x11(void * data, gavl_video_frame_t * frame);


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
  int scale_order;
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

  int brightness_i, saturation_i, contrast_i;
  float brightness_f, saturation_f, contrast_f;
  
  bg_parameter_info_t * brightness_parameter;
  bg_parameter_info_t * saturation_parameter;
  bg_parameter_info_t * contrast_parameter;
  
  int shm_completion_type;
  int wait_for_completion;
    
  /* Fullscreen stuff */

  Atom hints_atom;
    
  /* Drawing coords */
  
  gavl_rectangle_i_t src_rect_i;
  gavl_rectangle_f_t src_rect_f;
  gavl_rectangle_i_t dst_rect;
  
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

  int do_still;
  gavl_video_frame_t * still_frame;
  
  /* Turn off xscreensaver? */

  int disable_xscreensaver_fullscreen;
  int disable_xscreensaver_normal;
  

  float squeeze;
  float zoom;

  /* Overlay support */

  int num_overlay_streams;

  struct
    {
    gavl_overlay_blend_context_t * ctx;
    gavl_overlay_t * ovl;
    } * overlay_streams;
  } x11_t;

#ifdef HAVE_LIBXV
static int set_xv_parameter(x11_t * p, const char * name, 
                            int value);
#endif


/* BCS (Brightness, Contrast, Saturation...) Adjustments */

static int bcs_f2i(bg_parameter_info_t * info, float f)
  {
  int ret;
  ret = info->val_min.val_i + (int)((float)(info->val_max.val_i - info->val_min.val_i)*f + 0.5);
  if(ret > info->val_max.val_i)
    ret = info->val_max.val_i;
  if(ret < info->val_min.val_i)
    ret = info->val_min.val_i;
  return ret;
  }

static float bcs_i2f(bg_parameter_info_t * info, int i)
  {
  return (float)(i - info->val_min.val_i) / (float)(info->val_max.val_i - info->val_min.val_i);
  }

/* Overlay support */

static int add_overlay_stream_x11(void * data, const gavl_video_format_t * format)
  {
  x11_t * x11 = (x11_t *)data;

  /* Realloc */
  x11->overlay_streams =
    realloc(x11->overlay_streams,
            (x11->num_overlay_streams+1) * sizeof(*(x11->overlay_streams)));
  memset(&x11->overlay_streams[x11->num_overlay_streams], 0,
         sizeof(x11->overlay_streams[x11->num_overlay_streams]));

  /* Initialize */

  x11->overlay_streams[x11->num_overlay_streams].ctx = gavl_overlay_blend_context_create();

  gavl_overlay_blend_context_init(x11->overlay_streams[x11->num_overlay_streams].ctx,
                                  &(x11->video_format), format);
  x11->num_overlay_streams++;
  return x11->num_overlay_streams - 1;
  }

static void destroy_overlay_streams(x11_t * x11)
  {
  int i;
  if(x11->num_overlay_streams)
    {
    for(i = 0; i < x11->num_overlay_streams; i++)
      {
      gavl_overlay_blend_context_destroy(x11->overlay_streams[i].ctx);
      }
    free(x11->overlay_streams);
    x11->overlay_streams = NULL;
    x11->num_overlay_streams = 0;
    }
  }
 

static void set_overlay_x11(void * data, int stream, gavl_overlay_t * ovl)
  {
  x11_t * x11 = (x11_t *)data;
  x11->overlay_streams[stream].ovl = ovl;
  gavl_overlay_blend_context_set_overlay(x11->overlay_streams[stream].ctx, ovl);
  }


static void create_parameters(x11_t * x11);	

static int handle_event(x11_t * priv, XEvent * evt);
static void close_x11(void*);

static
void set_callbacks_x11(void * data, bg_ov_callbacks_t * callbacks)
  {
  ((x11_t*)(data))->callbacks = callbacks;
  }

static gavl_pixelformat_t get_x11_pixelformat(Display * d)
  {
  int screen_number;
  Visual * visual;
  int depth;
  int bpp;
  XPixmapFormatValues * pf;
  int i;
  int num_pf;
  gavl_pixelformat_t ret = GAVL_PIXELFORMAT_NONE;
    
  screen_number = DefaultScreen(d);
  visual = DefaultVisual(d, screen_number);

  if(visual->class != TrueColor)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No True Color Visual");
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
                       priv->video_format.frame_width,
                       priv->video_format.frame_height, &(x11_frame->shminfo));
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

    video_format.pixelformat = priv->video_format.pixelformat;
    
    ret = gavl_video_frame_create(&video_format);
    x11_frame->xv_image = XvCreateImage(priv->dpy, priv->xv_port,
                                        priv->xv_format, (char*)(ret->planes[0]),
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
        (uint8_t*)(x11_frame->xv_image->data + x11_frame->xv_image->offsets[0]);
      ret->planes[1] =
        (uint8_t*)(x11_frame->xv_image->data + x11_frame->xv_image->offsets[2]);
      ret->planes[2] =
        (uint8_t*)(x11_frame->xv_image->data + x11_frame->xv_image->offsets[1]);
      
      ret->strides[0]      = x11_frame->xv_image->pitches[0];
      ret->strides[1]      = x11_frame->xv_image->pitches[2];
      ret->strides[2]      = x11_frame->xv_image->pitches[1];
      break;
    case XV_ID_I420:
      ret->planes[0] =
        (uint8_t*)(x11_frame->xv_image->data + x11_frame->xv_image->offsets[0]);
      ret->planes[1] =
        (uint8_t*)(x11_frame->xv_image->data + x11_frame->xv_image->offsets[1]);
      ret->planes[2] =
        (uint8_t*)(x11_frame->xv_image->data + x11_frame->xv_image->offsets[2]);
      
      ret->strides[0] = x11_frame->xv_image->pitches[0];
      ret->strides[1] = x11_frame->xv_image->pitches[1];
      ret->strides[2] = x11_frame->xv_image->pitches[2];
      break;
    case XV_ID_YUY2:
    case XV_ID_UYVY:
      ret->planes[0] =
        (uint8_t*)(x11_frame->xv_image->data + x11_frame->xv_image->offsets[0]);
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

      ret->planes[0] = (uint8_t*)(x11_frame->x11_image->data);
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

    video_format.pixelformat = format->pixelformat;

    ret = gavl_video_frame_create(&video_format);
    
    x11_frame->x11_image = XCreateImage(priv->dpy, visual, depth, ZPixmap,
                                        0, (char*)(ret->planes[0]),
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

  /* Log screensaver mode */

  switch(priv->win.screensaver_mode)
    {
    case SCREENSAVER_MODE_XLIB:
      bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Assuming Xlib screensaver support");
      break;
    case SCREENSAVER_MODE_GNOME:
      bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Assuming Gnome screensaver support");
      break;
    case SCREENSAVER_MODE_KDE:
      bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Assuming KDE screensaver support");
      break;
    }
  
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

  zoom_factor = priv->zoom * 0.01;
  squeeze_factor = priv->squeeze;
  
  
  priv->window_format.image_width = priv->win.window_width;
  priv->window_format.image_height = priv->win.window_height;
  
  if(priv->can_scale)
    {
    gavl_rectangle_f_set_all(&priv->src_rect_f, &priv->video_format);
    gavl_rectangle_fit_aspect(&priv->dst_rect,
                              &priv->video_format,
                              &priv->src_rect_f,
                              &priv->window_format,
                              zoom_factor, squeeze_factor);

   
    gavl_rectangle_crop_to_format_scale(&priv->src_rect_f,
                                        &priv->dst_rect,
                                        &priv->video_format,
                                        &priv->window_format);


    gavl_rectangle_f_to_i(&priv->src_rect_i, &priv->src_rect_f);
    gavl_rectangle_i_align_to_format(&priv->src_rect_i, &priv->video_format);
    
#ifdef HAVE_LIBXV
    if(priv->have_xv_colorkey)
      {
      XSetForeground(priv->dpy, priv->win.gc, priv->xv_colorkey);
      XFillRectangle(priv->dpy, priv->win.current_window, priv->win.gc,
                     priv->dst_rect.x, priv->dst_rect.y, priv->dst_rect.w,
                     priv->dst_rect.h);
      }
#endif
    }
  else
    {
    gavl_rectangle_crop_to_format_noscale(&priv->src_rect_i,
                                          &priv->dst_rect,
                                          &priv->video_format,
                                          &priv->window_format);
    }
  
  
  if(!priv->do_sw_scale)
    return;

  /* Allocate larger image if nesessary */
  //  gavl_rectangle_set_all(&priv->src_rect,
  //                         &priv->video_format);
  
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

  /* Clear window */

  gavl_video_frame_clear(priv->window_frame, &(priv->window_format));
  
  /* Reinitialize scaler */
  

  opt = gavl_video_scaler_get_options(priv->scaler);
  gavl_video_options_set_scale_mode(opt, priv->scale_mode);
  gavl_video_options_set_scale_order(opt, priv->scale_order);
  gavl_video_options_set_quality(opt, 2);

  gavl_video_options_set_rectangles(opt, &(priv->src_rect_f),
                                    &(priv->dst_rect));
  
  gavl_video_scaler_init(priv->scaler,
                         &(priv->video_format),
                         &(priv->window_format)); 

  
  }

static void update_aspect_x11(void * data, int pixel_width,
                              int pixel_height)
  {
  x11_t * priv;
  priv = (x11_t*)data;

  priv->video_format.pixel_width = pixel_width;
  priv->video_format.pixel_height = pixel_height;
  
  x11_window_clear(&priv->win);
  set_drawing_coords(priv);

  }
     


static int _open_x11(void * data,
                     gavl_video_format_t * format,
                     const char * window_title)
  {
  x11_t * priv;
  gavl_pixelformat_t x11_pixelformat;
  priv = (x11_t*)data;
  
  /* Set screensaver options */

  priv->win.disable_screensaver_fullscreen = priv->disable_xscreensaver_fullscreen;
  priv->win.disable_screensaver_normal     = priv->disable_xscreensaver_normal;
  
  x11_window_set_title(&(priv->win), window_title);
  
  /* Decide pixelformat */

  x11_pixelformat = get_x11_pixelformat(priv->dpy);


  
#ifdef HAVE_LIBXV
  priv->do_xv = 0;

  if(priv->xv_mode != XV_MODE_NEVER)
    {
    
    switch(format->pixelformat)
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
          format->pixelformat = GAVL_YUY2;
          }
        else if(priv->have_xv_uyvy)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_UYVY;
          format->pixelformat = GAVL_UYVY;
          }
        else
          format->pixelformat = x11_pixelformat;
        break;
      case GAVL_YUY2:
        if(priv->have_xv_yuy2)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_YUY2;
          }
        else if(priv->have_xv_uyvy)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_UYVY;
          format->pixelformat = GAVL_UYVY;
          }
        else if(priv->have_xv_yv12)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_YV12;
          format->pixelformat = GAVL_YUV_420_P;
          }
        else if(priv->have_xv_i420)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_I420;
          format->pixelformat = GAVL_YUV_420_P;
          }
        else
          format->pixelformat = x11_pixelformat;
        break;
      case GAVL_UYVY:
        if(priv->have_xv_uyvy)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_UYVY;
          format->pixelformat = GAVL_UYVY;
          }
        else if(priv->have_xv_yuy2)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_YUY2;
          }
        else if(priv->have_xv_yv12)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_YV12;
          format->pixelformat = GAVL_YUV_420_P;
          }
        else if(priv->have_xv_i420)
          {
          priv->do_xv = 1;
          priv->xv_format = XV_ID_I420;
          format->pixelformat = GAVL_YUV_420_P;
          }
        else
          format->pixelformat = x11_pixelformat;
        break;


      default:
        if(gavl_pixelformat_is_yuv(format->pixelformat) ||
           priv->xv_mode == XV_MODE_ALWAYS)
          {
          if(priv->have_xv_uyvy)
            {
            priv->do_xv = 1;
            priv->xv_format = XV_ID_UYVY;
            format->pixelformat = GAVL_UYVY;
            }
          else if(priv->have_xv_yuy2)
            {
            priv->do_xv = 1;
            priv->xv_format = XV_ID_YUY2;
            format->pixelformat = GAVL_YUY2;
            }
          else if(priv->have_xv_yv12)
            {
            priv->do_xv = 1;
            priv->xv_format = XV_ID_YV12;
            format->pixelformat = GAVL_YUV_420_P;
            }
          else if(priv->have_xv_i420)
            {
            priv->do_xv = 1;
            priv->xv_format = XV_ID_I420;
            format->pixelformat = GAVL_YUV_420_P;
            }
          else
            format->pixelformat = x11_pixelformat;
          }
        else
          format->pixelformat = x11_pixelformat;
      }

    /* Try to grab port, switch to XImage if unsuccessful */

    if(priv->do_xv)
      {
      if(Success != XvGrabPort(priv->dpy, priv->xv_port, CurrentTime))
        {
        priv->do_xv = 0;
        format->pixelformat = x11_pixelformat;
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

          XSetWindowBackground(priv->dpy, priv->win.normal_window, priv->xv_colorkey);
          XSetWindowBackground(priv->dpy, priv->win.fullscreen_window, priv->xv_colorkey);
          
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
    format->pixelformat = x11_pixelformat;
    }
  
#else
  format->pixelformat = x11_pixelformat;
  priv->do_sw_scale = priv->do_sw_scale_cfg;
  priv->can_scale = priv->do_sw_scale;
  
#endif // !HAVE_LIBXV
  priv->window_format.pixelformat = format->pixelformat;
  gavl_video_format_copy(&(priv->video_format), format);

  gavl_rectangle_i_set_all(&priv->src_rect_i, &priv->video_format);
  gavl_rectangle_f_set_all(&priv->src_rect_f, &priv->video_format);
    
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
  return _open_x11(data, format, window_title);
  }

static void close_x11(void * data)
  {
  x11_t * priv = (x11_t*)data;
#ifdef HAVE_LIBXV
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
  destroy_overlay_streams(priv);
  priv->do_still = 0;
  }

static void destroy_x11(void * data)
  {
  x11_t * priv = (x11_t*)data;

    
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
  int  key_code;
  int  done;
  int  x_image;
  int  y_image;
  int  button_number = 0;
  
  
  x11_window_handle_event(&(priv->win), evt);

  if(priv->win.do_delete)
    {
    if(priv->do_still)
      x11_window_show(&(priv->win), 0);
    }
  
  if(!evt)
    return 0;
  
  if(evt->type == priv->shm_completion_type)
    {
    priv->wait_for_completion = 0;
    return 1;
    }
  switch(evt->type)
    {
    case ButtonPress:
      if(((evt->xbutton.button == Button4) || 
          (evt->xbutton.button == Button5)) &&
         (evt->xbutton.state & (Mod1Mask|ControlMask)))
        {
        
        if(evt->xbutton.button == Button4)
          {
          if(evt->xbutton.state & Mod1Mask)
            {
            /* Increase Zoom */
            priv->zoom += ZOOM_DELTA;
            if(priv->zoom > ZOOM_MAX)
              priv->zoom = ZOOM_MAX;
            }
          else if(evt->xbutton.state & ControlMask)
            {
            /* Increase Squeeze */
            priv->squeeze += SQUEEZE_DELTA;
            if(priv->squeeze > SQUEEZE_MAX)
              priv->squeeze = SQUEEZE_MAX;
            }
          
          }
        else if(evt->xbutton.button == Button5)
          {
          if(evt->xbutton.state & Mod1Mask)
            {
            /* Decrease Zoom */
            priv->zoom -= ZOOM_DELTA;
            if(priv->zoom < ZOOM_MIN)
              priv->zoom = ZOOM_MIN;
            
            }
          else if(evt->xbutton.state & ControlMask)
            {
            /* Decrease Squeeze */
            priv->squeeze -= SQUEEZE_DELTA;
            if(priv->squeeze < SQUEEZE_MIN)
              priv->squeeze = SQUEEZE_MIN;
            
            }
          
          }
        x11_window_clear(&priv->win);
        set_drawing_coords(priv);
        return 0;
        }
      
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
                                           x_image, y_image, button_number, get_key_mask(evt->xbutton.state & ~ignore_mask));
        }


      break;
    case KeyPress:
      XLookupString(&(evt->xkey), &key_char, 1, &keysym, NULL);
      evt->xkey.state &= ~ignore_mask;
      done = 1;
      
      /* Handle some keys here */

      switch(keysym)
        {
        case XK_Tab:
        case XK_f:
          /* Non Fullscreen -> Fullscreen */
          
          if(priv->win.current_window == priv->win.normal_window)
            x11_window_set_fullscreen(&(priv->win), 1);
          /* Fullscreen -> Non Fullscreen */
          else
            x11_window_set_fullscreen(&(priv->win), 0);
          set_drawing_coords(priv);
          break;
        case XK_Escape:
          if(priv->win.current_window == priv->win.fullscreen_window)
            {
            x11_window_set_fullscreen(&(priv->win), 0);
            set_drawing_coords(priv);
            }
          break;
        case XK_Home:
          priv->zoom = 100.0;
          priv->squeeze = 0.0;
          x11_window_clear(&priv->win);
          set_drawing_coords(priv);
          break;
        case XK_plus:
          if(evt->xkey.state & Mod1Mask)
            {
            /* Increase Zoom */
            priv->zoom += ZOOM_DELTA;
            if(priv->zoom > ZOOM_MAX)
              priv->zoom = ZOOM_MAX;
            x11_window_clear(&priv->win);
            set_drawing_coords(priv);
            }
          else if(evt->xkey.state & ControlMask)
            {
            /* Increase Squeeze */
            priv->squeeze += SQUEEZE_DELTA;
            if(priv->squeeze > SQUEEZE_MAX)
              priv->squeeze = SQUEEZE_MAX;
            x11_window_clear(&priv->win);
            set_drawing_coords(priv);
            }
          else
            done = 0;
          break;
        case XK_minus:
          if(evt->xkey.state & Mod1Mask)
            {
            /* Decrease Zoom */
            priv->zoom -= ZOOM_DELTA;
            if(priv->zoom < ZOOM_MIN)
              priv->zoom = ZOOM_MIN;
            x11_window_clear(&priv->win);
            set_drawing_coords(priv);
            }
          else if(evt->xkey.state & ControlMask)
            {
            /* Decrease Squeeze */
            priv->squeeze -= SQUEEZE_DELTA;
            if(priv->squeeze < SQUEEZE_MIN)
              priv->squeeze = SQUEEZE_MIN;
            x11_window_clear(&priv->win);
            set_drawing_coords(priv);
            }
          else
            done = 0;
          break;
#ifdef HAVE_LIBXV
        case XK_B:
	  if(!priv->brightness_parameter)
	    break;
          /* Increase brightness */
          priv->brightness_f += BCS_DELTA;
          if(priv->brightness_f > 1.0)
            priv->brightness_f = 1.0;

          priv->brightness_i =
            bcs_f2i(priv->brightness_parameter, priv->brightness_f);
          set_xv_parameter(priv, priv->brightness_parameter->name,
                           priv->brightness_i);
          

          if(priv->callbacks && priv->callbacks->brightness_callback)
            priv->callbacks->brightness_callback(priv->callbacks->data,
                                                 priv->brightness_f);
          break;
        case XK_b:
	  if(!priv->brightness_parameter)
	    break;
          /* Decrease brightness */
          priv->brightness_f -= BCS_DELTA;
          if(priv->brightness_f < 0.0)
            priv->brightness_f = 0.0;
          
          priv->brightness_i =
            bcs_f2i(priv->brightness_parameter, priv->brightness_f);
          set_xv_parameter(priv, priv->brightness_parameter->name,
                           priv->brightness_i);

          if(priv->callbacks && priv->callbacks->brightness_callback)
            priv->callbacks->brightness_callback(priv->callbacks->data,
                                                 priv->brightness_f);
          
          break;
        case XK_S:
	  if(!priv->saturation_parameter)
	    break;
	
          /* Increase saturation */
          priv->saturation_f += BCS_DELTA;
          if(priv->saturation_f > 1.0)
            priv->saturation_f = 1.0;

          priv->saturation_i =
            bcs_f2i(priv->saturation_parameter, priv->saturation_f);
          set_xv_parameter(priv, priv->saturation_parameter->name,
                           priv->saturation_i);
          

          if(priv->callbacks && priv->callbacks->saturation_callback)
            priv->callbacks->saturation_callback(priv->callbacks->data,
                                                 priv->saturation_f);
          break;
        case XK_s:
 	  if(!priv->saturation_parameter)
	    break;
          /* Decrease saturation */
          priv->saturation_f -= BCS_DELTA;
          if(priv->saturation_f < 0.0)
            priv->saturation_f = 0.0;
          
          priv->saturation_i =
            bcs_f2i(priv->saturation_parameter, priv->saturation_f);
          set_xv_parameter(priv, priv->saturation_parameter->name,
                           priv->saturation_i);

          if(priv->callbacks && priv->callbacks->saturation_callback)
            priv->callbacks->saturation_callback(priv->callbacks->data,
                                                 priv->saturation_f);
          
          break;
        case XK_C:
 	  if(!priv->contrast_parameter)
	    break;
          /* Increase contrast */
          priv->contrast_f += BCS_DELTA;
          if(priv->contrast_f > 1.0)
            priv->contrast_f = 1.0;

          priv->contrast_i =
            bcs_f2i(priv->contrast_parameter, priv->contrast_f);
          set_xv_parameter(priv, priv->contrast_parameter->name,
                           priv->contrast_i);
          

          if(priv->callbacks && priv->callbacks->contrast_callback)
            priv->callbacks->contrast_callback(priv->callbacks->data,
                                                 priv->contrast_f);
          break;
        case XK_c:
 	  if(!priv->contrast_parameter)
	    break;
          /* Decrease contrast */
          priv->contrast_f -= BCS_DELTA;
          if(priv->contrast_f < 0.0)
            priv->contrast_f = 0.0;
          
          priv->contrast_i =
            bcs_f2i(priv->contrast_parameter, priv->contrast_f);
          set_xv_parameter(priv, priv->contrast_parameter->name,
                           priv->contrast_i);

          if(priv->callbacks && priv->callbacks->contrast_callback)
            priv->callbacks->contrast_callback(priv->callbacks->data,
                                                 priv->contrast_f);
          
          break;
#endif // HAVE_LIBXV
        default:
          done = 0;
        }
      
      if(done)
        return 0;
            
      /* Pass the rest to the player */

      key_code = get_key_code(keysym);
      
      if((key_code != BG_KEY_NONE) &&
         priv->callbacks &&
         priv->callbacks->key_callback)
        {
        priv->callbacks->key_callback(priv->callbacks->data,
                                      get_key_code(keysym), get_key_mask(evt->xkey.state));
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
    case Expose:
      if(priv->do_still)
        write_frame_x11(priv, priv->still_frame);
      break;
    }
  return 0;
  }

static void handle_events_x11(void * data)
  {
  XEvent * event;
  x11_t * priv = (x11_t *)data;
  if(priv->wait_for_completion)
    {
    while(priv->wait_for_completion)
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
      event = x11_window_next_event(&(priv->win), 0);
      if(!event)
        break;
      handle_event(priv, event);
      }
    x11_window_handle_event(&(priv->win), (XEvent*)0);
    }
  }

static void write_frame_x11(void * data, gavl_video_frame_t * frame)
  {
  XImage * image;
  x11_t * priv = (x11_t*)data;
  x11_frame_t * x11_frame = (x11_frame_t*)(frame->user_data);
  int x, y;
  
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
                    priv->src_rect_i.x,  /* src_x  */
                    priv->src_rect_i.y,  /* src_y  */
                    priv->src_rect_i.w,  /* src_w  */
                    priv->src_rect_i.h,  /* src_h  */
                    priv->dst_rect.x,  /* dest_x */
                    priv->dst_rect.y,  /* dest_y */
                    priv->dst_rect.w,  /* dest_w */
                    priv->dst_rect.h,  /* dest_h */
                    True);
      priv->wait_for_completion = 1;
      }
    else
      {
      XvPutImage(priv->dpy,
                 priv->xv_port,
                 priv->win.current_window,
                 priv->win.gc,
                 x11_frame->xv_image,
                 priv->src_rect_i.x,  /* src_x  */
                 priv->src_rect_i.y,  /* src_y  */
                 priv->src_rect_i.w,  /* src_w  */
                 priv->src_rect_i.h,  /* src_h  */
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

      x = priv->dst_rect.x;
      y = priv->dst_rect.y;
      
      }
    else
      {
      x11_frame = (x11_frame_t*)(frame->user_data);
      image = x11_frame->x11_image;
      x = priv->src_rect_i.x;
      y = priv->src_rect_i.y;
      }
    if(priv->have_shm)
      {
      XShmPutImage(priv->dpy,            /* dpy        */
                   priv->win.current_window, /* d          */
                   priv->win.gc,             /* gc         */
                   image, /* image      */
                   x,    /* src_x      */
                   y,    /* src_y      */
                   priv->dst_rect.x,          /* dst_x      */
                   priv->dst_rect.y,          /* dst_y      */
                   priv->dst_rect.w,    /* src_width  */
                   priv->dst_rect.h,   /* src_height */
                   True                  /* send_event */);
      priv->wait_for_completion = 1;
      }
    else
      {
      XPutImage(priv->dpy,            /* display */
                priv->win.current_window, /* d       */
                priv->win.gc,             /* gc      */
                image, /* image   */
                x,                    /* src_x   */
                y,                    /* src_y   */
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
  }

static void blend_overlays(x11_t * priv, gavl_video_frame_t * frame)
  {
  int i;
  for(i = 0; i < priv->num_overlay_streams; i++)
    gavl_overlay_blend(priv->overlay_streams[i].ctx, frame);
  }

static void put_video_x11(void * data, gavl_video_frame_t * frame)
  {
  x11_t * priv = (x11_t*)data;
  priv->still_frame = NULL;
  priv->do_still = 0;

  blend_overlays(priv, frame);
  write_frame_x11(data, frame);
  }

static void put_still_x11(void * data, gavl_video_frame_t * frame)
  {
  
  x11_t * priv = (x11_t*)data;
  
  priv->still_frame = frame;

  blend_overlays(priv, frame);
  write_frame_x11(data, priv->still_frame);
  priv->do_still = 1;
  }

static void show_window_x11(void * data, int show)
  {
  x11_t * priv = (x11_t*)data;
  XEvent * evt;
#ifdef HAVE_XDPMS
  int dpms_disabled = priv->win.dpms_disabled;
#endif
  
  x11_window_show(&(priv->win), show);
#ifdef HAVE_XDPMS
  if(dpms_disabled && !priv->win.dpms_disabled)
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Enabled DPMS");
  else if(!dpms_disabled && priv->win.dpms_disabled)
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Disabled DPMS");
#endif
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
  
  }

#ifdef HAVE_LIBXV

static int get_num_xv_parameters(Display * dpy, XvPortID xv_port)
  {
  XvAttribute *attr;
  int         nattr;
  int i, j;
  int ret = 0;
  attr = XvQueryPortAttributes(dpy, xv_port, &nattr);

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

          /* Save parameters for hue, brightness and contrast */
          if(!strcmp(info[index].name, "XV_BRIGHTNESS"))
            {
            x11->brightness_i = info[index].val_default.val_i;
            x11->brightness_parameter = &info[index];
            info[index].flags |= BG_PARAMETER_HIDE_DIALOG;
            x11->brightness_f = bcs_i2f(&info[index], x11->brightness_i);
            }
          if(!strcmp(info[index].name, "XV_SATURATION"))
            {
            x11->saturation_i = info[index].val_default.val_i;
            x11->saturation_parameter = &info[index];
            info[index].flags |= BG_PARAMETER_HIDE_DIALOG;
            x11->saturation_f = bcs_i2f(&info[index], x11->saturation_i);
            }
          if(!strcmp(info[index].name, "XV_CONTRAST"))
            {
            x11->contrast_i = info[index].val_default.val_i;
            x11->contrast_parameter = &info[index];
            info[index].flags |= BG_PARAMETER_HIDE_DIALOG;
            x11->contrast_f = bcs_i2f(&info[index], x11->contrast_i);
            }
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
#ifdef HAVE_LIBXV
    {
      name:        "xv_mode",
      long_name:   TRS("Try XVideo"),
      type:        BG_PARAMETER_STRINGLIST,

      multi_names: (char*[]){ "never", "yuv_only", "always", (char*)0},
      multi_labels: (char*[]){ TRS("Never"), TRS("For YCbCr formats only"), TRS("Always"), (char*)0},
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
      name:        "squeeze",
      long_name:   "Squeeze",
      type:        BG_PARAMETER_SLIDER_FLOAT,
      flags:       BG_PARAMETER_SYNC | BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_f: 0.0 },
      val_min:     { val_f: SQUEEZE_MIN },
      val_max:     { val_f: SQUEEZE_MAX },
      num_digits:  3
    },
    {
      name:        "zoom",
      long_name:   "Zoom",
      type:        BG_PARAMETER_SLIDER_FLOAT,
      flags:       BG_PARAMETER_SYNC | BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_f: 100.0 },
      val_min:     { val_f: ZOOM_MIN },
      val_max:     { val_f: ZOOM_MAX },
    },
    {
      name:        "sw_scaler",
      long_name:   TRS("Software scaler"),
      type:        BG_PARAMETER_SECTION,
    },
    {
      name:        "do_sw_scale",
      long_name:   TRS("Enable software scaler"),
      type:        BG_PARAMETER_CHECKBUTTON,
      help_string: TRS("This enables software scaling for the case that no hardware scaling is available")
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
    }
  };

#define NUM_COMMON_PARAMETERS sizeof(common_parameters)/sizeof(common_parameters[0])

static void create_parameters(x11_t * x11)
  {
  int i;
  int index;
#ifdef HAVE_LIBXV
  int num_xv_parameters = 0;
#endif
  int num_parameters;
    


  if(x11->parameters)
    return;    
  /* Count parameters */
  num_parameters = NUM_COMMON_PARAMETERS;
#ifdef HAVE_LIBXV
  if(x11->have_xv_yv12 || x11->have_xv_yuy2 || x11->have_xv_i420 || x11->have_xv_uyvy)
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
#ifdef HAVE_LIBXV
  if(num_xv_parameters)
    {
    get_xv_parameters(x11, &(x11->parameters[index]));
    index += num_xv_parameters;
    }
#endif
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
      XvSetPortAttribute (p->dpy, p->xv_port,
                        p->xv_attr_atoms[j], value);
      return 1;
      }
    }
  return 0;
  }

static int get_xv_parameter(x11_t * p, const char * name, 
                            int * value)
  {
  int j;
 
  for(j = 0; j < NUM_XV_PARAMETERS; j++)
    {
    if(!strcmp(name, xv_parameters[j].xv_name))
      {
      XvGetPortAttribute (p->dpy, p->xv_port,
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
  else if(!strcmp(name, "squeeze"))
    {
    p->squeeze = val->val_f;
    x11_window_clear(&p->win);
    set_drawing_coords(p);
    }
  else if(!strcmp(name, "zoom"))
    {
    p->zoom = val->val_f;
    x11_window_clear(&p->win);
    set_drawing_coords(p);
    }
  else if(!strcmp(name, "do_sw_scale"))
    {
    p->do_sw_scale_cfg = val->val_i;
    }

  else if(!strcmp(name, "scale_mode"))
    {
    if(!strcmp(val->val_str, "auto"))
      p->scale_mode = GAVL_SCALE_AUTO;
    else if(!strcmp(val->val_str, "nearest"))
      p->scale_mode = GAVL_SCALE_NEAREST;
    else if(!strcmp(val->val_str, "bilinear"))
      p->scale_mode = GAVL_SCALE_BILINEAR;
    else if(!strcmp(val->val_str, "quadratic"))
      p->scale_mode = GAVL_SCALE_QUADRATIC;
    else if(!strcmp(val->val_str, "cubic_bspline"))
      p->scale_mode = GAVL_SCALE_CUBIC_BSPLINE;
    else if(!strcmp(val->val_str, "cubic_mitchell"))
      p->scale_mode = GAVL_SCALE_CUBIC_MITCHELL;
    else if(!strcmp(val->val_str, "cubic_catmull"))
      p->scale_mode = GAVL_SCALE_CUBIC_CATMULL;
    else if(!strcmp(val->val_str, "sinc_lanczos"))
      p->scale_mode = GAVL_SCALE_SINC_LANCZOS;
    }
  else if(!strcmp(name, "scale_order"))
    {
    p->scale_order = val->val_i;
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

#ifdef HAVE_LIBXV
  else if(get_xv_parameter(p, name, &val->val_i))
    return 1;
#endif

  else if(!strcmp(name, "window_x"))
    {
    val->val_i = p->win.window_x;
    return 1;
    }
  else if(!strcmp(name, "window_y"))
    {
    val->val_i = p->win.window_y;
    return 1;
    }
  else if(!strcmp(name, "window_width"))
    {
    val->val_i = p->win.window_width;
    return 1;
    }
  else if(!strcmp(name, "window_height"))
    {
    val->val_i = p->win.window_height;
    return 1;
    }
  else if(!strcmp(name, "zoom"))
    {
    val->val_f = p->zoom;
    return 1;
    }
  else if(!strcmp(name, "squeeze"))
    {
    val->val_f = p->squeeze;
    return 1;
    }
  else
    return 0;
  }

bg_ov_plugin_t the_plugin =
  {
    common:
    {
      BG_LOCALE,
      name:          "ov_x11",
      long_name:     TRS("X11"),
      description:   TRS("X11 display driver with support for XShm, XVideo and XImage"),
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

    open:           open_x11,
    put_video:      put_video_x11,

    add_overlay_stream: add_overlay_stream_x11,
    set_overlay:        set_overlay_x11,

    
    alloc_frame:    alloc_frame_x11,
    free_frame:     free_frame_x11,
    handle_events:  handle_events_x11,
    close:          close_x11,
    put_still:      put_still_x11,

    update_aspect:  update_aspect_x11,

    set_callbacks:  set_callbacks_x11,
    show_window:    show_window_x11
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
