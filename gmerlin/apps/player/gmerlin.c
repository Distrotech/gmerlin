/*****************************************************************
 
  gmerlin.c
 
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>

#include "gmerlin.h"

#include <utils.h>

static void set_logo(bg_plugin_registry_t * reg, bg_player_t * player)
  {
  gavl_video_format_t format;
  gavl_video_frame_t * frame;
  char * filename;
  filename = bg_search_file_read("icons", "gmerlin.jpg");

  if(!filename)
    return;
  if(!(frame = bg_plugin_registry_load_image(reg, filename, &format)))
    {
    free(filename);
    return;
    }
  free(filename);
  bg_player_set_logo(player, &format, frame);
  }

static void tree_play_callback(void * data)
  {
  gmerlin_t * g = (gmerlin_t*)data;
  
  gmerlin_play(g, 0);
  }

static void tree_error_callback(bg_media_tree_t * t, void * data, const char * message)
  {
  gmerlin_t * g = (gmerlin_t*)data;

  bg_player_error(g->player, message);
  }

static void gmerlin_apply_config(gmerlin_t * g)
  {
  bg_parameter_info_t * parameters;

  parameters = display_get_parameters(g->player_window->display);

  bg_cfg_section_apply(g->display_section, parameters,
                       display_set_parameter, (void*)(g->player_window->display));

  parameters = bg_media_tree_get_parameters(g->tree);
  bg_cfg_section_apply(g->tree_section, parameters,
                       bg_media_tree_set_parameter, (void*)(g->tree));

  parameters = bg_player_get_audio_parameters(g->player);
  
  bg_cfg_section_apply(g->audio_section, parameters,
                       bg_player_set_audio_parameter, (void*)(g->player));

  parameters = bg_player_get_video_parameters(g->player);
  
  bg_cfg_section_apply(g->video_section, parameters,
                       bg_player_set_video_parameter, (void*)(g->player));

  
  parameters = gmerlin_get_parameters(g);

  bg_cfg_section_apply(g->general_section, parameters,
                       gmerlin_set_parameter, (void*)(g));

  parameters = bg_lcdproc_get_parameters(g->lcdproc);
  bg_cfg_section_apply(g->lcdproc_section, parameters,
                       bg_lcdproc_set_parameter, (void*)(g->lcdproc));
  
  
  }

static void gmerlin_get_config(gmerlin_t * g)
  {
  bg_parameter_info_t * parameters;
#if 0
  parameters = display_get_parameters(g->player_window->display);

  bg_cfg_section_apply(g->display_section, parameters,
                       display_set_parameter, (void*)(g->player_window->display));
  parameters = bg_media_tree_get_parameters(g->tree);
  bg_cfg_section_apply(g->tree_section, parameters,
                       bg_media_tree_set_parameter, (void*)(g->tree));

  parameters = bg_player_get_audio_parameters(g->player);
  
  bg_cfg_section_apply(g->audio_section, parameters,
                       bg_player_set_audio_parameter, (void*)(g->player));

  parameters = bg_player_get_video_parameters(g->player);
  
  bg_cfg_section_apply(g->video_section, parameters,
                       bg_player_set_video_parameter, (void*)(g->player));
#endif

  parameters = gmerlin_get_parameters(g);

  bg_cfg_section_get(g->general_section, parameters,
                     gmerlin_get_parameter, (void*)(g));
  
  }

static void infowindow_close_callback(bg_gtk_info_window_t * w, void * data)
  {
  gmerlin_t * g;
  g = (gmerlin_t*)data;
  main_menu_set_info_window_item(g->player_window->main_menu, 0);
  g->show_info_window = 0;
  }

static void pluginwindow_close_callback(plugin_window_t * w, void * data)
  {
  gmerlin_t * g;
  g = (gmerlin_t*)data;
  main_menu_set_plugin_window_item(g->player_window->main_menu, 0);
  }

void treewindow_close_callback(bg_gtk_tree_window_t * win,
                               void * data)
  {
  gmerlin_t * g;
  g = (gmerlin_t*)data;
  main_menu_set_tree_window_item(g->player_window->main_menu, 0);
  g->show_tree_window = 0;
  }

gmerlin_t * gmerlin_create(bg_cfg_registry_t * cfg_reg)
  {
  bg_album_t * album;
  gavl_time_t duration_before, duration_current, duration_after;
  char * tmp_string;
    
  gmerlin_t * ret;
  bg_cfg_section_t * cfg_section;
  ret = calloc(1, sizeof(*ret));
  
  ret->cfg_reg = cfg_reg;
  
  /* Create plugin registry */
  cfg_section     = bg_cfg_registry_find_section(cfg_reg, "plugins");
  ret->plugin_reg = bg_plugin_registry_create(cfg_section);

  ret->display_section =
    bg_cfg_registry_find_section(cfg_reg, "Display");
  ret->tree_section =
    bg_cfg_registry_find_section(cfg_reg, "Tree");
  ret->general_section =
    bg_cfg_registry_find_section(cfg_reg, "General");
  ret->audio_section =
    bg_cfg_registry_find_section(cfg_reg, "Audio");
  ret->video_section =
    bg_cfg_registry_find_section(cfg_reg, "Video");
  ret->lcdproc_section =
    bg_cfg_registry_find_section(cfg_reg, "LCDproc");
    
  /* Create player instance */
  
  ret->player = bg_player_create();

  /* Set the Logo */

  set_logo(ret->plugin_reg, ret->player);
  
  /* Create media tree */

  tmp_string = bg_search_file_write("player/tree", "tree.xml");
  
  if(!tmp_string)
    {
    fprintf(stderr, "Cannot open media tree\n");
    goto fail;
    }
  
  ret->tree = bg_media_tree_create(tmp_string, ret->plugin_reg);

  bg_media_tree_set_play_callback(ret->tree, tree_play_callback, ret);
  bg_media_tree_set_error_callback(ret->tree, tree_error_callback, ret);
  
  free(tmp_string);
  
  ret->tree_window = bg_gtk_tree_window_create(ret->tree,
                                               treewindow_close_callback,
                                               ret);
  
  /* Create player window */
    
  ret->player_window = player_window_create(ret);
  
  //  gmerlin_skin_load(&(ret->skin), "Default");
  //  gmerlin_skin_set(ret);

  /* Create subwindows */

  ret->info_window = bg_gtk_info_window_create(ret->player,
                                               infowindow_close_callback, 
                                               ret);

  ret->plugin_window = plugin_window_create(ret,
                                            pluginwindow_close_callback, 
                                            ret);
  
  ret->lcdproc = bg_lcdproc_create(ret->player);
  
  gmerlin_create_dialog(ret);
  
  /* Set playlist times for the display */
  
  album = bg_media_tree_get_current_album(ret->tree);

  if(album)
    {
    bg_album_get_times(album,
                       &duration_before,
                       &duration_current,
                       &duration_after);
    display_set_playlist_times(ret->player_window->display,
                               duration_before,
                               duration_current,
                               duration_after);
    }

  
  return ret;
  fail:
  gmerlin_destroy(ret);
  return (gmerlin_t*)0;
  }

