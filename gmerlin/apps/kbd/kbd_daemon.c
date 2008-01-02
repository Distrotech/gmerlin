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

/* System includes */
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>



#include <syslog.h>

#include <X11/Xlib.h>

/* Gmerlin includes */
#include <config.h>
#include <utils.h>
#include <remote.h>
#include <subprocess.h>
#include <log.h>

#define LOG_DOMAIN "gmerlin_kbd"

#include "kbd_remote.h"
#include "kbd.h"

static int ignore_mask = Mod2Mask;

/* Gdk Modifiers */

static struct
  {
  uint32_t flag;
  char * name;
  }
modifiers[] =
  {
    { ShiftMask,   "Shift" }, //   = 1 << 0,
    { LockMask,    "Lock" }, //	    = 1 << 1,
    { ControlMask, "Control" }, //  = 1 << 2,
    { Mod1Mask,    "Mod1" }, //	    = 1 << 3,
    { Mod2Mask,    "Mod2" }, //	    = 1 << 4,
    { Mod3Mask,    "Mod3" }, //	    = 1 << 5,
    { Mod4Mask,    "Mod4" }, //	    = 1 << 6,
    { Mod5Mask,    "Mod5" }, //	    = 1 << 7,
    { Button1Mask, "Button1" }, //  = 1 << 8,
    { Button2Mask, "Button2" }, //  = 1 << 9,
    { Button3Mask, "Button3" }, //  = 1 << 10,
    { Button4Mask, "Button4" }, //  = 1 << 11,
    { Button5Mask, "Button5" }, //  = 1 << 12,
    { /* End of array */ }, //  = 1 << 12,
  };

static uint32_t get_modifiers(const char * str)
  {
  char ** mods;
  int i, j;
  uint32_t ret = 0;
  
  if(!str)
    return ret;

  mods = bg_strbreak(str, '+');

  i = 0;
  while(mods[i])
    {
    j = 0;
    while(modifiers[j].name)
      {
      if(!strcmp(modifiers[j].name, mods[i]))
        {
        ret |= modifiers[j].flag;
        break;
        }
      j++;
      }
    i++;
    }
  bg_strbreak_free(mods);
  return ret;
  }


typedef struct
  {
  /* Map of keys */
  kbd_table_t * keys;
  int num_keys;

  /* Server connection */
  Display * dpy;
  Window root;
  
  /* Remote control */
  bg_remote_server_t * remote;

  bg_msg_queue_t * log_queue;
  
  } kbd_daemon_t;

/* X11 Error handler */

int got_error = 0;

static int x11_error(Display * d, XErrorEvent * evt)
  {
  got_error = 1;
  return 0;
  }

static struct
  {
  int gmerlin_level;
  int syslog_level;
  }
loglevels[] =
  {
    { BG_LOG_ERROR,   LOG_ERR },
    { BG_LOG_WARNING, LOG_WARNING },
    { BG_LOG_INFO,    LOG_INFO },
    { BG_LOG_DEBUG,   LOG_DEBUG },
  };

static void flush_log_queue(kbd_daemon_t * d)
  {
  bg_msg_t * msg;
  char * domain;
  char * message;
  int i;
  
  int gmerlin_level;
  int syslog_level = LOG_INFO;
  while((msg = bg_msg_queue_try_lock_read(d->log_queue)))
    {
    gmerlin_level = bg_msg_get_id(msg);

    domain = bg_msg_get_arg_string(msg, 0);
    message = bg_msg_get_arg_string(msg, 1);


    i = 0;
    for(i = 0; i < sizeof(loglevels) / sizeof(loglevels[0]); i++)
      {
      if(loglevels[i].gmerlin_level == gmerlin_level)
        {
        loglevels[i].syslog_level = syslog_level;
        break;
        }
      }
    syslog(syslog_level, "%s: %s", domain, message);
    free(domain);
    free(message);
    bg_msg_queue_unlock_read(d->log_queue);
    }
  }

static void grab_keys(kbd_daemon_t * d)
  {
  int i;
  for(i = 0; i < d->num_keys; i++)
    {
    if(d->keys[i].command && d->keys[i].scancode)
      {
      got_error = 0;
      XGrabKey(d->dpy, d->keys[i].scancode, d->keys[i].modifiers_i,
               d->root,
               False, // Bool owner_events,
               GrabModeAsync, // int pointer_mode,
               GrabModeAsync // int keyboard_mode
               );
      XGrabKey(d->dpy, d->keys[i].scancode, 
               d->keys[i].modifiers_i | ignore_mask,
               d->root,
               False, // Bool owner_events,
               GrabModeAsync, // int pointer_mode,
               GrabModeAsync // int keyboard_mode
               );

      XFlush(d->dpy);
      XSync(d->dpy, False);
      if(got_error)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,
               "Grabbing key (Code: %d, modifiers: %s) failed",
               d->keys[i].scancode, d->keys[i].modifiers);
        d->keys[i].scancode = 0;
        }
      }
    }
  flush_log_queue(d);
  }

static void ungrab_keys(kbd_daemon_t * d)
  {
  int i;
  for(i = 0; i < d->num_keys; i++)
    {
    if(d->keys[i].command && d->keys[i].scancode)
      {
      XUngrabKey(d->dpy, d->keys[i].scancode, d->keys[i].modifiers_i,
                 d->root);
      }
    }
  flush_log_queue(d);
  }

