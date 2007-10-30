#include <string.h>


#include <config.h>
#include <plugin.h>
#include <translation.h>
#include <keycodes.h>
#include <utils.h>
#include <log.h>
#define LOG_DOMAIN "ov_x11"

#include <x11/x11.h>

#define SQUEEZE_MIN -1.0
#define SQUEEZE_MAX 1.0
#define SQUEEZE_DELTA 0.04

#define ZOOM_MIN 20.0
#define ZOOM_MAX 180.0
#define ZOOM_DELTA 2.0

/* Keyboard accelerators */

#define ACCEL_TOGGLE_FULLSCREEN 1<<8
#define ACCEL_EXIT_FULLSCREEN   2<<8
#define ACCEL_RESET_ZOOMSQUEEZE 3<<8
#define ACCEL_INC_ZOOM          4<<8
#define ACCEL_DEC_ZOOM          5<<8
#define ACCEL_INC_SQUEEZE       6<<8
#define ACCEL_DEC_SQUEEZE       7<<8

static bg_accelerator_t accels[] =
  {
    { BG_KEY_TAB,                 0, ACCEL_TOGGLE_FULLSCREEN  },
    { BG_KEY_f,                   0, ACCEL_TOGGLE_FULLSCREEN  },
    { BG_KEY_ESCAPE,              0, ACCEL_EXIT_FULLSCREEN    },
    { BG_KEY_HOME,   BG_KEY_CONTROL_MASK, ACCEL_RESET_ZOOMSQUEEZE },
    { BG_KEY_PLUS,   BG_KEY_CONTROL_MASK, ACCEL_INC_SQUEEZE       },
    { BG_KEY_MINUS,  BG_KEY_CONTROL_MASK, ACCEL_DEC_SQUEEZE       },
    { BG_KEY_PLUS,   BG_KEY_ALT_MASK, ACCEL_INC_ZOOM              },
    { BG_KEY_MINUS,  BG_KEY_ALT_MASK, ACCEL_DEC_ZOOM              },
    { BG_KEY_NONE,   0,           0                           },
  };


typedef struct
  {
  bg_x11_window_t * win;
  int window_created;

  int is_open;
  int fullscreen;
  
  bg_parameter_info_t * parameters;
  char * window_id;
  
  /* Callbacks, we provide for the window */ 
  bg_x11_window_callbacks_t window_callbacks;

  /* Callbacks we call in the core */
  bg_ov_callbacks_t *callbacks;

  /* Zoom/Squeeze */
  float zoom;
  float squeeze;

  /* Width/Height of the window */
  int window_width, window_height;

  /* Currently played video format */
  gavl_video_format_t video_format;
  
  /* Drawing coords */
  gavl_rectangle_f_t src_rect_f;
  gavl_rectangle_i_t dst_rect;
  
  /* Format with updated window size */
  gavl_video_format_t window_format;
  
  /* Still image stuff */
  int do_still;
  gavl_video_frame_t * still_frame;
  

  /* Accelerator map */

  bg_accelerator_map_t * accel_map;
  
  } x11_t;

/* Utility functions */

#define PADD_IMAGE_SIZE(s) s = ((s + 15) / 16) * 16

static void set_drawing_coords(x11_t * priv)
  {
  float zoom_factor;
  float squeeze_factor;
  //  
  
  zoom_factor = priv->zoom * 0.01;
  squeeze_factor = priv->squeeze;
  
  priv->window_format.image_width = priv->window_width;
  priv->window_format.image_height = priv->window_height;
  
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
  
  bg_x11_window_set_rectangles(priv->win, &priv->src_rect_f, &priv->dst_rect);
  }

static void ensure_window(x11_t * priv)
  {
  if(!priv->win)
    {
    priv->win = bg_x11_window_create(priv->window_id);
    bg_x11_window_set_callbacks(priv->win, &priv->window_callbacks);
    }
  }

