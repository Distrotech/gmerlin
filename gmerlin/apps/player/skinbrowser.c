/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <config.h>
#include <gmerlin/utils.h>

#include "gmerlin.h"

#include <gui_gtk/fileselect.h>
#include <gui_gtk/message.h>
#include <gui_gtk/gtkutils.h>


enum
  {
    COLUMN_NAME,
    NUM_COLUMNS
  };

typedef struct skin_info_s
  {
  char * name;
  char * directory;
  struct skin_info_s * next;
  } skin_info_t;

static skin_info_t * scan_directory(const char * directory)
  {
  skin_info_t * ret = (skin_info_t*)0;
  skin_info_t * ret_end = (skin_info_t*)0;
  skin_info_t * new_info;
  const char * start;
    
  DIR * dir;
  struct dirent * entry;
  char dir_filename[FILENAME_MAX];
  char skin_filename[FILENAME_MAX];
  struct stat st;

  dir = opendir(directory);

  if(!dir)
    return (skin_info_t*)0;

  while((entry = readdir(dir)))
    {
    if(entry->d_name[0] == '.') /* Skip ".", ".." and hidden files */
      continue;

    sprintf(dir_filename, "%s/%s", directory, entry->d_name);

    /* Check if we can do a stat */
    if(stat(dir_filename, &st))
      continue;

    /* Check if this is a directory */
    if(!S_ISDIR(st.st_mode))
      continue;

    /* Check if a file skin.xml is inside, this also
       checks if the directory is readable for us */

    sprintf(skin_filename, "%s/skin.xml", dir_filename);
    if(stat(skin_filename, &st))
      continue;

    /* Now, create the skin info */
    new_info = calloc(1, sizeof(*new_info));
    new_info->directory = bg_strdup(new_info->directory, dir_filename);

    start = strrchr(new_info->directory, '/');
    if(!start)
      continue;
    start++;

    new_info->name = bg_strdup(new_info->name, start);
    if(!ret)
      {
      ret = new_info;
      ret_end = ret;
      }
    else
      {
      ret_end->next = new_info;
      ret_end = ret_end->next;
      }
    }
  closedir(dir);  
  return ret;
  }

static skin_info_t * skin_info_create()
  {
  char * home_dir;
  char * directory;
  
  skin_info_t * system_skins;
  skin_info_t * home_skins = (skin_info_t*)0;

  skin_info_t * ret_end;

  directory = bg_sprintf("%s/%s", GMERLIN_DATA_DIR, "skins");

  system_skins = scan_directory(directory);
  free(directory);
  
  home_dir = getenv("HOME");
  if(home_dir)
    {
    directory = bg_sprintf("%s/%s", home_dir, ".gmerlin/skins");
    home_skins = scan_directory(directory);
    free(directory);
    }

  if(system_skins && home_skins)
    {
    ret_end = system_skins;
    while(ret_end->next)
      ret_end = ret_end->next;
    ret_end->next = home_skins;
    return system_skins;
    }
  else if(system_skins)
    return system_skins;
  else if(home_skins)
    return system_skins;
  else
    return (skin_info_t*)0;
  }

static void skin_info_destroy(skin_info_t * info)
  {
  skin_info_t * tmp_info;

  while(info)
    {
    tmp_info = info->next;
    free(info->name);
    free(info->directory);
    free(info);
    info = tmp_info;
    }
  }

struct gmerlin_skin_browser_s
  {
  GtkWidget * window;
  GtkWidget * treeview;

  GtkWidget * new_button;
  GtkWidget * close_button;
  
  gmerlin_t * g;

  guint select_handler_id;

  skin_info_t * skins;
  };

static void update_list(gmerlin_skin_browser_t * b)
  {
  skin_info_t * skin;
  GtkTreeSelection * selection;
  GtkTreeModel * model;
  GtkTreeIter iter;

  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(b->treeview));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(b->treeview));

  g_signal_handler_block(G_OBJECT(selection), b->select_handler_id);

  gtk_list_store_clear(GTK_LIST_STORE(model));

  skin = b->skins;

  while(skin)
    {
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model),
                       &iter,
                       COLUMN_NAME,
                       skin->name, -1);

    if(!strcmp(skin->directory, b->g->skin_dir))
      {
      gtk_tree_selection_select_iter(selection, &iter);
      }
    skin = skin->next;
    }

  while(gtk_events_pending())
    gtk_main_iteration();
  
  g_signal_handler_unblock(G_OBJECT(selection), b->select_handler_id);
  }

static void select_row_callback(GtkTreeSelection * sel,
                                gpointer data)
  {
  gmerlin_skin_browser_t * b;
  skin_info_t * skin;
  GtkTreeIter iter;
  GtkTreeSelection * selection;
  GtkTreeModel * model;
  
  b = (gmerlin_skin_browser_t*)data;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(b->treeview));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(b->treeview));
  
  if(!gtk_tree_model_get_iter_first(model, &iter))
    return;
  skin = b->skins;

  /* Find the selected skin */

  while(skin)
    {
    if(gtk_tree_selection_iter_is_selected(selection, &iter))
      break;

    skin = skin->next;

    if(skin)
      gtk_tree_model_iter_next(model, &iter);
    }

  if(!skin)
    return;

  /* Found the skin */

  if(!strcmp(skin->directory, b->g->skin_dir))
    return;

  b->g->skin_dir = bg_strdup(b->g->skin_dir, skin->directory);
  gmerlin_skin_load(&(b->g->skin), b->g->skin_dir);
  gmerlin_skin_set(b->g);
 
  }

