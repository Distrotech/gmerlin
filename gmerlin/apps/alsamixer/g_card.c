#include <utils.h>
#include "gui.h"

#include <gui_gtk/gtkutils.h>

#define LOWER_ROWS 3

#define TIMEOUT_INTERVAL 50

struct card_widget_s
  {
  char * label;

  int num_controls;
  int num_upper_controls;
  int num_lower_controls;
  control_widget_t ** controls;
  control_widget_t ** lower_controls;
  control_widget_t ** upper_controls;

  GtkWidget * upper_table;
    
  GtkWidget * w;
  snd_hctl_t *hctl;
  bg_cfg_section_t * section;

  GList * control_windows;
  };

static gboolean  timeout_func(gpointer data)
  {
  card_widget_t * w;
  w = (card_widget_t *)data;

  snd_hctl_nonblock(w->hctl, 1);
  if(snd_hctl_wait(w->hctl, 1) >= 0)
    {
    snd_hctl_handle_events(w->hctl);
    }
  return TRUE;
  }


static void sort_controls(control_widget_t ** controls, int num_controls)
  {
  int i, j;
  int keep_going;
  control_widget_t * tmp_widget;
  
  /* If the first index is -1, just give indices */

  if(control_widget_get_index(controls[0]) == -1)
    {
    for(i = 0; i < num_controls; i++)
      control_widget_set_index(controls[i], i);
    }

  else /* Bubblesort */
    {
    for(i = 0; i < num_controls-1; i++)
      {
      keep_going = 0;

      for(j = num_controls-1; j > i; j--)
        {
        if(control_widget_get_index(controls[j-1]) >
           control_widget_get_index(controls[j]))
          {
          tmp_widget = controls[j-1];
          controls[j-1] = controls[j];
          controls[j] = tmp_widget;
          keep_going = 1;
          }
        }

      if(!keep_going)
        break;
      }
    
    }
  
  }

card_widget_t * card_widget_create(alsa_card_t * c, bg_cfg_section_t * section)
  {
  int i;
  GtkWidget * scrolledwin;
  GtkWidget * table;
  //  GtkWidget * box;
  GtkWidget * w;
  GtkWidget * sep;
  int lower_cols;
  int upper_index = 0;
  int lower_index = 0;
  int row;
  int col;
  bg_cfg_section_t * subsection;
  card_widget_t * ret;
  
  ret = calloc(1, sizeof(*ret));
  ret->controls = calloc(c->num_groups, sizeof(*(ret->controls)));
  ret->hctl = c->hctl;
  ret->label = bg_strdup(NULL, c->name);
  /* Create controls and count them */
    
  for(i = 0; i < c->num_groups; i++)
    {
    subsection = bg_cfg_section_find_subsection(section, c->groups[i].label);
    
    ret->controls[ret->num_controls] =
      control_widget_create(&(c->groups[i]), subsection, ret);
    
    if(ret->controls[ret->num_controls])
      {
      if(control_widget_is_upper(ret->controls[ret->num_controls]))
        ret->num_upper_controls++;
      else
        ret->num_lower_controls++;

      ret->num_controls++;
      }
    
    }
  
  if(ret->num_upper_controls)
    ret->upper_controls = calloc(ret->num_upper_controls,
                                 sizeof(*ret->upper_controls));

  if(ret->num_lower_controls)
    ret->lower_controls = calloc(ret->num_lower_controls,
                                 sizeof(*ret->lower_controls));
  
  /* Fill in the arrays for the upper and lower controls */

  upper_index = 0;
  lower_index = 0;
  
  for(i = 0; i < ret->num_controls; i++)
    {
    control_widget_read_config(ret->controls[i]);
    if(control_widget_is_upper(ret->controls[i]))
      {
      ret->upper_controls[upper_index] = ret->controls[i];
      upper_index++;
      }
    else
      {
      ret->lower_controls[lower_index] = ret->controls[i];
      lower_index++;
      }
    }

  sort_controls(ret->upper_controls, ret->num_upper_controls);
  

  /* Pack the objects */
  
  ret->w = gtk_vbox_new(0, 0);
  if(ret->num_upper_controls)
    {
    scrolledwin =
      gtk_scrolled_window_new((GtkAdjustment*)0,
                              (GtkAdjustment*)0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_NEVER);
    
    ret->upper_table = gtk_table_new(1, 2 * ret->num_upper_controls - 1, 0);

    for(i = 0; i < ret->num_upper_controls; i++)
      {
      w = control_widget_get_widget(ret->upper_controls[i]);
      g_object_ref(G_OBJECT(w));
      gtk_table_attach(GTK_TABLE(ret->upper_table), w,
                       i*2, i*2+1, 0, 1, GTK_FILL|GTK_SHRINK, GTK_EXPAND|GTK_FILL, 0, 0);

      if(i < ret->num_upper_controls - 1)
        {
        sep = gtk_vseparator_new();
        gtk_widget_show(sep);
        gtk_table_attach(GTK_TABLE(ret->upper_table), sep,
                         i*2+1, i*2+2, 0, 1,
                         GTK_FILL|GTK_SHRINK, GTK_EXPAND|GTK_FILL, 0, 0);
        }
      }
    gtk_widget_show(ret->upper_table);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwin),
                                          ret->upper_table);

    gtk_widget_show(scrolledwin);
    gtk_box_pack_start_defaults(GTK_BOX(ret->w), scrolledwin);
    }

  if(ret->num_lower_controls)
    {
    lower_cols = ((ret->num_lower_controls+LOWER_ROWS-1)/LOWER_ROWS);
    scrolledwin =
      gtk_scrolled_window_new((GtkAdjustment*)0,
                              (GtkAdjustment*)0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_NEVER);
    
    table = gtk_table_new(LOWER_ROWS, lower_cols, FALSE);

    row = 0;
    col = 0;
    
    for(i = 0; i < ret->num_lower_controls; i++)
      {
      w = control_widget_get_widget(ret->lower_controls[i]);
      g_object_ref(G_OBJECT(w));
    
      gtk_table_attach(GTK_TABLE(table), w,
                       col, col+1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
      col++;
      if(col == lower_cols)
        {
        col = 0;
        row++;
        }
      }
    gtk_widget_show(table);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwin),
                                          table);
    
    gtk_widget_show(scrolledwin);
    gtk_box_pack_start(GTK_BOX(ret->w), scrolledwin, FALSE, FALSE, 0);
    }

  /* Tear off controls */

  for(i = 0; i < ret->num_upper_controls; i++)
    {
    if(control_widget_get_own_window(ret->upper_controls[i]))
      {
      card_widget_tearoff_control(ret, ret->upper_controls[i]);
      }
    }
  
  /* Add update callback */
  
  g_timeout_add(TIMEOUT_INTERVAL, timeout_func, ret);
  
  gtk_widget_show(ret->w);
  return ret;
  }