static void ensure_window_realized(x11_t * priv)
  {
  ensure_window(priv);
  if(!priv->window_created)
    {
    bg_x11_window_create_window(priv->win);
    priv->window_created = 1;
    }
  }
  
/* Callbacks */


static void accel_callback(void * data, int id)
  {
  x11_t * priv;
  priv = (x11_t *)data;
  switch(id)
    {
    case ACCEL_TOGGLE_FULLSCREEN:
      if(priv->fullscreen)
        bg_x11_window_set_fullscreen(priv->win, 0);
      else
        bg_x11_window_set_fullscreen(priv->win, 1);
      break;
    case ACCEL_EXIT_FULLSCREEN:
      if(priv->fullscreen)
        bg_x11_window_set_fullscreen(priv->win, 0);
      break;
    case ACCEL_RESET_ZOOMSQUEEZE:
      priv->zoom = 100.0;
      priv->squeeze = 0.0;
      set_drawing_coords(priv);
      return;
      break;
    case ACCEL_INC_ZOOM:
      /* Increase Zoom */
      priv->zoom += ZOOM_DELTA;
      if(priv->zoom > ZOOM_MAX)
        priv->zoom = ZOOM_MAX;
      set_drawing_coords(priv);
      break;
    case ACCEL_DEC_ZOOM:
      /* Decrease Zoom */
      priv->zoom -= ZOOM_DELTA;
      if(priv->zoom < ZOOM_MIN)
        priv->zoom = ZOOM_MIN;
      set_drawing_coords(priv);
      break;
    case ACCEL_INC_SQUEEZE:
      /* Increase Squeeze */
      priv->squeeze += SQUEEZE_DELTA;
      if(priv->squeeze > SQUEEZE_MAX)
        priv->squeeze = SQUEEZE_MAX;
      set_drawing_coords(priv);
      break;
    case ACCEL_DEC_SQUEEZE:
      /* Decrease Squeeze */
      priv->squeeze -= SQUEEZE_DELTA;
      if(priv->squeeze < SQUEEZE_MIN)
        priv->squeeze = SQUEEZE_MIN;
      set_drawing_coords(priv);
      break;
    default: // Propagate to outside
      if(priv->callbacks && priv->callbacks->accel_callback)
        priv->callbacks->accel_callback(priv->callbacks->data, id);
    }
  }

static int key_callback(void * data, int key, int mask)
  {
  x11_t * priv;
  priv = (x11_t *)data;

  /* Handle some keys here */
  switch(key)
    {
    case BG_KEY_TAB:
    case BG_KEY_F:
      if(priv->fullscreen)
        return bg_x11_window_set_fullscreen(priv->win, 0);
      else
        return bg_x11_window_set_fullscreen(priv->win, 1);
      break;
    case BG_KEY_ESCAPE:
      if(priv->fullscreen)
        return bg_x11_window_set_fullscreen(priv->win, 0);
      break;
    case BG_KEY_HOME:
      priv->zoom = 100.0;
      priv->squeeze = 0.0;
      set_drawing_coords(priv);
      return 1;
      break;
    case BG_KEY_PLUS:
      if(mask & BG_KEY_ALT_MASK)
        {
        /* Increase Zoom */
        priv->zoom += ZOOM_DELTA;
        if(priv->zoom > ZOOM_MAX)
          priv->zoom = ZOOM_MAX;
        set_drawing_coords(priv);
        return 1;
        }
      else if(mask & BG_KEY_CONTROL_MASK)
        {
        /* Increase Squeeze */
        priv->squeeze += SQUEEZE_DELTA;
        if(priv->squeeze > SQUEEZE_MAX)
          priv->squeeze = SQUEEZE_MAX;
        set_drawing_coords(priv);
        return 1;
        }
      break;
    case BG_KEY_MINUS:
      if(mask & BG_KEY_ALT_MASK)
        {
        /* Decrease Zoom */
        priv->zoom -= ZOOM_DELTA;
        if(priv->zoom < ZOOM_MIN)
          priv->zoom = ZOOM_MIN;
        set_drawing_coords(priv);
        return 1;
        }
      else if(mask & BG_KEY_CONTROL_MASK)
        {
        /* Decrease Squeeze */
        priv->squeeze -= SQUEEZE_DELTA;
        if(priv->squeeze < SQUEEZE_MIN)
          priv->squeeze = SQUEEZE_MIN;
        set_drawing_coords(priv);
        return 1;
        }
      break;
    }
#if 0
  if(priv->callbacks->key_callback)
    {
    priv->callbacks->key_callback(priv->callbacks->data, key, mask);
    return 1;
    }
#endif
  return 0;
  }

