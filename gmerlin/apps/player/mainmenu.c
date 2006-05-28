/*****************************************************************
 
  mainmenu.h
 
  Copyright (c) 2003-2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include "gmerlin.h"
#include <utils.h>
#include <gdk/gdkkeysyms.h>

typedef struct stream_menu_s
  {
  GSList * group;
  GtkWidget ** stream_items;
  guint * ids;

  GtkWidget * off_item;
  guint off_id;

  int num_streams;
  int streams_alloc;
  
  GtkWidget * menu;
  } stream_menu_t;

struct windows_menu_s
  {
  GtkWidget * mediatree;
  guint       mediatree_id;
  GtkWidget * infowindow;
  guint       infowindow_id;
  GtkWidget * logwindow;
  guint       logwindow_id;
  GtkWidget * menu;
  };

struct options_menu_s
  {
  GtkWidget * preferences;
  GtkWidget * plugins;
  GtkWidget * skins;
  GtkWidget * menu;
  };

struct command_menu_s
  {
  GtkWidget * inc_volume;
  GtkWidget * dec_volume;
  GtkWidget * seek_forward;
  GtkWidget * seek_backward;
  GtkWidget * next;
  GtkWidget * previous;

  GtkWidget * seek_start;
  GtkWidget * pause;
  
  GtkWidget * menu;
  };

struct accessories_menu_s
  {
  GtkWidget * transcoder;
  GtkWidget * visualizer;
  GtkWidget * mixer;

  GtkWidget * menu;
  };

struct main_menu_s
  {
  struct windows_menu_s     windows_menu;
  struct options_menu_s     options_menu;
  struct command_menu_s     command_menu;
  struct accessories_menu_s accessories_menu;
  struct stream_menu_s      audio_stream_menu;
  struct stream_menu_s      video_stream_menu;
  struct stream_menu_s      subtitle_stream_menu;
    
  GtkWidget * windows_item;
  GtkWidget * options_item;
  GtkWidget * accessories_item;
  
  GtkWidget * audio_stream_item;
  GtkWidget * video_stream_item;
  GtkWidget * subtitle_stream_item;
  
  GtkWidget * menu;
  gmerlin_t * g;
  };

static GtkWidget * create_menu()
  {
  GtkWidget * ret;
  GtkWidget * tearoff_item;

  ret = gtk_menu_new();
  tearoff_item = gtk_tearoff_menu_item_new();
  gtk_widget_show(tearoff_item);

  gtk_menu_shell_append(GTK_MENU_SHELL(ret), tearoff_item);
  return ret;
  }

static int stream_menu_has_widget(stream_menu_t * s,
                                  GtkWidget * w, int * index)
  {
  int i;
  if((w == s->off_item) &&
     gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(s->off_item)))
    {
    *index = -1;
    return 1;
    }
  for(i = 0; i < s->num_streams; i++)
    {
    if((w == s->stream_items[i]) &&
       gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(s->stream_items[i])))
      {
      *index = i;
      return 1;
      }
    }
  return 0;
  }

static void menu_callback(GtkWidget * w, gpointer data)
  {
  int stream_index;
  gmerlin_t * g;
  main_menu_t * the_menu;
  g = (gmerlin_t*)data;
  the_menu = g->player_window->main_menu;
  
  
  if(w == the_menu->options_menu.preferences)
    {
    //    fprintf(stderr, "Launching config dialog\n");
    gmerlin_configure(g);
    }
  else if(w == the_menu->options_menu.skins)
    {
    //    fprintf(stderr, "Launching config dialog\n");
    if(!g->skin_browser)
      g->skin_browser = gmerlin_skin_browser_create(g);
    gmerlin_skin_browser_show(g->skin_browser);
    }
  else if(w == the_menu->options_menu.plugins)
    {
    plugin_window_show(g->plugin_window);
    gtk_widget_set_sensitive(the_menu->options_menu.plugins, 0);
    }

  else if(w == the_menu->windows_menu.infowindow)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(the_menu->windows_menu.infowindow)))
      {
      bg_gtk_info_window_show(g->info_window);
      g->show_info_window = 1;
      }
    else
      {
      bg_gtk_info_window_hide(g->info_window);
      g->show_info_window = 0;
      }
    }
  else if(w == the_menu->windows_menu.logwindow)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(the_menu->windows_menu.logwindow)))
      {
      bg_gtk_log_window_show(g->log_window);
      g->show_log_window = 1;
      }
    else
      {
      bg_gtk_log_window_hide(g->log_window);
      g->show_log_window = 0;
      }
    }
  

  else if(w == the_menu->windows_menu.mediatree)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(the_menu->windows_menu.mediatree)))
      {
      bg_gtk_tree_window_show(g->tree_window);
      g->show_tree_window = 1;
      }
    else
      {
      bg_gtk_tree_window_hide(g->tree_window);
      g->show_tree_window = 0;
      }
    }
  else if(w == the_menu->accessories_menu.visualizer)
    {
    system("gmerlin_visualizer_launcher");
    }
  else if(w == the_menu->accessories_menu.mixer)
    {
    system("gmerlin_alsamixer &");
    }
  else if(w == the_menu->accessories_menu.transcoder)
    {
    system("gmerlin_transcoder_remote -launch");
    }
  else if(w == the_menu->command_menu.inc_volume)
    {
    //    fprintf(stderr, "inc volume\n");
    bg_player_set_volume_rel(g->player, 1.0);
    }
  else if(w == the_menu->command_menu.dec_volume)
    {
    //    fprintf(stderr, "dec volume\n");
    bg_player_set_volume_rel(g->player, -1.0);
    }
  else if(w == the_menu->command_menu.seek_backward)
    {
    bg_player_seek_rel(g->player,   -2 * GAVL_TIME_SCALE );
    //    fprintf(stderr, "seek_backward\n");
    }
  else if(w == the_menu->command_menu.seek_forward)
    {
    bg_player_seek_rel(g->player,    2 * GAVL_TIME_SCALE );
    //    fprintf(stderr, "seek_forward\n");
    }
  else if(w == the_menu->command_menu.next)
    {
    bg_media_tree_next(g->tree, 1, g->shuffle_mode);
    gmerlin_play(g, BG_PLAY_FLAG_IGNORE_IF_STOPPED);

    //    fprintf(stderr, "next\n");
    }
  else if(w == the_menu->command_menu.previous)
    {
    bg_media_tree_previous(g->tree, 1, g->shuffle_mode);
    gmerlin_play(g, BG_PLAY_FLAG_IGNORE_IF_STOPPED);

    //    fprintf(stderr, "previous\n");
    }
  else if(w == the_menu->command_menu.seek_start)
    {
    //    fprintf(stderr, "seek_start\n");
    bg_player_seek(g->player, 0 );
    }
  else if(w == the_menu->command_menu.pause)
    {
    //    fprintf(stderr, "pause\n");
    bg_player_pause(g->player);
    }
  
  else if(stream_menu_has_widget(&the_menu->audio_stream_menu, w, &stream_index))
    {
    //    fprintf(stderr, "Select audio stream: %d\n", stream_index);
    bg_player_set_audio_stream(g->player, stream_index);
    }
  else if(stream_menu_has_widget(&the_menu->video_stream_menu, w, &stream_index))
    {
    //    fprintf(stderr, "Select video stream: %d\n", stream_index);
    bg_player_set_video_stream(g->player, stream_index);
    }
  else if(stream_menu_has_widget(&the_menu->subtitle_stream_menu, w, &stream_index))
    {
    //    fprintf(stderr, "Select subtitle stream: %d\n", stream_index);
    bg_player_set_subtitle_stream(g->player, stream_index);
    }
#if 0
  else
    fprintf(stderr, "Unhandled menu item\n");
#endif    
  }

static GtkWidget *
create_pixmap_item(const char * label, const char * pixmap,
                   gmerlin_t * gmerlin,
                   GtkWidget * menu)
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
                   (gpointer)gmerlin);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), ret);
  return ret;
  }

static GtkWidget * create_item(const char * label,
                               gmerlin_t * gmerlin,
                               GtkWidget * menu)
  {
  GtkWidget * ret;
  ret = gtk_menu_item_new_with_label(label);
  g_signal_connect(G_OBJECT(ret), "activate",
                   G_CALLBACK(menu_callback),
                   gmerlin);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), ret);
  return ret;
  }

static GtkWidget * create_toggle_item(const char * label,
                                      gmerlin_t * gmerlin,
                                      GtkWidget * menu, guint * id)
  {
  GtkWidget * ret;
  ret = gtk_check_menu_item_new_with_label(label);
  *id = g_signal_connect(G_OBJECT(ret), "toggled",
                   G_CALLBACK(menu_callback),
                   gmerlin);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), ret);
  return ret;
  }

static GtkWidget * create_stream_item(gmerlin_t * gmerlin,
                                      stream_menu_t * m,
                                      guint * id)
  {
  GtkWidget * ret;
  ret = gtk_radio_menu_item_new_with_label(m->group, "");
  m->group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(ret));
  
  *id = g_signal_connect(G_OBJECT(ret), "activate",
                         G_CALLBACK(menu_callback),
                         gmerlin);
  gtk_menu_shell_append(GTK_MENU_SHELL(m->menu), ret);
  return ret;
  }

static GtkWidget * create_submenu_item(const char * label,
                                       GtkWidget * child_menu,
                                       GtkWidget * parent_menu)
  {
  GtkWidget * ret;
  ret = gtk_menu_item_new_with_label(label);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(ret), child_menu);
  gtk_widget_show(ret);

  gtk_menu_shell_append(GTK_MENU_SHELL(parent_menu), ret);
  return ret;
  }

#if 0
typedef struct stream_menu_s
  {
  GSList * group;
  GtkWidget ** stream_items;
  guint * ids;

  GtkWidget * off_item;
  guint off_id;

  int num_streams;
  int streams_alloc;
  
  GtkWidget * menu;
  } stream_menu_t;
#endif

static void stream_menu_init(stream_menu_t * s, gmerlin_t * gmerlin)
  {
  s->menu = create_menu();
  s->off_item = gtk_radio_menu_item_new_with_label((GSList*)0, "Off");
  
  s->off_id = g_signal_connect(G_OBJECT(s->off_item), "activate",
                               G_CALLBACK(menu_callback),
                               gmerlin);
  
  s->group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(s->off_item));
  gtk_widget_show(s->off_item);
  gtk_menu_shell_append(GTK_MENU_SHELL(s->menu), s->off_item);
  }

static void stream_menu_set_num(gmerlin_t * g, stream_menu_t * s, int num)
  {
  int i;
  if(num > s->streams_alloc)
    {
    s->stream_items = realloc(s->stream_items, num * sizeof(*s->stream_items));
    s->ids = realloc(s->ids, num * sizeof(*s->ids));

    for(i = s->streams_alloc; i < num; i++)
      s->stream_items[i] = create_stream_item(g, s, &(s->ids[i]));
    s->streams_alloc = num;
    }
  for(i = 0; i < num; i++)
    gtk_widget_show(s->stream_items[i]);
  for(i = num; i < s->streams_alloc; i++)
    gtk_widget_hide(s->stream_items[i]);
  s->num_streams = num;
  }

static void stream_menu_set_index(stream_menu_t * s, int index)
  {
  int i;
  /* Block event handlers */
  g_signal_handler_block(G_OBJECT(s->off_item), s->off_id);
  for(i = 0; i < s->streams_alloc; i++)
    g_signal_handler_block(G_OBJECT(s->stream_items[i]), s->ids[i]);

  /* Select item */
  if(index == -1)
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(s->off_item), 1);
  else
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(s->stream_items[index]), 1);

  /* Unblock event handlers */
  g_signal_handler_unblock(G_OBJECT(s->off_item), s->off_id);
  for(i = 0; i < s->streams_alloc; i++)
    g_signal_handler_unblock(G_OBJECT(s->stream_items[i]), s->ids[i]);

  }

