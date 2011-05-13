/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#include <gmerlin/parameter.h>
#include "gui.h"

#include <gmerlin/log.h>
#define LOG_DOMAIN "g_control"

#include <gui_gtk/aboutwindow.h>
#include <gui_gtk/gtkutils.h>

#if GTK_MINOR_VERSION >= 4
#define GTK_2_4
#endif

#define SLIDER_HEIGHT 120

#define TYPE_SINGLEBOOL 0
#define TYPE_BOOL       1
#define TYPE_ENUM       2
#define TYPE_SINGLEENUM 3
#define TYPE_INTEGER    4
#define TYPE_INTEGER64  5

/* Combined types */

#define TYPE_VOLUME     16
#define TYPE_TONE       17

static bg_gtk_about_window_t * about_window = (bg_gtk_about_window_t*)0;

static void about_window_close_callback(bg_gtk_about_window_t * w,
                                        void * data)
  {
  about_window = (bg_gtk_about_window_t*)0;
  }

static GtkWidget * create_pixmap_toggle_button(const char * filename)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();
  gtk_widget_show(image);
  button = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);
  return button;
  }

typedef struct
  {
  GtkWidget * w;
  GtkWidget * handler_widget;
  gulong id; /* ID of the signal handler */
  } widget_t;

static void widget_block(widget_t * w)
  {
  g_signal_handler_block(w->handler_widget, w->id);
  }

static void widget_unblock(widget_t * w)
  {
  g_signal_handler_unblock(w->handler_widget, w->id);
  }

typedef struct
  {
  int type;
  int num;

  widget_t * widgets;

  /* For sliders */

  float val_min;
  float val_max;

  /* Combo box stuff */
#ifndef GTK_2_4
  GList * popdown_strings;
#endif
  /* Control */

  alsa_mixer_control_t * control;

  /* Lock button */

  GtkWidget * lock_button;
  int locked;
  } widget_array_t;

/* Callbacks */

static void enum_callback(GtkWidget * w, gpointer data)
  {
  int index;
  int value;

#ifndef GTK_2_4
  GList * list;
  const char * label;
#endif
  
  widget_array_t * arr = (widget_array_t *)data;
  
  for(index = 0; index < arr->num; index++)
    {
#ifdef GTK_2_4
    if(arr->widgets[index].w == w)
      break;
#else
    if(GTK_COMBO(arr->widgets[index].w)->entry == w)
      break;
#endif
    }
  if(index == arr->num)
    {
    return;
    }
  
  /* Get the string and the index of the enum */
#ifdef GTK_2_4
  value = gtk_combo_box_get_active(GTK_COMBO_BOX(arr->widgets[index].w));
#else
  label =
    gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(arr->widgets[index].w)->entry));
  
  if(*label == '\0')
    return;
  
  list = arr->popdown_strings;
  value = 0;
  while(1)
    {
    if(!strcmp(list->data, label))
      break;
    else
      {
      list = list->next;
      value++;
      }
    }
#endif
  /* Transfer value */
  snd_ctl_elem_value_set_enumerated(arr->control->val, index, value);
  /* Write value */
  snd_hctl_elem_write(arr->control->hctl, arr->control->val);

  
  }

static int hctl_enum_callback(snd_hctl_elem_t *elem, unsigned int mask)
  {
  int i;
  widget_array_t * arr;
  unsigned int value;

#ifndef GTK_2_4
  char * val_str;
#endif
  
  if(mask & SND_CTL_EVENT_MASK_VALUE) 
    {
    arr = (widget_array_t*)snd_hctl_elem_get_callback_private(elem);
    snd_hctl_elem_read(elem, arr->control->val);

    for(i = 0; i < arr->num; i++)
      {
      value = snd_ctl_elem_value_get_enumerated(arr->control->val, i);
#ifdef GTK_2_4
      widget_block(&arr->widgets[i]);
      gtk_combo_box_set_active(GTK_COMBO_BOX(arr->widgets[i].w), value);
      widget_unblock(&arr->widgets[i]);
#else
      
      val_str = g_list_nth_data(arr->popdown_strings, value);

      
      widget_block(&arr->widgets[i]);
      gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(arr->widgets[i].w)->entry),
                         val_str);
      widget_unblock(&arr->widgets[i]);
#endif
      }
    }
  return 0;
  }

static void int_callback(GtkWidget * w, gpointer data)
  {
  int i;
  int index;
  int value;
  widget_array_t * arr = (widget_array_t *)data;

  for(index = 0; index < arr->num; index++)
    {
    if(arr->widgets[index].w == w)
      break;
    }
  if(index == arr->num)
    {
    return;
    }
  value = (int)(gtk_range_get_value(GTK_RANGE(arr->widgets[index].w)));

  widget_block(&arr->widgets[index]);
  gtk_range_set_value(GTK_RANGE(arr->widgets[index].w), (float)value);
  widget_unblock(&arr->widgets[index]);
  
  /* Transfer value */

  snd_ctl_elem_value_set_integer(arr->control->val, index, value);

  if(arr->locked)
    {
    for(i = 0; i < arr->num; i++)
      {
      if(i == index)
        continue;
      widget_block(&arr->widgets[i]);
      gtk_range_set_value(GTK_RANGE(arr->widgets[i].w), (float)value);
      widget_unblock(&arr->widgets[i]);
      snd_ctl_elem_value_set_integer(arr->control->val, i, value);
      }
    }
  /* Write value */
  snd_hctl_elem_write(arr->control->hctl, arr->control->val);
  }

static int hctl_int_callback(snd_hctl_elem_t *elem, unsigned int mask)
  {
  int i;
  widget_array_t * arr;
  int value;

  if(mask & SND_CTL_EVENT_MASK_VALUE) 
    {
    arr = (widget_array_t*)snd_hctl_elem_get_callback_private(elem);
    snd_hctl_elem_read(elem, arr->control->val);

    for(i = 0; i < arr->num; i++)
      {
      value = snd_ctl_elem_value_get_integer(arr->control->val, i);
      
      widget_block(&arr->widgets[i]);
      gtk_range_set_value(GTK_RANGE(arr->widgets[i].w), (float)value);
      widget_unblock(&arr->widgets[i]);
      }
    }
  return 0;
  }

