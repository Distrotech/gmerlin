#include <x11/x11.h>
#include <x11/x11_window_private.h>

/* XImage video driver */

static gavl_pixelformat_t get_pixelformat(bg_x11_window_t * win)
  {
  int bpp;
  XPixmapFormatValues * pf;
  int i;
  int num_pf;
  gavl_pixelformat_t ret = GAVL_PIXELFORMAT_NONE;
  
  bpp = 0;
  pf = XListPixmapFormats(win->dpy, &num_pf);
  for(i = 0; i < num_pf; i++)
    {
    if(pf[i].depth == win->vi->depth)
      bpp = pf[i].bits_per_pixel;
    }
  XFree(pf);
  
  ret = GAVL_PIXELFORMAT_NONE;
  switch(bpp)
    {
    case 16:
      if((win->vi->visual->red_mask == 63488) &&
         (win->vi->visual->green_mask == 2016) &&
         (win->vi->visual->blue_mask == 31))
        ret = GAVL_RGB_16;
      else if((win->vi->visual->blue_mask == 63488) &&
              (win->vi->visual->green_mask == 2016) &&
              (win->vi->visual->red_mask == 31))
        ret = GAVL_BGR_16;
      break;
    case 24:
      if((win->vi->visual->red_mask == 0xff) && 
         (win->vi->visual->green_mask == 0xff00) &&
         (win->vi->visual->blue_mask == 0xff0000))
        ret = GAVL_RGB_24;
      else if((win->vi->visual->red_mask == 0xff0000) && 
         (win->vi->visual->green_mask == 0xff00) &&
         (win->vi->visual->blue_mask == 0xff))
        ret = GAVL_BGR_24;
      break;
    case 32:
      if((win->vi->visual->red_mask == 0xff) && 
         (win->vi->visual->green_mask == 0xff00) &&
         (win->vi->visual->blue_mask == 0xff0000))
        ret = GAVL_RGB_32;
      else if((win->vi->visual->red_mask == 0xff0000) && 
         (win->vi->visual->green_mask == 0xff00) &&
         (win->vi->visual->blue_mask == 0xff))
        ret = GAVL_BGR_32;
      break;
    }
  return ret;
  }

static int init_ximage(driver_data_t * d)
  {
  d->pixelformats = malloc(2*sizeof(*d->pixelformats));
  d->pixelformats[0] = get_pixelformat(d->win);
  d->pixelformats[1] = GAVL_PIXELFORMAT_NONE;
  return 1;
  }

typedef struct 
  {
  XImage * x11_image;
  XShmSegmentInfo shminfo;
  }  ximage_frame_t;

static gavl_video_frame_t * create_frame_ximage(driver_data_t * d)
  {
  ximage_frame_t * frame;
  gavl_video_frame_t * ret;
  bg_x11_window_t * w = d->win;
  
  frame = calloc(1, sizeof(*frame));

  if(w->have_shm)
    {
    /* Create Shm Image */
    frame->x11_image = XShmCreateImage(w->dpy, w->vi->visual,
                                       w->vi->depth, ZPixmap,
                                       NULL, &(frame->shminfo),
                                       w->window_format.frame_width,
                                       w->window_format.frame_height);
    if(!frame->x11_image)
      w->have_shm = 0;
    else
      {
      if(!bg_x11_window_create_shm(w, &frame->shminfo,
                                frame->x11_image->height *
                                frame->x11_image->bytes_per_line))
        {
        XDestroyImage(frame->x11_image);

        w->have_shm = 0;
        }
      frame->x11_image->data = frame->shminfo.shmaddr;

      ret = calloc(1, sizeof(*ret));

      ret->planes[0] = (uint8_t*)(frame->x11_image->data);
      ret->strides[0] = frame->x11_image->bytes_per_line;
      }
    }
  if(!w->have_shm)
    {
    /* Use gavl to allocate memory aligned scanlines */
    ret = gavl_video_frame_create(&w->window_format);
    frame->x11_image = XCreateImage(w->dpy, w->vi->visual, w->vi->depth,
                                    ZPixmap,
                                    0, (char*)(ret->planes[0]),
                                    w->window_format.frame_width,
                                    w->window_format.frame_height,
                                    32,
                                    ret->strides[0]);
    }
  ret->user_data = frame;
  return ret;
  }

static void put_frame_ximage(driver_data_t * d, gavl_video_frame_t * f)
  {
  ximage_frame_t * frame = (ximage_frame_t*)f->user_data;
  bg_x11_window_t * w = d->win;
  
  if(w->have_shm)
    {
    XShmPutImage(w->dpy,            /* dpy        */
                 w->current_window, /* d          */
                 w->gc,             /* gc         */
                 frame->x11_image, /* image      */
                 w->dst_rect.x,    /* src_x      */
                 w->dst_rect.y,    /* src_y      */
                 w->dst_rect.x,          /* dst_x      */
                 w->dst_rect.y,          /* dst_y      */
                 w->dst_rect.w,    /* src_width  */
                 w->dst_rect.h,   /* src_height */
                 True                  /* send_event */);
    w->wait_for_completion = 1;
    }
  else
    {
    XPutImage(w->dpy,            /* dpy        */
              w->current_window, /* d          */
              w->gc,             /* gc         */
              frame->x11_image, /* image      */
              w->dst_rect.x,    /* src_x      */
              w->dst_rect.y,    /* src_y      */
              w->dst_rect.x,          /* dst_x      */
              w->dst_rect.y,          /* dst_y      */
              w->dst_rect.w,    /* src_width  */
              w->dst_rect.h);   /* src_height */
    }
  }

static void destroy_frame_ximage(driver_data_t * d, gavl_video_frame_t * f)
  {
  ximage_frame_t * frame = (ximage_frame_t*)f->user_data;
  bg_x11_window_t * w = d->win;
  if(frame->x11_image)
    XFree(frame->x11_image);
  if(w->have_shm)
    {
    bg_x11_window_destroy_shm(w, &frame->shminfo);
    gavl_video_frame_null(f);
    gavl_video_frame_destroy(f);
    }
  else
    {
    gavl_video_frame_destroy(f);
    }
  free(frame);
  }

video_driver_t ximage_driver =
  {
    .init               = init_ximage,
    .create_frame       = create_frame_ximage,
    .put_frame          = put_frame_ximage,
    .destroy_frame      = destroy_frame_ximage
  };