static void set_fullscreen(void * data, int fullscreen)
  {
  x11_t * priv;
  priv = (x11_t *)data;
  priv->fullscreen = fullscreen;
  }

static int button_callback(void * data, int x, int y, int button, int mask)
  {
  x11_t * priv;
  priv = (x11_t *)data;
  switch(button)
    {
    case 4:
      if((mask & BG_KEY_ALT_MASK) == BG_KEY_ALT_MASK)
        {
        /* Increase Zoom */
        priv->zoom += ZOOM_DELTA;
        if(priv->zoom > ZOOM_MAX)
          priv->zoom = ZOOM_MAX;
        set_drawing_coords(priv);
        return 1;
        }
      else if((mask & BG_KEY_CONTROL_MASK) == BG_KEY_CONTROL_MASK)
        {
        /* Increase Squeeze */
        priv->squeeze += SQUEEZE_DELTA;
        if(priv->squeeze > SQUEEZE_MAX)
          priv->squeeze = SQUEEZE_MAX;
        set_drawing_coords(priv);
        return 1;
        }
      break;
    case 5:
      if((mask & BG_KEY_ALT_MASK) == BG_KEY_ALT_MASK)
        {
        /* Decrease Zoom */
        priv->zoom -= ZOOM_DELTA;
        if(priv->zoom < ZOOM_MIN)
          priv->zoom = ZOOM_MIN;
        set_drawing_coords(priv);
        return 1;
        }
      else if((mask & BG_KEY_CONTROL_MASK) == BG_KEY_CONTROL_MASK)
        {
        /* Decrease Squeeze */
        priv->squeeze -= SQUEEZE_DELTA;
        if(priv->squeeze < SQUEEZE_MIN)
          priv->squeeze = SQUEEZE_MIN;
        set_drawing_coords(priv);
        return 1;
        }
      break;
    }
  /* Check for plugin callback*/
  if(priv->callbacks && priv->callbacks->button_callback)
    {
    

    priv->callbacks->button_callback(priv->callbacks->data,
                                     x, y, button, mask);
    return 1;
    }
  /* Propagate to parent */
  return 0;
  }

static void size_changed(void * data, int width, int height)
  {
  x11_t * priv;
  priv = (x11_t *)data;
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Got window size: %d %d",
         width, height);
  priv->window_width  = width;
  priv->window_height = height;
  if(priv->is_open)
    set_drawing_coords(priv);
  }

/* For updating OSD */
static void brightness_callback(void * data, float val)
  {
  x11_t * priv;
  priv = (x11_t *)data;
  if(priv->callbacks && priv->callbacks->brightness_callback)
    priv->callbacks->brightness_callback(priv->callbacks->data,
                                         val);
  }

static void saturation_callback(void * data, float val)
  {
  x11_t * priv;
  priv = (x11_t *)data;
  if(priv->callbacks && priv->callbacks->saturation_callback)
    priv->callbacks->saturation_callback(priv->callbacks->data,
                                         val);
 
  }