static void kbd_daemon_destroy(kbd_daemon_t * d)
  {
  ungrab_keys(d);
  kbd_table_destroy(d->keys, d->num_keys);
  XCloseDisplay(d->dpy);

  
  if(d->remote)
    {
    bg_remote_server_wait_close(d->remote);
    bg_remote_server_destroy(d->remote);
    }
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Exiting");
  
  flush_log_queue(d);
  bg_msg_queue_destroy(d->log_queue);
  free(d);
  }

static kbd_daemon_t * kbd_daemon_create()
  {
  int i;
  char * filename;
  kbd_daemon_t * ret;
  ret = calloc(1, sizeof(*ret));

  /* Initialize logging */
  ret->log_queue = bg_msg_queue_create();
  bg_log_set_dest(ret->log_queue);

  XSetErrorHandler(x11_error);
  
  /* Initialize Logging */
  openlog("gmerlin_kbd", LOG_PID, LOG_USER);
  
  /* Open X11 connection */
  ret->dpy = XOpenDisplay(NULL);
  ret->root = DefaultRootWindow(ret->dpy);
  
  filename = bg_search_file_read("kbd", "keys.xml");
  /* Load keyboad mapping */
  ret->keys = kbd_table_load(filename, &ret->num_keys);

  for(i = 0; i < ret->num_keys; i++)
    ret->keys[i].modifiers_i = get_modifiers(ret->keys[i].modifiers);
  
  free(filename);

  ret->remote = bg_remote_server_create(KBD_REMOTE_PORT,
                                        KBD_REMOTE_ID);
  
  if(!bg_remote_server_init(ret->remote))
    {
    kbd_daemon_destroy(ret);
    return (kbd_daemon_t*)0;
    }
  grab_keys(ret);
  flush_log_queue(ret);
  return ret;
  }



static void kbd_loop(kbd_daemon_t * d)
  {
  int i;
  XEvent evt;
  bg_msg_t * msg;
  bg_subprocess_t * proc;
  gavl_time_t delay_time = GAVL_TIME_SCALE / 100; // 10 ms
  int done = 0;
  char * filename;
  
  while(!done)
    {
    /* Check for X-Events */
    while(XPending(d->dpy))
      {
      XNextEvent(d->dpy, &evt);
      switch(evt.type)
        {
        case KeyPress:
          bg_log(BG_LOG_INFO, LOG_DOMAIN, 
                 "State %02x -> %02x\n", evt.xkey.state,
                 evt.xkey.state & ~ignore_mask);
          for(i = 0; i < d->num_keys; i++)
            {
            if((d->keys[i].scancode == evt.xkey.keycode) &&
               ((d->keys[i].modifiers_i & ~ignore_mask) == 
                (evt.xkey.state & ~ignore_mask)))
              {
              proc = bg_subprocess_create(d->keys[i].command, 0, 0, 0);
              bg_subprocess_close(proc);
              break;
              }
            }
          break;
        default:
          break;
        }
      }
    
    /* Check for remote commands */
    msg = bg_remote_server_get_msg(d->remote);
    
    if(msg)
      {
      switch(bg_msg_get_id(msg))
        {
        case KBD_CMD_RELOAD:
          ungrab_keys(d);
          kbd_table_destroy(d->keys, d->num_keys);
          d->keys = (kbd_table_t*)0;
          d->num_keys = 0;

          filename = bg_search_file_read("kbd", "keys.xml");
          /* Load keyboad mapping */
          d->keys = kbd_table_load(filename, &d->num_keys);
          for(i = 0; i < d->num_keys; i++)
            d->keys[i].modifiers_i = get_modifiers(d->keys[i].modifiers);
          free(filename);
          grab_keys(d);
          bg_log(BG_LOG_INFO, LOG_DOMAIN, "Reloaded keys");
          break;
        case KBD_CMD_QUIT:
          done = 1;
          break;
        }
      }
    flush_log_queue(d);
    gavl_time_delay(&delay_time);
    }
  }

int main(int argc, char ** argv)
  {
  /* Our process ID and Session ID */
  pid_t pid, sid;
  kbd_daemon_t * d;
  
#if 1
  /* Fork off the parent process */
  pid = fork();
  if(pid < 0)
    {
    exit(EXIT_FAILURE);
    }
  /* If we got a good PID, then
     we can exit the parent process. */
  if(pid > 0)
    {
    exit(EXIT_SUCCESS);
    }
  
  /* Change the file mode mask */
  umask(0);
                
  /* Open any logs here */        
                
  /* Create a new SID for the child process */
  sid = setsid();
  if (sid < 0)
    {
    /* Log the failure */
    exit(EXIT_FAILURE);
    }
  
  /* Change the current working directory */
  if ((chdir("/")) < 0)
    {
    /* Log the failure */
    exit(EXIT_FAILURE);
    }
  
  /* Close out the standard file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
#endif
  
  
  
  /* Create application */
  d = kbd_daemon_create();
  
  if(d)
    {
    /* Main loop */
    kbd_loop(d);
    
    /* Cleanup and exit */
    kbd_daemon_destroy(d);
    return 0;
    }
  else
    return EXIT_FAILURE;
  }
