#include <gtk/gtk.h>
#include <stdlib.h>

#include <gmerlin/cfg_registry.h>

#include <config.h>
#include <track.h>
#include <project.h>

#include <gui_gtk/trackwidget.h>
#include <gmerlin/utils.h>
#include <gmerlin/gui_gtk/gtkutils.h>

struct bg_nle_track_widget_s
  {
  GtkWidget * panel;
  GtkWidget * panel_child;
  //  GtkWidget * name;
  
  GtkWidget * preview;
  GtkWidget * preview_box;
  GtkWidget * selected;
  GtkWidget * config_button;

  int preview_width;
  int preview_height;
  
  bg_nle_time_ruler_t * ruler;  
  bg_nle_track_t * track;
  };

static void button_callback(GtkWidget * w, gpointer  data)
  {
  bg_nle_track_widget_t * t = data;
  
  if(w == t->config_button)
    {
    fprintf(stderr, "config button\n");
    }
  if(w == t->selected)
    {
    fprintf(stderr, "selected\n");
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t->selected)))
      t->track->flags |= BG_NLE_TRACK_SELECTED;
    else
      t->track->flags &= ~BG_NLE_TRACK_SELECTED;
    }
  }
     
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

static void
expander_callback(GObject    *object,
                  GParamSpec *param_spec,
                  gpointer    user_data)
  {
  GtkExpander *expander;
  bg_nle_track_widget_t * w = user_data;
  expander = GTK_EXPANDER (object);
  if (gtk_expander_get_expanded (expander))
    {
    /* Show or create widgets */
    gtk_widget_show(w->preview);
    w->track->flags |= BG_NLE_TRACK_EXPANDED;
    }
  else
    {
    /* Hide or destroy widgets */
    gtk_widget_hide(w->preview);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->selected), 0);
    w->track->flags &= ~BG_NLE_TRACK_EXPANDED;
    }
  }

void bg_nle_track_widget_redraw(bg_nle_track_widget_t * w)
  {
  gavl_time_t selection_start_time;
  gavl_time_t selection_end_time;
  float selection_start_pos;
  float selection_end_pos;
  
  cairo_t * c = gdk_cairo_create(w->preview->window);
  
  gdk_window_clear(w->preview->window);
  
  /* Draw preview */

  /* Draw selection */
  bg_nle_time_ruler_get_selection(w->ruler,
                                  &selection_start_time,
                                  &selection_end_time);

  selection_start_pos = bg_nle_time_ruler_time_2_pos(w->ruler,
                                                     selection_start_time);
  selection_end_pos = bg_nle_time_ruler_time_2_pos(w->ruler,
                                                   selection_end_time);
  
  if(selection_start_time >= 0)
    {
    cairo_move_to(c, selection_start_pos, 0);
    cairo_line_to(c, selection_start_pos, w->preview_height);
    cairo_set_source_rgb(c, 1.0, 0.0, 0.0);
    cairo_stroke(c);
    
    if(selection_end_time >= 0)
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
  
  if(evt->button == 1)
    bg_nle_time_ruler_handle_button_press(w->ruler, evt);
  
  return TRUE;
  }


static void size_allocate_callback(GtkWidget     *widget,
                                   GtkAllocation *allocation,
                                   gpointer       user_data)
  {
  bg_nle_track_widget_t * w = user_data;
  w->preview_width  = allocation->width;
  w->preview_height = allocation->height;
  
  }

static const GdkColor preview_bg =
  {
    0x00000000,
    0x4000,
    0x4000,
    0x4000
  };

bg_nle_track_widget_t *
bg_nle_track_widget_create(bg_nle_track_t * track,
                           bg_nle_time_ruler_t * ruler)
  {
  bg_nle_track_widget_t * ret;
  
  GtkSizeGroup * size_group;
  
  ret = calloc(1, sizeof(*ret));
  ret->track = track;
  ret->ruler = ruler;
  
  /* Create expander */
  ret->panel = gtk_expander_new(track->name);

  if(ret->track->flags & BG_NLE_TRACK_EXPANDED)
    gtk_expander_set_expanded(GTK_EXPANDER(ret->panel), TRUE);
  
  g_signal_connect (ret->panel, "notify::expanded",
                    G_CALLBACK (expander_callback), ret);
  
  /* Create panel widgets */
  ret->selected = gtk_check_button_new();
  bg_gtk_tooltips_set_tip(ret->selected,
                          TRS("Select track for editing operations"),
                          PACKAGE);
  
  if(ret->track->flags & BG_NLE_TRACK_SELECTED)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->selected),
                                 1);
  
  g_signal_connect(G_OBJECT(ret->selected), "toggled",
                   G_CALLBACK(button_callback), ret);
  
  gtk_widget_show(ret->selected);

  ret->config_button =
    create_pixmap_button(ret,
                         "config_16.png",
                         TRS("Configure track"));

  /* Pack panel */
  ret->panel_child = gtk_hbox_new(FALSE, 0);

  gtk_box_pack_start(GTK_BOX(ret->panel_child),
                     ret->selected, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(ret->panel_child),
                     ret->config_button, FALSE, FALSE, 0);
  
  gtk_widget_show(ret->panel_child);
  gtk_container_add(GTK_CONTAINER(ret->panel), ret->panel_child);
  gtk_widget_show(ret->panel);
  
  /* Preview */
  
  ret->preview = gtk_drawing_area_new();
  gtk_widget_set_events(ret->preview,
                        GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);

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
                   "size-allocate", G_CALLBACK(size_allocate_callback),
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
  return ret;
  }

void bg_nle_track_widget_destroy(bg_nle_track_widget_t * w)
  {
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
