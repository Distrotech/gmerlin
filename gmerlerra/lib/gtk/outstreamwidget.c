#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/cfg_dialog.h>

#include <config.h>
#include <track.h>
#include <project.h>

#include <gui_gtk/timerange.h>
#include <gui_gtk/outstreamwidget.h>
#include <gui_gtk/utils.h>

#include <gmerlin/utils.h>
#include <gmerlin/gui_gtk/gtkutils.h>

typedef struct
  {
  GtkWidget * menu;
  GtkWidget * up;
  GtkWidget * down;
  GtkWidget * delete;
  GtkWidget * configure;
  GtkWidget * filter;

  GtkWidget * tracks_item;

  struct
    {
    GtkWidget * menu;

    struct
      {
      guint id;
      GtkWidget * item;
      } * items;
    int num_items;
    int items_alloc;
    } tracks_menu;
  
  } menu_t;

struct bg_nle_outstream_widget_s
  {
  GtkWidget * panel;
  GtkWidget * panel_child;
  //  GtkWidget * name;
  
  GtkWidget * preview;
  GtkWidget * preview_box;
  //  GtkWidget * selected;
  GtkWidget * play_button;

  int preview_height;
  
  bg_nle_time_ruler_t * ruler;  
  bg_nle_outstream_t * outstream;

  void (*play_callback)(bg_nle_outstream_widget_t *, void *);
  void (*delete_callback)(bg_nle_outstream_widget_t *, void *);
  void * callback_data;
  
  bg_nle_timerange_widget_t * tr;
  };

static bg_nle_outstream_widget_t * menu_widget;

static menu_t the_menu;

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  bg_nle_outstream_widget_t * t = data;
  if(!name)
    return;
  if(!strcmp(name, "name"))
    {
    gtk_expander_set_label(GTK_EXPANDER(t->panel), val->val_str);
    gtk_container_check_resize(GTK_CONTAINER(gtk_widget_get_parent(t->panel)));
    }
  }

static void menu_callback(GtkWidget * w, gpointer data)
  {
  if(w == the_menu.up)
    {
    int index = bg_nle_project_outstream_index(menu_widget->outstream->p, menu_widget->outstream);
    if(index > 0)
      bg_nle_project_move_outstream(menu_widget->outstream->p, index, index-1);
    }
  else if(w == the_menu.down)
    {
    int index = bg_nle_project_outstream_index(menu_widget->outstream->p, menu_widget->outstream);
    if(index < menu_widget->outstream->p->num_outstreams-1)
      bg_nle_project_move_outstream(menu_widget->outstream->p, index, index+1);
    }
  else if(w == the_menu.delete)
    {
    bg_nle_project_delete_outstream(menu_widget->outstream->p,
                                    menu_widget->outstream);
    }
  else if(w == the_menu.filter)
    {
    
    }
  else if(w == the_menu.configure)
    {
    bg_dialog_t * dialog;
    dialog = bg_dialog_create(menu_widget->outstream->section,
                              set_parameter, // bg_set_parameter_func_t set_param,
                              NULL, // bg_get_parameter_func_t get_param,
                              menu_widget, // void * callback_data,
                              bg_nle_outstream_get_parameters(menu_widget->outstream),
                              TR("Outstream parameters"));
    bg_dialog_show(dialog, menu_widget->panel_child);
    bg_dialog_destroy(dialog);
    }
  }

static void track_toggle_callback(GtkWidget * w, gpointer data)
  {
  int i;
  int index = -1;
  
  for(i = 0; i < the_menu.tracks_menu.items_alloc; i++)
    {
    if(the_menu.tracks_menu.items[i].item == w)
      {
      index = i;
      break;
      }
    }
  if(index < 0)
    return;
  
  fprintf(stderr, "Toggle callback %d %d\n",
          index, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
  }

static GtkWidget *
create_menu_item(GtkWidget * parent,
                 const char * label, const char * pixmap)
  {
  GtkWidget * ret, *image;
  char * path;
  
  if(pixmap)
    {
    path = bg_search_file_read("icons", pixmap);
    if(path)
      {
      image = gtk_image_new_from_file(path);
      free(path);
      }
    else
      image = gtk_image_new();
    gtk_widget_show(image);
    ret = gtk_image_menu_item_new_with_label(label);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(ret), image);
    }
  else
    {
    ret = gtk_menu_item_new_with_label(label);
    }
  
  g_signal_connect(G_OBJECT(ret), "activate", G_CALLBACK(menu_callback),
                   (gpointer)0);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }


static void init_menu()
  {
  if(the_menu.menu)
    return;

  the_menu.tracks_menu.menu  = gtk_menu_new();
  
  the_menu.menu      = gtk_menu_new();
  the_menu.configure = create_menu_item(the_menu.menu,
                                        TR("Configure"),
                                        "config_16.png");
  
  the_menu.tracks_item = create_menu_item(the_menu.menu,
                                          TR("Source tracks"),
                                          NULL);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(the_menu.tracks_item),
                            the_menu.tracks_menu.menu);
  
  the_menu.filter    = create_menu_item(the_menu.menu,
                                        TR("Add filter"),
                                        "filter_16.png");
  the_menu.up        = create_menu_item(the_menu.menu,
                                        TR("Move up"),
                                        "up_16.png");
  the_menu.down      = create_menu_item(the_menu.menu,
                                        TR("Move down"),
                                        "down_16.png");
  the_menu.delete    = create_menu_item(the_menu.menu,
                                        TR("Delete"),
                                        "trash_16.png");
  
  }

static void init_tracks_menu(bg_nle_outstream_t * os)
  {
  int num_tracks;
  int new_items_alloc;
  int i, index = 0;
  GtkWidget * w;

  /* Count tracks */
  num_tracks = 0;
  for(i = 0; i < os->p->num_tracks; i++)
    {
    if(os->p->tracks[i]->type == os->type)
      num_tracks++;
    }
  
  /* Allocate items */
  
  if(num_tracks > the_menu.tracks_menu.items_alloc)
    {
    new_items_alloc = num_tracks + 16;
    
    the_menu.tracks_menu.items = realloc(the_menu.tracks_menu.items,
                                         new_items_alloc *
                                         sizeof(*the_menu.tracks_menu.items));

    for(i = the_menu.tracks_menu.items_alloc; i < new_items_alloc; i++)
      {
      the_menu.tracks_menu.items[i].item =
        gtk_check_menu_item_new_with_label("");
      gtk_menu_shell_append(GTK_MENU_SHELL(the_menu.tracks_menu.menu),
                            the_menu.tracks_menu.items[i].item);
      /* TODO: Callback */
      
      the_menu.tracks_menu.items[i].id =
        g_signal_connect(the_menu.tracks_menu.items[i].item,
                         "toggled", G_CALLBACK(track_toggle_callback), NULL);
      }
    the_menu.tracks_menu.items_alloc = new_items_alloc;
    }

  index = 0;
  for(i = 0; i < os->p->num_tracks; i++)
    {
    if(os->p->tracks[i]->type == os->type)
      {
      w = the_menu.tracks_menu.items[index].item;
      
      gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(w))),
                         bg_nle_track_get_name(os->p->tracks[i]));
      
      gtk_widget_show(w);
      index++;
      }
    }
  for(i = num_tracks; i < the_menu.tracks_menu.items_alloc; i++)
    gtk_widget_hide(the_menu.tracks_menu.items[i].item);
  }

static void button_callback(GtkWidget * w, gpointer  data)
  {
  bg_nle_outstream_widget_t * t = data;

  if(w == t->play_button)
    {
    if(t->play_callback)
      t->play_callback(t, t->callback_data);
    }
  }

#if 0
static GtkWidget * create_pixmap_button(bg_nle_outstream_widget_t * w,
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
                   G_CALLBACK(button_callback), w);
  
  gtk_widget_show(button);

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);
  
  return button;
  }
#endif

static GtkWidget * create_pixmap_toggle_button(bg_nle_outstream_widget_t * w,
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
  button = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);

  g_signal_connect(G_OBJECT(button), "toggled",
                   G_CALLBACK(button_callback), w);
  
  gtk_widget_show(button);

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);
  
  return button;
  }



