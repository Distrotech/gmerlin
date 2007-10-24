#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <X11/extensions/Xvlib.h>

#define XV_ID_YV12  0x32315659 
#define XV_ID_I420  0x30323449
#define XV_ID_YUY2  0x32595559
#define XV_ID_UYVY  0x59565955

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
  XvPortID port;
  int * format_ids;
  int have_xv_colorkey;
  int xv_colorkey;  
  int xv_colorkey_orig;
  Atom xv_attr_atoms[NUM_XV_PARAMETERS];
  Atom xv_colorkey_atom;
  int xv_colorkey_settable;
  int format;
  } xv_priv_t;

static void check_xv(Display * d, Window w,
                     XvPortID * port,
                     gavl_pixelformat_t * formats_ret,
                     int * format_ids_ret)
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

  int format_index = 0;
  int have_420 = 0;
  
  formats_ret[0] = GAVL_PIXELFORMAT_NONE;
  
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
            if(!have_420)
              {
              formats_ret[format_index] = GAVL_YUV_420_P;
              format_ids_ret[format_index++] = formatValues[k].id;
              found = 1;
              have_420 = 1;
              }
            }
          else if(formatValues[k].id == XV_ID_I420)
            {
            if(!have_420)
              {
              formats_ret[format_index] = GAVL_YUV_420_P;
              format_ids_ret[format_index++] = formatValues[k].id;
              found = 1;
              have_420 = 1;
              }
            }
          else if(formatValues[k].id == XV_ID_YUY2)
            {
            formats_ret[format_index] = GAVL_YUY2;
            format_ids_ret[format_index++] = formatValues[k].id;
            found = 1;
            }
          else if(formatValues[k].id == XV_ID_UYVY)
            {
            formats_ret[format_index] = GAVL_UYVY;
            format_ids_ret[format_index++] = formatValues[k].id;
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
  formats_ret[format_index] = GAVL_PIXELFORMAT_NONE;
  return;
  }

static int init_xv(driver_data_t * d)
  {
  xv_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  d->priv = priv;
  priv->format_ids = malloc(5*sizeof(*priv->format_ids));
  d->pixelformats = malloc(5*sizeof(*d->pixelformats));

  check_xv(d->win->dpy, d->win->root,
           &priv->port,
           d->pixelformats,
           priv->format_ids);
  return 0;
  }

static int open_xv(driver_data_t * d)
  {
  bg_x11_window_t * w;
  xv_priv_t * priv;
  int i;
  priv = (xv_priv_t *)(d->priv);
  w = d->win;
  /* Get the format */

  i = 0;
  while(d->pixelformats[i] != GAVL_PIXELFORMAT_NONE)
    {
    if(d->pixelformats[i] == d->pixelformat)
      {
      priv->format = priv->format_ids[i];
      break;
      }
    i++;
    }

  if(d->pixelformats[i] == GAVL_PIXELFORMAT_NONE)
    return 0;
  
  if(Success != XvGrabPort(w->dpy, priv->port, CurrentTime))
    return 0;
  else if(priv->have_xv_colorkey)
    {
    XvGetPortAttribute(w->dpy, priv->port, priv->xv_colorkey_atom, 
                       &(priv->xv_colorkey_orig));
    if(priv->xv_colorkey_settable)
      {
      priv->xv_colorkey = 0x00010100;
      XvSetPortAttribute(w->dpy, priv->port, priv->xv_colorkey_atom, 
                         priv->xv_colorkey);
      
      XSetWindowBackground(w->dpy, w->normal.win, priv->xv_colorkey);
      XSetWindowBackground(w->dpy, w->fullscreen.win, priv->xv_colorkey);
      
      }
    else
      priv->xv_colorkey = priv->xv_colorkey_orig;
    }
  return 1;
  }


typedef struct 
  {
  XvImage * xv_image;
  XShmSegmentInfo shminfo;
  }  xv_frame_t;

