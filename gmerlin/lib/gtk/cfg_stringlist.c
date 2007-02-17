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
#include <utils.h>

#include "gtk_dialog.h"

#include <gui_gtk/gtkutils.h>

#if GTK_MINOR_VERSION >= 4
#define GTK_2_4
#endif

typedef struct
  {
  GtkWidget * label;
  GtkWidget * combo;
#ifndef GTK_2_4
  GList * strings;
#endif
  int selected;
  const char * translation_domain;
  } stringlist_t;

static void get_value(bg_gtk_widget_t * w)
  {
  stringlist_t * priv;
  priv = (stringlist_t*)(w->priv);

  priv->selected = bg_parameter_get_selected(w->info, 
                                             w->value.val_str);
  
#ifdef GTK_2_4
  gtk_combo_box_set_active(GTK_COMBO_BOX(priv->combo), priv->selected);
#else
  if(w->info->multi_labels)
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(priv->combo)->entry),
                       w->info->multi_labels[priv->selected]);
  else
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(priv->combo)->entry),
                       w->info->multi_names[priv->selected]);
#endif
  }

static void set_value(bg_gtk_widget_t * w)
  {
  stringlist_t * priv;
  
  priv = (stringlist_t*)(w->priv);
  w->value.val_str = bg_strdup(w->value.val_str, w->info->multi_names[priv->selected]);
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

static void change_callback(GtkWidget * wid, gpointer data)
  {
#ifndef GTK_2_4
  const char * str;
#endif
  bg_gtk_widget_t * w;
  stringlist_t * priv;

  w = (bg_gtk_widget_t *)data;
  priv = (stringlist_t *)w->priv;
  
#ifndef GTK_2_4
  str = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(priv->combo)->entry));
  
  priv->selected = 0;

  if(str && *str)
    {
    if(w->info->multi_labels)
      {
      while(strcmp(w->info->multi_labels[priv->selected], str))
        {
        priv->selected++;
        }
      }
    else
      {
      while(strcmp(w->info->multi_names[priv->selected], str))
        priv->selected++;
      }
    }
#else
  priv->selected = gtk_combo_box_get_active(GTK_COMBO_BOX(priv->combo));
#endif

  if(w->info->flags & BG_PARAMETER_SYNC)
    bg_gtk_change_callback(wid, data);
  
  }

#ifdef GTK_2_4

/*
 *  Really dirty trick to get tooltips for a GtkComboBox working:
 *  loop through all container children and set the tooltip for
 *  the child, which is a button
 */

static void
set_combo_tooltip(GtkWidget *widget, gpointer   data)
  {
  bg_gtk_widget_t * w = (bg_gtk_widget_t *)data;
  stringlist_t * priv;
  priv = (stringlist_t *)w->priv;
  
  //  GtkTooltips *tooltips = (GtkTooltips *)data;

  if(GTK_IS_BUTTON (widget))
    bg_gtk_tooltips_set_tip(w->tooltips, widget,
                            w->info->help_string,
                            priv->translation_domain);
  }

static void
realize_combo(GtkWidget *combo, gpointer   data)
  {
  bg_gtk_widget_t * w = (bg_gtk_widget_t *)data;
  
  gtk_container_forall (GTK_CONTAINER (combo),
                        set_combo_tooltip,
                        w);
  }
#endif

void bg_gtk_create_stringlist(bg_gtk_widget_t * w,
                              bg_parameter_info_t * info,
                              const char * translation_domain)
  {
  int i;
#ifndef GTK_2_4
  char * c;
#endif
  stringlist_t * priv = calloc(1, sizeof(*priv));

  w->funcs = &funcs;
  w->priv = priv;
  
  priv->translation_domain = translation_domain;
  
#ifdef GTK_2_4
  priv->combo = gtk_combo_box_new_text();
  i = 0;

  if(info->help_string)
    {
    g_signal_connect (priv->combo, "realize",
                      G_CALLBACK (realize_combo), w);
    }
  
  if(info->multi_labels)
    {
    while(info->multi_labels[i])
      {
      gtk_combo_box_append_text(GTK_COMBO_BOX(priv->combo),
                                info->multi_labels[i]);
      i++;
      }
    }
  else
    {
    while(info->multi_names[i])
      {
      gtk_combo_box_append_text(GTK_COMBO_BOX(priv->combo),
                                info->multi_names[i]);
      i++;
      }
    }
  
  w->callback_widget = priv->combo;
  w->callback_id = g_signal_connect(G_OBJECT(w->callback_widget),
                                      "changed", G_CALLBACK(change_callback),
                                      (gpointer)w);
#else
  priv->combo = gtk_combo_new();
  gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(priv->combo)->entry),
                            FALSE);

  if(info->help_string)
    {
    bg_gtk_tooltips_set_tip(w->tooltips,
                            GTK_COMBO(priv->combo)->entry,
                            info->help_string, TRANSLATION_DOMAIN);
    }
  
  i = 0;

  if(info->multi_labels)
    {
    while(info->multi_labels[i])
      {
      c = g_strdup(info->multi_labels[i]);
      priv->strings = g_list_append(priv->strings, c);
      i++;
      }
    }
  else
    {
    while(info->multi_names[i])
      {
      c = g_strdup(info->multi_names[i]);
      priv->strings = g_list_append(priv->strings, c);
      i++;
      }
    }
  gtk_combo_set_popdown_strings(GTK_COMBO(priv->combo), priv->strings);

  w->callback_widget = GTK_COMBO(priv->combo)->entry;
  w->callback_id = g_signal_connect(G_OBJECT(w->callback_widget),
                   "changed", G_CALLBACK(change_callback),
                   (gpointer)w);
#endif
  
  //  GTK_WIDGET_UNSET_FLAGS(priv->combo, GTK_CAN_DEFAULT);

  gtk_widget_show(priv->combo);

  priv->label = gtk_label_new(TR_DOM(info->long_name));
  gtk_misc_set_alignment(GTK_MISC(priv->label), 0.0, 0.5);

  gtk_widget_show(priv->label);
    
  }
