/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <X11/extensions/Xvlib.h>
#include <string.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "xv"

#define XV_ID_YV12  0x32315659 
#define XV_ID_I420  0x30323449
#define XV_ID_YUY2  0x32595559
#define XV_ID_UYVY  0x59565955

#define XV_ATTR_BRIGHTNESS 0
#define XV_ATTR_SATURATION 1
#define XV_ATTR_CONTRAST   2
#define XV_ATTR_NUM        3

typedef struct
  {
  XvPortID start_port;
  XvPortID end_port;
  XvPortID current_port;
  
  
  int * format_ids;
  int have_xv_colorkey;
  int xv_colorkey;  
  int xv_colorkey_orig;
  Atom xv_colorkey_atom;
  int xv_colorkey_settable;
  int format;
  
  int max_width, max_height;

  struct
    {
    Atom atom;
    int index;
    } attributes[XV_ATTR_NUM];
  
  XvAttribute * xv_attributes;
  
  } xv_priv_t;

static void check_xv(driver_data_t * d)
  {
  xv_priv_t * priv;
  
  unsigned int version;
  unsigned int release;
  unsigned int dummy;
  unsigned int adaptors;
  unsigned int i;
  XvAdaptorInfo * adaptorInfo;
  XvEncodingInfo * encodingInfo;
  XvImageFormatValues * formatValues;
  int num;
  unsigned int numu;
  
  int formats;
  int k;
  int found = 0;

  int format_index = 0;
  int have_420 = 0;
  priv = (xv_priv_t*)d->priv;
  d->pixelformats[0] = GAVL_PIXELFORMAT_NONE;
  
  if ((XvQueryExtension (d->win->dpy, &version, &release,
                         &dummy, &dummy, &dummy) != Success) ||
      (version < 2) || ((version == 2) && (release < 2)))
    return;
  XvQueryAdaptors (d->win->dpy, d->win->root, &adaptors, &adaptorInfo);
  for(i = 0; i < adaptors; i++)
    {
    /* Continue if this adaptor doesn't support XvPutImage */
    if((adaptorInfo[i].type & (XvImageMask|XvInputMask)) != (XvImageMask|XvInputMask))
      continue;
    
    /* Image formats are the same for each port of a given adaptor, it is
       enough to list the image formats just for the base port */

    formatValues =
      XvListImageFormats(d->win->dpy,
                         adaptorInfo[i].base_id,
                         &formats);

    
    for (k = 0; k < formats; k++)
      {
      if (formatValues[k].id == XV_ID_YV12)
        {
        if(!have_420)
          {
          d->pixelformats[format_index] = GAVL_YUV_420_P;
          priv->format_ids[format_index++] = formatValues[k].id;
          found = 1;
          have_420 = 1;
          }
        }
      else if(formatValues[k].id == XV_ID_I420)
        {
        if(!have_420)
          {
          d->pixelformats[format_index] = GAVL_YUV_420_P;
          priv->format_ids[format_index++] = formatValues[k].id;
          found = 1;
          have_420 = 1;
          }
        }
      else if(formatValues[k].id == XV_ID_YUY2)
        {
        d->pixelformats[format_index] = GAVL_YUY2;
        priv->format_ids[format_index++] = formatValues[k].id;
        found = 1;
        }
      else if(formatValues[k].id == XV_ID_UYVY)
        {
        d->pixelformats[format_index] = GAVL_UYVY;
        priv->format_ids[format_index++] = formatValues[k].id;
        found = 1;
        }
      }
    
    XFree (formatValues);

    if(found)
      {
      priv->start_port = adaptorInfo[i].base_id;
      priv->end_port = adaptorInfo[i].base_id + adaptorInfo[i].num_ports;
      break;
      }
    }
  XvFreeAdaptorInfo (adaptorInfo);
  d->pixelformats[format_index] = GAVL_PIXELFORMAT_NONE;

  if(found)
    {
    /* Check for attributes */
    priv->xv_attributes =
      XvQueryPortAttributes(d->win->dpy, priv->start_port, &num);

    for(i = 0; i < num; i++)
      {
      if(!strcmp(priv->xv_attributes[i].name, "XV_BRIGHTNESS"))
        {
        priv->attributes[XV_ATTR_BRIGHTNESS].index = i;
        priv->attributes[XV_ATTR_BRIGHTNESS].atom =
          XInternAtom(d->win->dpy, "XV_BRIGHTNESS", False);
        d->flags |= DRIVER_FLAG_BRIGHTNESS;
        }
      else if(!strcmp(priv->xv_attributes[i].name, "XV_CONTRAST"))
        {
        priv->attributes[XV_ATTR_CONTRAST].index = i;
        priv->attributes[XV_ATTR_CONTRAST].atom =
          XInternAtom(d->win->dpy, "XV_CONTRAST", False);
        d->flags |= DRIVER_FLAG_CONTRAST;
        }
      else if(!strcmp(priv->xv_attributes[i].name, "XV_SATURATION"))
        {
        priv->attributes[XV_ATTR_SATURATION].index = i;
        priv->attributes[XV_ATTR_SATURATION].atom =
          XInternAtom(d->win->dpy, "XV_SATURATION", False);
        d->flags |= DRIVER_FLAG_SATURATION;
        }
      }
    
    num = 0;
    XvQueryEncodings(d->win->dpy, priv->start_port, &numu, &encodingInfo);

    for(i = 0; i < numu; i++)
      {
      if(!strcmp(encodingInfo[i].name, "XV_IMAGE"))
        {
        priv->max_width  = encodingInfo[i].width;
        priv->max_height = encodingInfo[i].height;
        }
      }
    if(numu)
      XvFreeEncodingInfo(encodingInfo);
    }
  return;
  }

