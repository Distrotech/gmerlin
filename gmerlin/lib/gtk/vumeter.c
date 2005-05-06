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

#include <gtk/gtk.h>
#include <gui_gtk/audio.h>

#include <utils.h>

/* Some constants */

#define BORDER_WIDTH  3
#define TIC_LENGTH    5
#define METER_WIDTH  14
#define OVER_HEIGHT  7

static int scale_tics[] =
  {
    0,
    -1,
    -3,
    -6,
    -10,
    -20,
    -30,
    -40,
    -50,
    -60
  };

static int num_scale_tics =
sizeof(scale_tics)/sizeof(scale_tics[0]);

#define LEVEL_MIN (float)(scale_tics[num_scale_tics-1])
#define LEVEL_MAX (float)(scale_tics[0])

#define LEVEL_1   -6.0
#define LEVEL_2   -3.0

/* Functions for getting the audio level */

/* Return value is overflow (boolean) */

static int get_level_s_16(gavl_audio_frame_t * frame,
                          int channel,
                          float * level)
  {
  int i;
  int ret = 0;
  int16_t sample;
  uint16_t max_sample = 0;
  uint16_t abs_sample;
  int num_overflow;
  
  num_overflow = 0;
  for(i = 0; i < frame->valid_samples; i++)
    {
    sample = frame->channels.s_16[channel][i];

    /* Detect overflow */
    
    if(!ret)
      {
      if((sample >= 32767) || ((sample <= -32768)))
        {
        num_overflow++;
        //        fprintf(stderr, "Overflow: %d\n", num_overflow);
        }
      //      else if(num_overflow < 3)
      //        num_overflow = 0;

      if(num_overflow >= 3)
        {
        ret = 1;
        }
      
      }

    /* Detect peak */

    abs_sample = abs(sample);
    if(abs_sample > max_sample)
      max_sample = abs_sample;
    }
  
  *level = (float)max_sample / 32768.0;
  return ret;
  }


static int get_level_u_16(gavl_audio_frame_t * frame,
                             int channel,
                             float * level)
  {
  int ret = 0;
  return ret;
  }

static int get_level_s_8(gavl_audio_frame_t * frame,
                         int channel,
                         float * level)
  {
  int ret = 0;
  return ret;
  

  }

static int get_level_s_32(gavl_audio_frame_t * frame,
                          int channel,
                          float * level)
  {
  int ret = 0;
  return ret;
  

  }

static int get_level_u_8(gavl_audio_frame_t * frame,
                         int num_channels,
                         float * level)
  {
  int ret = 0;
  return ret;
  
  }

static int get_level_float(gavl_audio_frame_t * frame,
                           int num_channels,
                           float * level)
  {
  int ret = 0;
  return ret;
  
  }



static bg_parameter_info_t parameters[] =
  {
    {
      name:        "appearance",
      long_name:   "Appearance",
      type:        BG_PARAMETER_SECTION
    },
    {
      name:        "font",
      long_name:   "Font",
      type:        BG_PARAMETER_FONT,
      val_default: { val_str: "9x15" },
    },
    {
      name:        "background",
      long_name:   "Background color",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 0.0, 0.0, 0.0, 1.0 } },
    },
    {
      name:        "foreground",
      long_name:   "Foreground color",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 1.0, 1.0, 1.0, 1.0 } },
    },
    {
      name:        "level_low",
      long_name:   "Low level color",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 0.0, 1.0, 0.0, 1.0 } },
    },
    {
      name:        "level_medium",
      long_name:   "Medium level color",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 1.0, 1.0, 0.0, 1.0 } },
    },
    {
      name:        "level_high",
      long_name:   "High level color",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 1.0, 0.0, 0.0, 1.0 } },
    },
    {
      name:        "properties",
      long_name:   "Properties",
      type:        BG_PARAMETER_SECTION
    },
    {
      name:        "peak_age",
      long_name:   "Peak hold time (sec.)",
      type:        BG_PARAMETER_SLIDER_FLOAT,
      val_min:     { val_f: 0.1 },
      val_max:     { val_f: 100.0 },
      val_default: { val_f: 4.0 }
    },
    { /* End of parameters */ }
  };