void
main_menu_set_audio_index(main_menu_t * m, int index)
  {
  stream_menu_set_index(&m->audio_stream_menu, index);
  }

void
main_menu_set_video_index(main_menu_t * m, int index)
  {
  stream_menu_set_index(&m->video_stream_menu, index);
  }

void
main_menu_set_subtitle_index(main_menu_t * m, int index)
  {
  stream_menu_set_index(&m->subtitle_stream_menu, index);
  }


void main_menu_set_num_streams(main_menu_t * m,
                               int audio_streams,
                               int video_streams,
                               int subtitle_streams)
  {
  if(!audio_streams)
    gtk_widget_set_sensitive(m->audio_stream_item, 0);
  else
    gtk_widget_set_sensitive(m->audio_stream_item, 1);
  stream_menu_set_num(m->g, &m->audio_stream_menu, audio_streams);

  if(!video_streams)
    gtk_widget_set_sensitive(m->video_stream_item, 0);
  else
    gtk_widget_set_sensitive(m->video_stream_item, 1);
  stream_menu_set_num(m->g, &m->video_stream_menu, video_streams);
  
  if(!subtitle_streams)
    gtk_widget_set_sensitive(m->subtitle_stream_item, 0);
  else
    gtk_widget_set_sensitive(m->subtitle_stream_item, 1);
  stream_menu_set_num(m->g, &m->subtitle_stream_menu, subtitle_streams);
  }

