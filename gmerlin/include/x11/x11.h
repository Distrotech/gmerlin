/*****************************************************************
 
  x11.h
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <parameter.h>
#include <gavl/gavl.h>

typedef struct
  {
  int (*key_callback)(void * data, int key, int mask);
  int (*button_callback)(void * data, int x, int y, int button, int mask);
  void (*show_window)(void * data, int show);
  void (*size_changed)(void * data, int width, int height);
  void (*set_fullscreen)(void * data, int fullscreen);
  
  /* For updating OSD */
  void (*brightness_callback)(void * data, float val);
  void (*saturation_callback)(void * data, float val);
  void (*contrast_callback)(void * data, float val);
  
  void * data;
  } bg_x11_window_callbacks_t;

typedef struct bg_x11_window_s bg_x11_window_t;


bg_x11_window_t * bg_x11_window_create(const char * display_string);

const char * bg_x11_window_get_display_string(bg_x11_window_t * w);

void bg_x11_window_destroy(bg_x11_window_t *);

bg_parameter_info_t * bg_x11_window_get_parameters(bg_x11_window_t *);

void
bg_x11_window_set_parameter(void * data, const char * name,
                            const bg_parameter_value_t * val);
int
bg_x11_window_get_parameter(void * data, const char * name,
                            bg_parameter_value_t * val);


void bg_x11_window_set_size(bg_x11_window_t *, int width, int height);

void bg_x11_window_clear(bg_x11_window_t *);

int bg_x11_window_create_window(bg_x11_window_t *);

int bg_x11_window_create_window_gl(bg_x11_window_t *,
                                   int *attribList );

/* Handle X11 events, callbacks are called from here */
void bg_x11_window_set_callbacks(bg_x11_window_t*, bg_x11_window_callbacks_t*);
void bg_x11_window_handle_events(bg_x11_window_t*, int milliseconds);


int bg_x11_window_set_fullscreen(bg_x11_window_t * w,int fullscreen);
void bg_x11_window_set_title(bg_x11_window_t * w, const char * title);
void bg_x11_window_set_class_hint(bg_x11_window_t * w,
                             char * name, char * klass);

void bg_x11_window_show(bg_x11_window_t * w, int show);

void bg_x11_window_resize(bg_x11_window_t * win, int width, int height);


/* For OpenGL support */

int bg_x11_window_init_gl(bg_x11_window_t *);

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

gavl_video_frame_t * bg_x11_window_create_frame(bg_x11_window_t*);
void bg_x11_window_destroy_frame(bg_x11_window_t*, gavl_video_frame_t *);

void bg_x11_window_set_rectangles(bg_x11_window_t * w,
                                  gavl_rectangle_f_t * src_rect,
                                  gavl_rectangle_i_t * dst_rect);


void bg_x11_window_put_frame(bg_x11_window_t*, gavl_video_frame_t * frame);



void bg_x11_window_close_video(bg_x11_window_t*);

