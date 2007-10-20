/*    
 *   Lemuria, an OpenGL music visualization
 *   Copyright (C) 2002 - 2007 Burkhard Plaum
 *
 *   Lemuria is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xinerama.h>

#include <GL/glx.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <lemuria_private.h>
#include <effect.h>
#include <x11_window.h>

typedef struct
  {
  x11_window_t win;
  
  Display * display;
    
  GLXContext glxcontext;

  int screen;

  } x11_data;

// extern void lemuria_handle_xmms_event(KeySym keysym);

void lemuria_create_window(lemuria_engine_t * e, const char * embed,
                           int * width, int * height)
  {
  //  int i;
  
  x11_data * data;
  
  int attr_list[] = {
    GLX_RGBA,
    GLX_DEPTH_SIZE, 16,
    GLX_ALPHA_SIZE, 8,
    GLX_DOUBLEBUFFER,
    None
  };
  //  int i_tmp;
  XVisualInfo *vis_info;

  data = calloc(1, sizeof(x11_data));

  e->window_data = data;

  if(!x11_window_open_display(&(data->win), embed))
    return;
  
  data->screen = data->win.screen;
  data->display = data->win.dpy;
  
  vis_info = glXChooseVisual(data->display, data->screen, attr_list);
  if (!vis_info) 
    {
    fprintf(stderr, "Your OpenGL setup is not sufficient.\n");
    fprintf(stderr, "Lemuria will most likely crash now, good bye\n");
    return;
    }

  /* Print alpha bits */
#if 0
  if(glXGetConfig(data->display, vis_info,
                  GLX_ALPHA_SIZE, &i_tmp))
    fprintf(stderr, "glXGetConfig failed\n");
  else
    fprintf(stderr, "Alpha Bits: %d\n", i_tmp);
#endif

  data->win.min_width = TEXTURE_SIZE;
  data->win.min_height = TEXTURE_SIZE;

  x11_window_create(&(data->win), vis_info->visual, vis_info->depth,
                    *width, *height,
                    "Lemuria-"VERSION);
  
  x11_window_init(&(data->win));

  if(data->win.current_window == data->win.fullscreen_window)
    e->fullscreen = 1;
  else
    e->fullscreen = 0;
  
  /* Window size might be different for embedded windows */
  *width  = data->win.window_width;
  *height = data->win.window_height;
  
  x11_window_select_input(&(data->win),
                          StructureNotifyMask | KeyPressMask |
                          PointerMotionMask | ButtonPressMask |
                          ButtonReleaseMask);
  
  x11_window_set_class_hint(&(data->win),
                            "xmms","visplugin");
  
  data->glxcontext = glXCreateContext(data->display, vis_info, NULL, True);
      
  x11_window_show(&(data->win), 1);
  XFlush(data->display);
  }

void lemuria_set_glcontext(lemuria_engine_t * e)
  {
  x11_data * data = (x11_data*)(e->window_data);
  glXMakeCurrent(data->display, data->win.current_window, data->glxcontext);
  }

void lemuria_unset_glcontext(lemuria_engine_t * e)
  {
  x11_data * data = (x11_data*)(e->window_data);
  glXMakeCurrent(data->display, 0, NULL);
  }

void lemuria_destroy_window(lemuria_engine_t * e)
  {
  x11_data * data = (x11_data*)(e->window_data);
  if(data->glxcontext)
    {
    glXDestroyContext(data->display, data->glxcontext);
    data->glxcontext = NULL;
    }

  if(data->win.dpy)
    x11_window_destroy(&(data->win));
  
  if(data->display)
    {
    XCloseDisplay(data->display);
    data->display = NULL;
    }
  free(data);
  e->window_data = NULL;
  }

static void toggle_fullscreen(lemuria_engine_t * e)
  {
  x11_data * data = (x11_data*)(e->window_data);
  e->fullscreen = !e->fullscreen;
  x11_window_set_fullscreen(&(data->win), e->fullscreen);
  }

