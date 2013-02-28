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

#include <config.h>
#include <x11/x11.h>
#include <x11/x11_window_private.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "GL"


/* For OpenGL support */

#ifdef HAVE_GLX

static const struct
  {
  int glx_attribute;
  }
gl_attribute_map[BG_GL_ATTRIBUTE_NUM] =
  {
    [BG_GL_ATTRIBUTE_BUFFER_SIZE]       = { GLX_BUFFER_SIZE,      },
    [BG_GL_ATTRIBUTE_LEVEL]             = { GLX_LEVEL,            },
    [BG_GL_ATTRIBUTE_RGBA]              = { GLX_RGBA,             },
    [BG_GL_ATTRIBUTE_DOUBLEBUFFER]      = { GLX_DOUBLEBUFFER,     },
    [BG_GL_ATTRIBUTE_STEREO]            = { GLX_STEREO,           },
    [BG_GL_ATTRIBUTE_AUX_BUFFERS]       = { GLX_AUX_BUFFERS,      },
    [BG_GL_ATTRIBUTE_RED_SIZE]          = { GLX_RED_SIZE,         },
    [BG_GL_ATTRIBUTE_GREEN_SIZE]        = { GLX_GREEN_SIZE,       },
    [BG_GL_ATTRIBUTE_BLUE_SIZE]         = { GLX_BLUE_SIZE,        },
    [BG_GL_ATTRIBUTE_ALPHA_SIZE]        = { GLX_ALPHA_SIZE,       },
    [BG_GL_ATTRIBUTE_DEPTH_SIZE]        = { GLX_DEPTH_SIZE,       },
    [BG_GL_ATTRIBUTE_STENCIL_SIZE]      = { GLX_STENCIL_SIZE,     },
    [BG_GL_ATTRIBUTE_ACCUM_RED_SIZE]    = { GLX_ACCUM_RED_SIZE,   },
    [BG_GL_ATTRIBUTE_ACCUM_GREEN_SIZE]  = { GLX_ACCUM_GREEN_SIZE, },
    [BG_GL_ATTRIBUTE_ACCUM_BLUE_SIZE]   = { GLX_ACCUM_BLUE_SIZE,  },
    [BG_GL_ATTRIBUTE_ACCUM_ALPHA_SIZE]  = { GLX_ACCUM_ALPHA_SIZE, },
  };

int bg_x11_window_init_gl(bg_x11_window_t * win)
  {
  int version_major, version_minor;
  int rgba = 0;
  int num_fbconfigs;
  /* Build the attribute list */

  int attr_index = 0, i;
  /* Attributes we need for video playback */

  int attr_list[64];

  /* Check glX presence and version */
  if(!glXQueryVersion(win->dpy, &version_major, &version_minor))
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "GLX extension missing");
    return 0;
    }

  if((version_major < 1) || (version_minor < 3))
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN,
           "GLX version too old: requested >= 1.3 but got %d.%d",
           version_major, version_minor);
    return 0;
    }

  bg_log(BG_LOG_DEBUG, LOG_DOMAIN,
         "Got GLX version %d.%d", version_major, version_minor);
  
  for(i = 0; i < BG_GL_ATTRIBUTE_NUM; i++)
    {
    if(!win->gl_attributes[i].changed)
      continue;

    if(i == BG_GL_ATTRIBUTE_RGBA)
      {
      if(win->gl_attributes[i].value)
        {
        attr_list[attr_index++] = GLX_RENDER_TYPE;
        attr_list[attr_index++] = GLX_RGBA_BIT;
        rgba = 1;
        }
      }
    else
      {
      attr_list[attr_index++] = gl_attribute_map[i].glx_attribute;
      attr_list[attr_index++] = win->gl_attributes[i].value;
      }
    }

  attr_list[attr_index] = None;

  /* Choose FBCOnfig */
  
  // if(!win->gl_vi)
  //    return 0;

  /* Check GLX version (need at least 1.3) */
  
  
  win->gl_fbconfigs = glXChooseFBConfig(win->dpy, win->screen,
                                        attr_list, &num_fbconfigs);

  if(!win->gl_fbconfigs)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "No framebuffer configs found (insufficient driver or hardware)");
    return 0;
    }
  /* Create context */
  
  win->glxcontext =
    glXCreateNewContext(win->dpy, win->gl_fbconfigs[0],
                        rgba ? GLX_RGBA_TYPE : GLX_COLOR_INDEX_TYPE,
                        NULL, True);
  
  if(!win->glxcontext)
    return 0;
  return 1;
  }

