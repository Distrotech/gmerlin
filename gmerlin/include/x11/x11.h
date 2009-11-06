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

#include <gmerlin/parameter.h>
#include <gmerlin/accelerator.h>
#include <gavl/gavl.h>

/* Min- Max values for Hue, Saturation, Brightness and Contrast */

#define BG_BRIGHTNESS_MIN  -10.0
#define BG_BRIGHTNESS_MAX   10.0
#define BG_BRIGHTNESS_DELTA  0.5

#define BG_SATURATION_MIN    -10.0
#define BG_SATURATION_MAX     10.0
#define BG_SATURATION_DELTA    0.5

#define BG_CONTRAST_MIN      -10.0
#define BG_CONTRAST_MAX       10.0
#define BG_CONTRAST_DELTA      0.5

typedef struct
  {
  const bg_accelerator_map_t * accel_map;
  int (*accel_callback)(void * data, int accel);
  
  int (*key_callback)(void * data, int key, int mask);
  int (*key_release_callback)(void * data, int key, int mask);

  int (*button_callback)(void * data, int x, int y, int button, int mask);
  int (*button_release_callback)(void * data, int x, int y, int button, int mask);
  int (*motion_callback)(void * data, int x, int y, int mask);
  
  void (*size_changed)(void * data, int width, int height);
  void (*set_fullscreen)(void * data, int fullscreen);
  
  void * data;
  } bg_x11_window_callbacks_t;

/* OpenGL display attributes (see bg_x11_window_set_gl_attribute()) */

#define BG_GL_ATTRIBUTE_BUFFER_SIZE       0 /**< Depth of the color buffer. */
#define BG_GL_ATTRIBUTE_LEVEL             1 /**< Level in plane stacking. */
#define BG_GL_ATTRIBUTE_RGBA              2 /**< True if RGBA mode. */
#define BG_GL_ATTRIBUTE_DOUBLEBUFFER      3 /**< Double buffering supported. */
#define BG_GL_ATTRIBUTE_STEREO            4 /**< Stereo buffering supported. */
#define BG_GL_ATTRIBUTE_AUX_BUFFERS       5 /**< Number of aux buffers. */
#define BG_GL_ATTRIBUTE_RED_SIZE          6 /**< Number of red component bits. */
#define BG_GL_ATTRIBUTE_GREEN_SIZE        7 /**< Number of green component bits. */
#define BG_GL_ATTRIBUTE_BLUE_SIZE         8 /**< Number of blue component bits. */
#define BG_GL_ATTRIBUTE_ALPHA_SIZE        9 /**< Number of alpha component bits. */
#define BG_GL_ATTRIBUTE_DEPTH_SIZE       10 /**< Number of depth bits. */
#define BG_GL_ATTRIBUTE_STENCIL_SIZE     11 /**< Number of stencil bits. */
#define BG_GL_ATTRIBUTE_ACCUM_RED_SIZE   12 /**< Number of red accum bits. */
#define BG_GL_ATTRIBUTE_ACCUM_GREEN_SIZE 13 /**< Number of green accum bits. */
#define BG_GL_ATTRIBUTE_ACCUM_BLUE_SIZE  14 /**< Number of blue accum bits. */
#define BG_GL_ATTRIBUTE_ACCUM_ALPHA_SIZE 15 /**< Number of alpha accum bits. */
#define BG_GL_ATTRIBUTE_NUM              16 /* Must be last and highest */

typedef struct bg_x11_window_s bg_x11_window_t;

bg_x11_window_t * bg_x11_window_create(const char * display_string);

/* For attribute artgument see BG_GL_ATTRIBUTE_ above */
void bg_x11_window_set_gl_attribute(bg_x11_window_t * win, int attribute, int value);

const char * bg_x11_window_get_display_string(bg_x11_window_t * w);

void bg_x11_window_destroy(bg_x11_window_t *);

const bg_parameter_info_t * bg_x11_window_get_parameters(bg_x11_window_t *);