static void handle_event(lemuria_engine_t * e, XEvent * event)
  {
  /* Set to 1 if we send this event to the parent window */
  int do_send_event; 
  KeySym keysym;
  XKeyEvent key_event;
  x11_data * data = (x11_data*)(e->window_data);

  x11_window_handle_event(&(data->win), event);

  if(data->win.size_changed)
    {
    e->width = data->win.window_width;
    e->height = data->win.window_height;
    }
  
  if(!event)
    return;
    
  switch(event->type)
    {
    case ConfigureNotify:
      break;
    case KeyPress:
      do_send_event = 0;
      keysym = XLookupKeysym(&(event->xkey), 0);
      switch(keysym)
        {
        case XK_Tab:
          if(((data->win.normal_parent == data->win.root) &&
              (data->win.normal_window == data->win.current_window)) ||
             ((data->win.fullscreen_parent == data->win.root) &&
              (data->win.fullscreen_window == data->win.current_window)))
            {
            toggle_fullscreen(e);
            e->width = data->win.window_width;
            e->height = data->win.window_height;
            }
          else
            do_send_event = 1;
          break;
        case XK_a:
          if(event->xkey.state & ControlMask)
            lemuria_set_foreground(e);
          else
            lemuria_next_foreground(e);
          break;
        case XK_w:
          if(event->xkey.state & ControlMask)
            lemuria_set_background(e);
          else
            lemuria_next_background(e);
          break;
        case XK_t:
          if(event->xkey.state & ControlMask)
            lemuria_set_texture(e);
          else
            lemuria_next_texture(e);
          break;
        case XK_h:
          if(e->background.frame_counter != -1)
            {
            e->background.frame_counter = -1;
            e->foreground.frame_counter = -1;
            e->texture.frame_counter    = -1;
            lemuria_put_text(e,
                             "Holding Effects",
                             0.5,
                             0.5,
                             0.0,
                             0.0,
                             2.0,
                             70);
            }
          else
            {
            e->background.frame_counter = 0;
            e->foreground.frame_counter = 0;
            e->texture.frame_counter    = 0;
            lemuria_put_text(e,
                             "Unholding Effects",
                             0.5,
                             0.5,
                             0.0,
                             0.0,
                             2.0,
                             70);
            }
          break;
        case XK_l:
          e->antialias++;
          switch(e->antialias)
            {
            case 1:
              lemuria_put_text(e,
                               "Antialiasing: Fastest",
                               0.5,
                               0.5,
                               0.0,
                               0.0,
                               2.0,
                               70);
              break;
            case 2:
              lemuria_put_text(e,
                               "Antialiasing: Best",
                               0.5,
                               0.5,
                               0.0,
                               0.0,
                               2.0,
                               70);
              break;
            case 3:
              
              e->antialias = 0;
              lemuria_put_text(e,
                               "Antialiasing: Off",
                               0.5,
                               0.5,
                               0.0,
                               0.0,
                               2.0,
                               70);
              break;
            }
          break;
        case XK_Pause:
          e->paused = !e->paused;
          fprintf(stderr, "Pause %d\n", e->paused);
          break;
        case XK_F1:
          lemuria_print_help(e);
          break;
        case XK_i:
          lemuria_print_info(e);
          break;
        default:
          do_send_event = 1;
          break;
        }

      if(do_send_event)
        {
        memset(&key_event, 0, sizeof(key_event));
        key_event.display = data->win.dpy;
        
        if(data->win.normal_window == data->win.current_window)
          key_event.window = data->win.normal_parent;
        else
          key_event.window = data->win.fullscreen_parent;
        
        key_event.root = data->win.root;
        key_event.subwindow = None;
        key_event.time = CurrentTime;
        key_event.x = 0;
        key_event.y = 0;
        key_event.x_root = 0;
        key_event.y_root = 0;
        key_event.same_screen = True;
        
        key_event.type = KeyPress;;
        key_event.keycode = event->xkey.keycode;
        key_event.state = event->xkey.state;

        XSendEvent(key_event.display,
                   key_event.window,
                   False, KeyPressMask, (XEvent *)(&key_event));
        XFlush(data->win.dpy);
        }
      break;
    }
  }

/* Process X11 events */

void lemuria_check_events(lemuria_engine_t * e)
  {
  int paused;
  XEvent * event;
  x11_data * data = (x11_data*)(e->window_data);
  
  paused = e->paused;

  if(paused)
    {
    event = x11_window_next_event(&(data->win), 100);
    handle_event(e, event);
    }
  else
    {
    while(1)
      {
      event = x11_window_next_event(&(data->win), 0);
      handle_event(e, event);
      if(!event)
        break;
      }
    }
  }

void lemuria_flash_frame(lemuria_engine_t * e)
  {
  x11_data * data = (x11_data*)(e->window_data);
  
  lemuria_set_glcontext(e);
  glXSwapBuffers(data->display, data->win.current_window);
  lemuria_unset_glcontext(e);
  }