static gavl_video_frame_t * create_frame_xv(driver_data_t * d)
  {
  xv_frame_t * frame;
  gavl_video_frame_t * ret;
  bg_x11_window_t * w;
  xv_priv_t * priv;
  priv = (xv_priv_t *)(d->priv);
  w = d->win;

  frame = calloc(1, sizeof(*frame));

  if(w->have_shm)
    {
    frame->xv_image =
      XvShmCreateImage(w->dpy, priv->port, priv->format, NULL,
                       w->video_format.frame_width,
                       w->video_format.frame_height, &(frame->shminfo));
    if(!frame->xv_image)
      w->have_shm = 0;
    else
      {
      if(!bg_x11_window_create_shm(w, &(frame->shminfo),
                                frame->xv_image->data_size))
        {
        XFree(frame->xv_image);
        frame->xv_image = NULL;
        w->have_shm = 0;
        }
      else
        frame->xv_image->data = frame->shminfo.shmaddr;
      }
    }

  if(!w->have_shm)
    {
    ret = gavl_video_frame_create(&w->video_format);
    frame->xv_image =
      XvCreateImage(w->dpy, priv->port,
                    priv->format, (char*)(ret->planes[0]),
                    w->video_format.frame_width,
                    w->video_format.frame_height);
    }
  else
    ret = gavl_video_frame_create(NULL);

  switch(priv->format)
    {
    case XV_ID_YV12:
      ret->planes[0] =
        (uint8_t*)(frame->xv_image->data + frame->xv_image->offsets[0]);
      ret->planes[1] =
        (uint8_t*)(frame->xv_image->data + frame->xv_image->offsets[2]);
      ret->planes[2] =
        (uint8_t*)(frame->xv_image->data + frame->xv_image->offsets[1]);
      
      ret->strides[0]      = frame->xv_image->pitches[0];
      ret->strides[1]      = frame->xv_image->pitches[2];
      ret->strides[2]      = frame->xv_image->pitches[1];
      break;
    case XV_ID_I420:
      ret->planes[0] =
        (uint8_t*)(frame->xv_image->data + frame->xv_image->offsets[0]);
      ret->planes[1] =
        (uint8_t*)(frame->xv_image->data + frame->xv_image->offsets[1]);
      ret->planes[2] =
        (uint8_t*)(frame->xv_image->data + frame->xv_image->offsets[2]);
      
      ret->strides[0] = frame->xv_image->pitches[0];
      ret->strides[1] = frame->xv_image->pitches[1];
      ret->strides[2] = frame->xv_image->pitches[2];
      break;
    case XV_ID_YUY2:
    case XV_ID_UYVY:
      ret->planes[0] =
        (uint8_t*)(frame->xv_image->data + frame->xv_image->offsets[0]);
      ret->strides[0] = frame->xv_image->pitches[0];
      break;
    }

  
  ret->user_data = frame;
  return ret;
  }

static void put_frame_xv(driver_data_t * d, gavl_video_frame_t * f)
  {
  xv_priv_t * priv;
  xv_frame_t * frame = (xv_frame_t*)f->user_data;
  
  bg_x11_window_t * w;
  
  priv = (xv_priv_t *)(d->priv);
  w = d->win;

  if(w->have_shm)
    {
    XvShmPutImage(w->dpy,
                  priv->port,
                  w->current->win,
                  w->gc,
                  frame->xv_image,
                  (int)w->src_rect.x,  /* src_x  */
                  (int)w->src_rect.y,  /* src_y  */
                  (int)w->src_rect.w,  /* src_w  */
                  (int)w->src_rect.h,  /* src_h  */
                  w->dst_rect.x,  /* dest_x */
                  w->dst_rect.y,  /* dest_y */
                  w->dst_rect.w,  /* dest_w */
                  w->dst_rect.h,  /* dest_h */
                  True);
    w->wait_for_completion = 1;
    }
  else
    {
    XvPutImage(w->dpy,
               priv->port,
               w->current->win,
               w->gc,
               frame->xv_image,
               (int)w->src_rect.x,  /* src_x  */
               (int)w->src_rect.y,  /* src_y  */
               (int)w->src_rect.w,  /* src_w  */
               (int)w->src_rect.h,  /* src_h  */
               w->dst_rect.x,          /* dest_x  */
               w->dst_rect.y,          /* dest_y  */
               w->dst_rect.w,          /* dest_w  */
               w->dst_rect.h);         /* dest_h  */
    }
  }

static void destroy_frame_xv(driver_data_t * d, gavl_video_frame_t * f)
  {
  xv_frame_t * frame = (xv_frame_t*)f->user_data;
  bg_x11_window_t * w = d->win;

  if(frame->xv_image)
    XFree(frame->xv_image);

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

static void close_xv(driver_data_t * d)
  {
  xv_priv_t * priv;
  bg_x11_window_t * w;
  
  priv = (xv_priv_t *)(d->priv);
  w = d->win;
  
  XvUngrabPort(w->dpy, priv->port, CurrentTime);
  XvStopVideo(w->dpy, priv->port, w->current->win);
  if(priv->xv_colorkey_settable)
    XvSetPortAttribute(w->dpy, priv->port, priv->xv_colorkey_atom, 
                       priv->xv_colorkey_orig);
  }

static void cleanup_xv(driver_data_t * d)
  {
  xv_priv_t * priv;
  priv = (xv_priv_t *)(d->priv);
  if(priv->format_ids)
    free(priv->format_ids);
  free(priv);
  }



video_driver_t xv_driver =
  {
    .can_scale          = 1,
    .init               = init_xv,
    .open               = open_xv,
    .create_frame       = create_frame_xv,
    .put_frame          = put_frame_xv,
    .destroy_frame      = destroy_frame_xv,
    .close              = close_xv,
    .cleanup            = cleanup_xv,
  };