static int init_xv(driver_data_t * d)
  {
  xv_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  d->priv = priv;
  priv->format_ids = malloc(5*sizeof(*priv->format_ids));
  d->pixelformats = malloc(5*sizeof(*d->pixelformats));
  check_xv(d);
  return 0;
  }

static int open_xv(driver_data_t * d)
  {
  bg_x11_window_t * w;
  xv_priv_t * priv;
  int i;
  int result;
  priv = (xv_priv_t *)(d->priv);
  w = d->win;

  /* Check agaist the maximum size */
  if(priv->max_width && (d->win->video_format.frame_width > priv->max_width))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Image width out of range");
    return 0;
    }
  if(priv->max_height && (d->win->video_format.frame_height > priv->max_height))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Image height out of range");
    return 0;
    }
  /* Get the format */
  i = 0;
  while(d->pixelformats[i] != GAVL_PIXELFORMAT_NONE)
    {
    if(d->pixelformats[i] == d->pixelformat)
      {
      priv->format = priv->format_ids[i];
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using XV format: %c%c%c%c",
             priv->format & 0xff,
             (priv->format >> 8) & 0xff, 
             (priv->format >> 16) & 0xff, 
             (priv->format >> 24) & 0xff);
      break;
      }
    i++;
    }

  if(d->pixelformats[i] == GAVL_PIXELFORMAT_NONE)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No suitable pixelformat");
    return 0;
    }

  priv->current_port = priv->start_port;

  while(priv->current_port < priv->end_port)
    {
    result = XvGrabPort(w->dpy, priv->current_port, CurrentTime);
    
    switch(result)
      {
      case Success:
        break;
      case XvAlreadyGrabbed:
        bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Port %li already grabbed, trying next one",
               priv->current_port);
        priv->current_port++;
        break;
      case XvInvalidTime:
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Grabbing Port %li failed (invalid time), trying next one",
               priv->current_port);
        priv->current_port++;
        break;
      default:
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Grabbing Port %li failed (unknown error)", priv->current_port);
        return 0;
        break;
      }

    if(result == Success)
      break;
    }
  
  if(priv->current_port >= priv->end_port)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not find free port");
    return 0;
    }
  
  if(priv->have_xv_colorkey)
    {
    XvGetPortAttribute(w->dpy, priv->current_port, priv->xv_colorkey_atom, 
                       &(priv->xv_colorkey_orig));
    if(priv->xv_colorkey_settable)
      {
      priv->xv_colorkey = 0x00010100;
      XvSetPortAttribute(w->dpy, priv->current_port, priv->xv_colorkey_atom, 
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
      XvShmCreateImage(w->dpy, priv->current_port, priv->format, NULL,
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
      XvCreateImage(w->dpy, priv->current_port,
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
                  priv->current_port,
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
               priv->current_port,
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
  
  XvUngrabPort(w->dpy, priv->current_port, CurrentTime);
  XvStopVideo(w->dpy, priv->current_port, w->current->win);
  if(priv->xv_colorkey_settable)
    XvSetPortAttribute(w->dpy, priv->current_port, priv->xv_colorkey_atom, 
                       priv->xv_colorkey_orig);
  }

static void cleanup_xv(driver_data_t * d)
  {
  xv_priv_t * priv;
  priv = (xv_priv_t *)(d->priv);
  if(priv->format_ids)
    free(priv->format_ids);

  if(priv->xv_attributes)
    XFree(priv->xv_attributes);
  
  free(priv);
  }

static int rescale(XvAttribute * attr, float val, float min, float max)
  {
  int ret;
  ret = attr->min_value +
    (int)((attr->max_value - attr->min_value) * (val - min) / (max - min) +
          0.5);
  if(ret < attr->min_value)
    ret = attr->min_value;
  if(ret > attr->max_value)
    ret = attr->max_value;
  return ret;
  }

static void set_brightness_xv(driver_data_t* d,float val)
  {
  xv_priv_t * priv;
  int val_i;

  priv = d->priv;

  val_i =
    rescale(&priv->xv_attributes[priv->attributes[XV_ATTR_BRIGHTNESS].index],
            val,
            BG_BRIGHTNESS_MIN,
            BG_BRIGHTNESS_MAX);
  
  XvSetPortAttribute(d->win->dpy, priv->current_port,
                     priv->attributes[XV_ATTR_BRIGHTNESS].atom, val_i);
  }

static void set_saturation_xv(driver_data_t* d,float val)
  {
  xv_priv_t * priv;
  priv = (xv_priv_t *)(d->priv);
  XvSetPortAttribute(d->win->dpy, priv->current_port,
                     priv->attributes[XV_ATTR_SATURATION].atom, 
                     rescale(&priv->xv_attributes[priv->attributes[XV_ATTR_SATURATION].index],
                             val,
                             BG_SATURATION_MIN,
                             BG_SATURATION_MAX));

  }

static void set_contrast_xv(driver_data_t* d,float val)
  {
  xv_priv_t * priv;
  priv = (xv_priv_t *)(d->priv);
  XvSetPortAttribute(d->win->dpy, priv->current_port,
                     priv->attributes[XV_ATTR_CONTRAST].atom, 
                     rescale(&priv->xv_attributes[priv->attributes[XV_ATTR_CONTRAST].index],
                             val,
                             BG_CONTRAST_MIN,
                             BG_CONTRAST_MAX));
  
  }

const video_driver_t xv_driver =
  {
    .name               = "XVideo",
    .can_scale          = 1,
    .init               = init_xv,
    .open               = open_xv,
    .create_frame       = create_frame_xv,
    .set_brightness     = set_brightness_xv,
    .set_saturation     = set_saturation_xv,
    .set_contrast       = set_contrast_xv,
    .put_frame          = put_frame_xv,
    .destroy_frame      = destroy_frame_xv,
    .close              = close_xv,
    .cleanup            = cleanup_xv,
  };