void card_widget_destroy(card_widget_t * w)
  {
  int i;
  for(i = 0; i < w->num_controls; i++)
    {
    control_widget_write_config(w->controls[i]);
    g_object_unref(G_OBJECT(control_widget_get_widget(w->controls[i])));
    }
  if(w->label)
    free(w->label);
  
  free(w);
  }

GtkWidget * card_widget_get_widget(card_widget_t * w)
  {
  return w->w;
  }

int card_widget_num_upper_controls(card_widget_t * w)
  {
  return w->num_upper_controls;
  }

int card_widget_num_lower_controls(card_widget_t * w)
  {
  return w->num_lower_controls;
  }

/* Move functions */

static void move_left(card_widget_t * c, int index)
  {
  GtkWidget * wid;
  int new_index = index-1;
  
  wid = control_widget_get_widget(c->upper_controls[index]);
  
  c->upper_controls[new_index] = c->upper_controls[index];

  if(!control_widget_get_own_window(c->upper_controls[index]))
    {
    gtk_container_remove(GTK_CONTAINER(c->upper_table), wid);
    gtk_table_attach_defaults(GTK_TABLE(c->upper_table), wid,
                              new_index*2, new_index*2+1, 0, 1);
    }
  control_widget_set_index(c->upper_controls[new_index], new_index);
  
  }

static void move_right(card_widget_t * c, int index)
  {
  GtkWidget * wid;
  int new_index = index+1;
  
  wid = control_widget_get_widget(c->upper_controls[index]);
  
  c->upper_controls[new_index] = c->upper_controls[index];

  if(!control_widget_get_own_window(c->upper_controls[index]))
    {
    gtk_container_remove(GTK_CONTAINER(c->upper_table), wid);
    gtk_table_attach_defaults(GTK_TABLE(c->upper_table), wid,
                              new_index*2, new_index*2+1, 0, 1);
    }
  
  control_widget_set_index(c->upper_controls[new_index], new_index);
  }

void card_widget_move_control_left(card_widget_t * c, control_widget_t * w)
  {
  int oldpos, newpos;
  GtkWidget * wid;
  oldpos = control_widget_get_index(w);
  wid = control_widget_get_widget(w);

  /* Remove widget from table */
    
  gtk_container_remove(GTK_CONTAINER(c->upper_table), wid);

  /* Move other widgets */
  
  move_right(c, oldpos-1);

  /* Insert at new position */

  newpos = oldpos - 1;
  
  c->upper_controls[newpos] = w;
  control_widget_set_index(c->upper_controls[newpos], newpos);
  gtk_table_attach_defaults(GTK_TABLE(c->upper_table), wid,
                            newpos*2, newpos*2+1, 0, 1);
  
  }