static void filesel_close_callback(bg_gtk_filesel_t * f , void * data)
  {
  gmerlin_skin_browser_t * b;
  b = (gmerlin_skin_browser_t*)data;
  gtk_widget_set_sensitive(b->new_button, 1);
  }

static int install_skin(const char * filename, GtkWidget * parent)
  {
  int ret = 0;
  char * command;
  char * home_dir;
  char * error_msg;
  struct stat st;
  char * test_dir;
    
  home_dir = getenv("HOME");

  /* Create skin directory if it doesn't exist */

  test_dir = bg_sprintf("%s/.gmerlin", home_dir);
  if(stat(test_dir, &st))
    {
    if(mkdir(test_dir, S_IRUSR|S_IWUSR|S_IXUSR) == -1)
      {
      free(test_dir);
      return 0;
      }
    }
  free(test_dir);


  test_dir = bg_sprintf("%s/.gmerlin/skins", home_dir);
  if(stat(test_dir, &st))
    {
    if(mkdir(test_dir, S_IRUSR|S_IWUSR|S_IXUSR) == -1)
      {
      free(test_dir);
      return 0;
      }
    }
  free(test_dir);
    
  command = bg_sprintf("tar -C %s/.gmerlin/skins -xvzf %s", home_dir, filename);
  if(system(command))
    {
    error_msg = bg_sprintf(TR("Installing skin from\n%s\nfailed"), filename);
    bg_gtk_message(error_msg, BG_GTK_MESSAGE_ERROR, parent);
    free(error_msg);
    }
  else
    ret = 1;
  free(command);
  return ret;
  }

static void add_file_callback(char ** files, const char * plugin,
                              void * data)
  {
  int rescan = 0;
  int i;
  gmerlin_skin_browser_t * b;
  b = (gmerlin_skin_browser_t*)data;


  i = 0;
  while(files[i])
    {
    if(install_skin(files[i], b->window))
      rescan = 1;
    i++;
    }
  if(rescan)
    {
    skin_info_destroy(b->skins);
    b->skins = skin_info_create();
    update_list(b);
    }
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_filesel_t * filesel;
    
  gmerlin_skin_browser_t * b;
  b = (gmerlin_skin_browser_t*)data;
  
  if((w == b->window) ||
     (w == b->close_button))
    {
    gtk_widget_hide(b->window);
    }
  else if(w == b->new_button)
    {

    filesel = bg_gtk_filesel_create("Install skin",
                                    add_file_callback,
                                    filesel_close_callback,
                                    b, b->window /* parent */,
                                    (bg_plugin_registry_t*)0, 0, 0);

    gtk_widget_set_sensitive(b->new_button, 0);
    bg_gtk_filesel_run(filesel, 0);     
    }
  
  }

static int delete_callback(GtkWidget * w, GdkEvent * event, gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

gmerlin_skin_browser_t * gmerlin_skin_browser_create(gmerlin_t * g)
  {
  GtkWidget * mainbox;
  GtkWidget * buttonbox;
    
  GtkWidget * scrolledwin;
  GtkTreeViewColumn * col;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeSelection * selection;

  gmerlin_skin_browser_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->g = g;

  ret->skins = skin_info_create();
    
  /* Create window */
  
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   (gpointer)ret);
  gtk_window_set_title(GTK_WINDOW(ret->window), TR("Skin browser"));
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  
  /* Create skin list */

  store = gtk_list_store_new(NUM_COLUMNS,
                             G_TYPE_STRING);
  ret->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  gtk_widget_set_size_request(ret->treeview, 200, 200);
  
  selection =
    gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->treeview));

  ret->select_handler_id =
    g_signal_connect(G_OBJECT(selection), "changed",
                     G_CALLBACK(select_row_callback), (gpointer)ret);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title(col, "Installed skins");
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
  gtk_tree_view_column_add_attribute(col,
                                     renderer,
                                     "text", COLUMN_NAME);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview), col);

  gtk_widget_show(ret->treeview);
  scrolledwin =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(ret->treeview)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ret->treeview)));

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(scrolledwin), ret->treeview);
  gtk_widget_show(scrolledwin);

  /* Create buttons */

  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  ret->new_button   = gtk_button_new_from_stock(GTK_STOCK_NEW);

  g_signal_connect(G_OBJECT(ret->close_button),
                   "clicked", G_CALLBACK(button_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->new_button),
                   "clicked", G_CALLBACK(button_callback), (gpointer)ret);

  gtk_widget_show(ret->close_button);
  gtk_widget_show(ret->new_button);
  
  /* Pack the objects */

  mainbox = gtk_vbox_new(0, 5);
  gtk_container_set_border_width(GTK_CONTAINER(mainbox), 5);

  bg_gtk_box_pack_start_defaults(GTK_BOX(mainbox), scrolledwin);

  buttonbox = gtk_hbutton_box_new();
  gtk_box_set_spacing(GTK_BOX(buttonbox), 5);
  
  gtk_container_add(GTK_CONTAINER(buttonbox), 
                    ret->close_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), 
                    ret->new_button);

  gtk_widget_show(buttonbox);
  
  gtk_box_pack_start(GTK_BOX(mainbox), buttonbox, FALSE, FALSE, 0);
  
  gtk_widget_show(mainbox);
    
  gtk_container_add(GTK_CONTAINER(ret->window), mainbox);

  update_list(ret);
  
  return ret;
  }

void gmerlin_skin_browser_destroy(gmerlin_skin_browser_t * b)
  {
  
  g_object_unref(b->window);
  free(b);
  }

void gmerlin_skin_browser_show(gmerlin_skin_browser_t * b)
  {
  gtk_widget_show(b->window);
  }
