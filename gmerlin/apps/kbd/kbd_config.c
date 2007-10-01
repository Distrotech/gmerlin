#include <inttypes.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <stdio.h>

#include <config.h>
#include <translation.h>

#include "kbd.h"
#include "kbd_remote.h"
#include <gui_gtk/gtkutils.h>
#include <utils.h>
#include <remote.h>
#include <log.h>
#define LOG_DOMAIN "gmerlin_kbd_config"
#include <subprocess.h>

static int ignore_mask = GDK_MOD2_MASK;

/* Gdk Modifiers */

static struct
  {
  GdkModifierType type;
  char * name;
  }
modifiers[] =
  {
    { GDK_SHIFT_MASK,   TRS("Shift") }, //   = 1 << 0,
    { GDK_LOCK_MASK,    TRS("Lock") }, //	    = 1 << 1,
    { GDK_CONTROL_MASK, TRS("Control") }, //  = 1 << 2,
    { GDK_MOD1_MASK,    TRS("Mod1") }, //	    = 1 << 3,
    { GDK_MOD2_MASK,    TRS("Mod2") }, //	    = 1 << 4,
    { GDK_MOD3_MASK,    TRS("Mod3") }, //	    = 1 << 5,
    { GDK_MOD4_MASK,    TRS("Mod4") }, //	    = 1 << 6,
    { GDK_MOD5_MASK,    TRS("Mod5") }, //	    = 1 << 7,
    { GDK_BUTTON1_MASK, TRS("Button1") }, //  = 1 << 8,
    { GDK_BUTTON2_MASK, TRS("Button2") }, //  = 1 << 9,
    { GDK_BUTTON3_MASK, TRS("Button3") }, //  = 1 << 10,
    { GDK_BUTTON4_MASK, TRS("Button4") }, //  = 1 << 11,
    { GDK_BUTTON5_MASK, TRS("Button5") }, //  = 1 << 12,

  };

static char * get_modifier_string(uint32_t state)
  {
  int i;
  char * ret = (char*)0;

  for(i = 0; i < sizeof(modifiers)/sizeof(modifiers[0]); i++)
    {
    if(state & modifiers[i].type)
      {
      if(ret)
        {
        ret = bg_strcat(ret, "+");
        ret = bg_strcat(ret, dgettext(PACKAGE, modifiers[i].name));
        }
      else
        ret = bg_strdup(ret, dgettext(PACKAGE, modifiers[i].name));
      }
    }
  return ret;
  }

/* Config dialog */

typedef struct
  {
  GtkWidget * window;
  GtkWidget * scancode;
  GtkWidget * modifiers;
  GtkWidget * command;
  GtkWidget * grab_button;
  
  GtkWidget * ok_button;
  GtkWidget * cancel_button;
  int ret;
  int do_grab; /* 1 if we are currently grabbing */
  } edit_dialog_t;

static gboolean dialog_key_callback(GtkWidget * w,
                                GdkEventKey * evt,
                                gpointer data)
  {
  char * tmp_string;
  edit_dialog_t * dlg = (edit_dialog_t *)data;
  if(!dlg->do_grab)
    return FALSE;
  

  /* Check, if we have a modifier */

  switch(evt->keyval)
    {
    case GDK_Shift_L:
    case GDK_Shift_R:
    case GDK_Control_L:
    case GDK_Control_R:
    case GDK_Alt_L:
    case GDK_Alt_R:
    case GDK_Meta_L:
    case GDK_Meta_R:
    case GDK_Super_L:
    case GDK_Super_R:
    case GDK_ISO_Level3_Shift:
      return TRUE;
      break;
    }
  tmp_string = bg_sprintf("%d", evt->hardware_keycode);
  
  gtk_entry_set_text(GTK_ENTRY(dlg->scancode), tmp_string);
  free(tmp_string);
  
  tmp_string = get_modifier_string(evt->state & ~ignore_mask);
  
  if(tmp_string)
    {
    gtk_entry_set_text(GTK_ENTRY(dlg->modifiers), tmp_string);
    free(tmp_string);
    }
  else
    gtk_entry_set_text(GTK_ENTRY(dlg->modifiers), "");
  if(gdk_pointer_is_grabbed())
    gdk_pointer_ungrab(evt->time);

  gdk_keyboard_ungrab(evt->time);
  
  gdk_keyboard_ungrab(GDK_CURRENT_TIME);

  dlg->do_grab = 0;
  
  gtk_main_quit();
  return TRUE;
  }

