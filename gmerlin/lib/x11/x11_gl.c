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

#include <config.h>
#include <x11/x11.h>
#include <x11/x11_window_private.h>


/* For OpenGL support */

int bg_x11_window_init_gl(bg_x11_window_t * win)
  {
#ifdef HAVE_GLX
  if(!win->gl_vi)
    return 0;
  win->glxcontext =
    glXCreateContext(win->dpy, win->gl_vi, NULL, True);
  if(win->glxcontext == NULL)
    return 0;
  return 1;
#else
  return 0;
#endif
  }

void bg_x11_window_set_gl_attribute(bg_x11_window_t * win, int attribute, int value)
  {
  if((attribute < 0) || (attribute >= BG_GL_ATTRIBUTE_NUM))
    return;
  win->gl_attributes[attribute].value = value;
  win->gl_attributes[attribute].changed = 1;
  }

/*
 *   All opengl calls must be enclosed by x11_window_set_gl() and
 *   x11_window_unset_gl()
 */
   
void bg_x11_window_set_gl(bg_x11_window_t * win)
  {
#ifdef HAVE_GLX
  glXMakeCurrent(win->dpy, win->current->win, win->glxcontext);
#endif
  }

void bg_x11_window_unset_gl(bg_x11_window_t * win)
  {
#ifdef HAVE_GLX
  glXMakeCurrent(win->dpy, 0, NULL);
#endif
  }

/*
 *  Swap buffers and make your rendered work visible
 */
void bg_x11_window_swap_gl(bg_x11_window_t * win)
  {
#ifdef HAVE_GLX
  glXMakeCurrent(win->dpy, win->current->win, win->glxcontext);
  glXSwapBuffers(win->dpy, win->current->win);
  glXMakeCurrent(win->dpy, 0, NULL);
#endif
  }

void bg_x11_window_cleanup_gl(bg_x11_window_t * win)
  {
#ifdef HAVE_GLX
  if(win->glxcontext)
    {
    glXDestroyContext(win->dpy, win->glxcontext);
    win->glxcontext = NULL;
    }
#endif
  }