static void contrast_callback(void * data, float val)
  {
  x11_t * priv;
  priv = (x11_t *)data;
  if(priv->callbacks && priv->callbacks->contrast_callback)
    priv->callbacks->contrast_callback(priv->callbacks->data,
                                       val);
  }


static void * create_x11()
  {
  x11_t * priv;
  
  priv = calloc(1, sizeof(x11_t));
  priv->accel_map = bg_accelerator_map_create();
  
  bg_accelerator_map_append_array(priv->accel_map,
                                  accels);
  
  /* Initialize callbacks */
  
  //  priv->window_callbacks.key_callback = key_callback;
  priv->window_callbacks.accel_callback = accel_callback;
  priv->window_callbacks.accel_map = priv->accel_map;
  priv->window_callbacks.button_callback = button_callback;
  priv->window_callbacks.size_changed = size_changed;
  priv->window_callbacks.set_fullscreen = set_fullscreen;
  
  /* For updating OSD */
  priv->window_callbacks.brightness_callback = brightness_callback;
  priv->window_callbacks.saturation_callback = saturation_callback;
  priv->window_callbacks.contrast_callback = contrast_callback;
  
  priv->window_callbacks.data = priv;
  
  return priv;
  }

static void destroy_x11(void * data)
  {
  x11_t * priv = (x11_t*)data;
  
  if(priv->parameters)
    bg_parameter_info_destroy_array(priv->parameters);

  if(priv->win)
    bg_x11_window_destroy(priv->win);
  
  if(priv->window_id)
    free(priv->window_id);
  
  bg_accelerator_map_destroy(priv->accel_map);
  free(priv);
  }

