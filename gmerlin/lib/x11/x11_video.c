/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <string.h>
#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "x11_video"

static video_driver_t const * const drivers[] =
  {
    &ximage_driver,
#ifdef HAVE_LIBXV
    &xv_driver,
#endif
#ifdef HAVE_GLX
    &gl_driver,
#endif
  };

static gavl_video_frame_t * create_frame(bg_x11_window_t * w)
  {
  if(!TEST_FLAG(w, FLAG_DO_SW_SCALE) &&
     w->current_driver->driver->create_frame)
    return w->current_driver->driver->create_frame(w->current_driver);
  else
    return gavl_video_frame_create(&w->video_format);
  }

static void destroy_frame(bg_x11_window_t * w, gavl_video_frame_t * f)
  {
  if(!TEST_FLAG(w, FLAG_DO_SW_SCALE) &&
     w->current_driver->driver->destroy_frame)
    w->current_driver->driver->destroy_frame(w->current_driver, f);
  else
    gavl_video_frame_destroy(f);
  }

static int set_brightness(bg_x11_window_t * w)
  {
  if(TEST_FLAG(w, FLAG_VIDEO_OPEN) &&
     w->current_driver->driver->set_brightness &&
     (w->current_driver->flags & DRIVER_FLAG_BRIGHTNESS))
    {
    w->current_driver->driver->set_brightness(w->current_driver, w->brightness);
    return 1;
    }
  return 0;
  }

int bg_x11_window_set_brightness(bg_x11_window_t * w, float val)
  {
  w->brightness = val;
  return set_brightness(w);
  }

static int set_saturation(bg_x11_window_t * w)
  {
  if(TEST_FLAG(w, FLAG_VIDEO_OPEN) &&
     w->current_driver->driver->set_saturation &&
     (w->current_driver->flags & DRIVER_FLAG_SATURATION))
    {
    w->current_driver->driver->set_saturation(w->current_driver, w->saturation);
    return 1;
    }
  return 0;
  }

int bg_x11_window_set_saturation(bg_x11_window_t * w, float val)
  {
  w->saturation = val;
  return set_saturation(w);
  }

static int set_contrast(bg_x11_window_t * w)
  {
  if(TEST_FLAG(w, FLAG_VIDEO_OPEN) &&
     w->current_driver->driver->set_contrast &&
     (w->current_driver->flags & DRIVER_FLAG_CONTRAST))
    {
    w->current_driver->driver->set_contrast(w->current_driver, w->contrast);
    return 1;
    }
  return 0;
  }

int bg_x11_window_set_contrast(bg_x11_window_t * w, float val)
  {
  w->contrast = val;
  return set_contrast(w);
  }

static void init(bg_x11_window_t * w)
  {
  int num_drivers, i;
  num_drivers = sizeof(drivers) / sizeof(drivers[0]);
  
  for(i = 0; i < num_drivers; i++)
    {
    w->drivers[i].driver = drivers[i];
    w->drivers[i].win = w;
    if(w->drivers[i].driver->init)
      w->drivers[i].driver->init(&w->drivers[i]);
    }
  
  /*
   *  Possible TODO: Get screen resolution
   *  (maybe better if we don't care at all)
   */
  
  w->window_format.pixel_width = 1;
  w->window_format.pixel_height = 1;
  w->idle_counter = 0;
  }

void bg_x11_window_cleanup_video(bg_x11_window_t * w)
  {
  int num_drivers, i;
  num_drivers = sizeof(drivers) / sizeof(drivers[0]);

  /* Not initialized */
  if(!w->drivers[0].driver)
    return;
  
  for(i = 0; i < num_drivers; i++)
    {
    if(w->drivers[i].driver->cleanup)
      w->drivers[i].driver->cleanup(&w->drivers[i]);
    if(w->drivers[i].pixelformats)
      free(w->drivers[i].pixelformats);
    }
  }


/* For Video output */


static gavl_video_frame_t * get_frame(void * priv)
  {
  bg_x11_window_t * w = priv;
  if(!w->frame)
    w->frame = create_frame(w);
  return w->frame;
  }

#if 0
void bg_x11_window_put_frame(bg_x11_window_t * w, gavl_video_frame_t * f)
  {
  CLEAR_FLAG(w, FLAG_STILL_MODE);
  bg_x11_window_put_frame_internal(w, f);
  }

void bg_x11_window_put_still(bg_x11_window_t * w, gavl_video_frame_t * f)
  {
  SET_FLAG(w, FLAG_STILL_MODE);
  if(!w->still_frame)
    {
    w->still_frame = bg_x11_window_create_frame(w);
    }
  gavl_video_frame_copy(&w->video_format, w->still_frame, f);
  bg_x11_window_put_frame_internal(w, w->still_frame);
  }
