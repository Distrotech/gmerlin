
#include <string.h>

#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <keycodes.h>

#include <X11/keysym.h>

#define IDLE_MAX 10

#define STATE_IGNORE ~(Mod2Mask)

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
    { XK_Home,  BG_KEY_HOME },
    { XK_plus,  BG_KEY_PLUS },
    { XK_minus, BG_KEY_MINUS },
    { XK_Tab,   BG_KEY_TAB },
    { XK_Escape,   BG_KEY_ESCAPE },
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
    { XK_w,        BG_KEY_W },
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


static int x11_window_next_event(bg_x11_window_t * w,
                                 XEvent * evt,
                                 int milliseconds)
  {
  int fd;
  struct timeval timeout;
  fd_set read_fds;
  if(milliseconds < 0) /* Block */
    {
    XNextEvent(w->dpy, evt);
    return 1;
    }
  else if(!milliseconds)
    {
    if(!XPending(w->dpy))
      return 0;
    else
      {
      XNextEvent(w->dpy, evt);
      return 1;
      }
    }
  else /* Use timeout */
    {
    fd = ConnectionNumber(w->dpy);
    FD_ZERO (&read_fds);
    FD_SET (fd, &read_fds);

    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = 1000 * (milliseconds % 1000);
    if(!select(fd+1, &read_fds, (fd_set*)0,(fd_set*)0,&timeout))
      return 0;
    else
      {
      XNextEvent(w->dpy, evt);
      return 1;
      }
    }
  
  }

