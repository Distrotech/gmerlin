/*****************************************************************
 
  slider.c
 
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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <gui_gtk/slider.h>
#include <gui_gtk/gtkutils.h>

#include <utils.h>

struct bg_gtk_slider_s
  {
  GdkPixbuf * pixbuf_background;
  GdkPixbuf * pixbuf_normal;
  GdkPixbuf * pixbuf_highlight;
  GdkPixbuf * pixbuf_pressed;
  GdkPixbuf * pixbuf_inactive;
  int x, y;
  int width, height;
  int vertical;
  
  /* State information */

  int action;
  int mouse_inside;
  bg_gtk_slider_state_t state;
  int mouse_root;
  
  int total_size;
  int slider_size;
  
  int pos;
    
  void (*change_callback)(bg_gtk_slider_t*, float, void*);
  void * change_callback_data;
  
  void (*release_callback)(bg_gtk_slider_t*, float, void*);
  void * release_callback_data;

  /* Widget stuff */

  GtkWidget * background_layout;

  GtkWidget * slider_eventbox;
  GtkWidget * slider_image;
    
  };

/* Callbacks */

static gboolean button_press_callback(GtkWidget * w, GdkEventButton * evt,
                                      gpointer data)
  {
  bg_gtk_slider_t * s;
  s = (bg_gtk_slider_t *)data;

  if(s->state != BG_GTK_SLIDER_ACTIVE)
    return TRUE;
  
  gtk_image_set_from_pixbuf(GTK_IMAGE(s->slider_image), s->pixbuf_pressed);

  if(s->vertical)
    s->mouse_root = (int)(evt->y_root);
  else
    s->mouse_root = (int)(evt->x_root);

  s->action = 1;
  
  return TRUE;
  }

//static void 

static gboolean button_release_callback(GtkWidget * w, GdkEventButton * evt,
                                        gpointer data)
  {
  bg_gtk_slider_t * s;
  int mouse_root_1;
  s = (bg_gtk_slider_t *)data;

  if(s->state != BG_GTK_SLIDER_ACTIVE)
    return TRUE;

  if(s->mouse_inside)
    gtk_image_set_from_pixbuf(GTK_IMAGE(s->slider_image), s->pixbuf_highlight);
  else
    gtk_image_set_from_pixbuf(GTK_IMAGE(s->slider_image), s->pixbuf_normal);
  
  s->action = 0;

  if(s->release_callback)
    {
    if(s->vertical)
      mouse_root_1 = (int)(evt->y_root);
    else
      mouse_root_1 = (int)(evt->x_root);
    
    s->pos += (mouse_root_1 - s->mouse_root);
    if(s->pos > s->total_size - s->slider_size)
      s->pos = s->total_size - s->slider_size;
    else if(s->pos < 0)
      s->pos = 0;
    
    if(s->vertical)
      gtk_layout_move(GTK_LAYOUT(s->background_layout), s->slider_eventbox,
                      0, s->pos);
    else
      gtk_layout_move(GTK_LAYOUT(s->background_layout), s->slider_eventbox,
                      s->pos, 0);

    if(s->vertical)
      s->release_callback(s,
                          1.0 - (float)(s->pos)/(float)(s->total_size -
                                                        s->slider_size),
                          s->release_callback_data);
    else
      s->release_callback(s,
                          (float)(s->pos)/(float)(s->total_size -
                                                  s->slider_size),
                          s->release_callback_data);
    }

  return TRUE;
  }

static gboolean enter_notify_callback(GtkWidget *widget,
                                      GdkEventCrossing *event,
                                      gpointer data)
  {
  bg_gtk_slider_t * s;
  s = (bg_gtk_slider_t *)data;

  if(s->state != BG_GTK_SLIDER_ACTIVE)
    return FALSE;

  s->mouse_inside = 1;
  if(!s->action)
    gtk_image_set_from_pixbuf(GTK_IMAGE(s->slider_image), s->pixbuf_highlight);
  return FALSE;
  }

static gboolean leave_notify_callback(GtkWidget *widget,
                                      GdkEventCrossing *event,
                                      gpointer data)
  {
  bg_gtk_slider_t * s;
  s = (bg_gtk_slider_t *)data;
  if(s->state != BG_GTK_SLIDER_ACTIVE)
    return FALSE;

  s->mouse_inside = 0;
  if(!s->action)
    gtk_image_set_from_pixbuf(GTK_IMAGE(s->slider_image), s->pixbuf_normal);
  return FALSE;
  }

