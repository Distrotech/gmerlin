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

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include "gmerlin.h"

#include <utils.h>
#include <config.h>

#include <gui_gtk/display.h>
#include <gui_gtk/gtkutils.h>
#include <gui_gtk/scrolltext.h>

#define STATE_STOPPED     0
#define STATE_PLAYING     1
#define STATE_PAUSED      2
#define STATE_CHANGING    3
#define STATE_SEEKING     4
#define STATE_BUFFERING_1 5
#define STATE_BUFFERING_2 6
#define STATE_BUFFERING_3 7
#define STATE_BUFFERING_4 8
#define STATE_BUFFERING_5 9
#define STATE_ERROR       10
#define NUM_STATES        11

#define DIGIT_HEIGHT      32
#define DIGIT_WIDTH       20

typedef enum
  {
    DISPLAY_MODE_NONE,
    DISPLAY_MODE_ALL,
    DISPLAY_MODE_REM,
    DISPLAY_MODE_ALL_REM,
    NUM_DISPLAY_MODES
  } display_mode_t; /* Mode for the time display */

static bg_parameter_info_t parameters[] =
  {
    {
      name: "get_colors_from_skin",
      long_name: "Get colors from skin",
      type: BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name:      "background",
      long_name: "Background",
      type: BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 0.0, 0.0, 0.0, 1.0 } },
    },
    {
      name:      "foreground_normal",
      long_name: "Normal foreground",
      type: BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]) { 1.0, 0.5, 0.0, 1.0 } },
    },
    {
      name:      "foreground_error",
      long_name: "Error foreground",
      type: BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 1.0, 0.0, 0.0, 1.0 } },
    },
    {
      name:      "display_mode",
      long_name: "Display mode",
      type: BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_min:     { val_i:  DISPLAY_MODE_NONE },
      val_max:     { val_i:  NUM_DISPLAY_MODES - 1 },
      val_default: { val_i:  DISPLAY_MODE_NONE },
    },
    {
      name:      "repeat_mode",
      long_name: "Repeat mode",
      type: BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_min:     { val_i:  REPEAT_MODE_NONE },
      val_max:     { val_i:  NUM_REPEAT_MODES - 1 },
      val_default: { val_i:  REPEAT_MODE_NONE },
    },
    {
      name:      "font",
      long_name: "Font",
      type: BG_PARAMETER_FONT,
      val_default: { val_str:  "Sans Bold 10" },
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t * display_get_parameters(display_t * display)
  {
  return parameters;
  }

int pixbufs_loaded = 0;

GdkPixbuf * state_pixbufs[NUM_STATES];

GdkPixbuf * repeat_pixbufs[NUM_REPEAT_MODES];

GdkPixbuf * display_pixbufs[NUM_DISPLAY_MODES];

static GdkPixbuf * load_pixbuf(const char * filename)
  {
  char * tmp_string;
  GdkPixbuf * ret;

  tmp_string = bg_search_file_read("icons", filename);

  ret = gdk_pixbuf_new_from_file(tmp_string, NULL);
  free(tmp_string);

  return ret;
  }

static void load_pixbufs()
  {
  if(pixbufs_loaded)
    return;

  state_pixbufs[STATE_STOPPED]     = load_pixbuf("state_stopped.png");
  state_pixbufs[STATE_PLAYING]     = load_pixbuf("state_playing.png");
  state_pixbufs[STATE_PAUSED]      = load_pixbuf("state_paused.png");
  state_pixbufs[STATE_CHANGING]    = load_pixbuf("state_changing.png");
  state_pixbufs[STATE_SEEKING]     = load_pixbuf("state_seeking.png");
  state_pixbufs[STATE_BUFFERING_1] = load_pixbuf("state_buffering_1.png");
  state_pixbufs[STATE_BUFFERING_2] = load_pixbuf("state_buffering_2.png");
  state_pixbufs[STATE_BUFFERING_3] = load_pixbuf("state_buffering_3.png");
  state_pixbufs[STATE_BUFFERING_4] = load_pixbuf("state_buffering_4.png");
  state_pixbufs[STATE_BUFFERING_5] = load_pixbuf("state_buffering_5.png");
  state_pixbufs[STATE_ERROR]       = load_pixbuf("state_error.png");

  repeat_pixbufs[REPEAT_MODE_NONE] = load_pixbuf("repeat_mode_none.png");
  repeat_pixbufs[REPEAT_MODE_ALL]  = load_pixbuf("repeat_mode_all.png");
  repeat_pixbufs[REPEAT_MODE_1]    = load_pixbuf("repeat_mode_1.png");

  display_pixbufs[DISPLAY_MODE_NONE] = load_pixbuf("display_mode_none.png");
  display_pixbufs[DISPLAY_MODE_REM]  = load_pixbuf("display_mode_rem.png");
  display_pixbufs[DISPLAY_MODE_ALL]  = load_pixbuf("display_mode_all.png");
  display_pixbufs[DISPLAY_MODE_ALL_REM] = load_pixbuf("display_mode_all_rem.png");

  pixbufs_loaded = 1;
  }

struct display_s
  {
  bg_gtk_time_display_t * time_display;
  bg_gtk_scrolltext_t * scrolltext;

  GtkWidget * widget;
  GtkWidget * state_area;
  GtkWidget * repeat_area;
  GtkWidget * display_area;

  GdkGC * gc;
  
  GdkPixbuf * state_pixbufs[NUM_STATES];
  GdkPixbuf * repeat_pixbufs[NUM_REPEAT_MODES];
  GdkPixbuf * display_pixbufs[NUM_DISPLAY_MODES];

  float foreground_error[3];
  float foreground_normal[3];
  float background[3];

  /* From the config file */
  
  float foreground_error_cfg[3];
  float foreground_normal_cfg[3];
  float background_cfg[3];

  int get_colors_from_skin;
    
  gmerlin_t * gmerlin;
  repeat_mode_t repeat_mode;
  display_mode_t display_mode;
  int state_index;
  display_skin_t * skin;

  int error_active;

  char * track_name;

  gavl_time_t duration_before;
  gavl_time_t duration_after;
  gavl_time_t duration_current;
  gavl_time_t last_time;
  
  guint32 last_click_time;
  };

static void update_background(display_t * d)
  {
  GdkColor col;

  col.red   = (guint16)(d->background[0] * 65535.0);
  col.green = (guint16)(d->background[1] * 65535.0);
  col.blue  = (guint16)(d->background[2] * 65535.0);

  col.pixel =
    ((col.red >> 8)   << 16) |
    ((col.green >> 8) << 8) |
    ((col.blue >> 8));
  
  gdk_color_alloc(gdk_window_get_colormap(d->widget->window),
                  &col);
  gdk_gc_set_foreground(d->gc, &col);
  gdk_window_clear_area_e(GTK_LAYOUT(d->widget)->bin_window,
                          0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  }

static void update_colors(display_t * d)
  {
  int i;
  //  fprintf(stderr, "Update colors\n");
  
  for(i = 0; i < NUM_STATES; i++)
    {
    if(d->state_pixbufs[i])
      g_object_unref(G_OBJECT(d->state_pixbufs[i]));
    d->state_pixbufs[i] = bg_gtk_pixbuf_scale_alpha(state_pixbufs[i],
                                                    20, 32,
                                                    d->foreground_normal,
                                                    d->background);
    }
  for(i = 0; i < NUM_REPEAT_MODES; i++)
    {
    if(d->repeat_pixbufs[i])
      g_object_unref(G_OBJECT(d->repeat_pixbufs[i]));
    d->repeat_pixbufs[i] = bg_gtk_pixbuf_scale_alpha(repeat_pixbufs[i],
                                                     40, 16,
                                                     d->foreground_normal,
                                                     d->background);
    }
  for(i = 0; i < NUM_DISPLAY_MODES; i++)
    {
    if(d->display_pixbufs[i])
      g_object_unref(G_OBJECT(d->display_pixbufs[i]));
    d->display_pixbufs[i] = bg_gtk_pixbuf_scale_alpha(display_pixbufs[i],
                                                      40, 16,
                                                      d->foreground_normal,
                                                      d->background);
    }
  if(d->state_area->window)
    gdk_window_clear_area_e(d->state_area->window, 0, 0, 20, 32);

  if(d->repeat_area->window)
    gdk_window_clear_area_e(d->repeat_area->window, 0, 0, 40, 16);

  if(d->display_area->window)
    gdk_window_clear_area_e(d->display_area->window, 0, 0, 40, 16);

  if(d->error_active)
    bg_gtk_scrolltext_set_colors(d->scrolltext,
                                 d->foreground_error, d->background);
  else
    bg_gtk_scrolltext_set_colors(d->scrolltext,
                                 d->foreground_normal, d->background);

  bg_gtk_time_display_set_colors(d->time_display,
                                 d->foreground_normal,
                                 d->background);

  if(d->gc)
    update_background(d);
  
  
  }
static gboolean expose_callback(GtkWidget * w, GdkEventExpose * evt,
                                gpointer data)
  {
  display_t * d = (display_t*)data;
  if(!d->gc)
    return TRUE;
  if(w == d->widget)
    {
    gdk_draw_rectangle(GTK_LAYOUT(d->widget)->bin_window,
                       d->gc,
                       TRUE,
                       0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT); 
    }
  if(w == d->state_area)
    {
    gdk_draw_pixbuf(d->state_area->window,
                    (GdkGC*)0,
                    d->state_pixbufs[d->state_index],
                    0, // gint src_x,
                    0, // gint src_y,
                    0, // gint dest_x,
                    0, // gint dest_y,
                    -1, // gint width,
                    -1, // gint height,
                    GDK_RGB_DITHER_NONE, // GdkRgbDither dither,
                    0, // gint x_dither,
                    0 // gint y_dither
                    );
    }
  if(w == d->repeat_area)
    {
    gdk_draw_pixbuf(d->repeat_area->window,
                    (GdkGC*)0,
                    d->repeat_pixbufs[d->repeat_mode],
                    0, // gint src_x,
                    0, // gint src_y,
                    0, // gint dest_x,
                    0, // gint dest_y,
                    -1, // gint width,
                    -1, // gint height,
                    GDK_RGB_DITHER_NONE, // GdkRgbDither dither,
                    0, // gint x_dither,
                    0 // gint y_dither
                    );
    }
  if(w == d->display_area)
    {
    gdk_draw_pixbuf(d->display_area->window,
                    (GdkGC*)0,
                    d->display_pixbufs[d->display_mode],
                    0, // gint src_x,
                    0, // gint src_y,
                    0, // gint dest_x,
                    0, // gint dest_y,
                    -1, // gint width,
                    -1, // gint height,
                    GDK_RGB_DITHER_NONE, // GdkRgbDither dither,
                    0, // gint x_dither,
                    0 // gint y_dither
                    );
    }
  return TRUE;
  }

static void set_display_mode(display_t * d)
  {
  if(d->display_mode == NUM_DISPLAY_MODES)
    d->display_mode = 0;
  expose_callback(d->display_area, (GdkEventExpose*)0,
                  d);
  display_set_time(d, d->last_time);
  }

static void set_repeat_mode(display_t * d)
  {
  if(d->repeat_mode == NUM_REPEAT_MODES)
    d->repeat_mode = 0;
  expose_callback(d->repeat_area, (GdkEventExpose*)0,
                  d);
  d->gmerlin->repeat_mode = d->repeat_mode;
  }

void display_set_parameter(void * data, char * name,
                           bg_parameter_value_t * v)
  {
  display_t * d = (display_t*)data;
  if(!name)
    {
    if(d->get_colors_from_skin && d->skin)
      {
      memcpy(d->foreground_normal, d->skin->foreground_normal,
             3 * sizeof(float));
      memcpy(d->foreground_error, d->skin->foreground_error,
             3 * sizeof(float));
      memcpy(d->background, d->skin->background,
             3 * sizeof(float));
      update_colors(d);
      }
    else
      {
      memcpy(d->foreground_normal, d->foreground_normal_cfg,
             3 * sizeof(float));
      memcpy(d->foreground_error, d->foreground_error_cfg,
             3 * sizeof(float));
      memcpy(d->background, d->background_cfg,
             3 * sizeof(float));
      update_colors(d);
      }
    }
  else if(!strcmp(name, "get_colors_from_skin"))
    d->get_colors_from_skin = v->val_i;
  else if(!strcmp(name, "foreground_error"))
    memcpy(d->foreground_error_cfg, v->val_color, 3 * sizeof(float));
  else if(!strcmp(name, "foreground_normal"))
    memcpy(d->foreground_normal_cfg, v->val_color, 3 * sizeof(float));
  else if(!strcmp(name, "background"))
    memcpy(d->background_cfg, v->val_color, 3 * sizeof(float));
  else if(!strcmp(name, "display_mode"))
    {
    d->display_mode = v->val_i;
    set_display_mode(d);
    }
  else if(!strcmp(name, "repeat_mode"))
    {
    d->repeat_mode = v->val_i;
    set_repeat_mode(d);
    }
  else if(!strcmp(name, "font"))
    {
    bg_gtk_scrolltext_set_font(d->scrolltext, v->val_str);
    }
  }

int display_get_parameter(void * data, char * name,
                           bg_parameter_value_t * v)
  {
  display_t * d = (display_t*)data;

  // fprintf(stderr, "display_get_parameter, %s\n", name);

  if(!strcmp(name, "display_mode"))
    {
    v->val_i = d->display_mode;
    return 1;
    }
  else if(!strcmp(name, "repeat_mode"))
    {
    v->val_i = d->repeat_mode;
    return 1;
    }
  return 0;
  }

static void realize_callback(GtkWidget * w, gpointer data)
  {
  display_t * d = (display_t*)data;
  d->gc = gdk_gc_new(d->widget->window);
  update_background(d);
  }

static gboolean button_press_callback(GtkWidget * w, GdkEventButton * evt,
                                      gpointer data)
  {
  display_t * d = (display_t*)data;

  if(evt->time == d->last_click_time)
    return FALSE;
  
  if(w == d->repeat_area)
    {
    d->repeat_mode++;
    set_repeat_mode(d);
    d->last_click_time = evt->time;
    return TRUE;
    }
  else if(w == d->display_area)
    {
    d->display_mode++;
    set_display_mode(d);
    d->last_click_time = evt->time;
    return TRUE;
    }
  return FALSE;
  }

display_t * display_create(gmerlin_t * gmerlin, GtkTooltips * tooltips)
  {
  display_t * ret;
  bg_cfg_section_t * cfg_section;
  load_pixbufs();
  ret = calloc(1, sizeof(*ret));
    
  /* Create objects */
  ret->gmerlin = gmerlin;
  
  ret->widget = gtk_layout_new((GtkAdjustment*)0, (GtkAdjustment*)0);
  g_signal_connect(G_OBJECT(ret->widget),
                   "expose_event", G_CALLBACK(expose_callback),
                   (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->widget),
                   "realize", G_CALLBACK(realize_callback),
                   (gpointer)ret);
  
  ret->state_index = STATE_STOPPED;
  
  ret->scrolltext = bg_gtk_scrolltext_create(226, 18);

  /* State area */
  
  ret->state_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(ret->state_area, 20, 32);

  g_signal_connect(G_OBJECT(ret->state_area),
                   "expose_event", G_CALLBACK(expose_callback),
                   (gpointer)ret);

  
  gtk_widget_show(ret->state_area);

  /* Repeat area */
  
  ret->repeat_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(ret->repeat_area, 40, 16);

  gtk_widget_set_events(ret->repeat_area,
                        GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);

  gtk_tooltips_set_tip(tooltips, ret->repeat_area,
                       "Repeat mode\nClick to change",
                       "Repeat mode\nClick to change");
  
  g_signal_connect(G_OBJECT(ret->repeat_area),
                   "button_press_event",
                   G_CALLBACK (button_press_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->repeat_area),
                   "expose_event", G_CALLBACK(expose_callback),
                   (gpointer)ret);
  
  gtk_widget_show(ret->repeat_area);

  /* Display mode area */
  
  ret->display_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(ret->display_area, 40, 16);

  gtk_widget_set_events(ret->display_area,
                        GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  gtk_tooltips_set_tip(tooltips, ret->display_area,
                       "Time display mode\nClick to change",
                       "Time display mode\nClick to change");
  
  
  g_signal_connect(G_OBJECT(ret->display_area),
                   "button_press_event",
                   G_CALLBACK (button_press_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->display_area),
                   "expose_event", G_CALLBACK(expose_callback),
                   (gpointer)ret);
  
  gtk_widget_show(ret->display_area);

  /* Scrolltext */
  
  bg_gtk_scrolltext_set_font(ret->scrolltext, "Sans Bold 10");

  ret->time_display = bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_NORMAL, 0);

  
  /* Set attributes */

  gtk_widget_set_size_request(ret->widget, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  
  /* Set Callbacks */

  /* Pack */

  gtk_layout_put(GTK_LAYOUT(ret->widget),
                 bg_gtk_scrolltext_get_widget(ret->scrolltext),
                 3, 38);

  gtk_layout_put(GTK_LAYOUT(ret->widget),
                 ret->state_area,
                 3, 3);

  gtk_layout_put(GTK_LAYOUT(ret->widget),
                 ret->display_area,
                 189, 3);

  gtk_layout_put(GTK_LAYOUT(ret->widget),
                 ret->repeat_area,
                 189, 19);

  gtk_layout_put(GTK_LAYOUT(ret->widget),
                 bg_gtk_time_display_get_widget(ret->time_display),
                 26, 3);
  
  gtk_widget_show(ret->widget);

  cfg_section = bg_cfg_registry_find_section(gmerlin->cfg_reg, "Display");
  return ret;
  }

GtkWidget * display_get_widget(display_t * d)
  {
  return d->widget;
  }

void display_destroy(display_t * d)
  {
  bg_gtk_time_display_destroy(d->time_display);
  bg_gtk_scrolltext_destroy(d->scrolltext);
  free(d);
  }

void display_set_playlist_times(display_t * d,
                                gavl_time_t duration_before,
                                gavl_time_t duration_current,
                                gavl_time_t duration_after)
  {
  d->duration_before  = duration_before;
  d->duration_current = duration_current;
  d->duration_after   = duration_after;
  }

void display_set_time(display_t * d, gavl_time_t time)
  {
  gavl_time_t display_time;
  d->last_time = time;
  if(d->state_index ==  STATE_STOPPED)
    display_time = d->duration_current + d->duration_before +
      d->duration_after;
  else
    {
    switch(d->display_mode)
      {
      case DISPLAY_MODE_NONE:
        display_time = time;
      break;
      case DISPLAY_MODE_ALL:
        if(d->duration_before == GAVL_TIME_UNDEFINED)
          display_time = GAVL_TIME_UNDEFINED;
        else
          display_time = time + d->duration_before;
        break;
      case DISPLAY_MODE_REM:
        if(d->duration_current == GAVL_TIME_UNDEFINED)
          display_time = GAVL_TIME_UNDEFINED;
        else
          display_time = d->duration_current - time;
        break;
      case DISPLAY_MODE_ALL_REM:
        if((d->duration_current == GAVL_TIME_UNDEFINED) ||
           (d->duration_after == GAVL_TIME_UNDEFINED))
          display_time = GAVL_TIME_UNDEFINED;
        else
          display_time = d->duration_after + d->duration_current - time;
        break;
      default:
        display_time = time;
        break;
      }
    }
  bg_gtk_time_display_update(d->time_display, display_time);
  }

void display_set_state(display_t * d, int state,
                       const void * arg)
  {
  float percentage;
  switch(state)
    {
    case BG_PLAYER_STATE_STOPPED:

      //      fprintf(stderr, "display_set_state: Stopped\n");
      d->state_index = STATE_STOPPED;
      display_set_track_name(d, "Gmerlin player (version "VERSION")");

      bg_gtk_time_display_update(d->time_display,
                                 d->duration_before+d->duration_current+
                                 d->duration_after);
      break;
    case BG_PLAYER_STATE_SEEKING:
      //      fprintf(stderr, "STATE SEEKING\n");
      d->state_index = STATE_SEEKING;
      break;
    case BG_PLAYER_STATE_CHANGING:
    case BG_PLAYER_STATE_STARTING:
      //      fprintf(stderr, "STATE SEEKING\n");
      d->state_index = STATE_CHANGING;
      break;
    case BG_PLAYER_STATE_BUFFERING:
      percentage = *((float*)arg);
      if((d->state_index != STATE_BUFFERING_5) &&
         (d->state_index != STATE_BUFFERING_4) &&
         (d->state_index != STATE_BUFFERING_3) &&
         (d->state_index != STATE_BUFFERING_2) &&
         (d->state_index != STATE_BUFFERING_1))
        {
        bg_gtk_scrolltext_set_text(d->scrolltext,
                                   "Buffering...",
                                   d->foreground_normal, d->background);

        }
      if(percentage > 0.8)
        d->state_index = STATE_BUFFERING_5;
      else if(percentage > 0.6)
        d->state_index = STATE_BUFFERING_4;
      else if(percentage > 0.4)
        d->state_index = STATE_BUFFERING_3;
      else if(percentage > 0.2)
        d->state_index = STATE_BUFFERING_2;
      else
        d->state_index = STATE_BUFFERING_1;
      break;
    case BG_PLAYER_STATE_PAUSED:
      d->state_index = STATE_PAUSED;
      break;
    default: /* BG_PLAYER_STATE_PLAYING, BG_PLAYER_STATE_FINISHING */
      if(d->state_index != STATE_PLAYING)
        bg_gtk_scrolltext_set_text(d->scrolltext,
                                   d->track_name,
                                   d->foreground_normal, d->background);
      d->state_index = STATE_PLAYING;
      break;
    case BG_PLAYER_STATE_ERROR:
      d->state_index = STATE_ERROR;
      bg_gtk_scrolltext_set_text(d->scrolltext,
                                 (const char*)arg,
                                 d->foreground_error, d->background);
      
      break;
    }
  expose_callback(d->state_area, (GdkEventExpose*)0, d);
  }

void display_skin_load(display_skin_t * s,
                       xmlDocPtr doc, xmlNodePtr node)
  {
  char * tmp_string;
  char * rest;
  char * pos;
  char * old_locale;
  
  node = node->children;
  old_locale = setlocale(LC_NUMERIC, "C");
  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    tmp_string = (char*)xmlNodeListGetString(doc, node->children, 1);

    if(!BG_XML_STRCMP(node->name, "X"))
      s->x = atoi(tmp_string);
    else if(!BG_XML_STRCMP(node->name, "Y"))
      s->y = atoi(tmp_string);
    else if(!BG_XML_STRCMP(node->name, "BACKGROUND"))
      {
      pos = tmp_string;
      s->background[0] = strtod(pos, &rest);
      pos = rest;
      s->background[1] = strtod(pos, &rest);
      pos = rest;
      s->background[2] = strtod(pos, &rest);
      
      }
    else if(!BG_XML_STRCMP(node->name, "FOREGROUND_NORMAL"))
      {
      //      fprintf(stderr, "FOREGROUND_NORMAL %s\n", tmp_string);
      pos = tmp_string;
      s->foreground_normal[0] = strtod(pos, &rest);
      pos = rest;
      s->foreground_normal[1] = strtod(pos, &rest);
      pos = rest;
      s->foreground_normal[2] = strtod(pos, &rest);
      //      fprintf(stderr, "FOREGROUND_NORMAL %f %f %f",
      //              s->foreground_normal[0], s->foreground_normal[1], s->foreground_normal[2]);
      }
    else if(!BG_XML_STRCMP(node->name, "FOREGROUND_ERROR"))
      {
      pos = tmp_string;
      s->foreground_error[0] = strtod(pos, &rest);
      pos = rest;
      s->foreground_error[1] = strtod(pos, &rest);
      pos = rest;
      s->foreground_error[2] = strtod(pos, &rest);
      }
    node = node->next;
    xmlFree(tmp_string);
    }
  setlocale(LC_NUMERIC, old_locale);
  }

void display_set_skin(display_t * d,
                      display_skin_t * s)
  {
  d->skin = s;

  if(d->get_colors_from_skin)
    {
    memcpy(d->foreground_normal, d->skin->foreground_normal,
           3 * sizeof(float));
    memcpy(d->foreground_error, d->skin->foreground_error,
           3 * sizeof(float));
    memcpy(d->background, d->skin->background,
           3 * sizeof(float));
    update_colors(d);
    }

  }

void display_get_coords(display_t * d, int * x, int * y)
  {
  *x = d->skin->x;
  *y = d->skin->y;
  }

void display_set_track_name(display_t * d, char * name)
  {
  d->track_name = bg_strdup(d->track_name, name);
  d->error_active = 0;
  bg_gtk_scrolltext_set_text(d->scrolltext,
                             d->track_name,
                             d->foreground_normal, d->background);
  }

void display_set_error_msg(display_t * d, char * msg)
  {
  d->error_active = 1;
  bg_gtk_scrolltext_set_text(d->scrolltext,
                             msg,
                             d->foreground_error, d->background);
  
  }
