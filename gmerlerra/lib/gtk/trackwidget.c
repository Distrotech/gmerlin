#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/cfg_dialog.h>

#include <config.h>
#include <track.h>
#include <project.h>

#include <gui_gtk/utils.h>
#include <gui_gtk/timerange.h>
#include <gui_gtk/trackwidget.h>
#include <gmerlin/utils.h>
#include <gmerlin/gui_gtk/gtkutils.h>

typedef struct
  {
  bg_nle_track_segment_t seg; /* Local copy */
  
  /* TODO: Pixmap for preview */
  
  } segment_t;

struct bg_nle_track_widget_s
  {
  GtkWidget * panel; // GtkExpander
  GtkWidget * panel_child;
  //  GtkWidget * name;
  
  GtkWidget * preview;
  GtkWidget * preview_box;
  GtkWidget * selected;
  GtkWidget * play_button;
  
  int preview_height;
  
  bg_nle_track_t * track;
  bg_nle_time_ruler_t * ruler;

  bg_nle_timerange_widget_t * tr;

  int callback;

  segment_t * segments;
  int num_segments;
  int segments_alloc;
  };

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  bg_nle_track_widget_t * t = data;
  if(!name)
    return;
  if(!strcmp(name, "name"))
    {
    gtk_expander_set_label(GTK_EXPANDER(t->panel), val->val_str);
    gtk_container_check_resize(GTK_CONTAINER(gtk_widget_get_parent(t->panel)));
    }
  }

void bg_nle_track_widget_update_parameters(bg_nle_track_widget_t * w, bg_cfg_section_t * s)
  {
  const char * str;
  bg_cfg_section_get_parameter_string(s, "name", &str);
  gtk_expander_set_label(GTK_EXPANDER(w->panel), str);
  gtk_container_check_resize(GTK_CONTAINER(gtk_widget_get_parent(w->panel)));
  }


typedef struct
  {
  GtkWidget * menu;
  GtkWidget * up;
  GtkWidget * down;
  GtkWidget * delete;
  GtkWidget * configure;
  GtkWidget * filter;
  } menu_t;

static bg_nle_track_widget_t * menu_widget;

static menu_t the_menu;

static void menu_callback(GtkWidget * w, gpointer data)
  {
  if(w == the_menu.up)
    {
    int index = bg_nle_project_track_index(menu_widget->track->p, menu_widget->track);
    if(index > 0)
      bg_nle_project_move_track(menu_widget->track->p, index, index-1);
    }
  else if(w == the_menu.down)
    {
    int index = bg_nle_project_track_index(menu_widget->track->p, menu_widget->track);
    if(index < menu_widget->track->p->num_tracks-1)
      bg_nle_project_move_track(menu_widget->track->p, index, index+1);
    
    }
  else if(w == the_menu.delete)
    {
    bg_nle_project_delete_track(menu_widget->track->p, menu_widget->track);
    }
  else if(w == the_menu.filter)
    {
    
    }
  else if(w == the_menu.configure)
    {
    bg_dialog_t * dialog;
    int result;
    bg_cfg_section_t * s = bg_nle_project_set_track_parameters_start(menu_widget->track->p,
                                                                 menu_widget->track);
    
    dialog = bg_dialog_create(s,
                              set_parameter, // bg_set_parameter_func_t set_param,
                              NULL, // bg_get_parameter_func_t get_param,
                              menu_widget, // void * callback_data,
                              bg_nle_track_get_parameters(menu_widget->track),
                              TR("Track parameters"));
    result = bg_dialog_show(dialog, menu_widget->panel_child);
    bg_dialog_destroy(dialog);
    bg_nle_project_set_track_parameters_end(menu_widget->track->p,
                                            s, result,
                                            menu_widget->track);
    }
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
  the_menu.menu      = gtk_menu_new();
  the_menu.configure = create_menu_item(the_menu.menu, TR("Configure"), "config_16.png");
  the_menu.filter    = create_menu_item(the_menu.menu, TR("Add filter"), "filter_16.png");
  the_menu.up        = create_menu_item(the_menu.menu, TR("Move up"),   "up_16.png");
  the_menu.down      = create_menu_item(the_menu.menu, TR("Move down"), "down_16.png");
  the_menu.delete    = create_menu_item(the_menu.menu, TR("Delete"),    "trash_16.png");
  }



static void button_callback(GtkWidget * w, gpointer  data)
  {
  bg_nle_track_widget_t * t = data;
  int flags = t->track->flags;
  if(t->callback)
    return;
  if(w == t->selected)
    {
    // fprintf(stderr, "selected\n");
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t->selected)))
      flags |= BG_NLE_TRACK_SELECTED;
    else
      flags &= ~BG_NLE_TRACK_SELECTED;
    }
  if(w == t->play_button)
    {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t->play_button)))
      flags |= BG_NLE_TRACK_PLAYBACK;
    else
      flags &= ~BG_NLE_TRACK_PLAYBACK;
    }
  bg_nle_project_set_track_flags(t->track->p, t->track, flags);
  }