static gboolean motion_callback(GtkWidget * w, GdkEventMotion * evt,
                                gpointer data)
  {
  bg_gtk_slider_t * s;
  int mouse_root_1;
  s = (bg_gtk_slider_t *)data;

  if(s->state != BG_GTK_SLIDER_ACTIVE)
    return TRUE;
  
  if(s->vertical)
    mouse_root_1 = (int)(evt->y_root);
  else
    mouse_root_1 = (int)(evt->x_root);

  s->pos += (mouse_root_1 - s->mouse_root);
  if(s->pos > s->total_size - s->slider_size)
    s->pos = s->total_size - s->slider_size;
  else if(s->pos < 0)
    s->pos = 0;

  if(s->vertical)
    gtk_layout_move(GTK_LAYOUT(s->background_layout), s->slider_eventbox,
                    0, s->pos);
  else
    gtk_layout_move(GTK_LAYOUT(s->background_layout), s->slider_eventbox,
                    s->pos, 0);

  if(s->change_callback)
    {
    if(s->vertical)
      s->change_callback(s,
                         1.0 - (float)(s->pos)/(float)(s->total_size - s->slider_size),
                         s->change_callback_data);
    else
      s->change_callback(s,
                         (float)(s->pos)/(float)(s->total_size - s->slider_size),
                         s->change_callback_data);
    }
  s->mouse_root = mouse_root_1;
  return TRUE;
  }

static void set_background(bg_gtk_slider_t * s)
  {
  GdkPixmap * pixmap;
  bg_gdk_pixbuf_render_pixmap_and_mask(s->pixbuf_background,
                                    &pixmap, NULL);
  
  s->width  = gdk_pixbuf_get_width(s->pixbuf_background);
  s->height = gdk_pixbuf_get_height(s->pixbuf_background);
#if 0
  if(s->vertical)
    s->total_size = s->height;
  else
    s->total_size = s->width;
#endif
  gtk_widget_set_size_request(s->background_layout, s->width, s->height);

  gdk_window_set_back_pixmap(GTK_LAYOUT(s->background_layout)->bin_window,
                             pixmap, FALSE);

  g_object_unref(G_OBJECT(pixmap));
  }

static void set_shape(bg_gtk_slider_t * s)
  {
  GdkBitmap * mask;
#if 0
  if(s->vertical)
    s->slider_size = gdk_pixbuf_get_height(s->pixbuf_normal);
  else
    s->slider_size = gdk_pixbuf_get_width(s->pixbuf_normal);
#endif
  bg_gdk_pixbuf_render_pixmap_and_mask(s->pixbuf_normal,
                                       NULL, &mask);

  gdk_window_shape_combine_mask(s->slider_eventbox->window,
                                mask, 0, 0);
  if(mask)
    g_object_unref(G_OBJECT(mask));
  }

static void realize_callback(GtkWidget * w,
                             gpointer data)
  {
  bg_gtk_slider_t * s;
  s = (bg_gtk_slider_t *)data;

  if((w == s->background_layout) && (s->pixbuf_background))
    set_background(s);
  else if((w == s->slider_eventbox) && (s->pixbuf_normal))
    set_shape(s);
    
  //  if(b->pixbuf_normal)
  //    set_shape(b);
  }