static void
expander_callback(GObject    *object,
                  GParamSpec *param_spec,
                  gpointer    user_data)
  {
  GtkExpander *expander;
  bg_nle_outstream_widget_t * w = user_data;
  expander = GTK_EXPANDER (object);
  if (gtk_expander_get_expanded (expander))
    {
    /* Show or create widgets */
    gtk_widget_show(w->preview);
    w->outstream->flags |= BG_NLE_TRACK_EXPANDED;
    }
  else
    {
    /* Hide or destroy widgets */
    gtk_widget_hide(w->preview);
    w->outstream->flags &= ~BG_NLE_TRACK_EXPANDED;
    }
  }

void bg_nle_outstream_widget_redraw(bg_nle_outstream_widget_t * w)
  {
  float selection_start_pos;
  float selection_end_pos;
  cairo_t * c;

  if(!GTK_WIDGET_REALIZED(w->preview))
    return;
  
  c = gdk_cairo_create(w->preview->window);
  
  gdk_window_clear(w->preview->window);
  
  /* Draw preview */

  /* Draw selection */

  if(bg_nle_time_range_intersect(&w->tr->selection,
                                 &w->tr->visible))
    {
    
    selection_start_pos = bg_nle_time_2_pos(w->tr,
                                            w->tr->selection.start);
    selection_end_pos = bg_nle_time_2_pos(w->tr,
                                          w->tr->selection.end);
  
    if(w->tr->selection.start >= 0)
      {
      cairo_move_to(c, selection_start_pos, 0);
      cairo_line_to(c, selection_start_pos, w->preview_height);
      cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
      cairo_stroke(c);
    
      if(w->tr->selection.end >= 0)
        {
        GdkRectangle r;
        r.x = selection_start_pos;
        r.width = selection_end_pos - selection_start_pos;
        r.y = 0;
        r.height = w->preview_height;
        gdk_cairo_rectangle(c, &r);

        cairo_set_source_rgba(c, 1.0, 0.0, 0.0, 0.2);
        cairo_fill(c);

        cairo_move_to(c, selection_end_pos, 0);
        cairo_line_to(c, selection_end_pos, w->preview_height);
        cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
        cairo_stroke(c);
        }
      }
    }
  cairo_destroy(c);
  }

static gboolean expose_callback(GtkWidget *widget,
                                GdkEventExpose * evt,
                                gpointer       user_data)
  {
  bg_nle_outstream_widget_t * w = user_data;
  bg_nle_outstream_widget_redraw(w);
  return TRUE;
  }

static gboolean button_press_callback(GtkWidget *widget,
                                      GdkEventButton * evt,
                                      gpointer user_data)
  {
  bg_nle_outstream_widget_t * w = user_data;

  if(bg_nle_timerange_widget_handle_button_press(w->tr, widget, evt))
    return TRUE;
  else if(evt->button == 3)
    {
    menu_widget = w;
    init_tracks_menu(w->outstream);
    gtk_menu_popup(GTK_MENU(the_menu.menu),
                   (GtkWidget *)0,
                   (GtkWidget *)0,
                   (GtkMenuPositionFunc)0,
                   (gpointer)0,
                   3, evt->time);
    }
  return TRUE;
  }

static gboolean button_release_callback(GtkWidget *widget,
                                        GdkEventButton * evt,
                                        gpointer user_data)
  {
  gdk_window_set_cursor(widget->window, bg_nle_cursor_xterm);
  return TRUE;
  }

static void size_allocate_callback(GtkWidget     *widget,
                                   GtkAllocation *allocation,
                                   gpointer       user_data)
  {
  bg_nle_outstream_widget_t * w = user_data;
  w->preview_height = allocation->height;
  }

static const GdkColor preview_bg =
  {
    0x00000000,
    0x8000,
    0x8000,
    0xFFFF
  };

static void realize_callback(GtkWidget *widget,
                             gpointer       user_data)
  {
  gdk_window_set_cursor(widget->window, bg_nle_cursor_xterm);
  }

static gboolean motion_callback(GtkWidget *widget,
                                GdkEventMotion * evt,
                                gpointer user_data)
  {
  bg_nle_outstream_widget_t * r = user_data;
  
  bg_nle_timerange_widget_handle_motion(r->tr, evt);
  return FALSE;
  }


