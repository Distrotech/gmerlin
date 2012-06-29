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

#include "gui.h"
#include <gui_gtk/gtkutils.h>

struct mixer_window_s
  {
  GtkWidget * main_win;
  //  GtkWidget * aux_win;

  int num_cards;
  card_widget_t ** cards;
  bg_cfg_registry_t * cfg_reg;

  int x, y, width, height;
  };

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "x",
      .long_name = "X",
      .type =      BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 100 },
    },
    {
      .name =      "y",
      .long_name = "Y",
      .type =      BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 100 },
    },
    {
      .name =      "width",
      .long_name = "Width",
      .type =      BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 300 },
    },
    {
      .name =      "height",
      .long_name = "Height",
      .type =      BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 200 },
    },
    { /* End of parameters */ }
  };

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  mixer_window_t * w;
  w = (mixer_window_t*)data;
  
  if(!name)
    {
    return;
    }
  if(!strcmp(name, "x"))
    {
    w->x = val->val_i;
    }
  else if(!strcmp(name, "y"))
    {
    w->y = val->val_i;
    }
  else if(!strcmp(name, "width"))
    {
    w->width = val->val_i;
    }
  else if(!strcmp(name, "height"))
    {
    w->height = val->val_i;
    }
  }

static int get_parameter(void * data, const char * name,
                         bg_parameter_value_t * val)
  {
  mixer_window_t * w;
  w = (mixer_window_t*)data;
  if(!name)
    {
    return 1;
    }
  if(!strcmp(name, "x"))
    {
    val->val_i = w->x;
    return 1;
    }
  else if(!strcmp(name, "y"))
    {
    val->val_i = w->y;
    return 1;
    }
  else if(!strcmp(name, "width"))
    {
    val->val_i = w->width;
    return 1;
    }
  else if(!strcmp(name, "height"))
    {
    val->val_i = w->height;
    return 1;
    }
  return 0;
  }



static gboolean delete_callback(GtkWidget * wid, GdkEventAny * evt,
                                gpointer data)
  {
  int i;
  mixer_window_t * w;
  bg_cfg_section_t * section;
  
  w = (mixer_window_t*)data;
  section =
    bg_cfg_registry_find_section(w->cfg_reg, "General");

  gtk_window_get_position(GTK_WINDOW(w->main_win), &w->x, &w->y);
  gtk_window_get_size(GTK_WINDOW(w->main_win), &w->width, &w->height);
  bg_cfg_section_get(section, parameters, get_parameter, w);

  for(i = 0; i < w->num_cards; i++)
    card_widget_get_window_coords(w->cards[i]);
  
  gtk_main_quit();
  return TRUE;
  }



static void map_callback(GtkWidget * wid, gpointer data)
  {
  mixer_window_t * w;
  w = (mixer_window_t*)data;
  gtk_window_resize(GTK_WINDOW(w->main_win), w->width, w->height);
  gtk_window_move(GTK_WINDOW(w->main_win), w->x, w->y);
  }

mixer_window_t * mixer_window_create(alsa_mixer_t * mixer,
                                     bg_cfg_registry_t * cfg_reg)
  {
  int i;
  GtkWidget * notebook;
  GtkWidget * label;
  bg_cfg_section_t * section;
  mixer_window_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cfg_reg = cfg_reg;
  ret->main_win = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->main_win), "Gmerlin Alsamixer");
  
  
  g_signal_connect(G_OBJECT(ret->main_win),
                   "delete-event", G_CALLBACK(delete_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->main_win),
                   "map", G_CALLBACK(map_callback),
                   ret);
    
  ret->cards = calloc(mixer->num_cards, sizeof(*(ret->cards)));
  
  notebook = gtk_notebook_new();
  ret->num_cards = mixer->num_cards;
  for(i = 0; i < mixer->num_cards; i++)
    {
    if(mixer->cards[i]->num_groups)
      {
      section =
        bg_cfg_registry_find_section(ret->cfg_reg,
                                     mixer->cards[i]->name);
      label = gtk_label_new(mixer->cards[i]->name);
      gtk_widget_show(label);
      
      ret->cards[i] = card_widget_create(mixer->cards[i], section);
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                               card_widget_get_widget(ret->cards[i]),
                               label);
      }
    
    }
  
  gtk_widget_show(notebook);
  gtk_container_add(GTK_CONTAINER(ret->main_win), notebook);

  section =
    bg_cfg_registry_find_section(ret->cfg_reg, "General");
  bg_cfg_section_apply(section, parameters, set_parameter, ret);
  return ret;
  }

void mixer_window_destroy(mixer_window_t * w)
  {
  int i;
  
  for(i = 0; i < w->num_cards; i++)
    card_widget_destroy(w->cards[i]);
  
  free(w);
  }

void mixer_window_run(mixer_window_t * w)
  {
  gtk_widget_show(w->main_win);
  gtk_main();
  }
