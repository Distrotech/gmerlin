/*****************************************************************
 
  vumeter.c
 
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

#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <config.h>

#include <gtk/gtk.h>
#include <gui_gtk/audio.h>
#include <gui_gtk/gtkutils.h>

#include <utils.h>

/* Some constants */

#define TIC_LENGTH    5
#define METER_WIDTH  14

static struct
  {
  float value;
  char * label;
  }
scale_tics[] =
  {
    {   0.0,  "0"},
    {  -1.0,  "1"},
    {  -3.0,  "3"},
    {  -6.0,  "6"},
    { -10.0, "10"},
    { -20.0, "20"},
    { -30.0, "30"},
    { -40.0, "40"},
    { -50.0, "50"},
    { -60.0, "60"},
  };

#define NUM_TICS sizeof(scale_tics)/sizeof(scale_tics[0])

#define LEVEL_MIN (float)(scale_tics[NUM_TICS-1].value)
#define LEVEL_MAX (float)(scale_tics[0].value)

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "properties",
      long_name:   TRS("Properties"),
      type:        BG_PARAMETER_SECTION
    },
    {
      name:        "peak_age",
      long_name:   TRS("Peak hold time (sec.)"),
      type:        BG_PARAMETER_SLIDER_FLOAT,
      val_min:     { val_f: 0.1 },
      val_max:     { val_f: 100.0 },
      val_default: { val_f: 4.0 }
    },
    { /* End of parameters */ }
  };

struct bg_gtk_vumeter_s
  {
  GtkWidget * layout;
  gavl_peak_detector_t * pd;

  struct
    {
    GtkWidget * l;
    GdkRectangle coords;
    } labels[NUM_TICS];
  
  /* Colors */
  
  GdkPixbuf * pixbuf_on;
  GdkPixbuf * pixbuf_off;
  
  int width;
  int height;
  int max_channels;
  
  int scale_width;
  int scale_ascent;
  int scale_descent;
  
  int meter_width;
  int meter_offset_y;
  int meter_height;
  int over_height;

  int vertical;
  
  pthread_mutex_t analysis_mutex;

  /* Analysis stuff, level values are in dB */

  struct
    {
    float level;
    int64_t peak_age;
    GdkRectangle coords;
    }
  meters[GAVL_MAX_CHANNELS];
  
  int max_peak_age;
  int num_channels;
  };

/* Parameter stuff */

bg_parameter_info_t *
bg_gtk_vumeter_get_parameters(bg_gtk_vumeter_t * v)
  {
  return parameters;
  }

void bg_gtk_vumeter_set_parameter(void * vumeter, const char * name,
                                  const bg_parameter_value_t * val)
  {
  bg_gtk_vumeter_t * v;
  v = (bg_gtk_vumeter_t *)vumeter;
  if(!name)
    return;
  if(!strcmp(name, "peak_age"))
    {
    pthread_mutex_lock(&(v->analysis_mutex));
    v->max_peak_age = val->val_f;
    pthread_mutex_unlock(&(v->analysis_mutex));
    }
  }

static float level_2_dB(float level)
  {
  float ret;
  
  if(level == 0.0)
    return LEVEL_MIN;
  
  ret = log10(level) * 20;
  if(ret > LEVEL_MAX)
    ret = LEVEL_MAX;
  if(ret < LEVEL_MIN)
    ret = LEVEL_MIN;
  return ret;
  }

static void set_coords_horizontal(bg_gtk_vumeter_t * m)
  {
  int i;
  
  int max_label_w = 0;
  int max_label_h = 0;

  int total_width = 0;

  /* Get maximum size of the labels */
  for(i = 0; i < NUM_TICS; i++)
    {
    if(m->labels[i].coords.width > max_label_w)
      max_label_w = m->labels[i].coords.width;
    if(m->labels[i].coords.height > max_label_h)
      max_label_h = m->labels[i].coords.height;
    total_width += m->labels[i].coords.width;
    //    gtk_layout_move(GTK_LAYOUT(m->layout), m->labels[i].l, 100, 100);
    }
  
  /* Calculate meter coordinates */
  
  for(i = 0; i < 2; i++)
    {
    m->meters[i].coords.x     = m->labels[i].coords.width / 2;
    m->meters[i].coords.width = m->width - m->labels[NUM_TICS-1].coords.width / 2 -
      m->labels[i].coords.width / 2;
    }

  m->meters[0].coords.y      = 0;
  m->meters[0].coords.height = (m->height - 2 * TIC_LENGTH - max_label_h)/2;

  m->meters[1].coords.y      = m->meters[0].coords.height + 2 * TIC_LENGTH + max_label_h;
  m->meters[1].coords.height = (m->height - 2 * TIC_LENGTH - max_label_h)/2;
  
  /* Calculate label positions */
  
  for(i = 0; i < NUM_TICS; i++)
    {
    
    }
  
  fprintf(stderr, "Total width: %d, max_label_width: %d, max_label_height: %d\n",
          total_width, max_label_w, max_label_h);
  
  }

static void set_coords_vertical(bg_gtk_vumeter_t * m)
  {
  
  }

static void set_coords(bg_gtk_vumeter_t * m)
  {
  if(m->vertical)
    set_coords_vertical(m);
  else
    set_coords_horizontal(m);
  }

static void draw_static(bg_gtk_vumeter_t * m)
  {
  int i;
  
  }