void gmerlin_destroy(gmerlin_t * g)
  {
  //  fprintf(stderr, "gmerlin_destroy...\n");
    
  plugin_window_destroy(g->plugin_window);
  player_window_destroy(g->player_window);

  bg_player_destroy(g->player);

  //  fprintf(stderr, "Blupp 1\n");
  
  bg_gtk_tree_window_destroy(g->tree_window);

  //  fprintf(stderr, "Blupp 2\n");

  bg_gtk_info_window_destroy(g->info_window);

  //  fprintf(stderr, "Blupp 3\n");
  
  bg_media_tree_destroy(g->tree);

  //  fprintf(stderr, "Blupp 4\n");

  bg_plugin_registry_destroy(g->plugin_reg);

  //  fprintf(stderr, "Blupp 5\n");
  
  gmerlin_skin_destroy(&(g->skin));

  //  fprintf(stderr, "Blupp 6\n");
  bg_dialog_destroy(g->cfg_dialog);

  //  fprintf(stderr, "Blupp 7\n");
  free(g);
  
  //  fprintf(stderr, "gmerlin_destroy done\n");
  }

void gmerlin_run(gmerlin_t * g)
  {
  gmerlin_apply_config(g);
  
  if(g->show_tree_window)
    {
    bg_gtk_tree_window_show(g->tree_window);
    main_menu_set_tree_window_item(g->player_window->main_menu, 1);
    
    }
  else
    main_menu_set_tree_window_item(g->player_window->main_menu, 0);
  
  if(g->show_info_window)
    {
    bg_gtk_info_window_show(g->info_window);
    main_menu_set_info_window_item(g->player_window->main_menu, 1);
    }
  else
    {
    main_menu_set_info_window_item(g->player_window->main_menu, 0);
    }
  bg_player_run(g->player);
  
  player_window_show(g->player_window);

  gtk_main();

  /* The following saves the coords */
  
  if(g->show_tree_window)
    bg_gtk_tree_window_hide(g->tree_window);
    

  bg_player_quit(g->player);

  gmerlin_get_config(g);

  }

