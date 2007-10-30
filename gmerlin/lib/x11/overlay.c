#include <string.h>
#include <x11/x11.h>
#include <x11/x11_window_private.h>

int bg_x11_window_add_overlay_stream(bg_x11_window_t * w,
                                     gavl_video_format_t * format)
  {
  w->overlay_streams =
    realloc(w->overlay_streams,
            (w->num_overlay_streams+1) * sizeof(*(w->overlay_streams)));
  memset(&w->overlay_streams[w->num_overlay_streams], 0,
         sizeof(w->overlay_streams[w->num_overlay_streams]));
  
  /* Initialize */
  gavl_video_format_copy(&w->overlay_streams[w->num_overlay_streams].format,
                         format); 
  if(w->current_driver->driver->add_overlay_stream)
    {
    w->current_driver->driver->add_overlay_stream(w->current_driver);
    }
  else
    {
    w->overlay_streams[w->num_overlay_streams].ctx = gavl_overlay_blend_context_create();
    gavl_overlay_blend_context_init(w->overlay_streams[w->num_overlay_streams].ctx,
                                    &(w->video_format),
                                    &w->overlay_streams[w->num_overlay_streams].format);
    }
  
  gavl_video_format_copy(format,
                         &w->overlay_streams[w->num_overlay_streams].format); 
  
  w->num_overlay_streams++;
  return w->num_overlay_streams - 1;
  }

void bg_x11_window_set_overlay(bg_x11_window_t * w, int stream, gavl_overlay_t * ovl)
  {
  w->overlay_streams[stream].ovl = ovl;
  if(w->current_driver->driver->set_overlay)
    w->current_driver->driver->set_overlay(w->current_driver, stream, ovl);
  else
    gavl_overlay_blend_context_set_overlay(w->overlay_streams[stream].ctx, ovl);
  }

gavl_overlay_t *
bg_x11_window_create_overlay(bg_x11_window_t * w, int stream)
  {
  gavl_overlay_t * ret;
  ret = calloc(1, sizeof(*ret));
  if(w->current_driver->driver->create_overlay)
    ret->frame = w->current_driver->driver->create_overlay(w->current_driver,
                                                   stream);
  else
    ret->frame = gavl_video_frame_create(&w->overlay_streams[stream].format);
  return ret;
  }

void bg_x11_window_destroy_overlay(bg_x11_window_t * w, int stream,
                                   gavl_overlay_t * ovl)
  {
  if(w->current_driver->driver->destroy_overlay)
    w->current_driver->driver->destroy_overlay(w->current_driver,
                                               stream, ovl->frame);
  else
    gavl_video_frame_destroy(ovl->frame);
  free(ovl);
  }