struct bg_gtk_vumeter_s
  {
  GtkWidget * drawingarea;
  GdkPixmap * pixmap;
  GdkFont   * font;

  gavl_audio_converter_t * cnv;
  gavl_audio_frame_t * frame;

  gavl_audio_format_t internal_format;

  /* Colors */

  GdkColor gdk_background;
  GdkColor gdk_foreground;
  GdkColor gdk_level_low;
  GdkColor gdk_level_medium;
  GdkColor gdk_level_high;

  float background[3];
  float foreground[3];
  float level_low[3];
  float level_medium[3];
  float level_high[3];
  char * fontname;
  
  /* GC */

  GdkGC * gc;
    
  int width;
  int height;
  int realized;
  int max_channels;

  int scale_width;
  int scale_ascent;
  int scale_descent;
    
  int meter_width;
  int meter_offset_y;
  int meter_height;
  int over_height;
    
  pthread_mutex_t analysis_mutex;

  /* Analysis stuff, level values are in dB */

  int (*get_level)(gavl_audio_frame_t *,int, float*);
  float level[GAVL_MAX_CHANNELS];

  float level_dB[GAVL_MAX_CHANNELS];
  float peak_level_dB[GAVL_MAX_CHANNELS];
  float peak_age[GAVL_MAX_CHANNELS];
  int   overflow[GAVL_MAX_CHANNELS];
  float max_peak_age;
  };

/* Calculate coordinates */

static void get_coords(bg_gtk_vumeter_t * v)
  {
  int x_size;
  int y_size;
  
  v->scale_width = gdk_string_width(v->font, "-60");

  v->scale_ascent = (v->font->ascent / 2) + v->font->ascent % 2;
  v->scale_descent = (v->font->ascent / 2);

  v->meter_offset_y = OVER_HEIGHT + 2 * BORDER_WIDTH;
  v->meter_height   =
    v->height - v->meter_offset_y - BORDER_WIDTH - v->scale_descent;
  v->meter_width = METER_WIDTH;
  v->over_height = OVER_HEIGHT;

  x_size = BORDER_WIDTH + v->scale_width + BORDER_WIDTH +
    v->max_channels * (v->meter_width + TIC_LENGTH) + BORDER_WIDTH;
  
  y_size = v->meter_offset_y + v->scale_descent +
    (num_scale_tics-1)*(v->scale_ascent+v->scale_descent+BORDER_WIDTH);
  
  gtk_widget_set_size_request(v->drawingarea, x_size, y_size);
  }

/* Parameter stuff */

bg_parameter_info_t *
bg_gtk_vumeter_get_parameters(bg_gtk_vumeter_t * v)
  {
  return parameters;
  }

static void set_color(bg_gtk_vumeter_t * v,
                      float * color,
                      GdkColor * gdk_color)
  {
  if(!v->realized)
    return;
  
  gdk_color->red   = (guint16)(color[0] * 65535.0);
  gdk_color->green = (guint16)(color[1] * 65535.0);
  gdk_color->blue  = (guint16)(color[2] * 65535.0);

  gdk_color->pixel =
    ((gdk_color->red >> 8)   << 16) |
    ((gdk_color->green >> 8) << 8) |
    ((gdk_color->blue >> 8));

  gdk_color_alloc(gdk_window_get_colormap(v->drawingarea->window),
                  gdk_color);
  
  }