void main_menu_set_audio_info(main_menu_t * m, int stream,
                              const char * info,
                              const char * language)
  {
  char * label;
  GtkWidget * w;
  if(info && language)
    label = bg_sprintf("%s [%s]", info, bg_get_language_name(language));
  else if(info)
    label = bg_sprintf("%s", info);
  else if(language)
    label = bg_sprintf("Stream %d [%s]", stream+1, bg_get_language_name(language));
  else
    label = bg_sprintf("Stream %d", stream+1);

  w = m->audio_stream_menu.stream_items[stream];
  gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(w))), label);
  free(label);
  }

void main_menu_set_video_info(main_menu_t * m, int stream,
                              const char * info,
                              const char * language)
  {
  char * label;
  GtkWidget * w;
  if(info && language)
    label = bg_sprintf("%s [%s]", info, bg_get_language_name(language));
  else if(info)
    label = bg_sprintf("%s", info);
  else if(language)
    label = bg_sprintf("Stream %d [%s]", stream+1, bg_get_language_name(language));
  else
    label = bg_sprintf("Stream %d", stream+1);
  
  w = m->video_stream_menu.stream_items[stream];
  gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(w))), label);

  free(label);

  }

void main_menu_set_subtitle_info(main_menu_t * m, int stream,
                                 const char * info,
                                 const char * language)
  {
  char * label;
  GtkWidget * w;
  if(info && language && *language)
    label = bg_sprintf("%s [%s]", info, bg_get_language_name(language));
  else if(info)
    label = bg_sprintf("%s", info);
  else if(language && *language)
    label = bg_sprintf("Stream %d [%s]", stream+1, bg_get_language_name(language));
  else
    label = bg_sprintf("Stream %d", stream+1);
  
  w = m->subtitle_stream_menu.stream_items[stream];
  gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(w))), label);
  free(label);
  
  }


