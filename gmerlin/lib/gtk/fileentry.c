#include <stdlib.h>
#include <gtk/gtk.h>
#include <gui_gtk/fileentry.h>

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

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_file_entry_t * priv = (bg_gtk_file_entry_t*)data;

  if(w == priv->button)
    {
    if(!priv->fileselect)
      {
      priv->fileselect =  gtk_file_selection_new("Select a file");
      gtk_window_set_modal(GTK_WINDOW(priv->fileselect), TRUE);
      g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(priv->fileselect)->ok_button),
                       "clicked", G_CALLBACK(button_callback),
                         (gpointer)priv);
      g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(priv->fileselect)->cancel_button),
                         "clicked", G_CALLBACK(button_callback),
                         (gpointer)priv);
      if(priv->is_dir)
        gtk_widget_set_sensitive(GTK_FILE_SELECTION(priv->fileselect)->file_list, 0);
      }

    gtk_file_selection_set_filename(GTK_FILE_SELECTION(priv->fileselect),
                                    gtk_entry_get_text(GTK_ENTRY(priv->entry)));
    
    
    gtk_widget_show(priv->fileselect);
    gtk_main();
    }
  else if(w == priv->entry)
    {
    priv->name_changed_callback(priv, priv->name_changed_callback_data);
    }
  else if(priv->fileselect)
    {
    if(w == GTK_FILE_SELECTION(priv->fileselect)->ok_button)
      {
      gtk_widget_hide(priv->fileselect);
      gtk_main_quit();
      gtk_entry_set_text(GTK_ENTRY(priv->entry),
                         gtk_file_selection_get_filename(GTK_FILE_SELECTION(priv->fileselect)));
      }
    if(w == GTK_FILE_SELECTION(priv->fileselect)->cancel_button)
      {
      gtk_widget_hide(priv->fileselect);
      gtk_main_quit();
      }
    }
  
  }

bg_gtk_file_entry_t * bg_gtk_file_entry_create(int is_dir,
                                               void (*name_changed_callback)(bg_gtk_file_entry_t *,
                                                                             void * data),
                                               void * name_changed_callback_data)
  {
  bg_gtk_file_entry_t * priv = calloc(1, sizeof(*priv));
  priv->is_dir = is_dir;

  priv->name_changed_callback = name_changed_callback;
  priv->name_changed_callback_data = name_changed_callback_data;
  
  priv->entry = gtk_entry_new();

  if(priv->name_changed_callback)
    g_signal_connect(G_OBJECT(priv->entry), "changed",
                     G_CALLBACK(button_callback),
                     (gpointer)priv);
  
  gtk_widget_show(priv->entry);
  
  priv->button = gtk_button_new_with_label("Browse...");

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

