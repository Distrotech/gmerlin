/*****************************************************************
 
  cfg_file.c
 
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
#include <gui_gtk/fileentry.h>
#include <gui_gtk/gtkutils.h>

typedef struct
  {
  bg_gtk_file_entry_t * fe;
  GtkWidget * label;
  } file_t;

static void get_value(bg_gtk_widget_t * w)
  {
  file_t * priv;
  priv = (file_t*)(w->priv);

  if(!w->value.val_str || (*w->value.val_str == '\0'))
    {
    bg_gtk_file_entry_set_filename(priv->fe, "");
    return;
    }
  bg_gtk_file_entry_set_filename(priv->fe, w->value.val_str);
  }

static void set_value(bg_gtk_widget_t * w)
  {
  file_t * priv;
  const char * filename;
  
  priv = (file_t*)(w->priv);

  filename = bg_gtk_file_entry_get_filename(priv->fe);

  if(w->value.val_str)
    {
    free(w->value.val_str);
    w->value.val_str = (char*)0;
    }
  
  if(*filename != '\0')
    {
    w->value.val_str = malloc(strlen(filename)+1);
    strcpy(w->value.val_str, filename);
    }
  }


static void destroy(bg_gtk_widget_t * w)
  {
  file_t * priv = (file_t*)w->priv;

  bg_gtk_file_entry_destroy(priv->fe);
  free(priv);
  }

static void attach(void * priv, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  file_t * f = (file_t*)priv;

  if(*num_columns < 3)
    *num_columns = 3;
  
  gtk_table_resize(GTK_TABLE(table), *row+1, *num_columns);
  gtk_table_attach(GTK_TABLE(table), f->label,
                    0, 1, *row, *row+1, GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_file_entry_get_entry(f->fe),
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_file_entry_get_button(f->fe),
                   2, 3, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  (*row)++;
  }

static gtk_widget_funcs_t funcs =
  {
    get_value: get_value,
    set_value: set_value,
    destroy:   destroy,
    attach:    attach
  };


void bg_gtk_create_file(bg_gtk_widget_t * w, bg_parameter_info_t * info,
                        const char * translation_domain)
  {
  file_t * priv = calloc(1, sizeof(*priv));
  
  priv->fe = bg_gtk_file_entry_create((info->type == BG_PARAMETER_DIRECTORY) ? 1 : 0,
                                      NULL, NULL,
                                      info->help_string, translation_domain);
  
  priv->label = gtk_label_new(TR_DOM(info->long_name));
  gtk_misc_set_alignment(GTK_MISC(priv->label), 0.0, 0.5);
  gtk_widget_show(priv->label);
  
  w->funcs = &funcs;
  w->priv = priv;
  }

void bg_gtk_create_directory(bg_gtk_widget_t * w, bg_parameter_info_t * info,
                             const char * translation_domain)
  {
  file_t * f;
  bg_gtk_create_file(w, info, translation_domain);
  f = (file_t*)(w->priv);
  }