static void int64_callback(GtkWidget * w, gpointer data)
  {
  int index;
  int i;
  int64_t value;
  widget_array_t * arr = (widget_array_t *)data;

  for(index = 0; index < arr->num; index++)
    {
    if(arr->widgets[index].w == w)
      break;
    }
  if(index == arr->num)
    {
    return;
    }
  value = (int64_t)(gtk_range_get_value(GTK_RANGE(arr->widgets[index].w)));

  /* Transfer value */

  snd_ctl_elem_value_set_integer64(arr->control->val, index, value);

  if(arr->locked)
    {
    for(i = 0; i < arr->num; i++)
      {
      if(i == index)
        continue;
      widget_block(&arr->widgets[i]);
      gtk_range_set_value(GTK_RANGE(arr->widgets[i].w), (float)value);
      widget_unblock(&arr->widgets[i]);
      snd_ctl_elem_value_set_integer64(arr->control->val, i, value);
      }
    }
  /* Write value */
  snd_hctl_elem_write(arr->control->hctl, arr->control->val);
  }

static int hctl_int64_callback(snd_hctl_elem_t *elem, unsigned int mask)
  {
  int i;
  widget_array_t * arr;
  int64_t value;

  if(mask & SND_CTL_EVENT_MASK_VALUE) 
    {
    arr = (widget_array_t*)snd_hctl_elem_get_callback_private(elem);
    snd_hctl_elem_read(elem, arr->control->val);
    for(i = 0; i < arr->num; i++)
      {
      value = snd_ctl_elem_value_get_integer64(arr->control->val, i);
      widget_block(&arr->widgets[i]);
      gtk_range_set_value(GTK_RANGE(arr->widgets[i].w), (float)value);
      widget_unblock(&arr->widgets[i]);
      }
    }
  return 0;
  }


static void bool_callback(GtkWidget * w, gpointer data)
  {
  int index;
  int value;
  widget_array_t * arr = (widget_array_t *)data;

  for(index = 0; index < arr->num; index++)
    {
    if(arr->widgets[index].w == w)
      break;
    }
  if(index == arr->num)
    {
    return;
    }
  value =
    (int)(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(arr->widgets[index].w)));

  /* Transfer value */
  snd_ctl_elem_value_set_boolean(arr->control->val, index, value);
  /* Write value */
  snd_hctl_elem_write(arr->control->hctl, arr->control->val);

  }

static int hctl_bool_callback(snd_hctl_elem_t *elem, unsigned int mask)
  {
  int i;
  widget_array_t * arr;
  int value;

  if(mask & SND_CTL_EVENT_MASK_VALUE) 
    {
    arr = (widget_array_t*)snd_hctl_elem_get_callback_private(elem);
    snd_hctl_elem_read(elem, arr->control->val);
    for(i = 0; i < arr->num; i++)
      {
      value = snd_ctl_elem_value_get_boolean(arr->control->val, i);

      widget_block(&arr->widgets[i]);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(arr->widgets[i].w), value);
      widget_unblock(&arr->widgets[i]);

      }

    }
  return 0;
  }

static void lock_callback(GtkWidget * w, gpointer data)
  {
  widget_array_t * arr = (widget_array_t *)data;
  arr->locked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  }