static void dialog_button_callback(GtkWidget * w, gpointer data)
  {
  GdkCursor * cursor;
  edit_dialog_t * dlg = (edit_dialog_t *)data;

  if(w == dlg->ok_button)
    {
    dlg->ret = 1;
    gtk_main_quit();
    }
  else if((w == dlg->cancel_button) || (w == dlg->window))
    {
    gtk_main_quit();
    }
  else if(w == dlg->grab_button)
    {
    
    if(GDK_GRAB_SUCCESS == gdk_keyboard_grab(dlg->window->window,
                                             FALSE,
                                             GDK_CURRENT_TIME))
      {
      cursor = gdk_cursor_new(GDK_CROSS);
      
      gdk_pointer_grab(dlg->window->window, FALSE, 
                       0, (GdkWindow*)0, cursor,
                       GDK_CURRENT_TIME);
      dlg->do_grab = 1;
      gtk_main();
      gdk_cursor_unref(cursor);
      }

    }

  }

static gboolean dialog_delete_callback(GtkWidget * w, GdkEventAny * evt, gpointer data)
  {
  dialog_button_callback(w, data);
  return TRUE;
  }

static edit_dialog_t * edit_dialog_create(kbd_table_t * kbd)
  {
  char * tmp_string;
  GtkWidget * box, *table;
  GtkWidget * label;

  edit_dialog_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(dialog_delete_callback),
                   ret);


  gtk_window_set_modal(GTK_WINDOW(ret->window), 1);
  gtk_window_set_title(GTK_WINDOW(ret->window),
                       TR("Edit key"));
  
  ret->scancode = gtk_entry_new();
  ret->modifiers = gtk_entry_new();
  ret->command = gtk_entry_new();
  ret->grab_button =
    gtk_button_new_with_label(TR("Grab key"));
  
  ret->ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
  ret->cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

  g_signal_connect(G_OBJECT(ret->window), "key-press-event",
                   G_CALLBACK(dialog_key_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->grab_button), "clicked",
                   G_CALLBACK(dialog_button_callback),
                   ret);
  g_signal_connect(G_OBJECT(ret->ok_button), "clicked",
                   G_CALLBACK(dialog_button_callback),
                   ret);
  g_signal_connect(G_OBJECT(ret->cancel_button), "clicked",
                   G_CALLBACK(dialog_button_callback),
                   ret);

  /* Show stuff */
  gtk_widget_show(ret->scancode);
  gtk_widget_show(ret->modifiers);
  gtk_widget_show(ret->command);
  gtk_widget_show(ret->grab_button);
  gtk_widget_show(ret->ok_button);
  gtk_widget_show(ret->cancel_button);
  
  table = gtk_table_new(5, 2, 0);

  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  
  /* Pack objects */

  label = gtk_label_new(TR("Scancode"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL,
                   0, 0);

  gtk_table_attach_defaults(GTK_TABLE(table),
                            ret->scancode, 1, 2, 0, 1);

  
  label = gtk_label_new(TR("Modifiers"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL,
                   0, 0);
  gtk_table_attach_defaults(GTK_TABLE(table),
                            ret->modifiers, 1, 2, 1, 2);

  
  gtk_table_attach(GTK_TABLE(table), ret->grab_button,
                   0, 2, 2, 3, GTK_FILL, GTK_FILL,
                   0, 0);
  
  label = gtk_label_new(TR("Command"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL,
                   0, 0);
  gtk_table_attach_defaults(GTK_TABLE(table),
                            ret->command, 1, 2, 3, 4);
  
  box = gtk_hbutton_box_new();
  gtk_container_add(GTK_CONTAINER(box), ret->ok_button);
  gtk_container_add(GTK_CONTAINER(box), ret->cancel_button);
  gtk_widget_show(box);
  
  gtk_table_attach(GTK_TABLE(table), box, 0, 2, 4, 5,
                   GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(table);

  gtk_container_add(GTK_CONTAINER(ret->window), table);

  /* Set values */

  if(kbd->scancode)
    {
    tmp_string = bg_sprintf("%d", kbd->scancode);
    gtk_entry_set_text(GTK_ENTRY(ret->scancode), tmp_string);
    free(tmp_string);
    }
  if(kbd->modifiers)
    gtk_entry_set_text(GTK_ENTRY(ret->modifiers), kbd->modifiers);

  if(kbd->command)
    gtk_entry_set_text(GTK_ENTRY(ret->command), kbd->command);
  
  return ret;
  }

static void edit_dialog_destroy(edit_dialog_t * dlg)
  {
  gtk_widget_hide(dlg->window);
  gtk_widget_destroy(dlg->window);
  free(dlg);
  }

static int edit_key(kbd_table_t * kbd)
  {
  int ret;
  edit_dialog_t * dlg;
  dlg = edit_dialog_create(kbd);
  gtk_widget_show(dlg->window);
  gtk_main();
  ret = dlg->ret;

  if(ret)
    {
    kbd->scancode = atoi(gtk_entry_get_text(GTK_ENTRY(dlg->scancode)));
    kbd->modifiers = bg_strdup(kbd->modifiers,
                               gtk_entry_get_text(GTK_ENTRY(dlg->modifiers)));
    kbd->command = bg_strdup(kbd->command,
                             gtk_entry_get_text(GTK_ENTRY(dlg->command)));
    }

  edit_dialog_destroy(dlg);
  return ret;
  }

enum
{
  COLUMN_SCANCODE,
  COLUMN_MODIFIERS,
  COLUMN_COMMAND,
  NUM_COLUMNS
};

typedef struct
  {
  GtkWidget * window;
  GtkWidget * list;
  GtkWidget * add_button;
  GtkWidget * delete_button;
  GtkWidget * edit_button;
  GtkWidget * kbd_button;
  
  GtkWidget * apply_button;
  GtkWidget * close_button;

  bg_remote_client_t * remote;
  int selected;

  kbd_table_t * keys;
  int num_keys;
  int keys_alloc;

  int daemon_running;
  guint kbd_id;
  } window_t;

static void update_list(window_t * win)
  {
  int i;
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  GtkTreeIter iter;
  char * tmp_string;
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->list));
  gtk_list_store_clear(GTK_LIST_STORE(model));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->list));
    
  for(i = 0; i < win->num_keys; i++)
    {
    tmp_string = bg_sprintf("%d", win->keys[i].scancode);

    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       COLUMN_SCANCODE,
                       tmp_string,
                       -1);
    if(win->keys[i].modifiers)
      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                         COLUMN_MODIFIERS,
                         win->keys[i].modifiers,
                         -1);
    if(win->keys[i].command)
      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                         COLUMN_COMMAND,
                         win->keys[i].command,
                         -1);
    if(win->selected == i)
      gtk_tree_selection_select_iter(selection, &iter);
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

