/*****************************************************************
 
  cfg_string.c
 
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
#include <string.h>

#include "gtk_dialog.h"

typedef struct
  {
  GtkWidget * entry;
  GtkWidget * label;
  } string_t;

static void get_value(bg_gtk_widget_t * w)
  {
  string_t * priv;
  priv = (string_t*)(w->priv);

  if(!w->value.val_str || (*w->value.val_str == '\0'))
    {
    gtk_entry_set_text(GTK_ENTRY(priv->entry), "");
    return;
    }
  gtk_entry_set_text(GTK_ENTRY(priv->entry), w->value.val_str);
  }

static void set_value(bg_gtk_widget_t * w)
  {
  string_t * priv;
  const char * str;
  
  priv = (string_t*)(w->priv);

  str = gtk_entry_get_text(GTK_ENTRY(priv->entry));

  if(w->value.val_str)
    {
    free(w->value.val_str);
    w->value.val_str = (char*)0;
    }
  
  if(*str != '\0')
    {
    w->value.val_str = malloc(strlen(str)+1);
    strcpy(w->value.val_str, str);
    }
  }

static void destroy(bg_gtk_widget_t * w)
  {
  string_t * priv = (string_t*)(w->priv);
  if(w->value.val_str)
    free(w->value.val_str);
  free(priv);
  }

static void attach(void * priv, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  string_t * b = (string_t*)priv;

  if(*num_columns < 2)
    *num_columns = 2;
  
  gtk_table_resize(GTK_TABLE(table), *row+1, *num_columns);
  //  gtk_table_attach_defaults(GTK_TABLE(table), b->button,
  //                            0, 1, *row, *row+1);
  gtk_table_attach(GTK_TABLE(table), b->label,
                   0, 1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), b->entry,
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);

  (*row)++;
  }

static gtk_widget_funcs_t funcs =
  {
    get_value: get_value,
    set_value: set_value,
    destroy:   destroy,
    attach:    attach
  };

void bg_gtk_create_string(bg_gtk_widget_t * w, bg_parameter_info_t * info)
  {
  string_t * priv = calloc(1, sizeof(*priv));

  priv->entry = gtk_entry_new();
  gtk_widget_show(priv->entry);

  priv->label = gtk_label_new(info->long_name);
  gtk_misc_set_alignment(GTK_MISC(priv->label), 0.0, 0.5);

  gtk_widget_show(priv->label);
  
  
  w->funcs = &funcs;
  w->priv = priv;
  }