void gmerlin_skin_destroy(gmerlin_skin_t * s)
  {
  if(s->directory)
    free(s->directory);
  player_window_skin_destroy(&(s->playerwindow));
  
  }

int gmerlin_play(gmerlin_t * g, int ignore_flags)
  {
  int track_index;
  bg_plugin_handle_t * handle;
  bg_album_t * album;
  gavl_time_t duration_before;
  gavl_time_t duration_current;
  gavl_time_t duration_after;
  //  fprintf(stderr, "gmerlin_play\n");
  handle = bg_media_tree_get_current_track(g->tree, &track_index);
  
  if(!handle)
    {
    //    fprintf(stderr, "Error playing file\n");
    return 0;
    }

  album = bg_media_tree_get_current_album(g->tree);
  
  bg_album_get_times(album,
                     &duration_before,
                     &duration_current,
                     &duration_after);

  //  fprintf(stderr, "Durations: %lld %lld %lld\n",
  //          duration_before,
  //          duration_current,
  //          duration_after);
  
  display_set_playlist_times(g->player_window->display,
                             duration_before,
                             duration_current,
                             duration_after);

  
  //  fprintf(stderr, "Track name: %s\n",
  //          bg_media_tree_get_current_track_name(g->tree));
  
  bg_player_play(g->player, handle, track_index,
                 ignore_flags, bg_media_tree_get_current_track_name(g->tree));
  
  return 1;
  }

void gmerlin_next_track(gmerlin_t * g)
  {
  int result, keep_going;
  
  if(g->playback_flags & PLAYBACK_NOADVANCE)
    {
    bg_player_stop(g->player);
    return;
    }
  
  result = 1;
  keep_going = 1;
  while(keep_going)
    {
    switch(g->repeat_mode)
      {
      case REPEAT_MODE_NONE:
        //      fprintf(stderr, "REPEAT_MODE_NONE\n");
        if(bg_media_tree_next(g->tree, 0, g->shuffle_mode))
          {
          result = gmerlin_play(g, BG_PLAYER_IGNORE_IF_PLAYING);
          if(result)
            keep_going = 0;
          }
        else
          {
          //          fprintf(stderr, "End of album, stopping\n");
          bg_player_stop(g->player);
          keep_going = 0;
          }
        break;
      case REPEAT_MODE_1:
        //      fprintf(stderr, "REPEAT_MODE_1\n");
        result = gmerlin_play(g, BG_PLAYER_IGNORE_IF_PLAYING);
        if(!result)
          bg_player_stop(g->player);
        keep_going = 0;
        break;
      case REPEAT_MODE_ALL:
        //        fprintf(stderr, "REPEAT_MODE_ALL\n");
        if(!bg_media_tree_next(g->tree, 1, g->shuffle_mode))
          {
          bg_player_stop(g->player);
          keep_going = 0;
          }
        else
          {
          result = gmerlin_play(g, BG_PLAYER_IGNORE_IF_PLAYING);
          if(result)
            keep_going = 0;
          }
        break;
      case  NUM_REPEAT_MODES:
        break;
      }
    if(!result && !(g->playback_flags & PLAYBACK_SKIP_ERROR))
      break;
    }
  
  }

static bg_parameter_info_t parameters[] =
  {
    {
      name:      "general_options",
      long_name: "General Options",
      type:      BG_PARAMETER_SECTION,
    },
    {
      name:      "skip_error_tracks",
      long_name: "Skip error tracks",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name:      "dont_advance",
      long_name: "Don't advance",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:      "shuffle_mode",
      long_name: "Shuffle mode",
      type:      BG_PARAMETER_STRINGLIST,
      multi_names: (char*[]){"Off",
                         "Current album",
                         "All open albums",
                         (char*)0 },
      val_default: { val_str: "Off" }
    },
    {
      name:        "show_tooltips",
      long_name:   "Show tooltips",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    {
      name:        "mainwin_x",
      long_name:   "mainwin_x",
      type:        BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 10 }
    },
    {
      name:        "mainwin_y",
      long_name:   "mainwin_y",
      type:        BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 10 }
    },
    {
      name:        "show_tree_window",
      long_name:   "show_tree_window",
      type:        BG_PARAMETER_CHECKBUTTON,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 1 }
    },
    {
      name:        "show_info_window",
      long_name:   "show_info_window",
      type:        BG_PARAMETER_CHECKBUTTON,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 0 }
    },
    {
      name:        "volume",
      long_name:   "Volume",
      type:        BG_PARAMETER_FLOAT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_min:     { val_f: VOLUME_MIN },
      val_max:     { val_f: VOLUME_MAX },
      val_default: { val_f: VOLUME_MAX },
    },
    {
      name:        "skin_dir",
      long_name:   "Skin Directory",
      type:        BG_PARAMETER_DIRECTORY,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_str: GMERLIN_DATA_DIR"/skins/Default" },
    },
    { /* End of Parameters */ }
  };