void bg_gtk_vumeter_set_parameter(void * vumeter, const char * name,
                                  bg_parameter_value_t * val)
  {
  bg_gtk_vumeter_t * v;
  v = (bg_gtk_vumeter_t *)vumeter;
  if(!name)
    return;
  if(!strcmp(name, "foreground"))
    {
    memcpy(v->foreground, val->val_color, 3 * sizeof(float));
    set_color(v, v->foreground, &(v->gdk_foreground));
    }
  else if(!strcmp(name, "background"))
    {
    memcpy(v->background, val->val_color, 3 * sizeof(float));
    set_color(v, v->background, &(v->gdk_background));
    }
  else if(!strcmp(name, "level_low"))
    {
    memcpy(v->level_low, val->val_color, 3 * sizeof(float));
    set_color(v, v->level_low, &(v->gdk_level_low));
    }
  else if(!strcmp(name, "level_medium"))
    {
    memcpy(v->level_medium, val->val_color, 3 * sizeof(float));
    set_color(v, v->level_medium, &(v->gdk_level_medium));
    }
  else if(!strcmp(name, "level_high"))
    {
    memcpy(v->level_high, val->val_color, 3 * sizeof(float));
    set_color(v, v->level_high, &(v->gdk_level_high));
    }
  else if(!strcmp(name, "font"))
    {
    if((!v->fontname) || (strcmp(val->val_str, v->fontname)))
      {
      if(v->font)
        gdk_font_unref(v->font);
      v->fontname = bg_strdup(v->fontname, val->val_str);
      v->font = gdk_font_load(v->fontname);
      get_coords(v);
      }
    }
  else if(!strcmp(name, "peak_age"))
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

static void analyze(bg_gtk_vumeter_t * m)
  {
  int i;

  float level;
  float frame_time;
  int overflow = 0;
  
  gavl_time_t gavl_time;

  gavl_time = gavl_samples_to_time(m->internal_format.samplerate,
                                   m->frame->valid_samples);

  frame_time = gavl_time_to_seconds(gavl_time);
    
  for(i = 0; i < m->internal_format.num_channels; i++)
    {
    m->peak_age[i] += frame_time;

    if(m->get_level)
      overflow = m->get_level(m->frame, i, &level);
    
    if(!m->overflow[i] && overflow)
      {
      m->overflow[i] = 1;
      }
    
    m->level[i] *= 0.94;

    if(level > m->level[i])
      m->level[i] = level;

    /* Convert to dB */
    
    m->level_dB[i] = level_2_dB(m->level[i]);

    if((m->peak_level_dB[i] < m->level_dB[i]) ||
       (m->peak_age[i] > m->max_peak_age))
      {
      m->peak_level_dB[i] = m->level_dB[i];
      m->peak_age[i] = 0.0;
      }
    }
  }

static void draw_static(bg_gtk_vumeter_t * m)
  {
  int i, j;
  char buf[10];
  int x_1, y_1, x_2, y_2;
  GdkPoint over_coords[3];
    
  gdk_gc_set_foreground(m->gc, &(m->gdk_background));

  gdk_draw_rectangle(m->pixmap, m->gc, 1, 0, 0, m->width, m->height);

  /* Draw the scale */

  gdk_gc_set_foreground(m->gc, &(m->gdk_foreground));
  for(i = 0; i < num_scale_tics; i++)
    {
    y_1 = m->meter_offset_y + m->meter_height * i / (num_scale_tics-1);
    sprintf(buf, "%d", scale_tics[i]);
    gdk_draw_string(m->pixmap, m->font, m->gc,
                    BORDER_WIDTH + m->scale_width -
                    gdk_string_width(m->font, buf),
                    y_1 + m->scale_descent,
                    buf);
    for(j = 0; j < m->max_channels; j++)
      {
      x_1 = 2 * BORDER_WIDTH + m->scale_width +
        j * (m->meter_width + TIC_LENGTH);
      x_2 = x_1 + TIC_LENGTH;
      
      gdk_draw_line(m->pixmap,m->gc,
                    x_1, y_1,
                    x_2, y_1);
      }
    }
  /* Draw meters */

  y_1 = m->meter_offset_y;
  y_2 = y_1 + m->meter_height;
  
  for(j = 0; j < m->max_channels; j++)
    {
    x_1 = 2 * BORDER_WIDTH + m->scale_width + TIC_LENGTH +
      j * (m->meter_width + TIC_LENGTH);
    x_2 = x_1 + m->meter_width;

    gdk_draw_rectangle(m->pixmap, m->gc,
                       0, x_1, y_1, x_2 - x_1, y_2 - y_1);

    /* Draw Overflow indicator */
    
    over_coords[0].x = x_1;
    over_coords[0].y = y_1 - BORDER_WIDTH;

    over_coords[1].x = x_2;
    over_coords[1].y = y_1 - BORDER_WIDTH;

    over_coords[2].x = (x_1 + x_2) >> 1;
    over_coords[2].y = y_1 - BORDER_WIDTH - m->over_height;

    gdk_draw_polygon(m->pixmap, m->gc,
                     0, over_coords, 3);
    
    }

  
  }

static int get_y(bg_gtk_vumeter_t * m, float level)
  {
  int i;
  int ret = 0;
  int y_1, y_2;
    
  if(level < LEVEL_MIN)
    return m->meter_offset_y + m->meter_offset_y - 2;

  if(level >= LEVEL_MAX)
    return m->meter_offset_y + 1;
  
  for(i = 0; i < num_scale_tics-1; i++)
    {
    if((level >= (float)scale_tics[i+1]) &&
       (level < (float)scale_tics[i]))
      break;
    }

  y_1 = m->meter_offset_y + m->meter_height * i     / (num_scale_tics-1);
  y_2 = m->meter_offset_y + m->meter_height * (i+1) / (num_scale_tics-1);

  ret = (int)(y_2 + (y_1 - y_2) *
              (level-scale_tics[i+1])/(scale_tics[i]-scale_tics[i+1]) + 0.5);
  if(ret < m->meter_offset_y + 1)
    {
    ret = m->meter_offset_y + 1;
    }
  
  return ret;
  
  }

static void draw_dynamic(bg_gtk_vumeter_t * m)
  {
  int i;
  int imax;
  int x_1, x_2, y_1, y_2;

  GdkPoint over_coords[3];
    
  imax = m->internal_format.num_channels > m->max_channels ?
    m->max_channels : m->internal_format.num_channels;

  for(i = 0; i < imax; i++)
    {
    x_1 = 2 * BORDER_WIDTH + m->scale_width + TIC_LENGTH +
      i * (m->meter_width + TIC_LENGTH);
    x_2 = x_1 + m->meter_width;

    x_1++;
    
    /* Clear background */

    y_1 = m->meter_offset_y + 1;
    y_2 = m->meter_offset_y + m->meter_height;

    gdk_gc_set_foreground(m->gc, &(m->gdk_background));
    gdk_draw_rectangle(m->pixmap, m->gc, 1, x_1, y_1, x_2 - x_1, y_2 - y_1);

    /* Lowest Level */
    
    gdk_gc_set_foreground(m->gc, &(m->gdk_level_low));
    
    if(m->level_dB[i] >= LEVEL_1)
      y_1 = get_y(m, LEVEL_1);
    else
      y_1 = get_y(m, m->level_dB[i]);

    gdk_draw_rectangle(m->pixmap, m->gc, 1, x_1, y_1, x_2 - x_1, y_2 - y_1);
    
    if(m->level_dB[i] > LEVEL_1)
      {
      y_2 = y_1;
      gdk_gc_set_foreground(m->gc, &(m->gdk_level_medium));
      
      if(m->level_dB[i] >= LEVEL_2)
        y_1 = get_y(m, LEVEL_2);
      else
        y_1 = get_y(m, m->level_dB[i]);
      gdk_draw_rectangle(m->pixmap, m->gc, 1, x_1, y_1, x_2 - x_1, y_2 - y_1);
      }
    if(m->level_dB[i] > LEVEL_2)
      {
      gdk_gc_set_foreground(m->gc, &(m->gdk_level_high));
      y_2 = y_1;
      y_1 = get_y(m, m->level_dB[i]);
      gdk_draw_rectangle(m->pixmap, m->gc, 1, x_1, y_1, x_2 - x_1, y_2 - y_1);
      }
    
    /* Draw peak */

    if(m->peak_level_dB[i] > m->level_dB[i])
      {
      if(m->peak_level_dB[i] >= LEVEL_2)
        gdk_gc_set_foreground(m->gc, &(m->gdk_level_high));
      else if(m->peak_level_dB[i] >= LEVEL_1)
        gdk_gc_set_foreground(m->gc, &(m->gdk_level_medium));
      else
        gdk_gc_set_foreground(m->gc, &(m->gdk_level_low));

      y_1 = get_y(m, m->peak_level_dB[i]);
      gdk_draw_line(m->pixmap,m->gc, x_1, y_1, x_2-1, y_1);
      gdk_draw_line(m->pixmap,m->gc, x_1, y_1+1, x_2-1, y_1+1);
      }
    /* Draw overflow */

    y_1 = m->meter_offset_y - BORDER_WIDTH;
    
    over_coords[0].x = x_1;
    over_coords[0].y = y_1;
    
    over_coords[1].x = x_2;
    over_coords[1].y = y_1;
    
    over_coords[2].x = (x_1 + x_2) >> 1;
    over_coords[2].y = y_1 - m->over_height;

    if(m->overflow[i])
      gdk_gc_set_foreground(m->gc, &(m->gdk_level_high));
    else
      gdk_gc_set_foreground(m->gc, &(m->gdk_background));

    gdk_draw_polygon(m->pixmap, m->gc,
                     1, over_coords, 3);
    }
  }

static void flash(bg_gtk_vumeter_t * m)
  {
  gdk_draw_drawable(m->drawingarea->window,
                    m->gc, m->pixmap,
                    0, 0, 0, 0, m->width, m->height);
  }

static void expose_callback(GtkWidget * w, GdkEventExpose *event,
                            gpointer data)
  {
  int width, height, x, y, depth;
  bg_gtk_vumeter_t * m = (bg_gtk_vumeter_t *)data;
  
  gdk_window_get_geometry(m->drawingarea->window, &x, &y, &width, &height,
                          &depth);


  if((width != m->width) || (height != m->height) || !(m->pixmap))
    {
    if(m->pixmap)
      gdk_pixmap_unref(m->pixmap);

    m->width = width;
    m->height = height;
    
    m->pixmap = gdk_pixmap_new(m->drawingarea->window, width, height, depth);
    get_coords(m);
    draw_static(m);
    }
  flash(m);
  }

static gboolean mouse_callback(GtkWidget * w, GdkEventButton *event,
                               gpointer(data))
  {
  bg_gtk_vumeter_t * m;
  m = (bg_gtk_vumeter_t *)data;

  bg_gtk_vumeter_reset_overflow(m);
  return TRUE;
  
  }

static void realize_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_vumeter_t * v;
  
  v = (bg_gtk_vumeter_t *)data;
  v->gc = gdk_gc_new(v->drawingarea->window);
  
  v->realized = 1;

  set_color(v, v->foreground, &(v->gdk_foreground));
  set_color(v, v->background, &(v->gdk_background));
  set_color(v, v->level_low, &(v->gdk_level_low));
  set_color(v, v->level_medium, &(v->gdk_level_medium));
  set_color(v, v->level_high, &(v->gdk_level_high));
  }