void card_widget_move_control_right(card_widget_t * c, control_widget_t * w)
  {
  int oldpos, newpos;
  GtkWidget * wid;
  oldpos = control_widget_get_index(w);
  wid = control_widget_get_widget(w);

  /* Remove widget from table */
    
  gtk_container_remove(GTK_CONTAINER(c->upper_table), wid);

  /* Move other widgets */
  
  move_left(c, oldpos+1);

  /* Insert at new position */

  newpos = oldpos + 1;
  
  c->upper_controls[newpos] = w;
  control_widget_set_index(c->upper_controls[newpos], newpos);
  gtk_table_attach_defaults(GTK_TABLE(c->upper_table), wid,
                            newpos*2, newpos*2+1, 0, 1);
  
  }

void card_widget_move_control_first(card_widget_t * c, control_widget_t * w)
  {
  int oldpos, newpos, i;
  GtkWidget * wid;
  oldpos = control_widget_get_index(w);
  wid = control_widget_get_widget(w);

  /* Remove widget from table */
    
  gtk_container_remove(GTK_CONTAINER(c->upper_table), wid);

  /* Move other widgets */
  for(i = oldpos - 1; i >= 0; i--)
    {
    move_right(c, i);
    }
  
  /* Insert at new position */

  newpos = 0;
  
  c->upper_controls[newpos] = w;
  control_widget_set_index(c->upper_controls[newpos], newpos);
  gtk_table_attach_defaults(GTK_TABLE(c->upper_table), wid,
                            newpos*2, newpos*2+1, 0, 1);
  
  }

void card_widget_move_control_last(card_widget_t * c, control_widget_t * w)
  {
  int oldpos, newpos, i;
  GtkWidget * wid;
  oldpos = control_widget_get_index(w);
  wid = control_widget_get_widget(w);

  /* Remove widget from table */
    
  gtk_container_remove(GTK_CONTAINER(c->upper_table), wid);

  /* Move other widgets */

  for(i = oldpos + 1; i < c->num_upper_controls; i++)
    {
    move_left(c, i);
    }
  
  /* Insert at new position */

  newpos = c->num_upper_controls-1;
  
  c->upper_controls[newpos] = w;
  control_widget_set_index(c->upper_controls[newpos], newpos);
  gtk_table_attach_defaults(GTK_TABLE(c->upper_table), wid,
                            newpos*2, newpos*2+1, 0, 1);
  
  }

/* Stuff for own windows */

typedef struct
  {
  control_widget_t * control;
  card_widget_t * card;
  GtkWidget * window;
  } own_window_t;

void card_widget_get_window_coords(card_widget_t * w)
  {
  int x, y, width, height;
  own_window_t * win;
  GList * item;
  
  item = w->control_windows;
  
  while(item)
    {
    win = (own_window_t*)(item->data);
    gtk_window_get_position(GTK_WINDOW(win->window), &(x), &(y));
    gtk_window_get_size(GTK_WINDOW(win->window), &(width), &(height));
    control_widget_set_coords(win->control, x, y, width, height);
    
    item = item->next;
    }
  
  }

static gboolean delete_callback(GtkWidget * w, GdkEvent * evt, gpointer data)
  {
  own_window_t * win;
  win = (own_window_t*)data;

  card_widget_tearon_control(win->card, win->control);
  return FALSE;
  }

static void unmap_callback(GtkWidget * w, gpointer data)
  {
  own_window_t * win;
  int x, y, width, height;

  win = (own_window_t*)data;
  gtk_window_get_position(GTK_WINDOW(win->window), &(x), &(y));
  gtk_window_get_size(GTK_WINDOW(win->window), &(width), &(height));

  
  control_widget_set_coords(win->control, x, y, width, height);
  
  }
     
static void map_callback(GtkWidget * w, gpointer data)
  {
  int x, y, width, height;
  own_window_t * win;
  win = (own_window_t*)data;

  control_widget_get_coords(win->control, &x, &y, &width, &height);

  gtk_window_resize(GTK_WINDOW(win->window), width, height);
  gtk_window_move(GTK_WINDOW(win->window), x, y);

  }



void card_widget_tearoff_control(card_widget_t * c, control_widget_t * w)
  {
  own_window_t * win;
  win = calloc(1, sizeof(*win));

  win->control = w;
  win->card    = c;

  c->control_windows = g_list_append(c->control_windows, win);
  
  gtk_container_remove(GTK_CONTAINER(c->upper_table), control_widget_get_widget(w));
  
  win->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win->window), "Gmerlin Alsamixer");
  gtk_window_set_position(GTK_WINDOW(win->window), GTK_WIN_POS_CENTER);

  gtk_container_add(GTK_CONTAINER(win->window), control_widget_get_widget(w));
  control_widget_set_own_window(w, 1);

  
  g_signal_connect(G_OBJECT(win->window), "delete-event", G_CALLBACK(delete_callback), win);
  g_signal_connect(G_OBJECT(win->window), "map", G_CALLBACK(map_callback), win);
  g_signal_connect(G_OBJECT(win->window), "unmap", G_CALLBACK(unmap_callback), win);
  
  gtk_widget_show(win->window);
  }

