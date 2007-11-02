/*****************************************************************
 
  cfg_font.c
 
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

#include <config.h>
#include <stdio.h>
#include <string.h>

#include "gtk_dialog.h"
#include <gui_gtk/gtkutils.h>

typedef struct
  {
  GtkWidget * entry;
  GtkWidget * label;
  GtkWidget * button;
  GtkWidget * fontselect;
  } font_t;

static void button_callback(GtkWidget * w, gpointer data);

static void get_value(bg_gtk_widget_t * w)
  {
  font_t * priv;
  char * tmp;
  priv = (font_t*)(w->priv);

  if(!w->value.val_str || (*w->value.val_str == '\0'))
    {
    gtk_entry_set_text(GTK_ENTRY(priv->entry), "");
    return;
    }
  tmp = bg_gtk_convert_font_name_to_pango(w->value.val_str);
  gtk_entry_set_text(GTK_ENTRY(priv->entry), tmp);
  free(tmp);
  }

static void set_value(bg_gtk_widget_t * w)
  {
  font_t * priv;
  const char * font;
  
  priv = (font_t*)(w->priv);

  font = gtk_entry_get_text(GTK_ENTRY(priv->entry));

  if(w->value.val_str)
    {
    free(w->value.val_str);
    w->value.val_str = (char*)0;
    }
  
  if(*font != '\0')
    {
    w->value.val_str = bg_gtk_convert_font_name_from_pango(font);
    }
  }

static void destroy(bg_gtk_widget_t * w)
  {
  font_t * priv = (font_t*)(w->priv);
  if(priv->fontselect)
    gtk_widget_destroy(priv->fontselect);
  free(priv);
  }

static void attach(void * priv, GtkWidget * table,
                   int * row,
                   int * num_columns)
  {
  font_t * f = (font_t*)priv;

  if(*num_columns < 3)
    *num_columns = 3;
  
  gtk_table_resize(GTK_TABLE(table), *row+1, *num_columns);

  gtk_table_attach(GTK_TABLE(table), f->label,
                    0, 1, *row, *row+1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), f->entry,
                   1, 2, *row, *row+1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  
  gtk_table_attach(GTK_TABLE(table), f->button,
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

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }


static void button_callback(GtkWidget * w, gpointer data)
  {
  char * fontname;
  font_t * priv = (font_t*)data;
  GtkWidget * toplevel;
  if(w == priv->button)
    {
    if(!priv->fontselect)
      {
      priv->fontselect =  gtk_font_selection_dialog_new("Select a font");
      gtk_window_set_modal(GTK_WINDOW(priv->fontselect), TRUE);
      toplevel = bg_gtk_get_toplevel(priv->button);
      if(toplevel)
        gtk_window_set_transient_for(GTK_WINDOW(priv->fontselect),
                                     GTK_WINDOW(toplevel));
      
      g_signal_connect(G_OBJECT(GTK_FONT_SELECTION_DIALOG(priv->fontselect)->ok_button),
                         "clicked", G_CALLBACK(button_callback),
                         (gpointer)priv);
      g_signal_connect(G_OBJECT(GTK_FONT_SELECTION_DIALOG(priv->fontselect)->cancel_button),
                         "clicked", G_CALLBACK(button_callback),
                         (gpointer)priv);
      g_signal_connect(G_OBJECT(priv->fontselect),
                       "delete_event", G_CALLBACK(delete_callback),
                       (gpointer)priv);
      
      }

    gtk_font_selection_set_font_name(GTK_FONT_SELECTION(GTK_FONT_SELECTION_DIALOG(priv->fontselect)->fontsel), gtk_entry_get_text(GTK_ENTRY(priv->entry)));
    
    gtk_widget_show(priv->fontselect);
    gtk_main();
    }
  else if(priv->fontselect)
    {
    if(w == GTK_FONT_SELECTION_DIALOG(priv->fontselect)->ok_button)
      {
      gtk_widget_hide(priv->fontselect);
      gtk_main_quit();
      fontname = gtk_font_selection_get_font_name(GTK_FONT_SELECTION(GTK_FONT_SELECTION_DIALOG(priv->fontselect)->fontsel));
      
      gtk_entry_set_text(GTK_ENTRY(priv->entry), fontname);
      g_free(fontname);
      }
    if((w == GTK_FONT_SELECTION_DIALOG(priv->fontselect)->cancel_button) ||
       (w == priv->fontselect))
      {
      gtk_widget_hide(priv->fontselect);
      gtk_main_quit();
      }
    }
  
  }

void bg_gtk_create_font(bg_gtk_widget_t * w, bg_parameter_info_t * info,
                        const char * translation_domain)
  {
  font_t * priv = calloc(1, sizeof(*priv));

  priv->entry = gtk_entry_new();

  if(info->help_string)
    {
    bg_gtk_tooltips_set_tip(priv->entry, info->help_string, translation_domain);
    }
  
  gtk_widget_show(priv->entry);

  priv->label = gtk_label_new(TR_DOM(info->long_name));
  gtk_misc_set_alignment(GTK_MISC(priv->label), 0.0, 0.5);

  gtk_widget_show(priv->label);

  priv->button = gtk_button_new_with_label(TR("Browse..."));

  g_signal_connect(G_OBJECT(priv->button),
                     "clicked", G_CALLBACK(button_callback),
                     (gpointer)priv);
  gtk_widget_show(priv->button);
  
  w->funcs = &funcs;
  w->priv = priv;
  }