static void init_array(widget_array_t * ret, alsa_mixer_control_t * c,
                snd_ctl_elem_info_t * _info, int lock_button)
  {
  int req_width, req_height;
  int i;
#ifdef GTK_2_4
  int j;
#endif
  int num_items;
  snd_ctl_elem_type_t type;
  snd_ctl_elem_info_t * info;
  snd_hctl_elem_t * hctl = c->hctl;
  //  ret
  if(_info)
    info = _info;
  else
    {
    snd_ctl_elem_info_malloc(&info);
    snd_hctl_elem_info(hctl, info);
    }
  ret->control = c;
  ret->num = snd_ctl_elem_info_get_count(info);
  type = snd_ctl_elem_info_get_type(info);

  ret->widgets = calloc(ret->num, sizeof(*(ret->widgets)));
  switch(type)
    {
    case SND_CTL_ELEM_TYPE_BOOLEAN:
      for(i = 0; i < ret->num; i++)
        {
        ret->widgets[i].w = gtk_toggle_button_new();

        ret->widgets[i].id = g_signal_connect(G_OBJECT(ret->widgets[i].w),
                                              "toggled",
                                              G_CALLBACK(bool_callback),
                                              (gpointer)ret);
        ret->widgets[i].handler_widget = ret->widgets[i].w;


        gtk_widget_show(ret->widgets[i].w);
        }
      snd_hctl_elem_set_callback(c->hctl, hctl_bool_callback);
      snd_hctl_elem_set_callback_private(c->hctl, ret);

      hctl_bool_callback(c->hctl, SND_CTL_EVENT_MASK_VALUE);
      
      break;
    case SND_CTL_ELEM_TYPE_INTEGER64:
      ret->val_min = (float)snd_ctl_elem_info_get_min64(info);
      ret->val_max = (float)snd_ctl_elem_info_get_max64(info);

      for(i = 0; i < ret->num; i++)
        {
        ret->widgets[i].w = 
          gtk_vscale_new_with_range(ret->val_min, ret->val_max,
                                    1.0);

        gtk_widget_get_size_request(ret->widgets[i].w, &req_width, &req_height);
        gtk_widget_set_size_request(ret->widgets[i].w, req_width, SLIDER_HEIGHT);
                
        gtk_range_set_inverted(GTK_RANGE(ret->widgets[i].w), TRUE);
        
        ret->widgets[i].id = g_signal_connect(G_OBJECT(ret->widgets[i].w),
                                              "value_changed",
                                              G_CALLBACK(int64_callback),
                                              (gpointer)ret);
        ret->widgets[i].handler_widget = ret->widgets[i].w;

        gtk_scale_set_draw_value(GTK_SCALE(ret->widgets[i].w),
                                 0);
        
        gtk_scale_set_digits(GTK_SCALE(ret->widgets[i].w), 0);
        
        gtk_widget_show(ret->widgets[i].w);
        }
      snd_hctl_elem_set_callback(c->hctl, hctl_int64_callback);
      snd_hctl_elem_set_callback_private(c->hctl, ret);

      hctl_int64_callback(c->hctl, SND_CTL_EVENT_MASK_VALUE);

      break;
    case SND_CTL_ELEM_TYPE_INTEGER:
      ret->val_min = (float)snd_ctl_elem_info_get_min(info);
      ret->val_max = (float)snd_ctl_elem_info_get_max(info);

      for(i = 0; i < ret->num; i++)
        {
        ret->widgets[i].w = 
          gtk_vscale_new_with_range(ret->val_min, ret->val_max,
                                    1.0);

        gtk_widget_get_size_request(ret->widgets[i].w, &req_width, &req_height);
        gtk_widget_set_size_request(ret->widgets[i].w, req_width, SLIDER_HEIGHT);
        
        gtk_range_set_inverted(GTK_RANGE(ret->widgets[i].w), TRUE);
        
        ret->widgets[i].id = g_signal_connect(G_OBJECT(ret->widgets[i].w),
                                              "value_changed",
                                              G_CALLBACK(int_callback),
                                              (gpointer)ret);
        ret->widgets[i].handler_widget = ret->widgets[i].w;
        gtk_scale_set_draw_value(GTK_SCALE(ret->widgets[i].w),
                                 0);
        gtk_scale_set_digits(GTK_SCALE(ret->widgets[i].w), 0);
        gtk_widget_show(ret->widgets[i].w);
        }
      snd_hctl_elem_set_callback(c->hctl, hctl_int_callback);
      snd_hctl_elem_set_callback_private(c->hctl, ret);

      hctl_int_callback(c->hctl, SND_CTL_EVENT_MASK_VALUE);

      break;
    case SND_CTL_ELEM_TYPE_ENUMERATED:
      num_items = snd_ctl_elem_info_get_items(info);
#ifndef GTK_2_4
      for(i = 0; i < num_items; i++)
        {
        snd_ctl_elem_info_set_item(info,i);
        snd_hctl_elem_info(hctl,info);

        ret->popdown_strings =
          g_list_append(ret->popdown_strings,
                        bg_strdup(NULL,
                                  snd_ctl_elem_info_get_item_name(info)));
        }
#endif
      for(i = 0; i < ret->num; i++)
        {
#ifdef GTK_2_4
        ret->widgets[i].w = bg_gtk_combo_box_new_text();
        ret->widgets[i].handler_widget = ret->widgets[i].w;
        
        for(j = 0; j < num_items; j++)
          {
          snd_ctl_elem_info_set_item(info,j);
          snd_hctl_elem_info(hctl,info);
          bg_gtk_combo_box_append_text(ret->widgets[i].w,
                                    snd_ctl_elem_info_get_item_name(info));
          }
        ret->widgets[i].id =
          g_signal_connect(G_OBJECT(ret->widgets[i].w),
                           "changed", G_CALLBACK(enum_callback),
                           (gpointer)ret);
        
#else
        ret->widgets[i].w = gtk_combo_new();
        gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(ret->widgets[i].w)->entry),
                                  FALSE);
        ret->widgets[i].handler_widget =
          GTK_COMBO(ret->widgets[i].w)->entry;
        
        gtk_combo_set_popdown_strings(GTK_COMBO(ret->widgets[i].w), ret->popdown_strings);

        /* The signal must be connected AFTER the popdown strings are set */
        
        ret->widgets[i].id =
          g_signal_connect(G_OBJECT(GTK_EDITABLE(GTK_COMBO(ret->widgets[i].w)->entry)),
                           "changed", G_CALLBACK(enum_callback),
                         (gpointer)ret);
#endif
        gtk_widget_show(ret->widgets[i].w);
        }
      snd_hctl_elem_set_callback(c->hctl, hctl_enum_callback);
      snd_hctl_elem_set_callback_private(c->hctl, ret);

      hctl_enum_callback(c->hctl, SND_CTL_EVENT_MASK_VALUE);

      break;
    case SND_CTL_ELEM_TYPE_NONE:
    case SND_CTL_ELEM_TYPE_BYTES:
    case SND_CTL_ELEM_TYPE_IEC958:
      break;
      
    }
  
  if(!_info)
    snd_ctl_elem_info_free(info);

  /* Create lock button */
  if(lock_button && (ret->num > 1))
    {
    ret->lock_button = create_pixmap_toggle_button("lock_16.png");
    g_signal_connect(G_OBJECT(ret->lock_button),
                     "toggled", G_CALLBACK(lock_callback), ret);
    gtk_widget_show(ret->lock_button);
    }
  
  }

typedef struct
  {
  GtkWidget * left;
  GtkWidget * right;
  GtkWidget * first;
  GtkWidget * last;
  GtkWidget * tearoff;
  GtkWidget * config;
  GtkWidget * about;
  GtkWidget * help;
  GtkWidget * menu;
  } menu_t;

struct control_widget_s
  {
  int type;
  char * label;
  
  union
    {
    struct
      {
      widget_array_t playback_sliders;
      widget_array_t capture_sliders;

      widget_array_t playback_switches;
      widget_array_t capture_switches;
      } volume;
    struct
      {
      widget_array_t treble;
      widget_array_t bass;
      widget_array_t switches;

      } tone;
    struct
      {
      widget_t checkbutton;
      alsa_mixer_control_t * control;
      } singlebool;
    struct
      {
      widget_array_t buttons;
      } bool;
    struct
      {
      widget_array_t combos;
      } enumerated;
    struct
      {
      widget_array_t sliders;
      alsa_mixer_control_t * control;
      } integer;
    
    } priv;
  GtkWidget * w;
  int upper;
  bg_cfg_section_t * section;
  bg_parameter_info_t * parameters;
  
  /* Index within the card widget (only for upper widgets) */
  
  int index;
  card_widget_t * card;  
  menu_t menu;

  /* True if we are in an own window */
  int own_window;
  
  /* Coordinates for own window */
  int x, y, width, height;

  /* True if we don't show this at all */

  int hidden;
  };