bg_gtk_vumeter_t *
bg_gtk_vumeter_create(int max_num_channels)
  {
  int index;
  
  bg_gtk_vumeter_t * ret = calloc(1, sizeof(*ret));
  ret->max_channels = max_num_channels;

  ret->drawingarea = gtk_drawing_area_new();

  gtk_widget_set_size_request(ret->drawingarea,
                              80,
                              ret->drawingarea->requisition.height);

  gtk_widget_set_events(ret->drawingarea,
                        GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK);
  
  g_signal_connect(G_OBJECT(ret->drawingarea), "expose_event",
                   G_CALLBACK(expose_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->drawingarea), "button_press_event",
                   G_CALLBACK(mouse_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->drawingarea), "realize",
                   G_CALLBACK(realize_callback), (gpointer)ret);
    
  gtk_widget_show(ret->drawingarea);

  index = 0;

  while(parameters[index].name)
    {
    if(parameters[index].type != BG_PARAMETER_SECTION)
      bg_gtk_vumeter_set_parameter(ret, parameters[index].name,
                                   &(parameters[index].val_default));
    index++;
    }
  ret->cnv = gavl_audio_converter_create();
  pthread_mutex_init(&(ret->analysis_mutex), (pthread_mutexattr_t*)0);
  return ret;
  }