bg_parameter_info_t common_parameters[] =
  {
    {
      BG_LOCALE,
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
    { /* End of parameters */ }
  };

static void create_parameters(x11_t * priv)
  {
  bg_parameter_info_t * parameters[3];

  ensure_window(priv);
  parameters[0] = bg_x11_window_get_parameters(priv->win);
  parameters[1] = common_parameters;
  parameters[2] = (bg_parameter_info_t *)0;

  priv->parameters = bg_parameter_info_concat_arrays(parameters);
  }

static bg_parameter_info_t * get_parameters_x11(void * data)
  {
  x11_t * priv = (x11_t*)data;
  if(!priv->parameters)
    create_parameters(priv);
  return priv->parameters;
  }

static void set_parameter_x11(void * data,
                              const char * name,
                              const bg_parameter_value_t * val)
  {
  x11_t * priv = (x11_t*)data;
  ensure_window(priv);

  if(!name)
    {
    bg_x11_window_set_parameter(priv->win, name, val);
    }
  else if(!strcmp(name, "squeeze"))
    {
    priv->squeeze = val->val_f;
    if(priv->is_open)
      set_drawing_coords(priv);
    }
  else if(!strcmp(name, "zoom"))
    {
    priv->zoom = val->val_f;
    if(priv->is_open)
      set_drawing_coords(priv);
    }
  else
    bg_x11_window_set_parameter(priv->win, name, val);
  }

static int get_parameter_x11(void * data, const char * name,
                             bg_parameter_value_t * val)
  {
  x11_t * priv = (x11_t*)data;

  if(!name)
    return 0;
  else if(!strcmp(name, "zoom"))
    {
    val->val_f = priv->zoom;
    return 1;
    }
  else if(!strcmp(name, "squeeze"))
    {
    val->val_f = priv->squeeze;
    return 1;
    }
  else
    return bg_x11_window_get_parameter(priv->win, name, val);
  }

static void set_callbacks_x11(void * data, bg_ov_callbacks_t * callbacks)
  {
  x11_t * priv = (x11_t*)data;
  priv->callbacks = callbacks;
  if(priv->callbacks && priv->callbacks->accel_map)
    bg_accelerator_map_append_array(priv->accel_map,
                                    bg_accelerator_map_get_accels(priv->callbacks->accel_map));
  }

static void set_window_x11(void * data, const char * window_id)
  {
  x11_t * priv = (x11_t*)data;
  priv->window_id = bg_strdup(priv->window_id, window_id);
  }

static const char * get_window_x11(void * data)
  {
  x11_t * priv = (x11_t*)data;
  return bg_x11_window_get_display_string(priv->win);
  }

static void set_window_title_x11(void * data, const char * title)
  {
  x11_t * priv = (x11_t*)data;
  bg_x11_window_set_title(priv->win, title);
  }

static int open_x11(void * data, gavl_video_format_t * format)
  {
  x11_t * priv = (x11_t*)data;
  int result;
  ensure_window_realized(priv);

  result = bg_x11_window_open_video(priv->win, format);
  gavl_video_format_copy(&priv->video_format, format);
  gavl_video_format_copy(&priv->window_format, format);
  
  priv->is_open = 1;
  
  set_drawing_coords(priv);
  return result;
  }

static gavl_video_frame_t * create_frame_x11(void * data)
  {
  x11_t * priv = (x11_t*)data;
  return bg_x11_window_create_frame(priv->win);
  }

static void destroy_frame_x11(void * data, gavl_video_frame_t * frame)
  {
  x11_t * priv = (x11_t*)data;
  bg_x11_window_destroy_frame(priv->win, frame);
  }


static int
add_overlay_stream_x11(void * data, gavl_video_format_t * format)
  {
  x11_t * priv = (x11_t*)data;
  /* Realloc */
  return bg_x11_window_add_overlay_stream(priv->win, format);
  }

static void set_overlay_x11(void * data, int stream, gavl_overlay_t * ovl)
  {
  x11_t * priv = (x11_t*)data;
  bg_x11_window_set_overlay(priv->win, stream, ovl);
  }


static void put_video_x11(void * data, gavl_video_frame_t * frame)
  {
  x11_t * priv = (x11_t*)data;
  bg_x11_window_put_frame(priv->win, frame);
  }

static void put_still_x11(void * data, gavl_video_frame_t * frame)
  {
  x11_t * priv = (x11_t*)data;
  bg_x11_window_put_frame(priv->win, frame);
  
  }

static void handle_events_x11(void * data)
  {
  x11_t * priv = (x11_t*)data;
  bg_x11_window_handle_events(priv->win, 0);
  }

static void close_x11(void * data)
  {
  x11_t * priv = (x11_t*)data;
  if(priv->is_open)
    {
    priv->is_open = 0;
    bg_x11_window_close_video(priv->win);
    }
  }

static void update_aspect_x11(void * data, int pixel_width,
                              int pixel_height)
  {
  
  }

static void show_window_x11(void * data, int show)
  {
  x11_t * priv = (x11_t*)data;
  ensure_window_realized(priv);
  bg_x11_window_show(priv->win, show);
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
      flags:         BG_PLUGIN_PLAYBACK | BG_PLUGIN_EMBED_WINDOW,
      priority:      BG_PLUGIN_PRIORITY_MAX,
      mimetypes:     (char*)0,
      extensions:    (char*)0,
      create:        create_x11,
      destroy:       destroy_x11,

      get_parameters: get_parameters_x11,
      set_parameter:  set_parameter_x11,
      get_parameter:  get_parameter_x11
    },
    set_callbacks:  set_callbacks_x11,
    
    set_window:         set_window_x11,
    get_window:         get_window_x11,
    set_window_title:   set_window_title_x11,
    open:               open_x11,
    create_frame:    create_frame_x11,

    add_overlay_stream: add_overlay_stream_x11,
    set_overlay:        set_overlay_x11,

    put_video:          put_video_x11,
    put_still:          put_still_x11,

    handle_events:  handle_events_x11,
    
    destroy_frame:     destroy_frame_x11,
    close:          close_x11,
    update_aspect:  update_aspect_x11,
    
    show_window:    show_window_x11
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
