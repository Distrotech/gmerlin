/*****************************************************************
 
  display.c
 
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

#include <inttypes.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <ctype.h> 

#include <gavl/gavl.h>

#include <gtk/gtk.h>
#include <gui_gtk/display.h>
#include <gui_gtk/gtkutils.h>
#include <utils.h>

#define DIGIT_0     0
#define DIGIT_1     1
#define DIGIT_2     2
#define DIGIT_3     3
#define DIGIT_4     4
#define DIGIT_5     5
#define DIGIT_6     6
#define DIGIT_7     7
#define DIGIT_8     8
#define DIGIT_9     9
#define DIGIT_COLON 10

static int pixbufs_loaded = 0;

static GdkPixbuf * digit_pixbufs[11];

static void load_pixbufs()
  {
  char * c_tmp1 = (char*)0;
  char * c_tmp2 = (char*)0;
  int i;
  
  if(pixbufs_loaded)
    return;
  pixbufs_loaded = 1;

  for(i = 0; i < 10; i++)
    {
    c_tmp1 = bg_sprintf("digit_%d.png", i);
    //    fprintf(stderr, "Loading %s\n", c_tmp1);
    c_tmp2 = bg_search_file_read("icons", c_tmp1);
    digit_pixbufs[i] = gdk_pixbuf_new_from_file(c_tmp2, NULL);    
    free(c_tmp1);
    free(c_tmp2);
    }

  c_tmp2 = bg_search_file_read("icons", "digit_colon.png");
  digit_pixbufs[10] = gdk_pixbuf_new_from_file(c_tmp2, NULL);    
  free(c_tmp2);
  }

struct bg_gtk_time_display_s
  {
  GdkPixbuf * pixbufs[11];
  float foreground_color[3];
  float background_color[3];
  int height;
  int digit_width;
  int colon_width;
  GtkWidget * widget;
  int seconds;
  int indices[GAVL_TIME_STRING_LEN];
  
  GdkGC * gc;
  };

static void set_bg_color(bg_gtk_time_display_t * d)
  {
  GdkColor bg;
  if(!d->gc)
    return;
  
  bg.red   = (guint16)(d->background_color[0]*65535.0);
  bg.green = (guint16)(d->background_color[1]*65535.0);
  bg.blue  = (guint16)(d->background_color[2]*65535.0);
  bg.pixel =
    (bg.red >> 8)   << 16 |
    (bg.green >> 8) << 8  |
    (bg.blue >> 8);
  gdk_color_alloc(gdk_window_get_colormap(d->widget->window),
                  &(bg));
  gdk_gc_set_foreground(d->gc, &bg);
  }



static void create_pixmaps(bg_gtk_time_display_t * d)
  {
  int i;
  
  if(d->pixbufs[0])
    {
    for(i = 0; i < 11; i++)
      g_object_unref(G_OBJECT(d->pixbufs[i]));
    }

  /* Scale down the pixmaps */

  
  for(i = 0; i < 10; i++)
    {
    d->pixbufs[i] = bg_gtk_pixbuf_scale_alpha(digit_pixbufs[i],
                                              d->digit_width,
                                              d->height,
                                              d->foreground_color,
                                              d->background_color);
    }
  d->pixbufs[10] = bg_gtk_pixbuf_scale_alpha(digit_pixbufs[10],
                                             d->colon_width,
                                             d->height,
                                             d->foreground_color,
                                             d->background_color);
  }

static gboolean expose_callback(GtkWidget * w, GdkEventExpose * evt,
                                gpointer data)
  {
  int pos_i;
  bg_gtk_time_display_t * d;
  int x;

  d = (bg_gtk_time_display_t *)data;

  if(!d->widget->window)
    return TRUE;
    
  pos_i = 0;
  x = 7 * d->digit_width + 2 * d->colon_width;
  
  while(d->indices[pos_i] >= 0)
    {
    if(d->indices[pos_i] == DIGIT_COLON)
      {
      x -= d->colon_width;
      gdk_draw_pixbuf(d->widget->window,
                      (GdkGC *)0,
                      d->pixbufs[d->indices[pos_i]],
                      0, // gint src_x,
                      0, // gint src_y,
                      x,
                      0,
                      d->colon_width,
                      d->height,
                      GDK_RGB_DITHER_NONE,
                      0, 0);
      }
    else
      {
      x -= d->digit_width;
      gdk_draw_pixbuf(d->widget->window,
                      (GdkGC *)0,
                      d->pixbufs[d->indices[pos_i]],
                      0, // gint src_x,
                      0, // gint src_y,
                      x,
                      0,
                      d->digit_width,
                      d->height,
                      GDK_RGB_DITHER_NONE,
                      0, 0);
      }
    pos_i++;
    }
  if(x)
    gdk_draw_rectangle(d->widget->window,
                       d->gc,
                       TRUE,
                       0, 0, x, d->height);
  return TRUE;
  }