void bg_x11_window_set_gl_attribute(bg_x11_window_t * win,
                                    int attribute, int value)
  {
  if((attribute < 0) || (attribute >= BG_GL_ATTRIBUTE_NUM))
    return;
  win->gl_attributes[attribute].value = value;
  win->gl_attributes[attribute].changed = 1;
  }

int bg_x11_window_start_gl(bg_x11_window_t * win)
  {
  XVisualInfo * vi;

  if(!win->gl_fbconfigs)
    return 0;
  
  vi = glXGetVisualFromFBConfig(win->dpy, win->gl_fbconfigs[0]);
  
  /* Create subwindows */
  bg_x11_window_create_subwins(win, vi->depth, vi->visual);

  /* Create GLX windows */
  win->normal.glx_win = glXCreateWindow(win->dpy,
                                        win->gl_fbconfigs[0],
                                        win->normal.subwin, NULL);
  win->fullscreen.glx_win = glXCreateWindow(win->dpy,
                                            win->gl_fbconfigs[0],
                                            win->fullscreen.subwin, NULL);
  XFree(vi);
  return 1;
  }

void bg_x11_window_stop_gl(bg_x11_window_t * win)
  {
  /* Destroy GLX windows */
  if(win->normal.glx_win != None)
    {
    glXDestroyWindow(win->dpy, win->normal.glx_win);
    win->normal.glx_win = None;
    }
  if(win->fullscreen.glx_win != None)
    {
    glXDestroyWindow(win->dpy, win->fullscreen.glx_win);
    win->fullscreen.glx_win = None;
    }
  /* Destroy subwindows */
  bg_x11_window_destroy_subwins(win);
  }

/*
 *   All opengl calls must be enclosed by x11_window_set_gl() and
 *   x11_window_unset_gl()
 */
   
void bg_x11_window_set_gl(bg_x11_window_t * win)
  {
  glXMakeContextCurrent(win->dpy, win->current->glx_win,
                        win->current->glx_win, win->glxcontext);
  // glXMakeCurrent(win->dpy, win->current->win, win->glxcontext);
  }

void bg_x11_window_unset_gl(bg_x11_window_t * win)
  {
  glXMakeContextCurrent(win->dpy, None, None, NULL);
  
  //  glXMakeCurrent(win->dpy, 0, NULL);
  }

/*
 *  Swap buffers and make your rendered work visible
 */
void bg_x11_window_swap_gl(bg_x11_window_t * win)
  {
  glXSwapBuffers(win->dpy, win->current->glx_win);
  }

void bg_x11_window_cleanup_gl(bg_x11_window_t * win)
  {
  if(win->glxcontext)
    {
    glXDestroyContext(win->dpy, win->glxcontext);
    win->glxcontext = NULL;
    }
  if(win->gl_fbconfigs)
    {
    XFree(win->gl_fbconfigs);
    win->gl_fbconfigs = NULL;
    }
  }

#else // !HAVE_GLX
int bg_x11_window_init_gl(bg_x11_window_t * win)
  {
  return 0;
  }

int bg_x11_window_start_gl(bg_x11_window_t * win)
  {
  return 0;
  }

void bg_x11_window_stop_gl(bg_x11_window_t * win)
  {
  
  }

void bg_x11_window_set_gl_attribute(bg_x11_window_t * win,
                                    int attribute, int value)
  {

  }

void bg_x11_window_set_gl(bg_x11_window_t * win)
  {
  }

void bg_x11_window_unset_gl(bg_x11_window_t * win)
  {
  }

void bg_x11_window_swap_gl(bg_x11_window_t * win)
  {
  }

void bg_x11_window_cleanup_gl(bg_x11_window_t * win)
  {
  }


#endif