static void select_row_callback(GtkTreeSelection * sel,
                                gpointer data)
  {
  int i;
  window_t * win;
  GtkTreeModel * model;
  GtkTreeSelection * selection;
  GtkTreeIter iter;
  
  win = (window_t *)data;
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->list));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->list));
  
  win->selected = -1;

  if(!gtk_tree_model_get_iter_first(model, &iter))
    return;
  
  for(i = 0; i < win->num_keys; i++)
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



static void window_button_callback(GtkWidget * w, gpointer data)
  {
  gavl_time_t delay_time = GAVL_TIME_SCALE/200;

  int i;
  bg_msg_t * msg;
  char * tmp_string;
  bg_subprocess_t * proc;
  window_t * win;
  char * daemon_path;
  
  win = (window_t *)data;

  if(w == win->apply_button)
    {
    tmp_string = bg_search_file_write("kbd", "keys.xml");
    kbd_table_save(tmp_string, win->keys, win->num_keys);
    free(tmp_string);

    if(win->daemon_running)
      {
      msg = bg_remote_client_get_msg_write(win->remote);
      bg_msg_set_id(msg, KBD_CMD_RELOAD); 
      bg_remote_client_done_msg_write(win->remote);
      }
    }
  else if(w == win->add_button)
    {
    if(win->keys_alloc < win->num_keys + 1)
      {
      win->keys_alloc++;
      win->keys = realloc(win->keys, win->keys_alloc * sizeof(*win->keys));
      memset(&win->keys[win->num_keys], 0, sizeof(win->keys[win->num_keys]));
      }
    win->num_keys++;
    if(!edit_key(&win->keys[win->num_keys-1]))
      win->num_keys--;
    else
      {
      /* Update list */
      update_list(win);
      }
    }
  else if((w == win->close_button) || (w == win->window))
    {
    gtk_main_quit();
    }
  else if(w == win->edit_button)
    {
    if(edit_key(&win->keys[win->selected]))
      update_list(win);
    }
  else if(w == win->delete_button)
    {
    kbd_table_entry_free(&win->keys[win->selected]);
    
    if(win->selected < win->num_keys - 1)
      {
      memmove(&win->keys[win->selected], &win->keys[win->selected+1],
              sizeof(win->keys[win->selected])*
              (win->num_keys - 1 - win->selected));
      }
    win->num_keys--;
    update_list(win);
    }
  else if(w == win->kbd_button)
    {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->kbd_button)))
      {
      /* Start daemon */
      daemon_path = (char*)0;
      if(!bg_search_file_exec("gmerlin_kbd", &daemon_path))
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Keyboard daemon not found");
        return;
        }
      proc = bg_subprocess_create(daemon_path, 0, 0, 0);
      bg_subprocess_close(proc);
      free(daemon_path);
      
      for(i = 0; i < 10; i++)
        {
        gavl_time_delay(&delay_time);
        if(bg_remote_client_init(win->remote,
                                 "localhost", KBD_REMOTE_PORT, 10000))
          break;
        }
      
      if(i == 10)
        {
        g_signal_handler_block(G_OBJECT(win->kbd_button), win->kbd_id);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->kbd_button),
                                     0);
        g_signal_handler_unblock(G_OBJECT(win->kbd_button), win->kbd_id);
        win->daemon_running = 0;
        }
      else
        win->daemon_running = 1;
      }
    else
      {
      /* Stop daemon */
      msg = bg_remote_client_get_msg_write(win->remote);
      bg_msg_set_id(msg, KBD_CMD_QUIT); 
      bg_remote_client_done_msg_write(win->remote);
      bg_remote_client_close(win->remote);
      }
    }

  }