bg_gtk_slider_t * bg_gtk_slider_create()
  {
  bg_gtk_slider_t * ret = calloc(1, sizeof(*ret));
  //  ret->vertical = vertical;

  /* Create objects */

  ret->background_layout = gtk_layout_new((GtkAdjustment*)0,
                                          (GtkAdjustment*)0);

  ret->slider_eventbox = gtk_event_box_new();

  ret->slider_image  = gtk_image_new_from_pixbuf((GdkPixbuf*)0);
    
  /* Set Attributes */

  gtk_widget_set_events(ret->slider_eventbox,
                        GDK_BUTTON1_MOTION_MASK|
                        GDK_BUTTON2_MOTION_MASK|
                        GDK_BUTTON3_MOTION_MASK|
                        GDK_BUTTON_PRESS_MASK|
                        GDK_ENTER_NOTIFY_MASK|
                        GDK_LEAVE_NOTIFY_MASK);
  
  
  /* Set Callbacks */

  g_signal_connect(G_OBJECT(ret->slider_eventbox),
                   "button_press_event",
                   G_CALLBACK (button_press_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->slider_eventbox),
                   "button_release_event",
                   G_CALLBACK (button_release_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->slider_eventbox),
                   "enter_notify_event",
                   G_CALLBACK (enter_notify_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->slider_eventbox),
                   "leave_notify_event",
                   G_CALLBACK (leave_notify_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->slider_eventbox),
                   "motion_notify_event",
                   G_CALLBACK (motion_callback),
                   ret);
  
  g_signal_connect(G_OBJECT(ret->slider_eventbox),
                   "realize",
                   G_CALLBACK (realize_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->background_layout),
                   "realize",
                   G_CALLBACK (realize_callback),
                   ret);
  
  /* Pack */

  gtk_widget_show(ret->slider_image);
  
  gtk_container_add(GTK_CONTAINER(ret->slider_eventbox), ret->slider_image);
  //  gtk_widget_show(ret->slider_eventbox);
  
  gtk_layout_put(GTK_LAYOUT(ret->background_layout),
                 ret->slider_eventbox, 0, 0);
  gtk_widget_show(ret->background_layout);

  bg_gtk_slider_set_state(ret, BG_GTK_SLIDER_ACTIVE);
  
  return ret;
  }

void bg_gtk_slider_set_state(bg_gtk_slider_t * s,
                             bg_gtk_slider_state_t state)
  {
  s->state = state;

  switch(state)
    {
    case BG_GTK_SLIDER_ACTIVE:
      if(s->mouse_inside)
        gtk_image_set_from_pixbuf(GTK_IMAGE(s->slider_image), s->pixbuf_highlight);
      else
        gtk_image_set_from_pixbuf(GTK_IMAGE(s->slider_image), s->pixbuf_normal);
      gtk_widget_show(s->slider_eventbox);
      break;
    case BG_GTK_SLIDER_INACTIVE:
      gtk_image_set_from_pixbuf(GTK_IMAGE(s->slider_image), s->pixbuf_inactive);
      gtk_widget_show(s->slider_eventbox);
      break;
    case BG_GTK_SLIDER_HIDDEN:
      gtk_widget_hide(s->slider_eventbox);
      break;
    }
  }

void bg_gtk_slider_destroy(bg_gtk_slider_t * s)
  {
  g_object_unref(s->pixbuf_background);
  g_object_unref(s->pixbuf_normal);
  g_object_unref(s->pixbuf_highlight);
  g_object_unref(s->pixbuf_pressed);
  g_object_unref(s->pixbuf_inactive);
  free(s);
  }

/* Set attributes */

void bg_gtk_slider_set_change_callback(bg_gtk_slider_t * s,
                                       void (*func)(bg_gtk_slider_t *,
                                                    float perc,
                                                    void * data),
                                       void * data)
  {
  s->change_callback = func;
  s->change_callback_data = data;
  
  }

void bg_gtk_slider_set_release_callback(bg_gtk_slider_t * s,
                                        void (*func)(bg_gtk_slider_t *,
                                                     float perc,
                                                     void * data),
                                        void * data)
  {
  s->release_callback = func;
  s->release_callback_data = data;
  }

static GdkPixbuf * make_pixbuf(GdkPixbuf * old,
                               const char * filename)
  {
  GdkPixbuf * ret;
  
  if(old)
    g_object_unref(G_OBJECT(old));
  
  ret = gdk_pixbuf_new_from_file(filename, NULL);

  if(!ret)
    fprintf(stderr, "Cannot open %s\n", filename);
  
  return ret;
  }