static void menu_callback(GtkWidget * w, gpointer data)
  {
  control_widget_t * wid;
  wid = (control_widget_t *)data;

  if(w == wid->menu.left)
    {
    card_widget_move_control_left(wid->card, wid);
    }
  else if(w == wid->menu.right)
    {
    card_widget_move_control_right(wid->card, wid);
    }
  else if(w == wid->menu.first)
    {
    card_widget_move_control_first(wid->card, wid);
    }
  else if(w == wid->menu.last)
    {
    card_widget_move_control_last(wid->card, wid);
    }
  else if(w == wid->menu.tearoff)
    {
    card_widget_tearoff_control(wid->card, wid);
    }
  else if(w == wid->menu.config)
    {
    card_widget_configure(wid->card);
    //    card_widget_tearoff_control(wid->card, wid);
    }
  else if(w == wid->menu.about)
    {
    about_window = bg_gtk_about_window_create("Gmerlin Alsamixer",
                                              VERSION,
                                              "mixer_icon.png",
                                              about_window_close_callback,
                                              (void*)0);
    }
  else if(w == wid->menu.help)
    bg_display_html_help("userguide/Alsamixer.html");
  }

static GtkWidget * create_pixmap_item(const char * filename, char * label)
  {
  GtkWidget * item;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();
  gtk_widget_show(image);

  item = gtk_image_menu_item_new_with_label(label);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
  return item;
  }



static void init_menu(control_widget_t * w)
  {
  w->menu.left    = create_pixmap_item("left_16.png", "Move control left by one");
  w->menu.right   = create_pixmap_item("right_16.png", "Move control right by one");
  w->menu.first   = create_pixmap_item("first_16.png",  "Move control to the leftmost position");
  w->menu.last    = create_pixmap_item("last_16.png",  "Move control to the rightmost position");
  w->menu.tearoff = create_pixmap_item("windowed_16.png", "Detach control");
  w->menu.config  = create_pixmap_item("config_16.png", "Alsamixer options");
  w->menu.about   = create_pixmap_item("about_16.png", "About...");
  w->menu.help    = create_pixmap_item("help_16.png", "Userguide");
  
  g_signal_connect(w->menu.left, "activate",
                   G_CALLBACK(menu_callback), w);

  g_signal_connect(w->menu.right, "activate",
                   G_CALLBACK(menu_callback), w);

  g_signal_connect(w->menu.first, "activate",
                   G_CALLBACK(menu_callback), w);

  g_signal_connect(w->menu.last, "activate",
                   G_CALLBACK(menu_callback), w);

  g_signal_connect(w->menu.tearoff, "activate",
                   G_CALLBACK(menu_callback), w);

  g_signal_connect(w->menu.config, "activate",
                   G_CALLBACK(menu_callback), w);

  g_signal_connect(w->menu.about, "activate",
                   G_CALLBACK(menu_callback), w);

  g_signal_connect(w->menu.help, "activate",
                   G_CALLBACK(menu_callback), w);
  
  gtk_widget_show(w->menu.left);
  gtk_widget_show(w->menu.right);
  gtk_widget_show(w->menu.first);
  gtk_widget_show(w->menu.last);
  gtk_widget_show(w->menu.tearoff);
  gtk_widget_show(w->menu.config);
  gtk_widget_show(w->menu.about);
  gtk_widget_show(w->menu.help);

  w->menu.menu = gtk_menu_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu), w->menu.left);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu), w->menu.right);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu), w->menu.first);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu), w->menu.last);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu), w->menu.tearoff);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu), w->menu.config);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu), w->menu.about);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menu.menu), w->menu.help);
  gtk_widget_show(w->menu.menu);
  }

int control_widget_get_index(control_widget_t * w)
  {
  return w->index;
  }
     
void control_widget_set_index(control_widget_t * w, int index)
  {
  int leftmost;
  int rightmost;

  w->index = index;
  if(w->index == card_widget_num_upper_controls(w->card)-1)
    rightmost = 1;
  else
    rightmost = 0;

  if(!w->index)
    leftmost = 1;
  else
    leftmost = 0;

  if(w->menu.left)
    gtk_widget_set_sensitive(w->menu.left, !leftmost);

  if(w->menu.last)
    gtk_widget_set_sensitive(w->menu.last, !rightmost);

  if(w->menu.right)
    gtk_widget_set_sensitive(w->menu.right, !rightmost);

  if(w->menu.first)
    gtk_widget_set_sensitive(w->menu.first, !leftmost);
  
  }