#endif

static gavl_sink_status_t put_frame(void * priv, gavl_video_frame_t * f)
  {
  bg_x11_window_t * w = priv;
  if(f->duration >= 0)
    {
    CLEAR_FLAG(w, FLAG_STILL_MODE);
    bg_x11_window_put_frame_internal(w, f);
    }
  else
    {
    SET_FLAG(w, FLAG_STILL_MODE);

    if(!w->still_frame)
      w->still_frame = create_frame(w);
    gavl_video_frame_copy(&w->video_format, w->still_frame, f);
    bg_x11_window_put_frame_internal(w, w->still_frame);
    }
  
  return GAVL_SINK_OK;
  }

#define PAD_SIZE 16
#define PAD(sz) (((sz+PAD_SIZE-1) / PAD_SIZE) * PAD_SIZE)

int bg_x11_window_open_video(bg_x11_window_t * w,
                             gavl_video_format_t * format)
  {
  int num_drivers;
  int i;
  int force_hw_scale;
  int min_penalty, min_index;
  gavl_video_sink_get_func get_func;
  
  CLEAR_FLAG(w, FLAG_DO_SW_SCALE);
  w->current_driver = NULL;  
  if(!TEST_FLAG(w, FLAG_DRIVERS_INITIALIZED))
    {
    init(w);
    SET_FLAG(w, FLAG_DRIVERS_INITIALIZED);
    }
  
  gavl_video_format_copy(&w->video_format, format);

  /* Pad sizes, which might screw up some drivers */

  w->video_format.frame_width = PAD(w->video_format.frame_width);
  w->video_format.frame_height = PAD(w->video_format.frame_height);
  
  if(TEST_FLAG(w, FLAG_AUTO_RESIZE))
    {
    bg_x11_window_resize(w,
                         (w->video_format.image_width *
                          w->video_format.pixel_width) /
                         w->video_format.pixel_height,
                         w->video_format.image_height);
    }
  
  num_drivers = sizeof(drivers) / sizeof(drivers[0]);

  /* Query the best pixelformats for each driver */

  for(i = 0; i < num_drivers; i++)
    {
    w->drivers[i].pixelformat =
      gavl_pixelformat_get_best(format->pixelformat, w->drivers[i].pixelformats,
                                &w->drivers[i].penalty);
    }


  /*
   *  Now, get the driver with the lowest penalty.
   *  Scaling would be nice as well
   */
  force_hw_scale = !!TEST_FLAG(w, FLAG_FORCE_HW_SCALE);
  
  while(1)
    {
    min_index = -1;
    min_penalty = -1;

    /* Find out best driver. Drivers, which don't initialize,
       have the pixelformat unset */
    for(i = 0; i < num_drivers; i++)
      {
      if(!w->drivers[i].driver->can_scale && force_hw_scale)
        continue;
      if(w->drivers[i].pixelformat != GAVL_PIXELFORMAT_NONE)
        {
        if((min_penalty < 0) || w->drivers[i].penalty < min_penalty)
          {
          min_penalty = w->drivers[i].penalty;
          min_index = i;
          }
        }
      }

    /* If we have found no driver, playing video is doomed to failure */
    if(min_penalty < 0)
      {
      if(force_hw_scale)
        {
        force_hw_scale = 0;
        continue;
        }
      else
        break;
      }

    
    if(!w->drivers[min_index].driver->open)
      {
      w->current_driver = &w->drivers[min_index];
      break;
      }
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Trying %s driver",
           w->drivers[min_index].driver->name);
    if(!w->drivers[min_index].driver->open(&w->drivers[min_index]))
      {
      bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Opening %s driver failed",
             w->drivers[min_index].driver->name);
      
      /* Flag as unusable */
      w->drivers[min_index].pixelformat = GAVL_PIXELFORMAT_NONE;
      }
    else
      {
      w->current_driver = &w->drivers[min_index];
      bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Opening %s driver succeeded",
             w->drivers[min_index].driver->name);
      break;
      }
    }
  if(!w->current_driver)
    return 0;
  
  w->video_format.pixelformat = w->current_driver->pixelformat;
  
  gavl_video_format_copy(format, &w->video_format);
  
  /* All other values are already set or will be set by set_rectangles */
  w->window_format.pixelformat = format->pixelformat;
  
  if(!w->current_driver->driver->can_scale)
    SET_FLAG(w, FLAG_DO_SW_SCALE);
  
  set_contrast(w);
  set_saturation(w);
  set_brightness(w);

  XSync(w->dpy, False);
  bg_x11_window_handle_events(w, 0);

  if(TEST_FLAG(w, FLAG_DO_SW_SCALE) ||
     !w->current_driver->driver->create_frame)
    get_func = NULL;
  else
    get_func = get_frame;
  
  w->sink =
    gavl_video_sink_create(TEST_FLAG(w, FLAG_DO_SW_SCALE) ? NULL : get_frame,
                           put_frame, w, &w->window_format);
  
  return 1;
  }

