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

/* System includes */
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include <X11/Xlib.h>

/* Gmerlin includes */
#include <config.h>
#include <gmerlin/utils.h>
#include <gmerlin/remote.h>
#include <gmerlin/subprocess.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "gmerlin_kbd"

#include "kbd_remote.h"
#include "kbd.h"


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
               d->keys[i].modifiers_i | kbd_ignore_mask,
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
  bg_log_syslog_flush();
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
  bg_log_syslog_flush();
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
  
  bg_log_syslog_flush();
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

  XSetErrorHandler(x11_error);
  
  bg_log_syslog_init("gmerlin_kbd");
  
  /* Open X11 connection */
  ret->dpy = XOpenDisplay(NULL);
  ret->root = DefaultRootWindow(ret->dpy);
  
  filename = bg_search_file_read("kbd", "keys.xml");
  /* Load keyboad mapping */
  ret->keys = kbd_table_load(filename, &ret->num_keys);

  for(i = 0; i < ret->num_keys; i++)
    ret->keys[i].modifiers_i = kbd_modifiers_from_string(ret->keys[i].modifiers);
  
  free(filename);

  ret->remote = bg_remote_server_create(KBD_REMOTE_PORT,
                                        KBD_REMOTE_ID);
  
  if(!bg_remote_server_init(ret->remote))
    {
    kbd_daemon_destroy(ret);
    return NULL;
    }
  grab_keys(ret);
  bg_log_syslog_flush();
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
                 evt.xkey.state & ~kbd_ignore_mask);
          for(i = 0; i < d->num_keys; i++)
            {
            if((d->keys[i].scancode == evt.xkey.keycode) &&
               ((d->keys[i].modifiers_i & ~kbd_ignore_mask) == 
                (evt.xkey.state & ~kbd_ignore_mask)))
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
          d->keys = NULL;
          d->num_keys = 0;

          filename = bg_search_file_read("kbd", "keys.xml");
          /* Load keyboad mapping */
          d->keys = kbd_table_load(filename, &d->num_keys);
          for(i = 0; i < d->num_keys; i++)
            d->keys[i].modifiers_i = kbd_modifiers_from_string(d->keys[i].modifiers);
          free(filename);
          grab_keys(d);
          bg_log(BG_LOG_INFO, LOG_DOMAIN, "Reloaded keys");
          break;
        case KBD_CMD_QUIT:
          done = 1;
          break;
        }
      }
    bg_log_syslog_flush();
    gavl_time_delay(&delay_time);
    }
  }

int main(int argc, char ** argv)
  {
  /* Our process ID and Session ID */
  kbd_daemon_t * d;
  
#if 1
  pid_t pid, sid;
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