void bg_gtk_slider_set_skin(bg_gtk_slider_t * s,
                            bg_gtk_slider_skin_t * skin,
                            const char * directory)
  {
  char * tmp_string = (char*)0;
  
  s->x = skin->x;
  s->y = skin->y;
  
  tmp_string = bg_sprintf("%s/%s", directory, skin->pixmap_normal);
  s->pixbuf_normal = make_pixbuf(s->pixbuf_normal, tmp_string);
  free(tmp_string);

  tmp_string = bg_sprintf("%s/%s", directory, skin->pixmap_highlight);
  s->pixbuf_highlight = make_pixbuf(s->pixbuf_highlight, tmp_string);
  free(tmp_string);

  tmp_string = bg_sprintf("%s/%s", directory, skin->pixmap_pressed);
  s->pixbuf_pressed = make_pixbuf(s->pixbuf_pressed, tmp_string);
  free(tmp_string);

  tmp_string = bg_sprintf("%s/%s", directory, skin->pixmap_inactive);
  s->pixbuf_inactive = make_pixbuf(s->pixbuf_inactive, tmp_string);
  free(tmp_string);

  tmp_string = bg_sprintf("%s/%s", directory, skin->pixmap_background);
  s->pixbuf_background = make_pixbuf(s->pixbuf_background, tmp_string);
  free(tmp_string);

  if(GTK_LAYOUT(s->background_layout)->bin_window)
    set_background(s);
  if(s->slider_eventbox->window)
    set_shape(s);
    
  gtk_image_set_from_pixbuf(GTK_IMAGE(s->slider_image), s->pixbuf_normal);

  /* Determine if it's a vertical or horizontal layout */

  if(gdk_pixbuf_get_width(s->pixbuf_background) ==
     gdk_pixbuf_get_width(s->pixbuf_normal))
    {
    s->vertical = 1;
    s->total_size  = gdk_pixbuf_get_height(s->pixbuf_background);
    s->slider_size = gdk_pixbuf_get_height(s->pixbuf_normal);
    }
  else
    {
    s->vertical = 0;
    s->total_size  = gdk_pixbuf_get_width(s->pixbuf_background);
    s->slider_size = gdk_pixbuf_get_width(s->pixbuf_normal);
    }
  }

/* Get stuff */

void bg_gtk_slider_get_coords(bg_gtk_slider_t * s, int * x, int * y)
  {
  *x = s->x;
  *y = s->y;
  
  }

GtkWidget * bg_gtk_slider_get_widget(bg_gtk_slider_t * s)
  {
  return s->background_layout;  
  }

GtkWidget * bg_gtk_slider_get_slider_widget(bg_gtk_slider_t * s)
  {
  return s->slider_eventbox;
  }

void bg_gtk_slider_skin_load(bg_gtk_slider_skin_t * s,
                             xmlDocPtr doc, xmlNodePtr node)
  {
  node = node->children;
  char * tmp_string;
  
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
    else if(!BG_XML_STRCMP(node->name, "NORMAL"))
      s->pixmap_normal = bg_strdup(s->pixmap_normal, tmp_string);
    else if(!BG_XML_STRCMP(node->name, "HIGHLIGHT"))
      s->pixmap_highlight = bg_strdup(s->pixmap_highlight, tmp_string);
    else if(!BG_XML_STRCMP(node->name, "PRESSED"))
      s->pixmap_pressed = bg_strdup(s->pixmap_pressed, tmp_string);
    else if(!BG_XML_STRCMP(node->name, "INACTIVE"))
      s->pixmap_inactive = bg_strdup(s->pixmap_inactive, tmp_string);
    else if(!BG_XML_STRCMP(node->name, "BACKGROUND"))
      s->pixmap_background = bg_strdup(s->pixmap_background, tmp_string);
    node = node->next;
    xmlFree(tmp_string);
    }

  }

#define MY_FREE(ptr) if(ptr){free(ptr);ptr=NULL;}

void bg_gtk_slider_skin_free(bg_gtk_slider_skin_t * s)
  {
  MY_FREE(s->pixmap_normal);
  MY_FREE(s->pixmap_highlight);
  MY_FREE(s->pixmap_pressed);
  MY_FREE(s->pixmap_inactive);
  MY_FREE(s->pixmap_background);
  }
#undef MY_FREE

void bg_gtk_slider_set_pos(bg_gtk_slider_t * s, float position)
  {
  if(s->action)
    return;

  if(s->vertical)
    s->pos = (int)((1.0 - position) * (float)(s->total_size - s->slider_size) + 0.5);
  else
    s->pos = (int)(position * (float)(s->total_size - s->slider_size) + 0.5);
  //  fprintf(stderr, "Position: %f Slider pos 1: %d\n", position, s->pos);
  if(s->pos < 0)
    s->pos = 0;
  else if(s->pos > s->total_size - s->slider_size)
    s->pos = s->total_size - s->slider_size;
  //  fprintf(stderr, "Slider pos 2: %d\n", s->pos);
  
  if(s->vertical)
    gtk_layout_move(GTK_LAYOUT(s->background_layout), s->slider_eventbox,
                    0, s->pos);
  else
    gtk_layout_move(GTK_LAYOUT(s->background_layout), s->slider_eventbox,
                    s->pos, 0);
  }
