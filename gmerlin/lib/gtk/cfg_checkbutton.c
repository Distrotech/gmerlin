/*****************************************************************
 
  cfg_checkbutton.c
 
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

typedef struct
  {
  GtkWidget * button;
  } checkbutton_t;

static void get_value(bg_gtk_widget_t * w)
  {
  checkbutton_t * priv;
  priv = (checkbutton_t*)(w->priv);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->button), w->value.val_i);
  }

static void set_value(bg_gtk_widget_t * w)
  {
  checkbutton_t * priv;
  priv = (checkbutton_t*)(w->priv);
  w->value.val_i =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->button));
  }

static void destroy(bg_gtk_widget_t * w)
  {
  checkbutton_t * priv = (checkbutton_t *)w->priv;
  free(priv);
  }

static void attach(void * priv, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  checkbutton_t * b = (checkbutton_t*)priv;

  if(*num_columns < 2)
    *num_columns = 2;
  
  gtk_table_resize(GTK_TABLE(table), *row+1, *num_columns);
  gtk_table_attach(GTK_TABLE(table), b->button,
                    0, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  (*row)++;
  }

static gtk_widget_funcs_t funcs =
  {
    get_value: get_value,
    set_value: set_value,
    destroy:   destroy,
    attach:    attach
  };


void bg_gtk_create_checkbutton(bg_gtk_widget_t * w, bg_parameter_info_t * info)
  {
  checkbutton_t * priv = calloc(1, sizeof(*priv));
  priv->button = gtk_check_button_new_with_label(info->long_name);

  if(info->flags & BG_PARAMETER_SYNC)
    {
    w->callback_id = 
      g_signal_connect(G_OBJECT(priv->button), "toggled",
                       G_CALLBACK(bg_gtk_change_callback), (gpointer)w);
    w->callback_widget = priv->button;
    }

  if(info->help_string)
    {
    gtk_tooltips_set_tip(w->tooltips, priv->button, info->help_string, info->help_string);
    }
  
  gtk_widget_show(priv->button);
 
  w->funcs = &funcs;
  w->priv = priv;
  }