static void init_tone(control_widget_t * w, alsa_mixer_group_t * c)
  {
  GtkWidget * label;
  int row, num_rows;
  int num_cols;
  int i;
  num_rows = 1;
  num_cols = 0;

  if(c->tone_switch)
    {
    num_rows++;
    init_array(&w->priv.tone.switches, c->tone_switch,
               NULL, 0);
    if(w->priv.tone.switches.num > num_cols)
      num_cols = w->priv.tone.switches.num;
    }
  if(c->tone_bass)
    {
    num_rows+=2;
    init_array(&w->priv.tone.bass, c->tone_bass,
               NULL, 1);

    if(w->priv.tone.bass.num > num_cols)
      num_cols = w->priv.tone.bass.num;
    if(w->priv.tone.bass.num > 1)
      num_rows++;
    }
  if(c->tone_treble)
    {
    num_rows+=2;
    init_array(&w->priv.tone.treble, c->tone_treble,
               NULL, 1);
    if(w->priv.tone.treble.num > num_cols)
      num_cols = w->priv.tone.treble.num;

    if(w->priv.tone.treble.num > 1)
      num_rows++;
    }
  w->w = gtk_table_new(num_rows, num_cols, 0);

  row = 0;
  
  w->label = bg_strdup(w->label, TR("Tone"));
  label = gtk_label_new(w->label);
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(w->w), label, 0, num_cols, row, row+1,
                   GTK_FILL, GTK_FILL, 0, 0);

  row++;
  
  /* Tone switch */
  
  if(c->tone_switch)
    {
    if(w->priv.tone.switches.num == 1)
      {
      gtk_table_attach(GTK_TABLE(w->w), w->priv.tone.switches.widgets[0].w,
                       0, num_cols, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
      }
    else
      {
      for(i = 0; i < w->priv.tone.switches.num; i++)
        {
        gtk_table_attach(GTK_TABLE(w->w), w->priv.tone.switches.widgets[i].w,
                         i, i+1, row, row+1,
                         GTK_FILL, GTK_FILL, 0, 0);
        }
      }
    row++;
    }

  if(c->tone_treble)
    {
    label = gtk_label_new(TR("Treble"));
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(w->w), label, 0, num_cols,
                     row, row+1, GTK_FILL, GTK_FILL, 0, 0);
    row++;

    if(w->priv.tone.treble.num == 1)
      {
      gtk_table_attach_defaults(GTK_TABLE(w->w),
                                w->priv.tone.treble.widgets[0].w,
                                0, num_cols, row, row+1);
      }
    else
      {
      for(i = 0; i < w->priv.tone.treble.num; i++)
        {
        gtk_table_attach_defaults(GTK_TABLE(w->w), w->priv.tone.treble.widgets[i].w,
                                  i, i+1, row, row+1);
        }
      row++;
      gtk_table_attach(GTK_TABLE(w->w),
                       w->priv.tone.treble.lock_button,
                       0, num_cols, row, row+1,
                       GTK_FILL, GTK_FILL, 0, 0);
      }
    row++;
    }

  if(c->tone_bass)
    {
    label = gtk_label_new(TR("Bass"));
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(w->w), label, 0, num_cols,
                     row, row+1, GTK_FILL, GTK_FILL, 0, 0);
    row++;

    if(w->priv.tone.bass.num == 1)
      {
      gtk_table_attach_defaults(GTK_TABLE(w->w), w->priv.tone.bass.widgets[0].w,
                       0, num_cols, row, row+1);
      }
    else
      {
      for(i = 0; i < w->priv.tone.bass.num; i++)
        {
        gtk_table_attach_defaults(GTK_TABLE(w->w), w->priv.tone.bass.widgets[i].w,
                         i, i+1, row, row+1);
        }
      row++;
      gtk_table_attach(GTK_TABLE(w->w),
                       w->priv.tone.bass.lock_button,
                       0, num_cols, row, row+1,
                       GTK_FILL, GTK_FILL, 0, 0);

      }
    row++;
    }
  gtk_widget_show(w->w);
  w->upper = 1;
  }

static void init_volume(control_widget_t * w, alsa_mixer_group_t * c)
  {
  int i;
  GtkWidget * label;
  int capture_width;
  int playback_width;
  int num_rows;
  
  if(c->playback_switch)
    {
    init_array(&w->priv.volume.playback_switches, c->playback_switch,
               NULL, 0);
    }
  if(c->playback_volume)
    {
    init_array(&w->priv.volume.playback_sliders, c->playback_volume,
               NULL, 1);
    }
  if(c->capture_volume)
    {
    init_array(&w->priv.volume.capture_sliders, c->capture_volume,
               NULL, 1);
    }
  if(c->capture_switch)
    {
    init_array(&w->priv.volume.capture_switches, c->capture_switch,
               NULL, 0);
    }
  
  /* Pack the objects */

  capture_width =
    w->priv.volume.capture_sliders.num > w->priv.volume.capture_switches.num ? 
    w->priv.volume.capture_sliders.num : w->priv.volume.capture_switches.num;

  playback_width =
    w->priv.volume.playback_sliders.num > w->priv.volume.playback_switches.num ? 
    w->priv.volume.playback_sliders.num : w->priv.volume.playback_switches.num;

  num_rows = 4;
  if((w->priv.volume.playback_sliders.num > 1) &&
     (w->priv.volume.capture_sliders.num > 1))
    num_rows++;
  
  w->w = gtk_table_new(4, capture_width + playback_width, 0);

  w->label = bg_strdup(w->label, c->label);
  label = gtk_label_new(w->label);
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(w->w), label, 0,
                   capture_width + playback_width, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);

  /* Save label */
  if(capture_width)
    {
    label = gtk_label_new(TR("Rec"));
    gtk_widget_show(label);

    gtk_table_attach(GTK_TABLE(w->w), label, 0, capture_width, 1, 2,
                     GTK_FILL, GTK_FILL, 0, 0);
    
    /* Capture Switches */

    if(w->priv.volume.capture_switches.num == 1)
      {
      gtk_table_attach(GTK_TABLE(w->w),
                       w->priv.volume.capture_switches.widgets[0].w,
                       0, capture_width, 2, 3,
                       GTK_FILL, GTK_FILL, 0, 0);
      
      }
    else
      {
      for(i = 0; i < w->priv.volume.capture_switches.num; i++)
        {
        gtk_table_attach(GTK_TABLE(w->w),
                         w->priv.volume.capture_switches.widgets[i].w,
                         i, i + 1, 2, 3,
                         GTK_FILL, GTK_FILL, 0, 0);
        }
      }
    
    /* Capture Sliders */
    for(i = 0; i < w->priv.volume.capture_sliders.num; i++)
      {
      gtk_table_attach_defaults(GTK_TABLE(w->w),
                       w->priv.volume.capture_sliders.widgets[i].w,
                       i, i + 1, 3, 4);
      }
    if(w->priv.volume.capture_sliders.num > 1)
      {
      gtk_table_attach(GTK_TABLE(w->w),
                       w->priv.volume.capture_sliders.lock_button,
                       0, w->priv.volume.capture_sliders.num,
                       4, 5, GTK_FILL, GTK_FILL, 0, 0);
      
      }
    }

  if(playback_width)
    {
    label = gtk_label_new(TR("Play"));
    gtk_widget_show(label);

    gtk_table_attach(GTK_TABLE(w->w), label,
                     capture_width, capture_width + playback_width, 1, 2,
                     GTK_FILL, GTK_FILL, 0, 0);
    
    /* Playback Switches */
    if(w->priv.volume.playback_switches.num == 1)
      {
      gtk_table_attach(GTK_TABLE(w->w),
                       w->priv.volume.playback_switches.widgets[0].w,
                       capture_width, capture_width + playback_width, 2, 3,
                       GTK_FILL, GTK_FILL, 0, 0);
        
      }
    else
      for(i = 0; i < w->priv.volume.playback_switches.num; i++)
        {
        gtk_table_attach(GTK_TABLE(w->w),
                         w->priv.volume.playback_switches.widgets[i].w,
                         capture_width + i, capture_width + i + 1, 2, 3,
                         GTK_FILL, GTK_FILL, 0, 0);
        
        }
    
    /* Playback Sliders */
    
    for(i = 0; i < w->priv.volume.playback_sliders.num; i++)
      {
      gtk_table_attach_defaults(GTK_TABLE(w->w),
                                w->priv.volume.playback_sliders.widgets[i].w,
                                capture_width + i, capture_width + i + 1, 3, 4);
      }
    if(w->priv.volume.playback_sliders.num > 1)
      {
      gtk_table_attach(GTK_TABLE(w->w),
                       w->priv.volume.playback_sliders.lock_button,
                       capture_width, capture_width+
                       w->priv.volume.playback_sliders.num,
                       4, 5, GTK_FILL, GTK_FILL, 0, 0);
      
      }
    
    
    }
  gtk_widget_show(w->w);
  w->upper = 1;
  }

