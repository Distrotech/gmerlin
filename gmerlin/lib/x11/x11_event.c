/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <gmerlin/keycodes.h>

#include <X11/keysym.h>
#include <sys/select.h>

#define IDLE_MAX 50

#define STATE_IGNORE ~(Mod2Mask)


static const struct
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
    { XK_Menu,     BG_KEY_MENU },
    
    { XK_question,   BG_KEY_QUESTION }, //!< ?
    { XK_exclam,     BG_KEY_EXCLAM    }, //!< !
    { XK_quotedbl,   BG_KEY_QUOTEDBL,   }, //!< "
    { XK_dollar,     BG_KEY_DOLLAR,     }, //!< $
    { XK_percent,    BG_KEY_PERCENT,    }, //!< %
    { XK_ampersand,  BG_KEY_APMERSAND,  }, //!< &
    { XK_slash,      BG_KEY_SLASH,      }, //!< /
    { XK_parenleft,  BG_KEY_LEFTPAREN,  }, //!< (
    { XK_parenright, BG_KEY_RIGHTPAREN, }, //!< )
    { XK_equal,      BG_KEY_EQUAL,      }, //!< =
    { XK_backslash,  BG_KEY_BACKSLASH,  }, //!< :-)
    
    { XK_a,        BG_KEY_a },
    { XK_b,        BG_KEY_b },
    { XK_c,        BG_KEY_c },
    { XK_d,        BG_KEY_d },
    { XK_e,        BG_KEY_e },
    { XK_f,        BG_KEY_f },
    { XK_g,        BG_KEY_g },
    { XK_h,        BG_KEY_h },
    { XK_i,        BG_KEY_i },
    { XK_j,        BG_KEY_j },
    { XK_k,        BG_KEY_k },
    { XK_l,        BG_KEY_l },
    { XK_m,        BG_KEY_m },
    { XK_n,        BG_KEY_n },
    { XK_o,        BG_KEY_o },
    { XK_p,        BG_KEY_p },
    { XK_q,        BG_KEY_q },
    { XK_r,        BG_KEY_r },
    { XK_s,        BG_KEY_s },
    { XK_t,        BG_KEY_t },
    { XK_u,        BG_KEY_u },
    { XK_v,        BG_KEY_v },
    { XK_w,        BG_KEY_w },
    { XK_x,        BG_KEY_x },
    { XK_y,        BG_KEY_y },
    { XK_z,        BG_KEY_z },

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

static int keysym_to_key_code(KeySym x11)
  {
  int i;

  for(i = 0; i < sizeof(keycodes)/sizeof(keycodes[0]); i++)
    {
    if(x11 == keycodes[i].x11)
      return keycodes[i].bg;
    }
  return BG_KEY_NONE;
  }

static KeySym key_code_to_keysym(int bg_code)
  {
  int i;

  for(i = 0; i < sizeof(keycodes)/sizeof(keycodes[0]); i++)
    {
    if(bg_code == keycodes[i].bg)
      return keycodes[i].x11;
    }
  return XK_VoidSymbol;
  }

static int x11_to_key_mask(int x11_mask)
  {
  int ret = 0;

  if(x11_mask & ShiftMask)
    ret |= BG_KEY_SHIFT_MASK;
  if(x11_mask & ControlMask)
    ret |= BG_KEY_CONTROL_MASK;
  if(x11_mask & Mod1Mask)
    ret |= BG_KEY_ALT_MASK;
  if(x11_mask & Mod4Mask)
    ret |= BG_KEY_SUPER_MASK;
  if(x11_mask & Button1Mask)
    ret |= BG_KEY_BUTTON1_MASK;
  if(x11_mask & Button2Mask)
    ret |= BG_KEY_BUTTON2_MASK;
  if(x11_mask & Button3Mask)
    ret |= BG_KEY_BUTTON3_MASK;
  if(x11_mask & Button4Mask)
    ret |= BG_KEY_BUTTON4_MASK;
  if(x11_mask & Button5Mask)
    ret |= BG_KEY_BUTTON5_MASK;
  return ret;
  }

