#include <string.h>
#include <x11/x11.h>
#include <x11/x11_window_private.h>

static video_driver_t * drivers[] =
  {
    &ximage_driver,
#ifdef HAVE_LIBXV
    &xv_driver,
#endif
    &gl_driver,
  };

static int set_brightness(bg_x11_window_t * w)
  {
  if(w->video_open &&
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
  if(w->video_open &&
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
  if(w->video_open &&
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

static int set_hue(bg_x11_window_t * w)
  {
  if(w->video_open &&
     w->current_driver->driver->set_hue &&
     (w->current_driver->flags & DRIVER_FLAG_HUE))
    {
    w->current_driver->driver->set_hue(w->current_driver, w->hue);
    return 1;
    }
  return 0;
  }

int bg_x11_window_set_hue(bg_x11_window_t * w, float val)
  {
  w->hue = val;
  return set_hue(w);
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
  
  /* TODO: Get screen resolution */
  w->window_format.pixel_width = 1;
  w->window_format.pixel_height = 1;
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

int bg_x11_window_open_video(bg_x11_window_t * w,
                             gavl_video_format_t * format)
  {
  int num_drivers;
  int i;
  int force_hw_scale;
  
  int min_penalty, min_index;
  w->do_sw_scale = 0;
  w->current_driver = (driver_data_t*)0;  
  if(!w->drivers_initialized)
    {
    init(w);
    w->drivers_initialized = 1;
    }
  
  
  gavl_video_format_copy(&w->video_format, format);

  if(w->auto_resize)
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
  force_hw_scale = w->force_hw_scale;
  
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
    /* Flag as unusable */
    if(!w->drivers[min_index].driver->open(&w->drivers[min_index]))
      w->drivers[min_index].pixelformat = GAVL_PIXELFORMAT_NONE;
    else
      {
      w->current_driver = &w->drivers[min_index];
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
    {
    w->do_sw_scale = 1;
    
    }

  set_contrast(w);
  set_saturation(w);
  set_brightness(w);
  set_hue(w);
  

  return 1;
  }

gavl_video_frame_t * bg_x11_window_create_frame(bg_x11_window_t * w)
  {
  if(!w->do_sw_scale)
    {
    if(w->current_driver->driver->create_frame)
      return w->current_driver->driver->create_frame(w->current_driver);
    else
      return gavl_video_frame_create(&w->video_format);
    }
  else
    return gavl_video_frame_create(&w->video_format);
  }

void bg_x11_window_destroy_frame(bg_x11_window_t * w, gavl_video_frame_t * f)
  {
  if(!w->do_sw_scale)
    {
    if(w->current_driver->driver->destroy_frame)
      w->current_driver->driver->destroy_frame(w->current_driver, f);
    else
      gavl_video_frame_destroy(f);
    }
  else
    gavl_video_frame_destroy(f);
  }

#define PADD_SIZE 128
#define PADD_IMAGE_SIZE(s) \
s = ((s + PADD_SIZE - 1) / PADD_SIZE) * PADD_SIZE

void bg_x11_window_set_rectangles(bg_x11_window_t * w,
                                  gavl_rectangle_f_t * src_rect,
                                  gavl_rectangle_i_t * dst_rect)
  {
  gavl_video_options_t * opt;
  gavl_rectangle_f_copy(&w->src_rect, src_rect);
  gavl_rectangle_i_copy(&w->dst_rect, dst_rect);
  
  if(!w->video_open)
    w->video_open = 1;
  
  if(w->current_driver && w->do_sw_scale)
    {

    if((w->window_format.image_width > w->window_format.frame_width) ||
       (w->window_format.image_height > w->window_format.frame_height))
      {
      w->window_format.frame_width = w->window_format.image_width;
      w->window_format.frame_height = w->window_format.image_height;
      
      PADD_IMAGE_SIZE(w->window_format.frame_width);
      PADD_IMAGE_SIZE(w->window_format.frame_height);
      
      if(w->window_frame)
        {
        if(w->current_driver->driver->destroy_frame)
          w->current_driver->driver->destroy_frame(w->current_driver,
                                                   w->window_frame);
        else
          gavl_video_frame_destroy(w->window_frame);
        }
      
      if(w->current_driver->driver->create_frame)
        w->window_frame = w->current_driver->driver->create_frame(w->current_driver);
      else
        w->window_frame = gavl_video_frame_create(&w->window_format);
      }
    /* Clear window */
    gavl_video_frame_clear(w->window_frame, &(w->window_format));
    
    /* Reinitialize scaler */
    
    opt = gavl_video_scaler_get_options(w->scaler);
    gavl_video_options_set_rectangles(opt, &(w->src_rect),
                                      &(w->dst_rect));
    
    gavl_video_scaler_init(w->scaler,
                           &(w->video_format),
                           &(w->window_format)); 
    }
  bg_x11_window_clear(w);
  }

#undef PADD_IMAGE_SIZE

void bg_x11_window_put_frame(bg_x11_window_t * w, gavl_video_frame_t * f)
  {
  int i;

  if(!w->current_driver->driver->add_overlay_stream)
    {
    for(i = 0; i < w->num_overlay_streams; i++)
      gavl_overlay_blend(w->overlay_streams[i].ctx, f);
    }
  
  if(w->do_sw_scale)
    {
    gavl_video_scaler_scale(w->scaler, f, w->window_frame);
    w->current_driver->driver->put_frame(w->current_driver,
                                         w->window_frame);
    }
  else
    {
    w->current_driver->driver->put_frame(w->current_driver, f);
    }
  
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
    w->window_frame = (gavl_video_frame_t*)0;
    }
  
  if(w->overlay_streams)
    {
    for(i = 0; i < w->num_overlay_streams; i++)
      {
      if(w->overlay_streams[i].ctx)
        gavl_overlay_blend_context_destroy(w->overlay_streams[i].ctx);
      }
    free(w->overlay_streams);
    w->num_overlay_streams = 0;
    w->overlay_streams = NULL;
    }
  
  if(w->current_driver->driver->close)
    w->current_driver->driver->close(w->current_driver);
  
  w->video_open = 0;
  }

