#include <x11/x11.h>
#include <x11/x11_window_private.h>


/* For OpenGL support */

int bg_x11_window_init_gl(bg_x11_window_t * win)
  {
#ifdef HAVE_GLX
  win->glxcontext =
    glXCreateContext(win->dpy, win->vi, NULL, True);
  if(win->glxcontext == NULL)
    return 0;
  return 1;
#else
  return 0;
#endif
  }

/*
 *   All opengl calls must be enclosed by x11_window_set_gl() and
 *   x11_window_unset_gl()
 */
   
void bg_x11_window_set_gl(bg_x11_window_t * win)
  {
  glXMakeCurrent(win->dpy, win->current_window, win->glxcontext);
  }

void bg_x11_window_unset_gl(bg_x11_window_t * win)
  {
  glXMakeCurrent(win->dpy, 0, NULL);
  }

/*
 *  Swap buffers and make your rendered work visible
 */
void bg_x11_window_swap_gl(bg_x11_window_t * win)
  {
  glXMakeCurrent(win->dpy, win->current_window, win->glxcontext);
  glXSwapBuffers(win->dpy, win->current_window);
  glXMakeCurrent(win->dpy, 0, NULL);
  }

void bg_x11_window_cleanup_gl(bg_x11_window_t * win)
  {
  if(win->glxcontext)
    {
    glXDestroyContext(win->dpy, win->glxcontext);
    win->glxcontext = NULL;
    }
  }
