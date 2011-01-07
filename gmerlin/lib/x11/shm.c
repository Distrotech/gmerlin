/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <sys/ipc.h>
#include <sys/shm.h>

#define LOG_DOMAIN "x11_shm"
#include <gmerlin/log.h>

/* Check for xshm extension */

int bg_x11_window_check_shm(Display * dpy, int * completion_type)
  {
#if 0
  return 0;
#else
  int major;
  int minor;
  Bool pixmaps;
  int ret;
  if ((XShmQueryVersion (dpy, &major, &minor,
                         &pixmaps) == 0) ||
      (major < 1) || ((major == 1) && (minor < 1)))
    ret = 0;
  else
    ret = 1;
  if(ret)
    *completion_type = XShmGetEventBase(dpy) + ShmCompletion;
  return ret;
#endif
  }

static int shmerror = 0;

static int handle_error (Display * display, XErrorEvent * error)
  {
  shmerror = 1;
  return 0;
  }

int bg_x11_window_create_shm(bg_x11_window_t * win,
                          XShmSegmentInfo * shminfo, int size)
  {
  shmerror = 0;
  shminfo->shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);
  if (shminfo->shmid == -1)
    goto error;

  shminfo->shmaddr = (char *) shmat (shminfo->shmid, 0, 0);
  if (shminfo->shmaddr == (char *)-1)
    goto error;

  XSync (win->dpy, False);
  XSetErrorHandler (handle_error);

  shminfo->readOnly = True;
  if(!(XShmAttach (win->dpy, shminfo)))
    shmerror = 1;
  
  XSync (win->dpy, False);
  XSetErrorHandler(NULL);
  
  if(shmerror)
    {
    error:
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Cannot create shared memory");
    return 0;
    }

  return 1;
 
  }


void bg_x11_window_destroy_shm(bg_x11_window_t * win,
                            XShmSegmentInfo * shminfo)
  {
  XShmDetach (win->dpy, shminfo);
  shmdt (shminfo->shmaddr);
  shmctl (shminfo->shmid, IPC_RMID, 0);
  }

