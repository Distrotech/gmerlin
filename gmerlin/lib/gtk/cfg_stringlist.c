/*****************************************************************
 
  cfg_stringlist.c
 
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
  GtkWidget * label;
  GtkWidget * combo;
  GList * strings;
  } stringlist_t;

static void get_value(bg_gtk_widget_t * w)
  {
  stringlist_t * priv;
  priv = (stringlist_t*)(w->priv);
  if(!w->value.val_str || (*w->value.val_str == '\0'))
    {
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(priv->combo)->entry), "");
    return;
    }
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(priv->combo)->entry),
                     w->value.val_str);
  }

static void set_value(bg_gtk_widget_t * w)
  {
  stringlist_t * priv;
  const char * str;
  
  priv = (stringlist_t*)(w->priv);

  str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(priv->combo)->entry));

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
  stringlist_t * priv = (stringlist_t*)(w->priv);
  if(w->value.val_str)
    free(w->value.val_str);
  free(priv);
  }

static void attach(void * priv, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  stringlist_t * s = (stringlist_t*)priv;

  if(*num_columns < 2)
    *num_columns = 2;
  
  gtk_table_resize(GTK_TABLE(table), *row+1, *num_columns);

  gtk_table_attach(GTK_TABLE(table), s->label,
                   0, 1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), s->combo,
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

static void change_callback(GtkWidget * w, gpointer data)
  {
  const char * str;
  str = gtk_entry_get_text(GTK_ENTRY(w));

  if(str && (*str != '\0'))
    bg_gtk_change_callback(w, data);
  }

void bg_gtk_create_stringlist(bg_gtk_widget_t * w, bg_parameter_info_t * info)
  {
  int i;
  char * c;
  stringlist_t * priv = calloc(1, sizeof(*priv));

  priv->combo = gtk_combo_new();
  gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(priv->combo)->entry),
                            FALSE);
  
  i = 0;
  while(info->options[i])
    {
    c = g_strdup(info->options[i]);
    priv->strings = g_list_append(priv->strings, c);
    i++;
    }
  gtk_combo_set_popdown_strings(GTK_COMBO(priv->combo), priv->strings);

  if(info->flags & BG_PARAMETER_SYNC)
    {
    w->callback_widget = GTK_COMBO(priv->combo)->entry;
    w->callback_id = g_signal_connect(G_OBJECT(w->callback_widget),
                     "changed", G_CALLBACK(change_callback),
                     (gpointer)w);
    }
  
  gtk_widget_show(priv->combo);

  priv->label = gtk_label_new(info->long_name);
  gtk_misc_set_alignment(GTK_MISC(priv->label), 0.0, 0.5);

  gtk_widget_show(priv->label);
    
  w->funcs = &funcs;
  w->priv = priv;
  }