static int hctl_singlebool_callback(snd_hctl_elem_t *elem, unsigned int mask)
  {
  control_widget_t * c;

  int value;
  
  if(mask & SND_CTL_EVENT_MASK_VALUE) 
    {
    c = (control_widget_t*)snd_hctl_elem_get_callback_private(elem);
    snd_hctl_elem_read(elem, c->priv.singlebool.control->val);

    value = snd_ctl_elem_value_get_boolean(c->priv.singlebool.control->val, 0);

    widget_block(&c->priv.singlebool.checkbutton);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->priv.singlebool.checkbutton.w),
                                 value);
    widget_unblock(&c->priv.singlebool.checkbutton);
    }
  return 0;
  }


static void singlebool_callback(GtkWidget * w, gpointer data)
  {
  int value;
  control_widget_t * c = (control_widget_t *)data;

  value =
    (int)(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c->priv.singlebool.checkbutton.w)));
  
  snd_ctl_elem_value_set_boolean(c->priv.singlebool.control->val, 0, value);
  /* Write value */
  snd_hctl_elem_write(c->priv.singlebool.control->hctl,
                      c->priv.singlebool.control->val);
  }


static void init_singlebool(control_widget_t * w, alsa_mixer_group_t * c,
                            snd_ctl_elem_info_t * info)
  {
  w->priv.singlebool.checkbutton.w =
    gtk_toggle_button_new_with_label(c->label);
  gtk_widget_show(w->priv.singlebool.checkbutton.w);
  w->priv.singlebool.checkbutton.id =
    g_signal_connect(G_OBJECT(w->priv.singlebool.checkbutton.w),
                     "toggled", G_CALLBACK(singlebool_callback), 
                     w);
  w->priv.singlebool.checkbutton.handler_widget =
    w->priv.singlebool.checkbutton.w;
  w->priv.singlebool.control = c->ctl;
  w->w = w->priv.singlebool.checkbutton.w;

  snd_hctl_elem_set_callback(c->ctl->hctl, hctl_singlebool_callback);
  snd_hctl_elem_set_callback_private(c->ctl->hctl, w);
  hctl_singlebool_callback(c->ctl->hctl, SND_CTL_EVENT_MASK_VALUE);

  w->priv.singlebool.control = c->ctl;
  }

static void init_integer(control_widget_t * w, alsa_mixer_group_t * c,
                         snd_ctl_elem_info_t * info)
  {
  int i;
  GtkWidget * label;
  
  init_array(&w->priv.integer.sliders, c->ctl,
             info, 0);
  
  w->w = gtk_table_new(2, w->priv.integer.sliders.num,
                       0);

  w->label = bg_strdup(w->label, c->label);
  label = gtk_label_new(w->label);
  
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(w->w), label,
                   0, w->priv.integer.sliders.num, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);
  
  for(i = 0; i < w->priv.integer.sliders.num; i++)
    {
    gtk_table_attach_defaults(GTK_TABLE(w->w), w->priv.integer.sliders.widgets[i].w,
                              i, i+1, 1, 2);
    }
  gtk_widget_show(w->w);
  w->upper = 1;
  }
     

static void init_bool(control_widget_t * w, alsa_mixer_group_t * c,
                      snd_ctl_elem_info_t * info)
  {
  int i;
  GtkWidget * label;

  init_array(&w->priv.bool.buttons, c->ctl,
             info, 0);
  
  w->w = gtk_table_new(w->priv.bool.buttons.num + 1,
                       1, 0);

  w->label = bg_strdup(w->label, c->label);
  label = gtk_label_new(w->label);
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(w->w), label, 0, 1, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);

  init_array(&w->priv.bool.buttons, c->ctl,
             info, 0);
  
  for(i = 0; i < w->priv.bool.buttons.num; i++)
    {
    gtk_table_attach(GTK_TABLE(w->w),
                     w->priv.bool.buttons.widgets[i].w, 0, 1,
                     i+1, i+2,
                     GTK_FILL, GTK_FILL, 0, 0);
    }
  gtk_widget_show(w->w);
  w->upper = 1;
  }

static void init_enumerated(control_widget_t * w, alsa_mixer_group_t * c,
                            snd_ctl_elem_info_t * info)
  {
  GtkWidget * label;
  int i;
  init_array(&w->priv.enumerated.combos, c->ctl,
             info, 0);
  w->w = gtk_table_new(w->priv.enumerated.combos.num + 1,
                       1, 0);

  w->label = bg_strdup(w->label, c->label);
  label = gtk_label_new(w->label);
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(w->w), label, 0, 1, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);
  for(i = 0; i < w->priv.enumerated.combos.num; i++)
    {
    gtk_table_attach(GTK_TABLE(w->w),
                     w->priv.enumerated.combos.widgets[i].w,
                     0, 1, i+1, i+2, GTK_FILL, GTK_FILL, 0, 0);
    }
  gtk_widget_show(w->w);
  w->upper = 1;
  }