#if 0
static GtkWidget * create_pixmap_button(bg_nle_track_widget_t * w,
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

static GtkWidget * create_pixmap_toggle_button(bg_nle_track_widget_t * w,
                                               const char * filename,
                                               const char * tooltip, int active)
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

  if(active)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), 1);
  
  gtk_container_add(GTK_CONTAINER(button), image);
  
  g_signal_connect(G_OBJECT(button), "toggled",
                   G_CALLBACK(button_callback), w);
  
  gtk_widget_show(button);

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);
  
  return button;
  }

static void
expander_callback(GtkWidget *wid,
                  GParamSpec *param_spec,
                  gpointer    user_data)
  {
  bg_nle_track_widget_t * w = user_data;
  int flags = w->track->flags;

  if(w->callback)
    return;
  
  if(gtk_expander_get_expanded(GTK_EXPANDER(wid)))
    flags |= BG_NLE_TRACK_EXPANDED;
  else
    flags &= ~BG_NLE_TRACK_EXPANDED;

  bg_nle_project_set_track_flags(w->track->p,
                                 w->track, flags);
  }

static void draw_segment(bg_nle_track_widget_t * w, cairo_t * c, int i)
  {
  bg_nle_time_range_t r;
  GdkRectangle rect;
  float x1, x2;
  
  r.start = w->segments[i].seg.dst_pos;
  r.end   = w->segments[i].seg.dst_pos + w->segments[i].seg.len;
  
  if(!bg_nle_time_range_intersect(&r, &w->tr->visible))
    return;

  x1 = bg_nle_time_2_pos(w->tr, r.start);
  x2 = bg_nle_time_2_pos(w->tr, r.end);
  
  rect.x = x1;
  rect.width = x2 - x1;
  rect.y = 0;
  rect.height = w->preview_height;
  gdk_cairo_rectangle(c, &rect);
  
  cairo_set_source_rgba(c, 1.0, 1.0, 1.0, 1.0);
  cairo_fill(c);
  }

