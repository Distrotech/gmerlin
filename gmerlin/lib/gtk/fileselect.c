/*****************************************************************
 
  fileselect.c
 
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gui_gtk/fileselect.h>
#include <gui_gtk/question.h>

#include <utils.h>

struct bg_gtk_filesel_s
  {
  GtkWidget * filesel;
  GtkWidget * plugin_menu;

  void (*add_files)(char ** files, const char * plugin,
                   void * data);

  void (*close_notify)(bg_gtk_filesel_t * f, void * data);
  
  void * callback_data;

  char * cwd;
  int is_modal;
  };

static void add_selected(bg_gtk_filesel_t * f)
  {
  char ** filenames;
  int i;
  
  filenames =
    gtk_file_selection_get_selections(GTK_FILE_SELECTION(f->filesel));
  i = 0;
  
  f->add_files(filenames, (const char *)NULL,
               f->callback_data);
  g_strfreev(filenames);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_filesel_t * f;
  GtkFileSelection * filesel;

  f = (bg_gtk_filesel_t *)data;
  filesel = GTK_FILE_SELECTION(f->filesel);

  if((w == f->filesel) || (w == filesel->cancel_button))
    {
    gtk_widget_hide(f->filesel);
    bg_gtk_filesel_destroy(f);
    if(f->is_modal)
      gtk_main_quit();
    }
  else if(w ==  filesel->ok_button)
    {
    add_selected(f);
    }
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

static gboolean destroy_callback(GtkWidget * w, GdkEvent * event,
                                  gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

bg_gtk_filesel_t *
bg_gtk_filesel_create(const char * title,
                      void (*add_files)(char ** files, const char * plugin,
                                       void * data),
                      void (*close_notify)(bg_gtk_filesel_t *,
                                           void * data),
                      char ** plugins,
                      void * user_data,
                      GtkWidget * parent_window)
  {
  bg_gtk_filesel_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->filesel = gtk_file_selection_new(title);

  if(parent_window)
    {
    gtk_window_set_transient_for(GTK_WINDOW(ret->filesel),
                                 GTK_WINDOW(parent_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(ret->filesel), TRUE);
    g_signal_connect(G_OBJECT(ret->filesel), "destroy-event",
                     G_CALLBACK(destroy_callback), ret);
    }
  
  gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(ret->filesel),
                                         TRUE);

  /* Set callbacks */

  g_signal_connect(G_OBJECT(ret->filesel), "delete-event",
                   G_CALLBACK(delete_callback), ret);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(ret->filesel)->ok_button),
                   "clicked", G_CALLBACK(button_callback), ret);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(ret->filesel)->cancel_button),
                   "clicked", G_CALLBACK(button_callback), ret);

  /* Re-label buttons */

  gtk_button_set_label(GTK_BUTTON(GTK_FILE_SELECTION(ret->filesel)->ok_button),
                       "Add");
  gtk_button_set_label(GTK_BUTTON(GTK_FILE_SELECTION(ret->filesel)->cancel_button),
                       "Close");
    
  ret->add_files = add_files;
  ret->close_notify = close_notify;
  

  ret->callback_data = user_data;

  return ret;
  }

/* Destroy fileselector */

void bg_gtk_filesel_destroy(bg_gtk_filesel_t * filesel)
  {
  if(filesel->cwd)
    free(filesel->cwd);
  //  g_object_unref(G_OBJECT(filesel));
  free(filesel);
  }

/* Show the window */

void bg_gtk_filesel_run(bg_gtk_filesel_t * filesel, int modal)
  {
  gtk_window_set_modal(GTK_WINDOW(filesel->filesel), modal);
  gtk_widget_show(filesel->filesel);
  filesel->is_modal = modal;
  if(modal)
    gtk_main();
  
  }

/* Get the current working directory */

