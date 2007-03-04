/*****************************************************************
 
  cfg_slider.c
 
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

#include <stdio.h>

#include "gtk_dialog.h"
#include <gui_gtk/gtkutils.h>


typedef struct
  {
  GtkWidget * label;
  GtkWidget * slider;
  } slider_t;

/*
  typedef enum
{
  GTK_EXPAND = 1 << 0,
  GTK_SHRINK = 1 << 1,
  GTK_FILL   = 1 << 2
} GtkAttachOptions;
*/

static void destroy(bg_gtk_widget_t * w)
  {
  slider_t * priv = (slider_t*)(w->priv);
  free(priv);
  }

static void get_value_int(bg_gtk_widget_t * w)
  {
  slider_t * priv;
  priv = (slider_t*)(w->priv);
  gtk_range_set_value(GTK_RANGE(priv->slider),
                      (gdouble)w->value.val_i);
  
  }

static void set_value_int(bg_gtk_widget_t * w)
  {
  slider_t * priv;
  priv = (slider_t*)(w->priv);

  w->value.val_i =
    (int)(gtk_range_get_value(GTK_RANGE(priv->slider)));
  
  }

static void get_value_float(bg_gtk_widget_t * w)
  {
  slider_t * priv;
  priv = (slider_t*)(w->priv);
  gtk_range_set_value(GTK_RANGE(priv->slider),
                      (gdouble)w->value.val_f);
  
  }

static void set_value_float(bg_gtk_widget_t * w)
  {
  slider_t * priv;
  priv = (slider_t*)(w->priv);
  w->value.val_f =
    gtk_range_get_value(GTK_RANGE(priv->slider));
  }

static gboolean button_callback(GtkWidget * wid, GdkEventButton * evt,
                                gpointer data)
  {
  bg_gtk_widget_t * w;
  slider_t * priv;
  
  w = (bg_gtk_widget_t*)data;
  priv = (slider_t*)(w->priv);

  if(evt->type == GDK_2BUTTON_PRESS)
    {
    if(w->info->type == BG_PARAMETER_SLIDER_FLOAT)
      {
      w->value.val_f = w->info->val_default.val_f;
      get_value_float(w);
      }
    else if(w->info->type == BG_PARAMETER_SLIDER_INT)
      {
      w->value.val_i = w->info->val_default.val_i;
      get_value_int(w);
      }
    return TRUE;
    }
  return FALSE;
  }

static void attach(void * priv, GtkWidget * table, int * row, int * num_columns)
  {
  slider_t * s = (slider_t*)priv;

  if(*num_columns < 2)
    *num_columns = 2;

  gtk_table_resize(GTK_TABLE(table), *row + 1, *num_columns);

  gtk_table_attach(GTK_TABLE(table), s->label,
                   0, 1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
  
  gtk_table_attach(GTK_TABLE(table), s->slider,
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  *row += 1;
  }

static gtk_widget_funcs_t int_funcs =
  {
    get_value: get_value_int,
    set_value: set_value_int,
    destroy: destroy,
    attach:  attach
  };

static gtk_widget_funcs_t float_funcs =
  {
    get_value: get_value_float,
    set_value: set_value_float,
    destroy: destroy,
    attach:  attach
  };


static void create_common(bg_gtk_widget_t * w,
                          bg_parameter_info_t * info,
                          float min_value,
                          float max_value,
                          const char * translation_domain)
  {
  float step;
  slider_t * s = calloc(1, sizeof(*s));
  int i;
  s->label = gtk_label_new(TR_DOM(info->long_name));
  step = 1.0;
  for(i = 0; i < info->num_digits; i++)
    step /= 10.0;
  
  gtk_misc_set_alignment(GTK_MISC(s->label), 0.0, 0.5);
  gtk_widget_show(s->label);

  s->slider =
    gtk_hscale_new_with_range(min_value, max_value,  step);

  if(info->help_string)
    {
    bg_gtk_tooltips_set_tip(w->tooltips, s->slider,
                            info->help_string, translation_domain);
    }

  
  if(info->flags & BG_PARAMETER_SYNC)
    {
    w->callback_id =
      g_signal_connect(G_OBJECT(s->slider), "value-changed",
                       G_CALLBACK(bg_gtk_change_callback), (gpointer)w);
    w->callback_widget = s->slider;
    }
  gtk_scale_set_value_pos(GTK_SCALE(s->slider), GTK_POS_LEFT);
  gtk_widget_set_events(s->slider, GDK_BUTTON_PRESS_MASK);

  g_signal_connect(G_OBJECT(s->slider), "button-press-event",
                   G_CALLBACK(button_callback), (gpointer)w);
  
  gtk_widget_show(s->slider);
  gtk_widget_show(s->label);
  w->priv = s;
  }

void 
bg_gtk_create_slider_int(bg_gtk_widget_t * w,
                         bg_parameter_info_t * info,
                         const char * translation_domain)
  {
  float min_value;
  float max_value;
  
  slider_t * s;

  min_value = (float)(info->val_min.val_i);
  max_value = (float)(info->val_max.val_i);

  if(min_value >= max_value)
    {
    min_value = 0.0;
    max_value = 100000.0;
    }
  
  create_common(w, info, min_value, max_value, translation_domain);
  s = (slider_t *)(w->priv);
  w->funcs = &int_funcs;
  gtk_scale_set_digits(GTK_SCALE(s->slider), 0);
  }

void 
bg_gtk_create_slider_float(bg_gtk_widget_t * w,
                           bg_parameter_info_t * info,
                           const char * translation_domain)
  {
  float min_value;
  float max_value;
  
  slider_t * s;

  min_value = (float)(info->val_min.val_f);
  max_value = (float)(info->val_max.val_f);

  if(min_value >= max_value)
    {
    min_value = 0.0;
    max_value = 100000.0;
    }
  
  create_common(w, info, min_value, max_value, translation_domain);
  s = (slider_t *)(w->priv);

  gtk_scale_set_digits(GTK_SCALE(s->slider),
                       info->num_digits);

  w->funcs = &float_funcs;
  }