void bg_nle_track_widget_redraw(bg_nle_track_widget_t * w)
  {
  float x1, x2;
  cairo_t * c;
  int i;
  
  if(!GTK_WIDGET_REALIZED(w->preview))
    return;
  
  c = gdk_cairo_create(w->preview->window);
  
  gdk_window_clear(w->preview->window);

  //  fprintf(stderr, "Track widget Selection: %ld %ld\n",
  //          w->tr->selection.start, w->tr->selection.end);
  
  /* Draw segments */
  
  for(i = 0; i < w->num_segments; i++)
    {
    draw_segment(w, c, i);
    }
  
  /* Draw selection */

  if(bg_nle_time_range_intersect(&w->tr->selection,
                                 &w->tr->visible))
    {
    x1 = bg_nle_time_2_pos(w->tr,
                           w->tr->selection.start);
    x2 = bg_nle_time_2_pos(w->tr,
                           w->tr->selection.end);
  
    if(w->tr->selection.start >= 0)
      {
      cairo_move_to(c, x1, 0);
      cairo_line_to(c, x1, w->preview_height);
      cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
      cairo_stroke(c);
    
      if(w->tr->selection.end >= 0)
        {
        GdkRectangle r;
        r.x = x1;
        r.width = x2 - x1;
        r.y = 0;
        r.height = w->preview_height;
        gdk_cairo_rectangle(c, &r);

        cairo_set_source_rgba(c, 1.0, 0.0, 0.0, 0.2);
        cairo_fill(c);

        cairo_move_to(c, x2, 0);
        cairo_line_to(c, x2, w->preview_height);
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
  bg_nle_track_widget_t * w = user_data;
  bg_nle_track_widget_redraw(w);
  return TRUE;
  }

static gboolean button_press_callback(GtkWidget *widget,
                                      GdkEventButton * evt,
                                      gpointer user_data)
  {
  bg_nle_track_widget_t * w = user_data;

  if(bg_nle_timerange_widget_handle_button_press(w->tr, widget, evt))
    return TRUE;
  else if(evt->button == 3)
    {
    menu_widget = w;
    gtk_menu_popup(GTK_MENU(the_menu.menu),
                   (GtkWidget *)0,
                   (GtkWidget *)0,
                   (GtkMenuPositionFunc)0,
                   (gpointer)0,
                   3, evt->time);
    return TRUE;
    }
  return FALSE;
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
  bg_nle_track_widget_t * w = user_data;
  w->preview_height = allocation->height;
  }

static const GdkColor preview_bg =
  {
    0x00000000,
    0x4000,
    0x4000,
    0x4000
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
  bg_nle_track_widget_t * r = user_data;
  bg_nle_timerange_widget_handle_motion(r->tr, widget, evt);
  return TRUE;
  }


bg_nle_track_widget_t *
bg_nle_track_widget_create(bg_nle_track_t * track,
                           bg_nle_timerange_widget_t * tr,
                           bg_nle_time_ruler_t * ruler)
  {
  int i;
  bg_nle_track_widget_t * ret;
  
  GtkSizeGroup * size_group;

  init_menu();

  ret = calloc(1, sizeof(*ret));
  ret->track = track;
  ret->ruler = ruler;
  ret->tr = tr;
  /* Create expander */
  ret->panel = gtk_expander_new(bg_nle_track_get_name(track));
  
  if(ret->track->flags & BG_NLE_TRACK_EXPANDED)
    gtk_expander_set_expanded(GTK_EXPANDER(ret->panel), TRUE);
  
  g_signal_connect (ret->panel, "notify::expanded",
                    G_CALLBACK (expander_callback), ret);
  
  /* Create panel widgets */
  ret->selected =
    create_pixmap_toggle_button(ret,
                                "record_16.png",
                                TRS("Select track for editing operations"),
                                !!(ret->track->flags & BG_NLE_TRACK_SELECTED));
  
  ret->play_button =
    create_pixmap_toggle_button(ret,
                                "gmerlerra/play.png",
                                TRS("Select track for playback"),
                                !!(ret->track->flags & BG_NLE_TRACK_PLAYBACK));
  
  /* Pack panel */
  ret->panel_child = gtk_table_new(1, 3, FALSE);

  gtk_table_attach(GTK_TABLE(ret->panel_child), ret->play_button, 
                   0, 1, 0, 1, GTK_FILL,
                   GTK_FILL|GTK_SHRINK, 0, 0);
  
  gtk_table_attach(GTK_TABLE(ret->panel_child), ret->selected, 
                   1, 2, 0, 1, GTK_FILL,
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
                        GDK_POINTER_MOTION_MASK);
  
  if(ret->track->type == BG_NLE_TRACK_VIDEO)
    {
    gtk_widget_set_size_request(ret->preview, -1, 64);
    }

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
  
  if(ret->track->flags & BG_NLE_TRACK_EXPANDED)
    gtk_widget_show(ret->preview);
  gtk_box_pack_start(GTK_BOX(ret->preview_box), ret->preview,
                     TRUE, TRUE, 0);
  gtk_widget_show(ret->preview_box);
  
  size_group = gtk_size_group_new(GTK_SIZE_GROUP_VERTICAL);

  gtk_size_group_add_widget(size_group, ret->panel);
  gtk_size_group_add_widget(size_group, ret->preview_box);
  g_object_unref(size_group);

  ret->num_segments = ret->track->num_segments;
  ret->segments_alloc = ret->num_segments;
  ret->segments = calloc(ret->segments_alloc, sizeof(*ret->segments));

  for(i = 0; i < ret->num_segments; i++)
    {
    memcpy(&ret->segments[i].seg, &ret->track->segments[i], sizeof(ret->segments[i].seg));
    }
  
  return ret;
  }

void bg_nle_track_widget_destroy(bg_nle_track_widget_t * w)
  {
  gtk_widget_destroy(w->panel);
  gtk_widget_destroy(w->preview_box);
  free(w);
  }

GtkWidget * bg_nle_track_widget_get_panel(bg_nle_track_widget_t * w)
  {
  return w->panel;
  }

GtkWidget * bg_nle_track_widget_get_preview(bg_nle_track_widget_t * w)
  {
  return w->preview_box;
  }

void bg_nle_track_widget_update_selection(bg_nle_track_widget_t * w)
  {
  bg_nle_track_widget_redraw(w);
  }

void bg_nle_track_widget_update_visible(bg_nle_track_widget_t * w)
  {
  bg_nle_track_widget_redraw(w);
  }

void bg_nle_track_widget_update_zoom(bg_nle_track_widget_t * w)
  {
  bg_nle_track_widget_redraw(w);
  }

void bg_nle_track_widget_set_flags(bg_nle_track_widget_t * w, int flags)
  {
  w->callback = 1;
  //  fprintf(stderr, "bg_nle_track_widget_set_flags\n");
  /* Selected */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->selected),
                               !!(flags & BG_NLE_TRACK_SELECTED));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->play_button),
                               !!(flags & BG_NLE_TRACK_PLAYBACK));
  
  /* Expanded */
  if(flags & BG_NLE_TRACK_EXPANDED)
    {
    gtk_expander_set_expanded(GTK_EXPANDER(w->panel), 1);
    gtk_widget_show(w->preview);
    }
  else
    {
    gtk_expander_set_expanded(GTK_EXPANDER(w->panel), 0);
    gtk_widget_hide(w->preview);
    }
  w->callback = 0;
  }