void bg_gtk_filesel_set_directory(bg_gtk_filesel_t * filesel,
                                  const char * dir)
  {
  char * pos;
  char * tmp_string;

  if(!dir)
    return;

  tmp_string = bg_utf8_to_system(dir, -1);
  pos = strrchr(tmp_string, '/');
  if(!pos)
    {
    free(tmp_string);
    return;
    }
  pos++;
  *pos = '\0';
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel->filesel),
                                  tmp_string);
  free(tmp_string);
  }


const char * bg_gtk_filesel_get_directory(bg_gtk_filesel_t * filesel)
  {
  char * pos;
  if(filesel->cwd)
    free(filesel->cwd);
  filesel->cwd =
    bg_system_to_utf8(gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel->filesel)), -1);

  pos = strrchr(filesel->cwd, '/');
  if(!pos)
    {
    free(filesel->cwd);
    filesel->cwd = (char*)0;
    return (char*)0;
    }
  pos++;
  *pos = '\0';
  return filesel->cwd;
  }

/*
 *  Create a temporary fileselector and ask
 *  for a file to save something
 *
 *  Return value should be freed with free();
 */

typedef struct
  {
  GtkWidget * w;
  GtkWidget * ok_button;
  GtkWidget * cancel_button;
  int answer;
  } filesel_write_struct;

static void write_button_callback(GtkWidget * w, gpointer data)
  {
  filesel_write_struct * s = (filesel_write_struct*)data;
  if(w == s->ok_button)
    s->answer = 1;
  else
    s->answer = 0;

  gtk_widget_hide(s->w);
  gtk_main_quit();
  }

static gboolean write_delete_callback(GtkWidget * w,
                                      GdkEventAny * evt,
                                      gpointer data)
  {
  write_button_callback(w, data);
  return TRUE;
  }

char * bg_gtk_get_filename_write(const char * title,
                                 char ** directory,
                                 int ask_overwrite)
  {
  char * ret;
  char * last_slash;
  struct stat stat_buf;
  char * question;
  filesel_write_struct f;

  ret = (char*)0;

  f.w = gtk_file_selection_new(title);

  f.ok_button = GTK_FILE_SELECTION(f.w)->ok_button;
  f.cancel_button = GTK_FILE_SELECTION(f.w)->cancel_button;

  /* Set attributes */
  
  gtk_window_set_modal(GTK_WINDOW(f.w), 1);
  f.answer = 0;

  /* Set callbacks */

  g_signal_connect(G_OBJECT(f.w), "delete_event",
                   G_CALLBACK(write_delete_callback),
                   (gpointer)(&f));

  g_signal_connect(G_OBJECT(f.ok_button), "clicked",
                   G_CALLBACK(write_button_callback),
                   (gpointer)(&f));

  g_signal_connect(G_OBJECT(f.cancel_button), "clicked",
                   G_CALLBACK(write_button_callback),
                   (gpointer)(&f));

  /* Set the current directory */

  if(directory && *directory)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(f.w),
                                    *directory);
    
  /* Run the widget */
  
  gtk_widget_show(f.w);
  gtk_main();

  /* Fetch the answer */

  if(!f.answer)
    return (char*)0;

  ret = bg_strdup((char*)0,
                  gtk_file_selection_get_filename(GTK_FILE_SELECTION(f.w)));
  gtk_widget_destroy(f.w);

  /* Update current directory */
    
  if(directory)
    {
    last_slash = strrchr(ret, '/');
    if(last_slash)
      {
      last_slash++;
      *directory = bg_strndup(*directory, ret, last_slash);
      }
    }
  
  if(ret[strlen(ret)-1] == '/') /* No file was selected, return NULL */
    {
    free(ret);
    ret = (char*)0;
    }
  /* Ask for overwriting */

  else if(!stat(ret, &stat_buf))
    {
    question = bg_sprintf("File\n%s\nexists, overwrite?", ret);
    if(!bg_gtk_question(question))
      {
      free(ret);
      ret = (char*)0;
      }
    free(question);
    }
  return ret;
  }