#if 1
static int key_mask_to_xembed(int bg_mask)
  {
  int ret = 0;

  if(bg_mask & BG_KEY_SHIFT_MASK)
    ret |= XEMBED_MODIFIER_SHIFT;
  if(bg_mask & BG_KEY_CONTROL_MASK)
    ret |= XEMBED_MODIFIER_CONTROL;
  if(bg_mask & BG_KEY_ALT_MASK)
    ret |= XEMBED_MODIFIER_ALT;
  if(bg_mask & BG_KEY_SUPER_MASK)
    ret |= XEMBED_MODIFIER_SUPER;
  // XEMBED_MODIFIER_HYPER ??
  return ret;
  
  }
#endif

static int xembed_to_key_mask(int xembed_mask)
  {
  int ret = 0;

  if(xembed_mask & XEMBED_MODIFIER_SHIFT)
    ret |= BG_KEY_SHIFT_MASK;
  if(xembed_mask & XEMBED_MODIFIER_CONTROL)
    ret |= BG_KEY_CONTROL_MASK;
  if(xembed_mask & XEMBED_MODIFIER_ALT)
    ret |= BG_KEY_ALT_MASK;
  if(xembed_mask & XEMBED_MODIFIER_SUPER)
    ret |= BG_KEY_SUPER_MASK;
  // XEMBED_MODIFIER_HYPER ??
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

static int window_is_mapped(Display * dpy, Window w)
  {
  XWindowAttributes attr;
  if(w == None)
    return 0;

  XGetWindowAttributes(dpy, w, &attr);
  if(attr.map_state != IsUnmapped)
    return 1;
  return 0;
  }

static int window_is_viewable(Display * dpy, Window w)
  {
  XWindowAttributes attr;
  if(w == None)
    return 0;
  XGetWindowAttributes(dpy, w, &attr);
  if(attr.map_state == IsViewable)
    return 1;
  return 0;
  }

static void register_xembed_accelerators(bg_x11_window_t * w,
                                         window_t * win)
  {
  int i;
  const bg_accelerator_t * accels;
  
  if(!w->callbacks || !w->callbacks->accel_map)
    return;
  accels = bg_accelerator_map_get_accels(w->callbacks->accel_map);
  i = 0;
  while(accels[i].key != BG_KEY_NONE)
    {
    bg_x11_window_send_xembed_message(w, win->parent,
                                      CurrentTime,
                                      XEMBED_REGISTER_ACCELERATOR,
                                      accels[i].id,
                                      key_code_to_keysym(accels[i].key),
                                      key_mask_to_xembed(accels[i].mask));
    i++;
    }
  }
#if 0
static void unregister_xembed_accelerators(bg_x11_window_t * w,
                                           window_t * win)
  {
  int i;
  const bg_accelerator_t * accels;
  if(!w->callbacks || !w->callbacks->accel_map)
    return;
  accels = bg_accelerator_map_get_accels(w->callbacks->accel_map);
  i = 0;
  while(accels[i].key != BG_KEY_NONE)
    {
    bg_x11_window_send_xembed_message(w, win->win,
                                      CurrentTime,
                                      XEMBED_UNREGISTER_ACCELERATOR,
                                      accels[i].id, 0, 0);
    i++;
    }
  }
#endif
/* Transform coordinates if we playback video */

static void transform_coords(bg_x11_window_t * w,
                             int x_raw, int y_raw,
                             int * x_ret, int * y_ret)
  {
  double coord_norm;
  if(!w->video_open)
    {
    *x_ret = x_raw;
    *y_ret = y_raw;
    }
  else
    {
    coord_norm = (double)(x_raw - w->dst_rect.x) / w->dst_rect.w;
    *x_ret = (int)(w->src_rect.x + coord_norm * w->src_rect.w + 0.5);
    
    coord_norm = (double)(y_raw - w->dst_rect.y) / w->dst_rect.h;
    *y_ret = (int)(w->src_rect.y + coord_norm * w->src_rect.h + 0.5);
    }
  }

void bg_x11_window_handle_event(bg_x11_window_t * w, XEvent * evt)
  {
  KeySym keysym;
  char key_char;
  int  key_code;
  int  key_mask;
  int  accel_id;
  int  button_number = 0;
  window_t * cur;
  int x_src, y_src;
  w->do_delete = 0;
  
  if(!evt || (evt->type != MotionNotify))
    {
    w->idle_counter++;
    if(w->idle_counter == IDLE_MAX)
      {
      if(!w->pointer_hidden)
        {
        if(w->current->child == None)
          XDefineCursor(w->dpy, w->current->win, w->fullscreen_cursor);
        XFlush(w->dpy);
        w->pointer_hidden = 1;
        }
      w->idle_counter = 0;
      }
    }

  bg_x11_screensaver_ping(&w->scr);
  
  if(w->need_focus && window_is_viewable(w->dpy, w->current->focus_child))
    {
    XSetInputFocus(w->dpy, w->current->focus_child,
                   RevertToParent, w->focus_time);
    w->need_focus = 0;
    }

  if(w->need_fullscreen &&
     window_is_viewable(w->dpy, w->fullscreen.win))
    {
    bg_x11_window_set_fullscreen_mapped(w, &w->fullscreen);

    w->need_fullscreen = 0;
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
    case Expose:
      if(w->still_mode && w->still_frame)
        bg_x11_window_put_frame_internal(w, w->still_frame);
      break;
    case PropertyNotify:
      if(evt->xproperty.atom == w->_XEMBED_INFO)
        {
        
        if(evt->xproperty.window == w->normal.child)
          {
          bg_x11_window_check_embed_property(w,
                                             &w->normal);
          }
        else if(evt->xproperty.window == w->fullscreen.child)
          {
          bg_x11_window_check_embed_property(w,
                                             &w->fullscreen);
          }
        }
      break;
    case ClientMessage:
      if(evt->xclient.message_type == w->WM_PROTOCOLS)
        {
        if(evt->xclient.data.l[0] == w->WM_DELETE_WINDOW)
          {
          w->do_delete = 1;
          return;
          }
        else if(evt->xclient.data.l[0] == w->WM_TAKE_FOCUS)
          {
          int result;
          if(window_is_viewable(w->dpy, w->current->focus_child))
            result = XSetInputFocus(w->dpy, w->current->focus_child,
                                    RevertToParent, evt->xclient.data.l[1]);
          else
            {
            w->need_focus = 1;
            w->focus_time = evt->xclient.data.l[1];
            result = 0;
            }
          }
        }
      if(evt->xclient.message_type == w->_XEMBED)
        {

        if((evt->xclient.window == w->normal.win) ||
           (evt->xclient.window == w->normal.child))
          cur = &w->normal;
        else if((evt->xclient.window == w->fullscreen.win) ||
                (evt->xclient.window == w->fullscreen.child))
          cur = &w->fullscreen;
        else
          return;
        
        switch(evt->xclient.data.l[1])
          {
          /* XEMBED messages */
          case XEMBED_EMBEDDED_NOTIFY:
            if(evt->xclient.window == cur->win)
              {
              cur->parent_xembed = 1;
              /*
               *  In gtk this isn't necessary, didn't try other
               *  toolkits. Strange that this is sometimes called with
               *  None as parent window, so we're better off ignoring this here
               */
              // fprintf(stderr, "XEMBED_EMBEDDED_NOTIFY %08lx %08lx\n",
              // cur->parent, evt->xclient.data.l[3]);
              // cur->parent = evt->xclient.data.l[3];
              
              if(window_is_mapped(w->dpy, cur->parent))
                {
                unsigned long buffer[2];
                                
                buffer[0] = 0; // Version
                buffer[1] = XEMBED_MAPPED; 
                
                XChangeProperty(w->dpy,
                                cur->win,
                                w->_XEMBED_INFO,
                                w->_XEMBED_INFO, 32,
                                PropModeReplace,
                                (unsigned char *)buffer, 2);
                XSync(w->dpy, False);
                }

              /* Register our accelerators */
              register_xembed_accelerators(w, cur);
              }
            break;
          case XEMBED_WINDOW_ACTIVATE:
            if((evt->xclient.window == cur->win) &&
               (cur->child_xembed))
              {
              /* Redirect to child */
              bg_x11_window_send_xembed_message(w, cur->child,
                                                XEMBED_WINDOW_ACTIVATE,
                                                evt->xclient.data.l[0],
                                                0, 0, 0);
              }
            break;
          case XEMBED_WINDOW_DEACTIVATE:
            if((evt->xclient.window == cur->win) &&
               (cur->child_xembed))
              {
              /* Redirect to child */
              bg_x11_window_send_xembed_message(w, cur->child,
                                                XEMBED_WINDOW_DEACTIVATE,
                                                evt->xclient.data.l[0],
                                                0, 0, 0);
              }
            break;
          case XEMBED_REQUEST_FOCUS:
            break;
          case XEMBED_FOCUS_IN:
            if((evt->xclient.window == cur->win) &&
               (cur->child_xembed))
              {
              /* Redirect to child */
              bg_x11_window_send_xembed_message(w, cur->child,
                                                evt->xclient.data.l[0],
                                                XEMBED_FOCUS_IN,
                                                evt->xclient.data.l[2], 0, 0);
              }
            break;
          case XEMBED_FOCUS_OUT:
            if((evt->xclient.window == cur->win) &&
               (cur->child_xembed))
              {
              /* Redirect to child */
              bg_x11_window_send_xembed_message(w, cur->child,
                                                evt->xclient.data.l[0],
                                                XEMBED_FOCUS_OUT,
                                                0, 0, 0);
              }

            break;
          case XEMBED_FOCUS_NEXT:
            break;
          case XEMBED_FOCUS_PREV:
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
            break;
          case XEMBED_MODALITY_ON:
            if((evt->xclient.window == cur->win) &&
               (cur->child_xembed))
              {
              /* Redirect to child */
              bg_x11_window_send_xembed_message(w, cur->child,
                                                evt->xclient.data.l[0],
                                                XEMBED_MODALITY_ON,
                                                0, 0, 0);
              }
            cur->modality = 1;
            break;
          case XEMBED_MODALITY_OFF:
            if((evt->xclient.window == cur->win) &&
               (cur->child_xembed))
              {
              /* Redirect to child */
              bg_x11_window_send_xembed_message(w, cur->child,
                                                evt->xclient.data.l[0],
                                                XEMBED_MODALITY_OFF,
                                                0, 0, 0);
              }
            cur->modality = 0;
            break;
          case XEMBED_REGISTER_ACCELERATOR:
            /* Don't process our own accelerators */
            
            
            /* Child wants to register an accelerator */
            /*
              detail	accelerator_id
              data1	X key symbol
              data2	bit field of modifier values
            */
            if(!cur->parent_xembed)
              {
              /* Add to out accel map */
              bg_accelerator_map_append(cur->child_accel_map,
                                        keysym_to_key_code(evt->xclient.data.l[3]),
                                        xembed_to_key_mask(evt->xclient.data.l[4]),
                                        evt->xclient.data.l[2]);
              }
            else
              {
              /* Propagate */
              bg_x11_window_send_xembed_message(w, cur->parent,
                                                evt->xclient.data.l[0],
                                                XEMBED_REGISTER_ACCELERATOR,
                                                evt->xclient.data.l[2],
                                                evt->xclient.data.l[3],
                                                evt->xclient.data.l[4]);
              }
            //
            break;
          case XEMBED_UNREGISTER_ACCELERATOR:
            /* Child wants to unregister an accelerator */
            if(!cur->parent_xembed)
              {
              /* Remove from our accel map */
              bg_accelerator_map_remove(cur->child_accel_map,
                                        evt->xclient.data.l[2]);
              }
            else
              {
              /* Propagate */
              bg_x11_window_send_xembed_message(w, cur->parent,
                                                evt->xclient.data.l[0],
                                                XEMBED_UNREGISTER_ACCELERATOR,
                                                evt->xclient.data.l[2], 0, 0);
              }
            break;
          case XEMBED_ACTIVATE_ACCELERATOR:
            /* Check if we have the accelerator */
            //  fprintf(stderr, "Activate accelerator\n");
            if(w->callbacks && w->callbacks->accel_map &&
               w->callbacks->accel_callback &&
               bg_accelerator_map_has_accel_with_id(w->callbacks->accel_map,
                                                    evt->xclient.data.l[2]))
              {
              w->callbacks->accel_callback(w->callbacks->data, evt->xclient.data.l[2]);
              return;
              }
            else
              {
              /* Propagate to child */
              bg_x11_window_send_xembed_message(w, cur->parent,
                                                evt->xclient.data.l[0],
                                                XEMBED_ACTIVATE_ACCELERATOR,
                                                evt->xclient.data.l[2],
                                                evt->xclient.data.l[3], 0);
              }
            break;
          }
        return;
        }
      break;
    case CreateNotify:
      if((evt->xcreatewindow.parent == w->normal.win) &&
         (evt->xcreatewindow.window != w->normal.focus_child))
        {
        w->normal.child = evt->xcreatewindow.window;
        bg_x11_window_embed_child(w, &w->normal);
        }
      if(evt->xcreatewindow.parent == w->fullscreen.win &&
         (evt->xcreatewindow.window != w->fullscreen.focus_child))
        {
        w->fullscreen.child = evt->xcreatewindow.window;
        bg_x11_window_embed_child(w, &w->fullscreen);
        }
      break;
    case DestroyNotify:
      if(evt->xdestroywindow.event == w->normal.win)
        {
        w->normal.child = None;
        w->normal.child_xembed = 0;
        if(w->normal.child_accel_map)
          bg_accelerator_map_clear(w->normal.child_accel_map);
        // XDefineCursor(w->dpy, w->normal.win, w->fullscreen_cursor);
        }
      if(evt->xdestroywindow.event == w->fullscreen.win)
        {
        w->fullscreen.child = None;
        w->fullscreen.child_xembed = 0;
        if(w->fullscreen.child_accel_map)
          bg_accelerator_map_clear(w->fullscreen.child_accel_map);
        // XDefineCursor(w->dpy, w->fullscreen.win,
        //              w->fullscreen_cursor);
        }
      break;
    case FocusIn:
      break;
    case FocusOut:
      break;
    case ConfigureNotify:
      if(w->current->win == w->normal.win)
        {
        if(evt->xconfigure.window == w->normal.win)
          {
          w->window_width  = evt->xconfigure.width;
          w->window_height = evt->xconfigure.height;
          
          if(w->normal.parent == w->root)
            {
            bg_x11_window_get_coords(w->dpy,
                                  w->normal.win,
                                  &w->window_x, &w->window_y,
                                  (int*)0, (int*)0);
            }
          bg_x11_window_size_changed(w);
          }
        else if(evt->xconfigure.window == w->normal.parent)
          {
          XResizeWindow(w->dpy,
                        w->normal.win,
                        evt->xconfigure.width,
                        evt->xconfigure.height);
          }
        }
      else
        {
        if(evt->xconfigure.window == w->fullscreen.win)
          {
          w->window_width  = evt->xconfigure.width;
          w->window_height = evt->xconfigure.height;
          bg_x11_window_size_changed(w);
          }
        else if(evt->xconfigure.window == w->fullscreen.parent)
          {
          XResizeWindow(w->dpy,
                        w->fullscreen.win,
                        evt->xconfigure.width,
                        evt->xconfigure.height);
          }
        }
      break;
    case MotionNotify:
      w->idle_counter = 0;
      if(w->pointer_hidden)
        {
        XDefineCursor(w->dpy, w->normal.win, None);
        XDefineCursor(w->dpy, w->fullscreen.win, None);
        XFlush(w->dpy);
        w->pointer_hidden = 0;
        }

      if(evt->xmotion.window == w->normal.win)
        cur = &w->normal;
      else if(evt->xmotion.window == w->fullscreen.win)
        cur = &w->fullscreen;
      else
        return;

      if(cur->modality)
        return;
      
      transform_coords(w, evt->xmotion.x, evt->xmotion.y, &x_src, &y_src);

      if(w->callbacks && w->callbacks->motion_callback)
        {
        key_mask = x11_to_key_mask(evt->xmotion.state);
        w->callbacks->motion_callback(w->callbacks->data,
                                      x_src, y_src, key_mask);
        }
      
      /* Send to parent */
      if(w->current->parent != w->root)
        {
        XMotionEvent motion_event;
        memset(&motion_event, 0, sizeof(motion_event));
        motion_event.type = MotionNotify;
        motion_event.display = w->dpy;
        motion_event.window = w->current->parent;
        motion_event.root = w->root;
        motion_event.time = evt->xmotion.time;
        motion_event.x = evt->xmotion.x;
        motion_event.y = evt->xmotion.y;
        motion_event.x_root = evt->xmotion.x_root;
        motion_event.y_root = evt->xmotion.y_root;
        motion_event.state  = evt->xmotion.state;
        motion_event.same_screen = evt->xmotion.same_screen;

        
        XSendEvent(motion_event.display,
                   motion_event.window,
                   False, None, (XEvent *)(&motion_event));
        // XFlush(w->dpy);
        }
      break;
    case UnmapNotify:
      if(evt->xunmap.window == w->normal.win)
        w->normal.mapped = 0;
      else if(evt->xunmap.window == w->fullscreen.win)
        w->fullscreen.mapped = 0;
      else if(evt->xunmap.window == w->fullscreen.toplevel)
        bg_x11_window_init(w);
      break;
    case MapNotify:
      if(evt->xmap.window == w->normal.win)
        {
        w->normal.mapped = 1;
        /* Kindly ask for keyboard focus */
        if(w->normal.parent_xembed)
          bg_x11_window_send_xembed_message(w, w->normal.parent,
                                            CurrentTime,
                                            XEMBED_REQUEST_FOCUS,
                                            0, 0, 0);
        }
      else if(evt->xmap.window == w->fullscreen.win)
        {
        w->fullscreen.mapped = 1;
        /* Kindly ask for keyboard focus */
        if(w->fullscreen.parent_xembed)
          bg_x11_window_send_xembed_message(w, w->fullscreen.parent,
                                            CurrentTime,
                                            XEMBED_REQUEST_FOCUS,
                                            0, 0, 0);
        
        //        bg_x11_window_set_fullscreen_mapped(w, &w->fullscreen);
        }
      else if(evt->xmap.window == w->fullscreen.toplevel)
        {
        bg_x11_window_init(w);
        bg_x11_window_show(w, 1);
        }
      break;
    case KeyPress:
    case KeyRelease:
      
      
      XLookupString(&(evt->xkey), &key_char, 1, &keysym, NULL);
      evt->xkey.state &= STATE_IGNORE;
      
      if((evt->xkey.window == w->normal.win) ||
         (evt->xkey.window == w->normal.focus_child))
        cur = &w->normal;
      else if((evt->xkey.window == w->fullscreen.win) ||
              (evt->xkey.window == w->fullscreen.focus_child))
        cur = &w->fullscreen;
      else
        {
        return;
        }
      key_code = keysym_to_key_code(keysym);
      key_mask = x11_to_key_mask(evt->xkey.state);
      if(key_code != BG_KEY_NONE)
        {
        if(evt->type == KeyPress)
          {
          /* Try if it's our accel */
          if(w->callbacks &&
             w->callbacks->accel_callback &&
             w->callbacks->accel_map &&
             bg_accelerator_map_has_accel(w->callbacks->accel_map,
                                          key_code,
                                          key_mask, &accel_id) &&
             w->callbacks->accel_callback(w->callbacks->data,
                                          accel_id))
            return;
          /* Check if the child wants tht shortcut */
          if(cur->child_accel_map &&
             cur->child &&
             cur->child_xembed &&
             bg_accelerator_map_has_accel(cur->child_accel_map,
                                          key_code,
                                          key_mask, &accel_id))
            {
            bg_x11_window_send_xembed_message(w, cur->child,
                                              evt->xkey.time,
                                              XEMBED_ACTIVATE_ACCELERATOR,
                                              accel_id, 0, 0);
            return;
            }
          
          /* Here, we see why generic key callbacks are bad:
             One key callback is ok, but if we have more than one
             (i.e. in embedded or embedding windows), one might always
             eat up all events. At lease callbacks should return zero
             to signify, that the event should be propagated */

          if(w->callbacks &&
             w->callbacks->key_callback &&
             w->callbacks->key_callback(w->callbacks->data,
                                        key_code, key_mask))
            return;
          }
        else // KeyRelease
          {
          if(w->callbacks &&
             w->callbacks->key_release_callback &&
             w->callbacks->key_release_callback(w->callbacks->data,
                                                key_code, key_mask))
            return;
          }
        }
      
      if(cur->child != None)
        {
        XKeyEvent key_event;
        /* Send event to child window */
        memset(&key_event, 0, sizeof(key_event));
        key_event.display = w->dpy;
        key_event.window = cur->child;
        key_event.root = w->root;
        key_event.subwindow = None;
        key_event.time = evt->xkey.time;
        key_event.x = 0;
        key_event.y = 0;
        key_event.x_root = 0;
        key_event.y_root = 0;
        key_event.same_screen = True;
        
        key_event.type = evt->type;
        key_event.keycode = evt->xkey.keycode;
        key_event.state = evt->xkey.state;

        XSendEvent(key_event.display,
                   key_event.window,
                   False, 0, (XEvent *)(&key_event));
        XFlush(w->dpy);
        }
      break;
    case ButtonPress:
    case ButtonRelease:
      
      if(evt->xbutton.window == w->normal.win)
        cur = &w->normal;
      else if(evt->xbutton.window == w->fullscreen.win)
        cur = &w->fullscreen;
      else
        return;

      if(cur->modality)
        return;
      
      transform_coords(w, evt->xbutton.x, evt->xbutton.y, &x_src, &y_src);
      evt->xkey.state &= STATE_IGNORE;
      if(w->callbacks)
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
        if(w->callbacks->button_callback &&
           (evt->type == ButtonPress))
          w->callbacks->button_callback(w->callbacks->data,
                                        x_src,
                                        y_src,
                                        button_number,
                                        x11_to_key_mask(evt->xbutton.state));
        else if(w->callbacks->button_release_callback &&
                (evt->type == ButtonRelease))
          w->callbacks->button_release_callback(w->callbacks->data,
                                                x_src,
                                                y_src,
                                                button_number,
                                                x11_to_key_mask(evt->xbutton.state));
        }
      /* Also send to parent */
      if(w->current->parent != w->root)
        {
        XButtonEvent button_event;
        memset(&button_event, 0, sizeof(button_event));
        button_event.type = evt->type;
        button_event.display = w->dpy;
        button_event.window = w->current->parent;
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
                   False, 0, (XEvent *)(&button_event));
        //        XFlush(w->dpy);
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
