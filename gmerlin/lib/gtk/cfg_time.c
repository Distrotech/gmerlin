/*****************************************************************
 
  cfg_time.c
 
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
  GtkWidget * label;
  GtkWidget * spinbutton_h;  /* Hours        */
  GtkWidget * spinbutton_m;  /* Minutes      */
  GtkWidget * spinbutton_s;  /* Seconds      */
  GtkWidget * spinbutton_ms; /* Milleseconds */
  GtkWidget * box;
  } spinbutton_t;

/*
  typedef enum
{
  GTK_EXPAND = 1 << 0,
  GTK_SHRINK = 1 << 1,
  GTK_FILL   = 1 << 2
} GtkAttachOptions;
*/

static void
destroy(bg_gtk_widget_t * w)
  {
  spinbutton_t * s = (spinbutton_t*)(w->priv);
  free(s);
  }

static void
get_value(bg_gtk_widget_t * w)
  {
  int i;
  gavl_time_t t;
  spinbutton_t * s = (spinbutton_t*)(w->priv);

  /* Fraction of a second */

  t = w->value.val_time;
  
  i = t % GAVL_TIME_SCALE;
  i /= (GAVL_TIME_SCALE / 1000);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_ms),
                            (gdouble)(i));

  /* Seconds */
  
  t /= GAVL_TIME_SCALE;
  
  i = t % 60;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_s),
                            (gdouble)(i));

  /* Minutes */

  t /= 60;
  
  i = t % 60;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_m),
                            (gdouble)(i));

  /* Hours */

  t /= 60;
  
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(s->spinbutton_h),
                            (gdouble)(t));
  
  }

static void
set_value(bg_gtk_widget_t * w)
  {
  int hours, minutes, seconds, milliseconds;

  spinbutton_t * s = (spinbutton_t*)(w->priv);

  milliseconds = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->spinbutton_ms));
  seconds      = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->spinbutton_s));
  minutes      = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->spinbutton_m));
  hours        = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->spinbutton_h));

  w->value.val_time = hours;
  w->value.val_time *= 60;
  w->value.val_time += minutes;
  w->value.val_time *= 60;
  w->value.val_time += seconds;
  w->value.val_time *= GAVL_TIME_SCALE;

  w->value.val_time += milliseconds * (GAVL_TIME_SCALE / 1000);
  }

static void
attach(void * priv, GtkWidget * table, int * row, int * num_columns)
  {
  spinbutton_t * s = (spinbutton_t*)priv;

  if(*num_columns < 2)
    *num_columns = 2;

  gtk_table_resize(GTK_TABLE(table), *row + 1, *num_columns);

  gtk_table_attach(GTK_TABLE(table), s->label,
                   0, 1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);
  
  gtk_table_attach(GTK_TABLE(table), s->box,
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  *row += 1;
  }

static gtk_widget_funcs_t funcs =
  {
    get_value: get_value,
    set_value: set_value,
    destroy: destroy,
    attach:  attach
  };

void 
bg_gtk_create_time(bg_gtk_widget_t * w,
                   bg_parameter_info_t * info)
  {
  GtkWidget * label;
  spinbutton_t * s = calloc(1, sizeof(*s));
  s->label = gtk_label_new(info->long_name);

  gtk_widget_show(s->label);
  gtk_misc_set_alignment(GTK_MISC(s->label), 0.0, 0.5);

  s->spinbutton_ms =
    gtk_spin_button_new_with_range(0.0, 999.0, 1.0);
  s->spinbutton_s =
    gtk_spin_button_new_with_range(0.0, 59.0, 1.0);
  s->spinbutton_m =
    gtk_spin_button_new_with_range(0.0, 59.0, 1.0);
  s->spinbutton_h =
    gtk_spin_button_new_with_range(0.0, 1000000.0, 1.0);
  
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_ms), 0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_s), 0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_m), 0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(s->spinbutton_h), 0);

  gtk_widget_show(s->spinbutton_ms);
  gtk_widget_show(s->spinbutton_s);
  gtk_widget_show(s->spinbutton_m);
  gtk_widget_show(s->spinbutton_h);

  s->box = gtk_hbox_new(0, 2);
  gtk_box_pack_start_defaults(GTK_BOX(s->box), s->spinbutton_h);
  label = gtk_label_new(":");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(s->box), label, FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(s->box), s->spinbutton_m);
  label = gtk_label_new(":");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(s->box), label, FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(s->box), s->spinbutton_s);
  label = gtk_label_new(".");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(s->box), label, FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(s->box), s->spinbutton_ms);
  
  gtk_widget_show(s->box);
  gtk_widget_show(s->label);
  w->priv = s;
  w->funcs = &funcs;
  }