static int get_y(bg_gtk_vumeter_t * m, float level)
  {
  int i;
  int ret = 0;
  
  return ret;
  
  }

static void draw_dynamic(bg_gtk_vumeter_t * m)
  {
  
  }

static void flash(bg_gtk_vumeter_t * m)
  {
  
  }

static gboolean expose_callback(GtkWidget * w, GdkEventExpose *event,
                                gpointer data)
  {
  int i;
  bg_gtk_vumeter_t * m = (bg_gtk_vumeter_t *)data;
  GtkStyle * style;
  fprintf(stderr, "Expose callback %d %d %d %d\n",
          m->meters[0].coords.x,
          m->meters[0].coords.y,
          m->meters[0].coords.width,
          m->meters[0].coords.height);
  
  style = gtk_widget_get_style(m->layout);
  
  gtk_paint_shadow(style,
                   GTK_LAYOUT(m->layout)->bin_window,
                   GTK_STATE_NORMAL,
                   GTK_SHADOW_IN,
                   NULL, m->layout, NULL,
                   m->meters[0].coords.x,
                   m->meters[0].coords.y,
                   10, 10);
  
  //                   m->meters[0].coords.width,
  //                   m->meters[0].coords.height);
  
  return FALSE;
  }

static gboolean mouse_callback(GtkWidget * w, GdkEventButton *event,
                               gpointer(data))
  {
  bg_gtk_vumeter_t * m;
  m = (bg_gtk_vumeter_t *)data;
  
  return TRUE;
  
  }

static void label_size_request_callback(GtkWidget * w, GtkRequisition *requisition,
                                        gpointer data)
  {
  bg_gtk_vumeter_t * v;
  int i; 
  v = (bg_gtk_vumeter_t *)data;

  fprintf(stderr, "Size request: %d %d\n", requisition->width,
          requisition->height);
  
  for(i = 0; i < NUM_TICS; i++)
    {
    if(v->labels[i].l == w)
      {
      v->labels[i].coords.width  = requisition->width;
      v->labels[i].coords.height = requisition->height;
      }
    }
  }


static void size_allocate_callback(GtkWidget * w,
                                   GtkAllocation * allocation,
                                   gpointer data)
  {
  int i;
  bg_gtk_vumeter_t * v;
  v = (bg_gtk_vumeter_t *)data;
  fprintf(stderr, "Size allocate: %d %d\n", allocation->width,
          allocation->height);
  v->width  = allocation->width;
  v->height = allocation->height;

  //  gtk_layout_move(

  gtk_layout_set_size(GTK_LAYOUT(v->layout), allocation->width, allocation->height);
  //set_coords(v);
  
  }

bg_gtk_vumeter_t *
bg_gtk_vumeter_create(int max_num_channels, int vertical)
  {
  int index, i;
  
  bg_gtk_vumeter_t * ret = calloc(1, sizeof(*ret));
  ret->max_channels = max_num_channels;

  ret->layout = gtk_layout_new(NULL, NULL);
  
  ret->vertical = vertical;
  
  gtk_widget_set_events(ret->layout, GDK_EXPOSURE_MASK);
  
  g_signal_connect(G_OBJECT(ret->layout), "expose-event",
                   G_CALLBACK(expose_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->layout), "size-allocate",
                   G_CALLBACK(size_allocate_callback), (gpointer)ret);
  
  gtk_widget_show(ret->layout);

  if(vertical)
    gtk_layout_set_size(GTK_LAYOUT(ret->layout), 40, 100);
  else
    gtk_layout_set_size(GTK_LAYOUT(ret->layout), 100, 40);
    
  
  /* Create labels */
  for(i = 0; i < NUM_TICS; i++)
    {
    ret->labels[i].l = gtk_label_new(scale_tics[i].label);
    
    g_signal_connect(G_OBJECT(ret->labels[i].l),
                     "size-request", G_CALLBACK(label_size_request_callback),
                     (gpointer)ret);
    gtk_widget_show(ret->labels[i].l);

    gtk_layout_put(GTK_LAYOUT(ret->layout), ret->labels[i].l, 100, 100);
    }
  
  index = 0;
  
  while(parameters[index].name)
    {
    if(parameters[index].type != BG_PARAMETER_SECTION)
      bg_gtk_vumeter_set_parameter(ret, parameters[index].name,
                                   &(parameters[index].val_default));
    index++;
    }

  gtk_widget_show(ret->layout);
  
  ret->pd = gavl_peak_detector_create();
  
  pthread_mutex_init(&(ret->analysis_mutex), (pthread_mutexattr_t*)0);
  return ret;
  }

GtkWidget *
bg_gtk_vumeter_get_widget(bg_gtk_vumeter_t * m)
  {
  return m->layout;
  }

void bg_gtk_vumeter_set_format(bg_gtk_vumeter_t * m,
                               gavl_audio_format_t * format)
  {
  gavl_peak_detector_set_format(m->pd, format);
  m->num_channels = format->num_channels;
  }

void bg_gtk_vumeter_update(bg_gtk_vumeter_t * m,
                           gavl_audio_frame_t * frame)
  {
  
  }

void bg_gtk_vumeter_draw(bg_gtk_vumeter_t * m)
  {
  pthread_mutex_lock(&(m->analysis_mutex));
  draw_dynamic(m);
  pthread_mutex_unlock(&(m->analysis_mutex));
  flash(m);
  }

void bg_gtk_vumeter_destroy(bg_gtk_vumeter_t * m)
  {
  
  }