gavl_video_sink_t * bg_x11_window_get_sink(bg_x11_window_t * w)
  {
  return w->sink;
  }


#define PADD_SIZE 128
#define PAD_IMAGE_SIZE(s) \
s = ((s + PADD_SIZE - 1) / PADD_SIZE) * PADD_SIZE

void bg_x11_window_set_rectangles(bg_x11_window_t * w,
                                  gavl_rectangle_f_t * src_rect,
                                  gavl_rectangle_i_t * dst_rect)
  {
  gavl_video_options_t * opt;
  gavl_rectangle_f_copy(&w->src_rect, src_rect);
  gavl_rectangle_i_copy(&w->dst_rect, dst_rect);
  
  if(!TEST_FLAG(w, FLAG_VIDEO_OPEN))
    SET_FLAG(w, FLAG_VIDEO_OPEN);
  
  if(w->current_driver && TEST_FLAG(w, FLAG_DO_SW_SCALE))
    {

    if((w->window_format.image_width > w->window_format.frame_width) ||
       (w->window_format.image_height > w->window_format.frame_height))
      {
      w->window_format.frame_width = w->window_format.image_width;
      w->window_format.frame_height = w->window_format.image_height;
      
      PAD_IMAGE_SIZE(w->window_format.frame_width);
      PAD_IMAGE_SIZE(w->window_format.frame_height);
      
      if(w->window_frame)
        {
        if(w->current_driver->driver->destroy_frame)
          w->current_driver->driver->destroy_frame(w->current_driver,
                                                   w->window_frame);
        else
          gavl_video_frame_destroy(w->window_frame);
        w->window_frame = NULL;
        }
      }

    if(!w->window_frame)
      {
      if(w->current_driver->driver->create_frame)
        w->window_frame = w->current_driver->driver->create_frame(w->current_driver);
      else
        w->window_frame = gavl_video_frame_create(&w->window_format);
      }
    
    /* Clear window */
    gavl_video_frame_clear(w->window_frame, &w->window_format);
    
    /* Reinitialize scaler */
    
    opt = gavl_video_scaler_get_options(w->scaler);
    gavl_video_options_set_rectangles(opt, &w->src_rect,
                                      &w->dst_rect);
    
    gavl_video_scaler_init(w->scaler,
                           &w->video_format,
                           &w->window_format); 
    }
  bg_x11_window_clear(w);
  }

#undef PAD_IMAGE_SIZE

void bg_x11_window_put_frame_internal(bg_x11_window_t * w,
                                      gavl_video_frame_t * f)
  {
  if(TEST_FLAG(w, FLAG_DO_SW_SCALE))
    {
    gavl_video_scaler_scale(w->scaler, f, w->window_frame);
    w->current_driver->driver->put_frame(w->current_driver,
                                         w->window_frame);
    }
  else
    w->current_driver->driver->put_frame(w->current_driver, f);

  CLEAR_FLAG(w, FLAG_OVERLAY_CHANGED);
  }



void bg_x11_window_close_video(bg_x11_window_t * w)
  {
  int i;
  if(w->window_frame)
    {
    if(w->current_driver->driver->destroy_frame)
      w->current_driver->driver->destroy_frame(w->current_driver, w->window_frame);
    else
      gavl_video_frame_destroy(w->window_frame);
    w->window_frame = NULL;
    }

  if(w->still_frame)
    {
    destroy_frame(w, w->still_frame);
    w->still_frame = NULL;
    }

  if(w->frame)
    {
    destroy_frame(w, w->frame);
    w->frame = NULL;
    }

  for(i = 0; i < w->num_overlay_streams; i++)
    {
    overlay_stream_t * str = w->overlay_streams + i;
    if(str->ovl)
      {
      if(w->current_driver->driver->destroy_overlay)
        w->current_driver->driver->destroy_overlay(w->current_driver, str, str->ovl);
      else
        gavl_video_frame_destroy(str->ovl);
      }
    if(str->sink)
      gavl_video_sink_destroy(str->sink);
    }

  if(w->sink)
    {
    gavl_video_sink_destroy(w->sink);
    w->sink = NULL;
    }
  if(w->current_driver->driver->close)
    w->current_driver->driver->close(w->current_driver);
  
  if(w->overlay_streams)
    {
    free(w->overlay_streams);
    w->num_overlay_streams = 0;
    w->overlay_streams = NULL;
    }
  
  CLEAR_FLAG(w, FLAG_VIDEO_OPEN);
  
  XSync(w->dpy, False);
  bg_x11_window_handle_events(w, 0);
  }