static void init_singleenumerated(control_widget_t * w, alsa_mixer_group_t * c,
                                  snd_ctl_elem_info_t * info)
  {
  GtkWidget * label;
  init_array(&w->priv.enumerated.combos, c->ctl,
             info, 0);
  w->w = gtk_table_new(1, 2, 0);
    
  label = gtk_label_new(c->label);
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(w->w), label, 0, 1, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach_defaults(GTK_TABLE(w->w),
                            w->priv.enumerated.combos.widgets[0].w,
                            1, 2, 0, 1);
  gtk_widget_show(w->w);
  }

static gboolean button_press_callback(GtkWidget * w, GdkEventButton * evt,
                                      gpointer data)
  {
  control_widget_t * wid;
  wid = (control_widget_t*)data;

  /* No menu of we are in an own window */

  if(wid->own_window)
    return FALSE;
  
  if(evt->button == 3)
    {
    if(about_window)
      gtk_widget_set_sensitive(wid->menu.about, 0);
    else
      gtk_widget_set_sensitive(wid->menu.about, 1);
    gtk_menu_popup(GTK_MENU(wid->menu.menu),
                   (GtkWidget *)0,
                   (GtkWidget *)0,
                   (GtkMenuPositionFunc)0,
                   (gpointer)0,
                   3, evt->time);
    return TRUE;
    }
  return FALSE;
  }
  

static void make_moveable(control_widget_t * w)
  {
  GtkWidget * event_box;
  init_menu(w);

  event_box = gtk_event_box_new();
  gtk_widget_set_events(event_box, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(G_OBJECT(event_box), "button-press-event",
                   G_CALLBACK(button_press_callback), (gpointer)w);
  gtk_container_add(GTK_CONTAINER(event_box), w->w);
  gtk_widget_show(event_box);
  w->w = event_box;
  }

control_widget_t * control_widget_create(alsa_mixer_group_t * c,
                                         bg_cfg_section_t * section,
                                         card_widget_t * card)
  {
  snd_ctl_elem_info_t * info;
  
  control_widget_t * ret = (control_widget_t *)0;
  snd_ctl_elem_type_t type;

  /* Check for volume controls */
  
  if((c->playback_volume) || 
     (c->capture_volume) || 
     (c->playback_switch && c->capture_switch))
    {
    ret = calloc(1, sizeof(*ret));
    init_volume(ret, c);
    ret->type = TYPE_VOLUME;
    }

  else if(c->playback_switch && !c->capture_switch)
    {
    c->ctl = c->playback_switch;
    c->playback_switch = NULL;
    }

  else if(!c->playback_switch && c->capture_switch)
    {
    c->ctl = c->capture_switch;
    c->capture_switch = NULL;
    }
  else if(c->tone_bass || c->tone_treble)
    {
    ret = calloc(1, sizeof(*ret));
    init_tone(ret, c);
    ret->type = TYPE_TONE;
    }
  
  if(!ret)
    {
    snd_ctl_elem_info_malloc(&info);
    snd_hctl_elem_info(c->ctl->hctl, info);
    type = snd_ctl_elem_info_get_type(info);
    switch(type)
      {
      case SND_CTL_ELEM_TYPE_BOOLEAN:
        if(snd_ctl_elem_info_get_count(info) > 1)
          {
          ret = calloc(1, sizeof(*ret));
          init_bool(ret, c, info);
          ret->type = TYPE_BOOL;
          }
        else
          {
          ret = calloc(1, sizeof(*ret));
          init_singlebool(ret, c, info);
          ret->type = TYPE_SINGLEBOOL;
          }
        break;
      case SND_CTL_ELEM_TYPE_INTEGER:
        if(snd_ctl_elem_info_get_min(info) >= snd_ctl_elem_info_get_max(info))
          {
          break;
          }
        ret = calloc(1, sizeof(*ret));
        init_integer(ret, c, info);
        ret->type = TYPE_INTEGER;
        break;
      case SND_CTL_ELEM_TYPE_ENUMERATED:
        if(snd_ctl_elem_info_get_count(info) > 1)
          {
          ret = calloc(1, sizeof(*ret));
          init_enumerated(ret, c, info);
          }
        else
          {
          ret = calloc(1, sizeof(*ret));
          init_singleenumerated(ret, c, info);
          }
        ret->type = TYPE_ENUM;
        break;
      case SND_CTL_ELEM_TYPE_INTEGER64:
        if(snd_ctl_elem_info_get_min64(info) >= snd_ctl_elem_info_get_max64(info))
          {
          break;
          }
        ret = calloc(1, sizeof(*ret));
        init_integer(ret, c, info);
        ret->type = TYPE_INTEGER64;
        break;
      case SND_CTL_ELEM_TYPE_NONE:
        
      case SND_CTL_ELEM_TYPE_BYTES:
      case SND_CTL_ELEM_TYPE_IEC958:
        bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Type %s not handled for %s",
                snd_ctl_elem_type_name(type), c->label);
        break;
      }
    snd_ctl_elem_info_free(info);
    }
  if(ret)
    {
    ret->section = section;
    ret->card = card;

    /* Check, if we are movable */

    if(ret->upper)
      make_moveable(ret);
    }
  return ret;
  }

int control_widget_is_upper(control_widget_t * w)
  {
  return w->upper;
  }

void control_widget_destroy(control_widget_t * w)
  {
  if(w->label)
    free(w->label);
  free(w);
  }

GtkWidget * control_widget_get_widget(control_widget_t * w)
  {
  return w->w;
  }

static const bg_parameter_info_t playback_lock_param =
  {
    .name = "playback_locked",
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 }
  };

static const bg_parameter_info_t capture_lock_param =
  {
    .name = "capture_locked",
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 }
  };

static const bg_parameter_info_t bass_lock_param =
  {
    .name = "bass_locked",
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 }
  };

static const bg_parameter_info_t treble_lock_param =
  {
    .name = "treble_locked",
    .type = BG_PARAMETER_CHECKBUTTON,
    .val_default = { .val_i = 1 }
  };

static const bg_parameter_info_t upper_params[] =
  {
    {
      .name = "hidden",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 0 }
    },
    {
      .name = "index",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = -1 }
    },
    {
      .name = "own_window",
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 }
    },
    {
      .name = "x",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 100 }
    },
    {
      .name = "y",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 100 }
    },
    {
      .name = "width",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 20 }
    },
    {
      .name = "height",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 100 }
    },
  };

