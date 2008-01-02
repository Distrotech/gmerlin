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

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gui_gtk/fileentry.h>
#include <gui_gtk/gtkutils.h>
#include <utils.h>

struct bg_gtk_file_entry_s
  {
  GtkWidget * entry;
  GtkWidget * button;
  int is_dir;
  GtkWidget * fileselect;

  void (*name_changed_callback)(bg_gtk_file_entry_t *,
                                void * data);
  void * name_changed_callback_data;
  };

static void
filesel_callback(GtkWidget *chooser,
                 gint       response_id,
                 gpointer data)
  {
  char * tmp_string;
  bg_gtk_file_entry_t * priv = (bg_gtk_file_entry_t*)data;
  
  if(response_id == GTK_RESPONSE_OK)
    {
    if(priv->is_dir)
      tmp_string = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(priv->fileselect));
    else
      tmp_string = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(priv->fileselect));
    
    gtk_entry_set_text(GTK_ENTRY(priv->entry), tmp_string);
    g_free(tmp_string);
    }
  gtk_widget_hide(priv->fileselect);
  gtk_main_quit();
  }
                             
static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  filesel_callback(w, GTK_RESPONSE_CANCEL, data);
  return TRUE;
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_file_entry_t * priv = (bg_gtk_file_entry_t*)data;
  GtkWidget * toplevel;
  
  if(w == priv->button)
    {
    if(!priv->fileselect)
      {
      toplevel = bg_gtk_get_toplevel(w);
      
      if(priv->is_dir)
        {
        priv->fileselect = 
          gtk_file_chooser_dialog_new(TR("Select a directory"),
                                      GTK_WINDOW(toplevel),
                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                      GTK_STOCK_CANCEL,
                                      GTK_RESPONSE_CANCEL,
                                      GTK_STOCK_OK,
                                      GTK_RESPONSE_OK,
                                      NULL);
        }
      else
        {
        priv->fileselect = 
          gtk_file_chooser_dialog_new(TR("Select a file"),
                                      GTK_WINDOW(toplevel),
                                      GTK_FILE_CHOOSER_ACTION_SAVE,
                                      GTK_STOCK_CANCEL,
                                      GTK_RESPONSE_CANCEL,
                                      GTK_STOCK_OK,
                                      GTK_RESPONSE_OK,
                                      NULL);
        }

      gtk_window_set_modal(GTK_WINDOW(priv->fileselect), TRUE);

      g_signal_connect(priv->fileselect, "response",
                       G_CALLBACK(filesel_callback),
                       (gpointer)priv);
      
      g_signal_connect(G_OBJECT(priv->fileselect),
                       "delete_event", G_CALLBACK(delete_callback),
                       (gpointer)priv);
      }
    
    if(priv->is_dir)
      gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(priv->fileselect),
                                            gtk_entry_get_text(GTK_ENTRY(priv->entry)));
    else
      gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(priv->fileselect),
                                      gtk_entry_get_text(GTK_ENTRY(priv->entry)));
    
    gtk_widget_show(priv->fileselect);
    gtk_main();
    }
  else if(w == priv->entry)
    {
    priv->name_changed_callback(priv, priv->name_changed_callback_data);
    }
  }

bg_gtk_file_entry_t * bg_gtk_file_entry_create(int is_dir,
                                               void (*name_changed_callback)(bg_gtk_file_entry_t *,
                                                                             void * data),
                                               void * name_changed_callback_data,
                                               const char * help_string,
                                               const char * translation_domain)
  {
  bg_gtk_file_entry_t * priv = calloc(1, sizeof(*priv));
  priv->is_dir = is_dir;

  priv->name_changed_callback = name_changed_callback;
  priv->name_changed_callback_data = name_changed_callback_data;
  
  priv->entry = gtk_entry_new();

  if(help_string)
    {
    bg_gtk_tooltips_set_tip(priv->entry, help_string, translation_domain);
    }
  
  if(priv->name_changed_callback)
    g_signal_connect(G_OBJECT(priv->entry), "changed",
                     G_CALLBACK(button_callback),
                     (gpointer)priv);
  
  gtk_widget_show(priv->entry);
  
  priv->button = gtk_button_new_with_label(TR("Browse..."));
  
  g_signal_connect(G_OBJECT(priv->button),
                   "clicked", G_CALLBACK(button_callback),
                   (gpointer)priv);
  
  gtk_widget_show(priv->button);
  return priv;
  }

void bg_gtk_file_entry_destroy(bg_gtk_file_entry_t * f)
  {
  if(f->fileselect)
    gtk_widget_destroy(f->fileselect);
  free(f);
  }

const char * bg_gtk_file_entry_get_filename(bg_gtk_file_entry_t * f)
  {
  return gtk_entry_get_text(GTK_ENTRY(f->entry));
  }

void bg_gtk_file_entry_set_filename(bg_gtk_file_entry_t * f, const char * s)
  {
  if(!s || (*s == '\0'))
    {
    gtk_entry_set_text(GTK_ENTRY(f->entry), "");
    return;
    }
  gtk_entry_set_text(GTK_ENTRY(f->entry), s);
  }

GtkWidget * bg_gtk_file_entry_get_entry(bg_gtk_file_entry_t * f)
  {
  return f->entry;
  }

GtkWidget * bg_gtk_file_entry_get_button(bg_gtk_file_entry_t *f)
  {
  return f->button;
  }