GtkWidget *
bg_gtk_vumeter_get_widget(bg_gtk_vumeter_t * m)
  {
  return m->drawingarea;
  }

void bg_gtk_vumeter_set_format(bg_gtk_vumeter_t * m,
                               gavl_audio_format_t * format)
  {

  memcpy(&(m->internal_format),
         format, sizeof(gavl_audio_format_t));

  m->internal_format.interleave_mode = GAVL_INTERLEAVE_NONE;

  switch(m->internal_format.sample_format)
    {
    case GAVL_SAMPLE_U8:
      m->get_level = get_level_u_8;
      break;
    case GAVL_SAMPLE_S8:
      m->get_level = get_level_s_8;
      break;
    case GAVL_SAMPLE_U16:
      m->get_level = get_level_u_16;
      break;
    case GAVL_SAMPLE_S16:
      m->get_level = get_level_s_16;
      break;
    case GAVL_SAMPLE_S32:
      m->get_level = get_level_s_32;
      break;
    case GAVL_SAMPLE_FLOAT:
      m->get_level = get_level_float;
      break;
    default:
      m->internal_format.sample_format = GAVL_SAMPLE_FLOAT;
      m->get_level = get_level_float;
      break;
    }
  
  gavl_audio_converter_init(m->cnv,
                            format,
                            &(m->internal_format));
  
  if(m->frame)
    gavl_audio_frame_destroy(m->frame);
  m->frame = gavl_audio_frame_create(&(m->internal_format));
  }

void bg_gtk_vumeter_update(bg_gtk_vumeter_t * m,
                           gavl_audio_frame_t * frame)
  {
  //  fprintf(stderr, "Update...");

  gavl_audio_convert(m->cnv, frame, m->frame);
  pthread_mutex_lock(&(m->analysis_mutex));
  analyze(m);
  pthread_mutex_unlock(&(m->analysis_mutex));
  //  fprintf(stderr, "done\n");
  }

void bg_gtk_vumeter_draw(bg_gtk_vumeter_t * m)
  {
  pthread_mutex_lock(&(m->analysis_mutex));
  draw_dynamic(m);
  pthread_mutex_unlock(&(m->analysis_mutex));
  flash(m);
  }

void bg_gtk_vumeter_reset_overflow(bg_gtk_vumeter_t * m)
  {
  int i;
  pthread_mutex_lock(&(m->analysis_mutex));
  for(i = 0; i < GAVL_MAX_CHANNELS; i++)
    {
    m->overflow[i] = 0;
    }
  pthread_mutex_unlock(&(m->analysis_mutex));
  }


void bg_gtk_vumeter_destroy(bg_gtk_vumeter_t * m)
  {
  
  }
