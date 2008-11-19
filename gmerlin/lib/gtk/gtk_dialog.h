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

#include <gtk/gtk.h>

#include <config.h>

#include <gmerlin/cfg_dialog.h>

#include <gui_gtk/gtkutils.h>

#include <stdlib.h>

typedef struct bg_gtk_widget_s bg_gtk_widget_t;

typedef struct
  {
  /* value -> widget */
  void (*get_value)(bg_gtk_widget_t * w);
  /* widget -> value */
  void (*set_value)(bg_gtk_widget_t * w);
  
  /* Subsections -> set_parameter() */
  void (*apply_sub_params)(bg_gtk_widget_t * w);
  
  void (*destroy)(bg_gtk_widget_t * w);
  void (*attach)(void * priv, GtkWidget * table, int * row, int * num_columns);
  } gtk_widget_funcs_t;

struct bg_gtk_widget_s
  {
  void * priv;
  const gtk_widget_funcs_t * funcs;
  bg_parameter_value_t value;
  bg_parameter_value_t last_value; /* For pressing ESC */
  const bg_parameter_info_t * info;

  /* For change callbacks */
  
  bg_set_parameter_func_t change_callback;
  void * change_callback_data;

  gulong callback_id;
  GtkWidget * callback_widget;

  /* position needs this */
  gulong callback_id_2;
  GtkWidget * callback_widget_2;

  
  bg_cfg_section_t * cfg_section;
  bg_cfg_section_t * cfg_subsection_save;
  };

void 
bg_gtk_create_checkbutton(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_int(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_float(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_slider_int(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_slider_float(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_string(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_stringlist(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_color_rgb(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_color_rgba(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_font(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_device(bg_gtk_widget_t *, const char * translation_domain);


void 
bg_gtk_create_time(bg_gtk_widget_t *, const char * translation_domain);


void
bg_gtk_create_multi_list(bg_gtk_widget_t *,
                         bg_set_parameter_func_t set_param,
                         void * data, const char * translation_domain);

void
bg_gtk_create_multi_chain(bg_gtk_widget_t *,
                          bg_set_parameter_func_t set_param,
                          void * data, const char * translation_domain);

void
bg_gtk_create_multi_menu(bg_gtk_widget_t *,
                         bg_set_parameter_func_t set_param,
                         bg_get_parameter_func_t get_param,
                         void * data, const char * translation_domain);

void 
bg_gtk_create_position(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_directory(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_file(bg_gtk_widget_t *, const char * translation_domain);

void 
bg_gtk_create_button(bg_gtk_widget_t *, const char * translation_domain);

void bg_gtk_change_callback(GtkWidget * gw, gpointer data);

void bg_gtk_change_callback_block(bg_gtk_widget_t * w, int block);