static void create_parameters(control_widget_t * w)
  {
  int i, j;
  int num_parameters = 0;
  
  if(w->upper) /* Index */
    {
    num_parameters+=7;
    }
  if(w->type == TYPE_VOLUME)
    {
    if(w->priv.volume.capture_sliders.lock_button)
      num_parameters++;
    if(w->priv.volume.playback_sliders.lock_button)
      num_parameters++;
    }
  if(w->type == TYPE_TONE)
    {
    if(w->priv.tone.bass.lock_button)
      num_parameters++;
    if(w->priv.tone.treble.lock_button)
      num_parameters++;
    }

  w->parameters = calloc(num_parameters+1, sizeof(*w->parameters));
  i = 0;

  if(w->upper)
    {
    for(j = 0; j < 7; j++)
      {
      bg_parameter_info_copy(&w->parameters[i], &upper_params[j]);
      i++;
      }
    }
  if(w->type == TYPE_VOLUME)
    {
    if(w->priv.volume.capture_sliders.lock_button)
      {
      bg_parameter_info_copy(&w->parameters[i], &capture_lock_param);
      i++;
      }
    if(w->priv.volume.playback_sliders.lock_button)
      {
      bg_parameter_info_copy(&w->parameters[i], &playback_lock_param);
      i++;
      }
    }
  
  if(w->type == TYPE_TONE)
    {
    if(w->priv.tone.bass.lock_button)
      {
      bg_parameter_info_copy(&w->parameters[i], &bass_lock_param);
      i++;
      }
    if(w->priv.tone.treble.lock_button)
      {
      bg_parameter_info_copy(&w->parameters[i], &treble_lock_param);
      i++;
      }
    }
  }

void control_widget_set_parameter(void * data, const char * name,
                                  const bg_parameter_value_t * v)
  {
  control_widget_t * w;
  w = (control_widget_t*)data;

  if(!name)
    return;

  
  if(!strcmp(name, "playback_locked"))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->priv.volume.playback_sliders.lock_button),
                                 v->val_i);
    }
  if(!strcmp(name, "capture_locked"))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->priv.volume.capture_sliders.lock_button),
                                 v->val_i);
    
    }
  if(!strcmp(name, "bass_locked"))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->priv.tone.bass.lock_button),
                                 v->val_i);

    }
  if(!strcmp(name, "treble_locked"))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->priv.tone.treble.lock_button),
                                 v->val_i);
    
    }
  if(!strcmp(name, "index"))
    {
    control_widget_set_index(w, v->val_i);
    }
  if(!strcmp(name, "own_window"))
    {
    w->own_window = v->val_i;
    }
  if(!strcmp(name, "x"))
    {
    w->x = v->val_i;
    }
  if(!strcmp(name, "y"))
    {
    w->y = v->val_i;
    }
  if(!strcmp(name, "width"))
    {
    w->width = v->val_i;
    }
  if(!strcmp(name, "height"))
    {
    w->height = v->val_i;
    }
  if(!strcmp(name, "hidden"))
    {
    //    w->hidden = v->val_i;
    control_widget_set_hidden(w, v->val_i);
    }
  }

int control_widget_get_parameter(void * data, const char * name,
                                 bg_parameter_value_t * v)
  {
  control_widget_t * w;
  w = (control_widget_t*)data;

  
  if(!strcmp(name, "playback_locked"))
    {
    v->val_i = w->priv.volume.playback_sliders.locked;
    return 1;
    }
  if(!strcmp(name, "capture_locked"))
    {
    v->val_i = w->priv.volume.capture_sliders.locked;
    return 1;
    }
  if(!strcmp(name, "bass_locked"))
    {
    v->val_i = w->priv.tone.bass.locked;
    return 1;
    }
  if(!strcmp(name, "treble_locked"))
    {
    v->val_i = w->priv.tone.treble.locked;
    return 1;
    }
  if(!strcmp(name, "index"))
    {
    v->val_i = w->index;
    return 1;
    }
  if(!strcmp(name, "own_window"))
    {
    v->val_i = w->own_window;
    }
  if(!strcmp(name, "x"))
    {
    v->val_i = w->x;
    }
  if(!strcmp(name, "y"))
    {
    v->val_i = w->y;
    }
  if(!strcmp(name, "width"))
    {
    v->val_i = w->width;
    }
  if(!strcmp(name, "height"))
    {
    v->val_i = w->height;
    }
  if(!strcmp(name, "hidden"))
    {
    v->val_i = w->hidden;
    }
  return 0; 
  }

void control_widget_read_config(control_widget_t * w)
  {
  if(!w->parameters)
    create_parameters(w);
  
  bg_cfg_section_apply(w->section,
                       w->parameters,
                       control_widget_set_parameter,
                       w);

  }

void control_widget_write_config(control_widget_t * w)
  {
  if(!w->parameters)
    create_parameters(w);
  
  bg_cfg_section_get(w->section,
                     w->parameters,
                     control_widget_get_parameter,
                     w);
  
  }

int control_widget_get_own_window(control_widget_t * w)
  {
  return w->own_window;
  }

void control_widget_set_own_window(control_widget_t * w, int own_window)
  {
  w->own_window = own_window;
  }

void control_widget_get_coords(control_widget_t * w, int * x, int * y, int * width, int * height)
  {
  *x = w->x;
  *y = w->y;
  *width = w->width;
  *height = w->height;
  
  }

void control_widget_set_coords(control_widget_t * w, int x, int y, int width, int height)
  {
  w->x = x;
  w->y = y;
  w->width = width;
  w->height = height;
  }

void control_widget_set_hidden(control_widget_t * w, int hidden)
  {
  w->hidden = hidden;
  if(hidden)
    gtk_widget_hide(w->w);
  else
    gtk_widget_show(w->w);
  }

int control_widget_get_hidden(control_widget_t * w)
  {
  return w->hidden;
  }

const char * control_widget_get_label(control_widget_t * w)
  {
  return w->label;
  }
