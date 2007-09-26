/*****************************************************************
 
  chapterdialog.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <gtk/gtk.h>

#include <config.h>

#include <cfg_dialog.h>
#include <streaminfo.h>
#include <gui_gtk/chapterdialog.h>
#include <gui_gtk/gtkutils.h>
#include <utils.h>


enum
{
  COLUMN_NAME,
  COLUMN_TIME,
  NUM_COLUMNS
};

typedef struct
  {
  GtkWidget * window;
  GtkWidget * add_button;
  GtkWidget * delete_button;
  GtkWidget * edit_button;
  GtkWidget * list;

  GtkWidget * ok_button;
  GtkWidget * cancel_button;
    
  bg_chapter_list_t * cl;

  int selected;
  int edited;
  
  GtkTooltips * tooltips;
  
  int is_ok;

  guint select_id;
  gavl_time_t duration;
  } bg_gtk_chapter_dialog_t;

static void select_row_callback(GtkTreeSelection * sel,
                                gpointer data)
  {
  int i;
  bg_gtk_chapter_dialog_t * win;
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  GtkTreeIter iter;
  
  win = (bg_gtk_chapter_dialog_t *)data;
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->list));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->list));
  
  win->selected = -1;

  if(!gtk_tree_model_get_iter_first(model, &iter))
    return;
  
  for(i = 0; i < win->cl->num_chapters; i++)
    {
    if(gtk_tree_selection_iter_is_selected(selection, &iter))
      {
      win->selected = i;
      break;
      }
    if(!gtk_tree_model_iter_next(model, &iter))
      break;
    }

  if(win->selected < 0)
    {
    gtk_widget_set_sensitive(win->edit_button, 0);
    gtk_widget_set_sensitive(win->delete_button, 0);
    }
  else
    {
    gtk_widget_set_sensitive(win->edit_button, 1);
    gtk_widget_set_sensitive(win->delete_button, 1);
    }
  }


static void update_list(bg_gtk_chapter_dialog_t * win)
  {
  int i;
  char time_string[GAVL_TIME_STRING_LEN];
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  GtkTreeIter iter;
  
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->list));

  g_signal_handler_block(G_OBJECT(selection), win->select_id);
    
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->list));
  gtk_list_store_clear(GTK_LIST_STORE(model));
  
  if(win->cl)
    {
    for(i = 0; i < win->cl->num_chapters; i++)
      {
      gtk_list_store_append(GTK_LIST_STORE(model), &iter);

      gavl_time_prettyprint(win->cl->chapters[i].time, time_string);
      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                         COLUMN_TIME,
                         time_string,
                         -1);
      
      if(win->cl->chapters[i].name)
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_NAME,
                           win->cl->chapters[i].name,
                           -1);
      if(win->selected == i)
        gtk_tree_selection_select_iter(selection, &iter);
      }
    }
  if((win->selected < 0) || (!win->cl))
    {
    gtk_widget_set_sensitive(win->edit_button, 0);
    gtk_widget_set_sensitive(win->delete_button, 0);
    }
  else
    {
    gtk_widget_set_sensitive(win->edit_button, 1);
    gtk_widget_set_sensitive(win->delete_button, 1);
    }

  g_signal_handler_unblock(G_OBJECT(selection), win->select_id);
  }

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  bg_gtk_chapter_dialog_t * win;
  win = (bg_gtk_chapter_dialog_t*)data;
  
  if(!name)
    {
    win->is_ok = 1;
    return;
    }
  else if(!strcmp(name, "name"))
    win->cl->chapters[win->edited].name =
      bg_strdup(win->cl->chapters[win->edited].name, val->val_str);
  else if(!strcmp(name, "time"))
    win->cl->chapters[win->edited].time = val->val_time;
  }


static int edit_chapter(bg_gtk_chapter_dialog_t * win)
  {
  bg_dialog_t * dialog;
  
  bg_parameter_info_t chapter_parameters[3];
  
  /* Set up parameters */
  memset(chapter_parameters, 0, sizeof(chapter_parameters));
  
  chapter_parameters[0].name      = "name";
  chapter_parameters[0].long_name = TRS("Name");
  chapter_parameters[0].type = BG_PARAMETER_STRING;
  chapter_parameters[0].val_default.val_str =
    win->cl->chapters[win->edited].name;
  
  /* Time can only be changed if this isn't the first chapter */
  if(win->edited)
    {
    chapter_parameters[1].name      = "time";
    chapter_parameters[1].long_name = TRS("Time");
    chapter_parameters[1].type = BG_PARAMETER_TIME;
    chapter_parameters[1].val_default.val_time =
      win->cl->chapters[win->edited].time;

    /* We set the min-max values of the time such that the
       resulting list will always be ordered */
    chapter_parameters[1].val_min.val_time =
      win->cl->chapters[win->edited-1].time + GAVL_TIME_SCALE / 1000;

    if(win->edited == win->cl->num_chapters - 1)
      chapter_parameters[1].val_max.val_time = win->duration -
        GAVL_TIME_SCALE / 1000;
    else
      chapter_parameters[1].val_max.val_time =
        win->cl->chapters[win->edited+1].time - GAVL_TIME_SCALE / 1000;

    if(chapter_parameters[1].val_default.val_time <
       chapter_parameters[1].val_min.val_time)
      chapter_parameters[1].val_default.val_time =
        chapter_parameters[1].val_min.val_time;
    }

  dialog = bg_dialog_create((bg_cfg_section_t*)0,
                            set_parameter,
                            win,
                            chapter_parameters,
                            TR("Edit chapter"));
  if(win->tooltips)
    bg_dialog_set_tooltips(dialog, 1);
  else
    bg_dialog_set_tooltips(dialog, 0);

  bg_dialog_show(dialog);
  bg_dialog_destroy(dialog);
  return 1;
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_chapter_dialog_t * win;
  win = (bg_gtk_chapter_dialog_t*)data;

  if(w == win->ok_button)
    {
    win->is_ok = 1;
    gtk_main_quit();
    gtk_widget_hide(win->window);
    }
  else if((w == win->cancel_button) || (w == win->window))
    {
    gtk_main_quit();
    gtk_widget_hide(win->window);
    win->is_ok = 0;
    }
  else if(w == win->add_button)
    {
    if(!win->cl)
      {
      win->cl = bg_chapter_list_create(0);
      win->selected = 0;
      win->edited = 0;
      }
    else
      win->edited = win->selected + 1;
    
    bg_chapter_list_insert(win->cl, win->edited,
                           0, (const char *)0);
    win->is_ok = 0;
    edit_chapter(win);
    
    if(!win->is_ok)
      bg_chapter_list_delete(win->cl, win->edited);
    else
      {
      win->selected = win->edited;
      update_list(win);
      }
    }
  else if(w == win->delete_button)
    {
    bg_chapter_list_delete(win->cl, win->selected);
    update_list(win);
    }
  else if(w == win->edit_button)
    {
    win->edited = win->selected;
    edit_chapter(win);
    update_list(win);
    }
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * evt, gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