bg_parameter_info_t * gmerlin_get_parameters(gmerlin_t * g)
  {
  return parameters;
  }

void gmerlin_set_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  gmerlin_t * g = (gmerlin_t*)data;
  if(!name)
    return;

  if(!strcmp(name, "skip_error_tracks"))
    {
    if(val->val_i)
      g->playback_flags |= PLAYBACK_SKIP_ERROR;
    else
      g->playback_flags &= ~PLAYBACK_SKIP_ERROR;
    }
  else if(!strcmp(name, "dont_advance"))
    {
    if(val->val_i)
      g->playback_flags |= PLAYBACK_NOADVANCE;
    else
      g->playback_flags &= ~PLAYBACK_NOADVANCE;
    }
  else if(!strcmp(name, "show_tree_window"))
    {
    g->show_tree_window = val->val_i;
    }
  else if(!strcmp(name, "show_info_window"))
    {
    g->show_info_window = val->val_i;
    }
  else if(!strcmp(name, "mainwin_x"))
    {
    g->player_window->window_x = val->val_i;
    }
  else if(!strcmp(name, "mainwin_y"))
    {
    g->player_window->window_y = val->val_i;
    }
  else if(!strcmp(name, "shuffle_mode"))
    {
    if(!strcmp(val->val_str, "Off"))
      {
      g->shuffle_mode = BG_SHUFFLE_MODE_OFF;
      }
    else if(!strcmp(val->val_str, "Current album"))
      {
      g->shuffle_mode = BG_SHUFFLE_MODE_CURRENT;
      }
    else if(!strcmp(val->val_str, "All open albums"))
      {
      g->shuffle_mode = BG_SHUFFLE_MODE_ALL;
      }
    }
  else if(!strcmp(name, "volume"))
    {
    bg_player_set_volume(g->player, val->val_f);
    g->player_window->volume = val->val_f;

    bg_gtk_slider_set_pos(g->player_window->volume_slider,
                          (g->player_window->volume - VOLUME_MIN)/(VOLUME_MAX - VOLUME_MIN));

    }
  else if(!strcmp(name, "show_tooltips"))
    {
    gmerlin_set_tooltips(g, val->val_i);
    }
  else if(!strcmp(name, "skin_dir"))
    {
    g->skin_dir = bg_strdup(g->skin_dir, val->val_str);
    fprintf(stderr, "Skin Directory: %s\n", g->skin_dir);
    gmerlin_skin_load(&(g->skin), g->skin_dir);
    gmerlin_skin_set(g);
    }
  }

int gmerlin_get_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  gmerlin_t * g = (gmerlin_t*)data;
  if(!name)
    return 0;
  if(!strcmp(name, "show_tree_window"))
    {
    val->val_i = g->show_tree_window;
    return 1;
    }
  else if(!strcmp(name, "show_info_window"))
    {
    val->val_i = g->show_info_window;
    return 1;
    }
  else if(!strcmp(name, "mainwin_x"))
    {
    val->val_i = g->player_window->window_x;
    }
  else if(!strcmp(name, "mainwin_y"))
    {
    val->val_i = g->player_window->window_y;
    }
  else if(!strcmp(name, "volume"))
    {
    val->val_f = g->player_window->volume;
    }
  else if(!strcmp(name, "skin_dir"))
    {
    val->val_str = bg_strdup(val->val_str, g->skin_dir);
    }
  
  return 0;
  }
  
void gmerlin_set_tooltips(gmerlin_t * g, int enable)
  {
  bg_gtk_tree_window_set_tooltips(g->tree_window, enable);
  player_window_set_tooltips(g->player_window, enable);
  plugin_window_set_tooltips(g->plugin_window, enable);
  bg_dialog_set_tooltips(g->cfg_dialog, enable);
  
  }