void
bg_x11_window_set_parameter(void * data, const char * name,
                            const bg_parameter_value_t * val);
int
bg_x11_window_get_parameter(void * data, const char * name,
                            bg_parameter_value_t * val);


void bg_x11_window_set_size(bg_x11_window_t *, int width, int height);

void bg_x11_window_clear(bg_x11_window_t *);

int bg_x11_window_realize(bg_x11_window_t *);

/* Handle X11 events, callbacks are called from here */
void bg_x11_window_set_callbacks(bg_x11_window_t*, bg_x11_window_callbacks_t*);
void bg_x11_window_handle_events(bg_x11_window_t*, int milliseconds);


int bg_x11_window_set_fullscreen(bg_x11_window_t * w,int fullscreen);
void bg_x11_window_set_title(bg_x11_window_t * w, const char * title);
void bg_x11_window_set_options(bg_x11_window_t * w,
                               const char * name, const char * klass,
                               const gavl_video_frame_t * icon,
                               const gavl_video_format_t * icon_format);

void bg_x11_window_show(bg_x11_window_t * w, int show);

void bg_x11_window_resize(bg_x11_window_t * win, int width, int height);
void bg_x11_window_get_size(bg_x11_window_t * win, int * width, int * height);


/*
 *   All opengl calls must be enclosed by x11_window_set_gl() and
 *   x11_window_unset_gl()
 */

void bg_x11_window_set_gl(bg_x11_window_t *);
void bg_x11_window_unset_gl(bg_x11_window_t *);

/*
 *  Swap buffers and make your rendered work visible
 */
void bg_x11_window_swap_gl(bg_x11_window_t *);

void bg_x11_window_cleanup_gl(bg_x11_window_t *);

/* For Video output */

int bg_x11_window_open_video(bg_x11_window_t*, gavl_video_format_t * format);

int bg_x11_window_add_overlay_stream(bg_x11_window_t*,
                                     gavl_video_format_t * format);

void bg_x11_window_set_overlay(bg_x11_window_t*, int stream, gavl_overlay_t * ovl);

gavl_overlay_t * bg_x11_window_create_overlay(bg_x11_window_t*, int);
void bg_x11_window_destroy_overlay(bg_x11_window_t*, int, gavl_overlay_t *);

gavl_video_frame_t * bg_x11_window_create_frame(bg_x11_window_t*);
void bg_x11_window_destroy_frame(bg_x11_window_t*, gavl_video_frame_t *);

void bg_x11_window_set_rectangles(bg_x11_window_t * w,
                                  gavl_rectangle_f_t * src_rect,
                                  gavl_rectangle_i_t * dst_rect);

int bg_x11_window_set_brightness(bg_x11_window_t*, float val);
int bg_x11_window_set_saturation(bg_x11_window_t*, float val);
int bg_x11_window_set_contrast(bg_x11_window_t*, float val);

void bg_x11_window_put_frame(bg_x11_window_t*, gavl_video_frame_t * frame);
void bg_x11_window_put_still(bg_x11_window_t*, gavl_video_frame_t * frame);

void bg_x11_window_close_video(bg_x11_window_t*);

/* Grab window */

typedef struct bg_x11_grab_window_s bg_x11_grab_window_t;

const bg_parameter_info_t * bg_x11_grab_window_get_parameters(bg_x11_grab_window_t * win);

void bg_x11_grab_window_set_parameter(void * data, const char * name,
                                      const bg_parameter_value_t * val);

bg_x11_grab_window_t * bg_x11_grab_window_create();
void bg_x11_grab_window_destroy(bg_x11_grab_window_t *);

int bg_x11_grab_window_init(bg_x11_grab_window_t *, gavl_video_format_t * format);
int bg_x11_grab_window_grab(bg_x11_grab_window_t *, gavl_video_frame_t  * frame);
void bg_x11_grab_window_close(bg_x11_grab_window_t * win);
