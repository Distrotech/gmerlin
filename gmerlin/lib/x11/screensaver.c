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


#include <string.h>

#include <x11/x11.h>
#include <x11/x11_window_private.h>

#ifdef HAVE_XTEST
#include <X11/extensions/XTest.h>
#endif

#include <X11/keysym.h>

void
bg_x11_screensaver_init(bg_x11_screensaver_t * scr, Display * dpy)
  {
  char * env;
#ifdef HAVE_XTEST
  int dummy;
#endif
  
  scr->dpy = dpy;
  scr->timer = gavl_timer_create();
  
  /* Get the screensaver mode */

#ifdef HAVE_XTEST
  if(XTestQueryExtension(scr->dpy,
                         &dummy, &dummy, &dummy, &dummy))
    {
    scr->mode = SCREENSAVER_MODE_XTEST;
    //    scr->fake_keycode = XKeysymToKeycode(scr->dpy, XK_Shift_L);
    //    fprintf(stderr, "Xtest detected\n");
    return;
    }
  //  else
  //    fprintf(stderr, "No Xtest detected\n");
  
#endif
  
  /* Check for gnome */
  env = getenv("GNOME_DESKTOP_SESSION_ID");
  if(env)
    {
    scr->mode = SCREENSAVER_MODE_GNOME;
    return;
    }

  /* Check for KDE */
  env = getenv("KDE_FULL_SESSION");
  if(env && !strcmp(env, "true"))
    {
    scr->mode = SCREENSAVER_MODE_KDE;
    return;
    }

  /* TODO: xfce4 */


  }

static int check_ping(bg_x11_screensaver_t * scr, int force, int seconds)
  {
  gavl_time_t cur = gavl_timer_get(scr->timer);
  if(force || ((cur - scr->last_ping_time) / GAVL_TIME_SCALE >= seconds))
    {
    scr->last_ping_time = cur;
    return 1;
    }
  return 0;
  }

static void
screensaver_ping(bg_x11_screensaver_t * scr, int force)
  {
  if(!scr->disabled)
    return;
  
  switch(scr->mode)
    {
    case SCREENSAVER_MODE_XLIB:
      break;
    case SCREENSAVER_MODE_GNOME:
      if(check_ping(scr, force, 40))
        system("gnome-screensaver-command --poke > /dev/null 2> /dev/null");
      break;
    case SCREENSAVER_MODE_KDE:
      break;
#ifdef HAVE_XTEST
    case SCREENSAVER_MODE_XTEST:
      if(check_ping(scr, force, 40))
        {
        XTestFakeRelativeMotionEvent(scr->dpy,
                                     1, 1, CurrentTime);
        scr->fake_motion++;
        // fprintf(stderr, "Sent fake motion event\n");
        }
      break;
#endif
    }
  }

void
bg_x11_screensaver_ping(bg_x11_screensaver_t * scr)
  {
  screensaver_ping(scr, 0);
  }


void
bg_x11_screensaver_enable(bg_x11_screensaver_t * scr)
  {
  int dummy, interval, prefer_blank, allow_exp;

  gavl_timer_stop(scr->timer);
  
  if(!scr->disabled)
    return;
  
#if HAVE_XDPMS
  if(scr->dpms_disabled)
    {
    if(DPMSQueryExtension(scr->dpy, &dummy, &dummy))
      {
      if(DPMSEnable(scr->dpy))
        {
        // DPMS does not seem to be enabled unless we call DPMSInfo
        BOOL onoff;
        CARD16 state;

        DPMSForceLevel(scr->dpy, DPMSModeOn);      
        DPMSInfo(scr->dpy, &state, &onoff);
        }

      }
    scr->dpms_disabled = 0;
    }
#endif // HAVE_XDPMS
  
  scr->disabled = 0;
  if(!scr->was_enabled)
    return;
  
  switch(scr->mode)
    {
    case SCREENSAVER_MODE_XLIB:
      XGetScreenSaver(scr->dpy, &dummy, &interval, &prefer_blank,
                      &allow_exp);
      XSetScreenSaver(scr->dpy, scr->saved_timeout, interval, prefer_blank,
                      allow_exp);
      break;
    case SCREENSAVER_MODE_GNOME:
      break;
    case SCREENSAVER_MODE_KDE:
      break;
    }
  }

void
bg_x11_screensaver_disable(bg_x11_screensaver_t * scr)
  {
  int interval, prefer_blank, allow_exp;

#if HAVE_XDPMS
  int nothing;
#endif // HAVE_XDPMS
  
  gavl_timer_start(scr->timer);
  
  if(scr->disabled)
    return;

#if HAVE_XDPMS
  if(DPMSQueryExtension(scr->dpy, &nothing, &nothing))
    {
    BOOL onoff;
    CARD16 state;
    
    DPMSInfo(scr->dpy, &state, &onoff);
    if(onoff)
      {
      scr->dpms_disabled = 1;
      DPMSDisable(scr->dpy);       // monitor powersave off
      }
    }
#endif // HAVE_XDPMS
    
  switch(scr->mode)
    {
    case SCREENSAVER_MODE_XLIB:
      XGetScreenSaver(scr->dpy, &scr->saved_timeout,
                      &interval, &prefer_blank,
                      &allow_exp);
      if(scr->saved_timeout)
        scr->was_enabled = 1;
      else
        scr->was_enabled = 0;
      XSetScreenSaver(scr->dpy, 0, interval, prefer_blank, allow_exp);
      break;
    case SCREENSAVER_MODE_GNOME:
      break;
    case SCREENSAVER_MODE_XTEST:
      scr->fake_motion = 0;
      break;
    case SCREENSAVER_MODE_KDE:
      scr->was_enabled =
        (system
             ("dcop kdesktop KScreensaverIface isEnabled 2>/dev/null | sed 's/1/true/g' | grep true 2>/dev/null >/dev/null")
         == 0);
      
      if(scr->was_enabled)
        system("dcop kdesktop KScreensaverIface enable false > /dev/null");
      break;
    }
  scr->disabled = 1;
  screensaver_ping(scr, 1);
  }

void
bg_x11_screensaver_cleanup(bg_x11_screensaver_t * scr)
  {
  if(scr->timer)
    gavl_timer_destroy(scr->timer);
  }