void bg_x11_window_handle_event(bg_x11_window_t * w, XEvent * evt)
  {
  KeySym keysym;
  char key_char;
  int  key_code;
  
  int  button_number = 0;
  
  w->do_delete = 0;
  
  if(!evt || (evt->type != MotionNotify))
    {
    w->idle_counter++;
    if(w->idle_counter == IDLE_MAX)
      {
      if(!w->pointer_hidden)
        {
        if(w->normal_child == None)
          XDefineCursor(w->dpy, w->normal_window, w->fullscreen_cursor);
        if(w->fullscreen_child == None)
          XDefineCursor(w->dpy, w->fullscreen_window, w->fullscreen_cursor);
        XFlush(w->dpy);
        w->pointer_hidden = 1;
        }

      if(w->screensaver_disabled)
        bg_x11_window_ping_screensaver(w);
      w->idle_counter = 0;
      }
    }
  if(!evt)
    return;

  if(evt->type == w->shm_completion_type)
    {
    w->wait_for_completion = 0;
    return;
    }
    
  switch(evt->type)
    {
    case CreateNotify:
      if(evt->xcreatewindow.parent == w->normal_window)
        {
        w->normal_child = evt->xcreatewindow.window;
        XDefineCursor(w->dpy, w->normal_window, None);
        }
      if(evt->xcreatewindow.parent == w->fullscreen_window)
        {
        w->fullscreen_child = evt->xcreatewindow.window;
        XDefineCursor(w->dpy, w->fullscreen_window, None);
        }
      /* We need to catch expose events by children */
      XSelectInput(w->dpy, evt->xcreatewindow.window,
                   ExposureMask);
      break;
    case DestroyNotify:
      if(evt->xdestroywindow.event == w->normal_window)
        {
        w->normal_child = None;
        XDefineCursor(w->dpy, w->normal_window, w->fullscreen_cursor);
        }
      if(evt->xdestroywindow.event == w->fullscreen_window)
        {
        w->fullscreen_child = None;
        XDefineCursor(w->dpy, w->fullscreen_window,
                      w->fullscreen_cursor);
        }
      break;
    case ConfigureNotify:
      if(w->current_window == w->normal_window)
        {
        if(evt->xconfigure.window == w->normal_window)
          {
          w->window_width  = evt->xconfigure.width;
          w->window_height = evt->xconfigure.height;
          
          if(w->normal_parent == w->root)
            {
            bg_x11_window_get_coords(w,
                                  w->normal_window,
                                  &w->window_x, &w->window_y,
                                  (int*)0, (int*)0);
            }
          bg_x11_window_size_changed(w);
          }
        else if(evt->xconfigure.window == w->normal_parent)
          {
          XResizeWindow(w->dpy,
                        w->normal_window,
                        evt->xconfigure.width,
                        evt->xconfigure.height);
          }
        }
      else
        {
        if(evt->xconfigure.window == w->fullscreen_window)
          {
          w->window_width  = evt->xconfigure.width;
          w->window_height = evt->xconfigure.height;
          bg_x11_window_size_changed(w);
          }
        else if(evt->xconfigure.window == w->fullscreen_parent)
          {
          XResizeWindow(w->dpy,
                        w->fullscreen_window,
                        evt->xconfigure.width,
                        evt->xconfigure.height);
          }
        }
      break;
    case MotionNotify:
      w->idle_counter = 0;
      if(w->pointer_hidden)
        {
        XDefineCursor(w->dpy, w->normal_window, None);
        XDefineCursor(w->dpy, w->fullscreen_window, None);
        XFlush(w->dpy);
        w->pointer_hidden = 0;
        }
    case ClientMessage:
      if((evt->xclient.message_type == w->WM_PROTOCOLS) &&
         (evt->xclient.data.l[0] == w->WM_DELETE_WINDOW))
        {
        w->do_delete = 1;
        }
      break;
    case UnmapNotify:
      if(evt->xunmap.window == w->normal_window)
        w->mapped = 0;
      break;
    case MapNotify:
      if(evt->xmap.window == w->normal_window)
        w->mapped = 1;
      else if((evt->xmap.window == w->normal_parent) ||
              (evt->xmap.window == w->fullscreen_parent))
        {
        bg_x11_window_init(w);
        }
      break;
    case KeyPress:
      XLookupString(&(evt->xkey), &key_char, 1, &keysym, NULL);
      evt->xkey.state &= STATE_IGNORE;

      key_code = get_key_code(keysym);
      
      if((key_code != BG_KEY_NONE) &&
         w->callbacks &&
         w->callbacks->key_callback)
        {
        if(w->callbacks->key_callback(w->callbacks->data,
                                      key_code,
                                      get_key_mask(evt->xkey.state)))
          return;
        }

      if(w->current_parent != w->root)
        {
        XKeyEvent key_event;
        /* Send event to parent window */
        memset(&key_event, 0, sizeof(key_event));
        key_event.display = w->dpy;
        key_event.window = w->current_parent;
        key_event.root = w->root;
        key_event.subwindow = None;
        key_event.time = CurrentTime;
        key_event.x = 0;
        key_event.y = 0;
        key_event.x_root = 0;
        key_event.y_root = 0;
        key_event.same_screen = True;
        
        key_event.type = KeyPress;;
        key_event.keycode = evt->xkey.keycode;
        key_event.state = evt->xkey.state;
        
        XSendEvent(key_event.display,
                   key_event.window,
                   False, KeyPressMask, (XEvent *)(&key_event));
        XFlush(w->dpy);
        }
      break;
    case ButtonPress:
      evt->xkey.state &= STATE_IGNORE;
      if(w->callbacks &&
         w->callbacks->button_callback)
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
        if(w->callbacks->button_callback(w->callbacks->data,
                                         evt->xbutton.x,
                                         evt->xbutton.y,
                                         button_number,
                                         get_key_mask(evt->xbutton.state)))
          return;
        }
      /* Send to parent */
      if(w->current_parent != w->root)
        {
        XButtonEvent button_event;
        memset(&button_event, 0, sizeof(button_event));
        button_event.type = ButtonPress;
        button_event.display = w->dpy;
        button_event.window = w->current_parent;
        button_event.root = w->root;
        button_event.time = CurrentTime;
        button_event.x = evt->xbutton.x;
        button_event.y = evt->xbutton.y;
        button_event.x_root = evt->xbutton.x_root;
        button_event.y_root = evt->xbutton.y_root;
        button_event.state  = evt->xbutton.state;
        button_event.button = evt->xbutton.button;
        button_event.same_screen = evt->xbutton.same_screen;

        XSendEvent(button_event.display,
                   button_event.window,
                   False, ButtonPressMask, (XEvent *)(&button_event));
        // XFlush(w->dpy);
        fprintf(stderr, "Propagate buttonpress\n");
        }

    }
  }

void bg_x11_window_handle_events(bg_x11_window_t * win, int milliseconds)
  {
  XEvent evt;

  if(win->wait_for_completion)
    {
    while(win->wait_for_completion)
      {
      x11_window_next_event(win, &evt, -1);
      bg_x11_window_handle_event(win, &evt);
      }
    }
  else
    {
    while(1)
      {
      if(!x11_window_next_event(win, &evt, milliseconds))
        {
        /* Still need to hide the mouse cursor and ping the screensaver */
        bg_x11_window_handle_event(win, (XEvent*)0);
        return;
        }
      bg_x11_window_handle_event(win, &evt);
      }
    }
  }