bg_nle_outstream_widget_t *
bg_nle_outstream_widget_create(bg_nle_outstream_t * outstream,
                               bg_nle_time_ruler_t * ruler,
                               bg_nle_timerange_widget_t * tr,
                               void (*play_callback)(bg_nle_outstream_widget_t *, void *),
                               void * callback_data)
  {
  bg_nle_outstream_widget_t * ret;
  
  GtkSizeGroup * size_group;

  init_menu();
  
  ret = calloc(1, sizeof(*ret));
  ret->outstream = outstream;
  ret->ruler = ruler;
  ret->tr = tr;
  
  ret->play_callback = play_callback;
  ret->callback_data = callback_data;
  
  /* Create expander */
  ret->panel = gtk_expander_new(bg_nle_outstream_get_name(outstream));
  
  if(ret->outstream->flags & BG_NLE_TRACK_EXPANDED)
    gtk_expander_set_expanded(GTK_EXPANDER(ret->panel), TRUE);
  
  g_signal_connect (ret->panel, "notify::expanded",
                    G_CALLBACK (expander_callback), ret);
  
  /* Create panel widgets */
  

  ret->play_button =
    create_pixmap_toggle_button(ret,
                                "gmerlerra_play.png",
                                TRS("Select outstream for playback"));
  
  /* Pack panel */
  ret->panel_child = gtk_table_new(1, 2, FALSE);

  gtk_table_attach(GTK_TABLE(ret->panel_child), ret->play_button, 
                   0, 1, 0, 1, GTK_FILL,
                   GTK_FILL|GTK_SHRINK, 0, 0);
  
  gtk_widget_show(ret->panel_child);

  
  gtk_container_add(GTK_CONTAINER(ret->panel), ret->panel_child);
  gtk_widget_show(ret->panel);
  
  /* Preview */
  
  ret->preview = gtk_drawing_area_new();
  gtk_widget_set_events(ret->preview,
                        GDK_EXPOSURE_MASK |
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_BUTTON1_MOTION_MASK |
                        GDK_BUTTON2_MOTION_MASK);
  
  gtk_widget_modify_bg(ret->preview,
                       GTK_STATE_NORMAL,
                       &preview_bg);
  
  g_signal_connect(ret->preview,
                   "expose-event", G_CALLBACK(expose_callback),
                   ret);

  g_signal_connect(ret->preview,
                   "button-press-event", G_CALLBACK(button_press_callback),
                   ret);
  g_signal_connect(ret->preview,
                   "button-release-event", G_CALLBACK(button_release_callback),
                   ret);
  g_signal_connect(ret->preview, "motion-notify-event",
                   G_CALLBACK(motion_callback),
                   ret);

  g_signal_connect(ret->preview,
                   "size-allocate", G_CALLBACK(size_allocate_callback),
                   ret);

  g_signal_connect(ret->preview, "realize", G_CALLBACK(realize_callback),
                   ret);

  
  ret->preview_box = gtk_hbox_new(FALSE, 0);
  
  if(ret->outstream->flags & BG_NLE_TRACK_EXPANDED)
    gtk_widget_show(ret->preview);
  gtk_box_pack_start(GTK_BOX(ret->preview_box), ret->preview,
                     TRUE, TRUE, 0);
  gtk_widget_show(ret->preview_box);
  
  size_group = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);

  gtk_size_group_add_widget(size_group, ret->panel);
  gtk_size_group_add_widget(size_group, ret->preview_box);
  g_object_unref(size_group);
  return ret;
  }

void bg_nle_outstream_widget_destroy(bg_nle_outstream_widget_t * w)
  {
  gtk_widget_destroy(w->panel);
  gtk_widget_destroy(w->preview_box);
  
  free(w);
  }

GtkWidget * bg_nle_outstream_widget_get_panel(bg_nle_outstream_widget_t * w)
  {
  return w->panel;
  }

GtkWidget * bg_nle_outstream_widget_get_preview(bg_nle_outstream_widget_t * w)
  {
  return w->preview_box;
  }

void bg_nle_outstream_widget_update_selection(bg_nle_outstream_widget_t * w)
  {
  bg_nle_outstream_widget_redraw(w);
  }

void bg_nle_outstream_widget_update_visible(bg_nle_outstream_widget_t * w)
  {
  bg_nle_outstream_widget_redraw(w);
  }

void bg_nle_outstream_widget_update_zoom(bg_nle_outstream_widget_t * w)
  {
  bg_nle_outstream_widget_redraw(w);
  }