static gboolean window_delete_callback(GtkWidget * w, GdkEventAny * evt, gpointer data)
  {
  window_button_callback(w, data);
  return TRUE;
  }

static GtkWidget * create_window_pixmap_button(window_t * win,
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
                   G_CALLBACK(window_button_callback), win);

  
  gtk_widget_show(button);

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);
  
  return button;
  }

static window_t * create_window()
  {
  GtkWidget * box, *table, *scrolled;
  char * filename;
  window_t * ret;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection * selection;


  ret = calloc(1, sizeof(*ret));

  filename = bg_search_file_read("kbd", "keys.xml");
  /* Load keyboad mapping */
  ret->keys = kbd_table_load(filename, &ret->num_keys);
  free(filename);
  ret->keys_alloc = ret->num_keys;
    
  /* Tooltips */
  
  /* Create Buttons */
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(window_delete_callback),
                   ret);
  
  gtk_window_set_title(GTK_WINDOW(ret->window), TR("Keyboard daemon configuration"));

  
  ret->apply_button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
  
  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);

  ret->kbd_button = gtk_check_button_new_with_label(TR("Daemon running"));

  bg_gtk_tooltips_set_tip(ret->kbd_button,
                          TRS("Switch deamon on and off. If you want to start the daemon with each start of X, add \"gmerlin_kbd\" to your startup programs"),
                          PACKAGE);
  
  ret->add_button =
    create_window_pixmap_button(ret, "add_16.png", TRS("Add new key"));
  ret->edit_button =
    create_window_pixmap_button(ret, "config_16.png", TRS("Edit key"));
  ret->delete_button =
    create_window_pixmap_button(ret, "trash_16.png", TRS("Delete key"));
  
  /* Create list */
  ret->selected = -1;

  store = gtk_list_store_new(NUM_COLUMNS,
                             G_TYPE_STRING,
                             G_TYPE_STRING,
                             G_TYPE_STRING);

  ret->list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->list));
  
  g_signal_connect(G_OBJECT(selection), "changed",
                   G_CALLBACK(select_row_callback), (gpointer)ret);

  
  /* Add columns */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (TR("Scancode"),
                                                     renderer,
                                                     "text",
                                                     COLUMN_SCANCODE,
                                                     NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (TR("Modifiers"),
                                                     renderer,
                                                     "text",
                                                     COLUMN_MODIFIERS,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (TR("Command"),
                                                     renderer,
                                                     "text",
                                                     COLUMN_COMMAND,
                                                     NULL);

  
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->list), column);

  gtk_widget_show(ret->list);
    
  /* Set callbacks */
  
  g_signal_connect(G_OBJECT(ret->apply_button), "clicked",
                   G_CALLBACK(window_button_callback), ret);
  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(window_button_callback), ret);
  
  ret->kbd_id = g_signal_connect(G_OBJECT(ret->kbd_button), "toggled",
                                 G_CALLBACK(window_button_callback), ret);
  
  /* Show objects */
  gtk_widget_show(ret->apply_button);
  gtk_widget_show(ret->close_button);
  gtk_widget_show(ret->kbd_button);
  
  /* Pack objects */
  scrolled =
    gtk_scrolled_window_new(gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(ret->list)),
                            gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ret->list)));

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(scrolled), ret->list);
  gtk_widget_show(scrolled);
  
  
  table = gtk_table_new(6, 2, 0);

  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  
  gtk_table_attach(GTK_TABLE(table), ret->add_button, 1, 2, 0, 1,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table), ret->edit_button, 1, 2, 1, 2,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table), ret->delete_button, 1, 2, 2, 3,
                   GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach_defaults(GTK_TABLE(table), scrolled, 0, 1, 0, 3);

  gtk_table_attach(GTK_TABLE(table), ret->kbd_button,
                   0, 2, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
  
  
  box = gtk_hbutton_box_new();
  gtk_container_add(GTK_CONTAINER(box), ret->apply_button);
  gtk_container_add(GTK_CONTAINER(box), ret->close_button);
  gtk_widget_show(box);
  
  gtk_table_attach(GTK_TABLE(table), box, 0, 2, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(table);

  gtk_container_add(GTK_CONTAINER(ret->window), table);

  update_list(ret);
  

  gtk_widget_show(ret->window);

  ret->remote = bg_remote_client_create(KBD_REMOTE_ID, 0);

  if(bg_remote_client_init(ret->remote,
                           "localhost", KBD_REMOTE_PORT, 10000))
    {
    g_signal_handler_block(G_OBJECT(ret->kbd_button), ret->kbd_id);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->kbd_button),
                               1);
    g_signal_handler_unblock(G_OBJECT(ret->kbd_button), ret->kbd_id);
    ret->daemon_running = 1;
    }
  
  return ret;
  }

static void destroy_window(window_t * win)
  {
  gtk_widget_destroy(win->window);
  bg_remote_client_destroy(win->remote);
  }

int main(int argc, char ** argv)
  {
  window_t * win;
  
  bg_gtk_init(&argc, &argv, "kbd_icon.png");
  
  win = create_window();
  gtk_main();
  destroy_window(win);
  return 0;
  }