void card_widget_tearon_control(card_widget_t * c, control_widget_t * w)
  {
  own_window_t * win = (own_window_t *)0;
  GList * item;
  int index;
  
  item = c->control_windows;
  
  while(item)
    {
    win = (own_window_t*)(item->data);

    if(w == win->control)
      break;
    item = item->next;
    }
  if(!win)
    return;
  gtk_widget_hide(win->window);
  gtk_container_remove(GTK_CONTAINER(win->window), control_widget_get_widget(win->control));
  
  //  g_object_unref(win->window);

  c->control_windows = g_list_remove(c->control_windows, win);

  free(win);

  
  index = control_widget_get_index(w);
  gtk_table_attach_defaults(GTK_TABLE(c->upper_table), control_widget_get_widget(w),
                            index*2, index*2+1, 0, 1);
  control_widget_set_own_window(w, 0);
  }

/* Configuration stuff */

typedef struct
  {
  card_widget_t * cw;
  GtkWidget ** control_buttons;
  GtkWidget * window;
  GtkWidget * close_button;
  } config_window;

static void config_button_callback(GtkWidget * w, gpointer data)
  {
  config_window * win;
  win = (config_window *)data;
  gtk_widget_hide(win->window);
  gtk_main_quit();
  //  g_object_unref(win->window);
  free(win->control_buttons);
  free(win);
  }

static void config_toggle_callback(GtkWidget * w, gpointer data)
  {
  int i;
  config_window * win;
  win = (config_window *)data;

  for(i = 0; i < win->cw->num_upper_controls; i++)
    {
    if(w == win->control_buttons[i])
      {
      if(control_widget_get_own_window(win->cw->upper_controls[i]))
        {
        card_widget_tearon_control(win->cw, win->cw->upper_controls[i]);
        }
      control_widget_set_hidden(win->cw->upper_controls[i],
                                !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->control_buttons[i])));
      }
    }
  }

static gboolean config_delete_callback(GtkWidget * w, GdkEvent * evt, gpointer data)
  {
  config_button_callback(w, data);
  return TRUE;
  }

void card_widget_configure(card_widget_t * c)
  {
  char * window_title;
  int i;
  config_window * win;
  GtkWidget * buttonbox;
  GtkWidget * mainbox;
  GtkWidget * scrolledwindow;
  
  win = calloc(1, sizeof(*win));
  win->cw = c;
  
  win->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win->window), "Card controls");
  gtk_window_set_modal(GTK_WINDOW(win->window), 1);
  g_signal_connect(G_OBJECT(win->window), "delete_event",G_CALLBACK(config_delete_callback),
                   win);

  window_title = bg_sprintf("Controls for %s", c->label);
  gtk_window_set_title(GTK_WINDOW(win->window), window_title);
  free(window_title);
  
  gtk_widget_set_size_request(win->window, 200, 300);
  gtk_window_set_position(GTK_WINDOW(win->window), GTK_WIN_POS_CENTER);
  
  /* Create buttons */
  
  win->control_buttons = calloc(c->num_upper_controls, sizeof(*win->control_buttons));

  buttonbox = gtk_vbox_new(1, 0);
  
  for(i = 0; i < c->num_upper_controls; i++)
    {
    win->control_buttons[i] =
      gtk_check_button_new_with_label(control_widget_get_label(c->upper_controls[i]));

    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->control_buttons[i]),
                                 !control_widget_get_hidden(c->upper_controls[i]));
    g_signal_connect(G_OBJECT(win->control_buttons[i]), "toggled",
                     G_CALLBACK(config_toggle_callback), win);
    gtk_widget_show(win->control_buttons[i]);
    gtk_box_pack_start_defaults(GTK_BOX(buttonbox), win->control_buttons[i]);
    }
  gtk_widget_show(buttonbox);

  /* Close button */

  win->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  g_signal_connect(G_OBJECT(win->close_button), "clicked",
                   G_CALLBACK(config_button_callback), win);
  gtk_widget_show(win->close_button);
  
  scrolledwindow = gtk_scrolled_window_new((GtkAdjustment*)0, (GtkAdjustment*)0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);


  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwindow), buttonbox);
  gtk_widget_show(scrolledwindow);

  mainbox = gtk_vbox_new(0, 5);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), scrolledwindow);
  gtk_box_pack_start(GTK_BOX(mainbox), win->close_button, FALSE, FALSE, 0);
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(win->window), mainbox);
  gtk_widget_show(win->window);
  gtk_main();
  
  }