static GtkWidget * create_window_pixmap_button(bg_gtk_chapter_dialog_t * win,
                                               const char * filename,
                                               const char * tooltip)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();

  gtk_widget_show(image);
  button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(button_callback), win);

  
  gtk_widget_show(button);

  if(win->tooltips)
    bg_gtk_tooltips_set_tip(win->tooltips, button, tooltip, PACKAGE);
  
  return button;
  }

static bg_gtk_chapter_dialog_t * create_dialog(bg_chapter_list_t * list,
                                               gavl_time_t duration,
                                               int show_tooltips)
  {
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection * selection;

  GtkWidget * scrolled;
  GtkWidget * table;
  GtkWidget * box;
    
  bg_gtk_chapter_dialog_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cl = bg_chapter_list_copy(list);
  ret->duration = duration;
  
  if(show_tooltips)
    {
    ret->tooltips = gtk_tooltips_new();
    g_object_ref (G_OBJECT (ret->tooltips));
    
#if GTK_MINOR_VERSION < 10
    gtk_object_sink (GTK_OBJECT (ret->tooltips));
#else
    g_object_ref_sink(G_OBJECT(ret->tooltips));
#endif
    }
  
  /* Create objects */
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  
  gtk_window_set_modal(GTK_WINDOW(ret->window), 1);
  gtk_window_set_title(GTK_WINDOW(ret->window), TR("Edit chapters"));
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   ret);
  
  ret->ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
  ret->cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

  g_signal_connect(G_OBJECT(ret->ok_button), "clicked",
                   G_CALLBACK(button_callback), ret);
  g_signal_connect(G_OBJECT(ret->cancel_button), "clicked",
                   G_CALLBACK(button_callback), ret);

  gtk_widget_show(ret->ok_button);
  gtk_widget_show(ret->cancel_button);

  ret->add_button =
    create_window_pixmap_button(ret, "add_16.png", TRS("Add new chapter"));
  ret->edit_button =
    create_window_pixmap_button(ret, "config_16.png", TRS("Edit chapter"));
  ret->delete_button =
    create_window_pixmap_button(ret, "trash_16.png", TRS("Delete chapter"));
  
  
  /* Create treeview */
  store = gtk_list_store_new(NUM_COLUMNS,
                             G_TYPE_STRING,
                             G_TYPE_STRING,
                             G_TYPE_STRING);

  ret->list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->list));
  
  ret->select_id =
    g_signal_connect(G_OBJECT(selection), "changed",
                     G_CALLBACK(select_row_callback), (gpointer)ret);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name",
                                                     renderer,
                                                     "text",
                                                     COLUMN_NAME,
                                                     NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Time",
                                                     renderer,
                                                     "text",
                                                     COLUMN_TIME,
                                                     NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->list), column);

  gtk_widget_show(ret->list);
  
  /* Pack objects */
  table = gtk_table_new(4, 2, 0);

  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  
  scrolled =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(ret->list)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ret->list)));

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(scrolled), ret->list);
  gtk_widget_show(scrolled);
  
  gtk_table_attach_defaults(GTK_TABLE(table), scrolled, 0, 1, 0, 3);

  gtk_table_attach(GTK_TABLE(table), ret->add_button, 1, 2, 0, 1,
                   GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), ret->edit_button, 1, 2, 1, 2,
                   GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), ret->delete_button, 1, 2, 2, 3,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  
  
  box = gtk_hbutton_box_new();
  gtk_box_set_spacing(GTK_BOX(box), 5);
  gtk_container_add(GTK_CONTAINER(box), ret->ok_button);
  gtk_container_add(GTK_CONTAINER(box), ret->cancel_button);
  gtk_widget_show(box);

  gtk_table_attach(GTK_TABLE(table), box, 0, 2, 3, 4,
                   GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(ret->window), table);

  update_list(ret);

  return ret;
  }

static void destroy_dialog(bg_gtk_chapter_dialog_t * dlg)
  {
  gtk_widget_destroy(dlg->window);

  if(dlg->cl)
    bg_chapter_list_destroy(dlg->cl);
  free(dlg);
  }

void bg_gtk_chapter_dialog_show(bg_chapter_list_t ** list,
                                gavl_time_t duration, int show_tooltips)
  {
  bg_gtk_chapter_dialog_t * dlg;
  dlg = create_dialog(*list, duration, show_tooltips);
  gtk_widget_show(dlg->window);
  gtk_main();

  if(dlg->is_ok)
    {
    if(*list) bg_chapter_list_destroy(*list);
    *list = bg_chapter_list_copy(dlg->cl);
    }
    
  destroy_dialog(dlg);
  }