main_menu_t * main_menu_create(gmerlin_t * gmerlin)
  {
  main_menu_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->g = gmerlin;
  /* Windows */
    
  ret->windows_menu.menu = create_menu();
  ret->windows_menu.mediatree =
    create_toggle_item("Media Tree", gmerlin, ret->windows_menu.menu,
                       &ret->windows_menu.mediatree_id);
  ret->windows_menu.infowindow =
    create_toggle_item("Info window", gmerlin, ret->windows_menu.menu,
                       &ret->windows_menu.infowindow_id);
  ret->windows_menu.logwindow =
    create_toggle_item("Log window", gmerlin, ret->windows_menu.menu,
                       &ret->windows_menu.logwindow_id);
  gtk_widget_show(ret->windows_menu.menu);

  /* Streams */

  stream_menu_init(&ret->audio_stream_menu, gmerlin);
  stream_menu_init(&ret->video_stream_menu, gmerlin);
  stream_menu_init(&ret->subtitle_stream_menu, gmerlin);
  
  /* Options */
  
  ret->options_menu.menu = create_menu();
  ret->options_menu.preferences =
    create_pixmap_item("Preferences...", "config_16.png", gmerlin, ret->options_menu.menu);

  gtk_widget_add_accelerator(ret->options_menu.preferences, "activate", ret->g->accel_group,
                             GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  

  ret->options_menu.plugins =
    create_pixmap_item("Plugins...", "plugin_16.png", gmerlin, ret->options_menu.menu);
  gtk_widget_add_accelerator(ret->options_menu.plugins, "activate", ret->g->accel_group,
                             GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  ret->options_menu.skins =
    create_item("Skins...", gmerlin, ret->options_menu.menu);

  /* Commands */
  ret->command_menu.menu = create_menu();
  ret->command_menu.inc_volume =
    create_item("Increase volume", gmerlin, ret->command_menu.menu);
  gtk_widget_add_accelerator(ret->command_menu.inc_volume, "activate", ret->g->player_window->accel_group,
                             GDK_Up, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  ret->command_menu.dec_volume =
    create_item("Decrease volume", gmerlin, ret->command_menu.menu);
  gtk_widget_add_accelerator(ret->command_menu.dec_volume, "activate", ret->g->player_window->accel_group,
                             GDK_Down, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  ret->command_menu.seek_forward =
    create_item("Seek forward", gmerlin, ret->command_menu.menu);
  gtk_widget_add_accelerator(ret->command_menu.seek_forward, "activate", ret->g->player_window->accel_group,
                             GDK_Right, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  ret->command_menu.seek_backward =
    create_item("Seek backward", gmerlin, ret->command_menu.menu);
  gtk_widget_add_accelerator(ret->command_menu.seek_backward, "activate", ret->g->player_window->accel_group,
                             GDK_Left, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  ret->command_menu.next =
    create_item("Next track", gmerlin, ret->command_menu.menu);
  gtk_widget_add_accelerator(ret->command_menu.next, "activate", ret->g->player_window->accel_group,
                             GDK_Page_Down, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  ret->command_menu.previous =
    create_item("Previous track", gmerlin, ret->command_menu.menu);
  gtk_widget_add_accelerator(ret->command_menu.previous, "activate", ret->g->player_window->accel_group,
                             GDK_Page_Up, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  ret->command_menu.seek_start =
    create_item("Seek to start", gmerlin, ret->command_menu.menu);
  gtk_widget_add_accelerator(ret->command_menu.seek_start, "activate", ret->g->player_window->accel_group,
                             GDK_0, 0, GTK_ACCEL_VISIBLE);

  ret->command_menu.pause =
    create_item("Pause", gmerlin, ret->command_menu.menu);
  gtk_widget_add_accelerator(ret->command_menu.pause, "activate", ret->g->player_window->accel_group,
                             GDK_space, 0, GTK_ACCEL_VISIBLE);

  /* Accessories */

  ret->accessories_menu.menu = create_menu();

  if(bg_search_file_exec("gmerlin_transcoder", (char**)0))
    ret->accessories_menu.transcoder =
      create_item("Transcoder", gmerlin, ret->accessories_menu.menu);
  else
    fprintf(stderr, "gmerlin_transcoder not found\n");

  if(bg_search_file_exec("gmerlin_visualizer", (char**)0))
    ret->accessories_menu.visualizer =
      create_item("Visualizer", gmerlin, ret->accessories_menu.menu);
  else
    fprintf(stderr, "gmerlin_visualizer not found\n");

  if(bg_search_file_exec("gmerlin_alsamixer", (char**)0))
    ret->accessories_menu.mixer =
      create_item("Mixer", gmerlin, ret->accessories_menu.menu);
  else
    fprintf(stderr, "gmerlin_alsamixer not found\n");

  
  /* Main menu */
    
  ret->menu = create_menu();

  ret->audio_stream_item = create_submenu_item("Audio...",
                                               ret->audio_stream_menu.menu,
                                               ret->menu);

  ret->video_stream_item = create_submenu_item("Video...",
                                               ret->video_stream_menu.menu,
                                               ret->menu);

  ret->subtitle_stream_item = create_submenu_item("Subtitles...",
                                                  ret->subtitle_stream_menu.menu,
                                                  ret->menu);
    
  ret->windows_item = create_submenu_item("Windows...",
                                          ret->windows_menu.menu,
                                          ret->menu);

  ret->options_item = create_submenu_item("Options...",
                                          ret->options_menu.menu,
                                          ret->menu);

  ret->options_item = create_submenu_item("Commands...",
                                          ret->command_menu.menu,
                                          ret->menu);

  ret->accessories_item = create_submenu_item("Accessories...",
                                              ret->accessories_menu.menu,
                                              ret->menu);
  gtk_widget_show(ret->menu);

  
  
  return ret;
  }

void main_menu_destroy(main_menu_t * m)
  {
  free(m);
  }

GtkWidget * main_menu_get_widget(main_menu_t * m)
  {
  return m->menu;
  }

void main_menu_set_tree_window_item(main_menu_t * m, int state)
  {
  g_signal_handler_block(G_OBJECT(m->windows_menu.mediatree), m->windows_menu.mediatree_id);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m->windows_menu.mediatree), state);
  g_signal_handler_unblock(G_OBJECT(m->windows_menu.mediatree), m->windows_menu.mediatree_id);
  
  }

void main_menu_set_info_window_item(main_menu_t * m, int state)
  {
  g_signal_handler_block(G_OBJECT(m->windows_menu.infowindow), m->windows_menu.infowindow_id);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m->windows_menu.infowindow), state);
  g_signal_handler_unblock(G_OBJECT(m->windows_menu.infowindow), m->windows_menu.infowindow_id);
  }

void main_menu_set_log_window_item(main_menu_t * m, int state)
  {
  g_signal_handler_block(G_OBJECT(m->windows_menu.logwindow), m->windows_menu.logwindow_id);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m->windows_menu.logwindow), state);
  g_signal_handler_unblock(G_OBJECT(m->windows_menu.logwindow), m->windows_menu.logwindow_id);
  }

void main_menu_set_plugin_window_item(main_menu_t * m, int state)
  {
  gtk_widget_set_sensitive(m->options_menu.plugins, !state);
  }