static void realize_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_time_display_t * d;

  d = (bg_gtk_time_display_t *)data;

  d->gc = gdk_gc_new(d->widget->window);
  set_bg_color(d);
  }

void bg_gtk_time_display_set_colors(bg_gtk_time_display_t * d,
                                    float * foreground,
                                    float * background)
  {
  memcpy(d->foreground_color, foreground, 3 * sizeof(float));
  memcpy(d->background_color, background, 3 * sizeof(float));
  create_pixmaps(d);

  set_bg_color(d);
  
  expose_callback(d->widget, (GdkEventExpose*)0, d);
  }


void bg_gtk_time_display_update(bg_gtk_time_display_t * d,
                                int seconds)
  {
  char * pos;
  char buf[GAVL_TIME_STRING_LEN];
  int pos_i;
  
  d->seconds = seconds;
  gavl_time_prettyprint_seconds(d->seconds,
                                buf);

  pos = &(buf[strlen(buf)]);

  
  pos_i = 0;
  
  do{
    pos--;
    if(*pos == ':')
      d->indices[pos_i] = DIGIT_COLON;
    else if(isdigit(*pos))
      d->indices[pos_i] = (int)(*pos) - (int)('0');
    pos_i++;
  } while(pos != buf);

  while(pos_i < GAVL_TIME_STRING_LEN)
    {
    d->indices[pos_i] = -1;
    pos_i++;
    }
  expose_callback(d->widget, (GdkEventExpose*)0, d);
  }

bg_gtk_time_display_t *
bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE size)
  {
  bg_gtk_time_display_t * ret;

  load_pixbufs();
  
  ret = calloc(1, sizeof(*ret));
  
  switch(size)
    {
    case BG_GTK_DISPLAY_SIZE_HUGE:   /* 480 x 96, 1/1 */
      ret->height       = 96;
      ret->digit_width  = 60;
      ret->colon_width  = 30;
      break;
    case BG_GTK_DISPLAY_SIZE_LARGE:  /* 240 x 48, 1/2 */
      ret->height       = 48;
      ret->digit_width  = 30;
      ret->colon_width  = 15;
      break;
    case BG_GTK_DISPLAY_SIZE_NORMAL: /* 160 x 32  1/3 */
      ret->height       = 32;
      ret->digit_width  = 20;
      ret->colon_width  = 10;
      break;
    case BG_GTK_DISPLAY_SIZE_SMALL:  /*  80 x 16  1/6 */
      ret->height       = 16;
      ret->digit_width  = 10;
      ret->colon_width  = 5;
      break;

    }
  ret->foreground_color[0] = 0.0;
  ret->foreground_color[1] = 1.0;
  ret->foreground_color[2] = 0.0;

  ret->background_color[0] = 0.0;
  ret->background_color[1] = 0.0;
  ret->background_color[2] = 0.0;
  create_pixmaps(ret);

  ret->widget = gtk_drawing_area_new();

  g_signal_connect(G_OBJECT(ret->widget), "expose_event",
                     G_CALLBACK(expose_callback), (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->widget), "realize",
                   G_CALLBACK(realize_callback), (gpointer)ret);

  gtk_widget_set_size_request(ret->widget,
                              7 * ret->digit_width + 2 * ret->colon_width,
                              ret->height);

  gtk_widget_show(ret->widget);
  bg_gtk_time_display_update(ret, 0);
  return ret;
  }

GtkWidget * bg_gtk_time_display_get_widget(bg_gtk_time_display_t * d)
  {
  return d->widget;
  }
