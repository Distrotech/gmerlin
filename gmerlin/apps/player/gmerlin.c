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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/keycodes.h>
 
#include "gmerlin.h"
#include "player_remote.h"

#include <gmerlin/utils.h>
#include <gui_gtk/auth.h>
#include <gui_gtk/gtkutils.h>



static bg_accelerator_t accels[] =
  {
    { BG_KEY_LEFT,    BG_KEY_SHIFT_MASK,                       ACCEL_VOLUME_DOWN             },
    { BG_KEY_RIGHT,   BG_KEY_SHIFT_MASK,                       ACCEL_VOLUME_UP               },
    { BG_KEY_LEFT,    BG_KEY_CONTROL_MASK,                     ACCEL_SEEK_BACKWARD           },
    { BG_KEY_RIGHT,   BG_KEY_CONTROL_MASK,                     ACCEL_SEEK_FORWARD            },
    { BG_KEY_0,       0,                                       ACCEL_SEEK_START              },
    { BG_KEY_SPACE,   0,                                       ACCEL_PAUSE                   },
    { BG_KEY_M,       0,                                       ACCEL_MUTE                    },
    { BG_KEY_PAGE_UP, BG_KEY_CONTROL_MASK|BG_KEY_SHIFT_MASK,   ACCEL_NEXT_CHAPTER            },
    { BG_KEY_PAGE_DOWN, BG_KEY_CONTROL_MASK|BG_KEY_SHIFT_MASK, ACCEL_PREV_CHAPTER            },
 
    { BG_KEY_PAGE_DOWN, BG_KEY_CONTROL_MASK,                   ACCEL_NEXT                    },
    { BG_KEY_PAGE_UP,   BG_KEY_CONTROL_MASK,                   ACCEL_PREV                    },
    { BG_KEY_q,         BG_KEY_CONTROL_MASK,                   ACCEL_QUIT                    },
    { BG_KEY_o,         BG_KEY_CONTROL_MASK,                   ACCEL_OPTIONS                 },
    { BG_KEY_g,         BG_KEY_CONTROL_MASK,                   ACCEL_GOTO_CURRENT            },
    { BG_KEY_F9,        0,                                     ACCEL_CURRENT_TO_FAVOURITES   },
    { BG_KEY_NONE,      0,                                     0                             },
  };

static void tree_play_callback(void * data)
  {
  gmerlin_t * g = (gmerlin_t*)data;
  
  gmerlin_play(g, 0);
  }


static void gmerlin_apply_config(gmerlin_t * g)
  {
  const bg_parameter_info_t * parameters;

  parameters = display_get_parameters(g->player_window->display);

  bg_cfg_section_apply(g->display_section, parameters,
                       display_set_parameter, (void*)(g->player_window->display));
#if 0
  parameters = bg_media_tree_get_parameters(g->tree);
  bg_cfg_section_apply(g->tree_section, parameters,
                       bg_media_tree_set_parameter, (void*)(g->tree));
#endif
  parameters = bg_remote_server_get_parameters(g->remote);
  bg_cfg_section_apply(g->remote_section, parameters,
                       bg_remote_server_set_parameter, (void*)(g->remote));

  parameters = bg_player_get_audio_parameters(g->player);
  
  bg_cfg_section_apply(g->audio_section, parameters,
                       bg_player_set_audio_parameter, (void*)(g->player));

  parameters = bg_player_get_audio_filter_parameters(g->player);
  
  bg_cfg_section_apply(g->audio_filter_section, parameters,
                       bg_player_set_audio_filter_parameter, (void*)(g->player));

  parameters = bg_player_get_video_parameters(g->player);
  
  bg_cfg_section_apply(g->video_section, parameters,
                       bg_player_set_video_parameter, (void*)(g->player));

  parameters = bg_player_get_video_filter_parameters(g->player);
  
  bg_cfg_section_apply(g->video_filter_section, parameters,
                       bg_player_set_video_filter_parameter, (void*)(g->player));

  
  parameters = bg_player_get_subtitle_parameters(g->player);
  
  bg_cfg_section_apply(g->subtitle_section, parameters,
                       bg_player_set_subtitle_parameter, (void*)(g->player));

  parameters = bg_player_get_osd_parameters(g->player);
  
  bg_cfg_section_apply(g->osd_section, parameters,
                       bg_player_set_osd_parameter, (void*)(g->player));

  parameters = bg_player_get_input_parameters(g->player);
  
  bg_cfg_section_apply(g->input_section, parameters,
                       bg_player_set_input_parameter, (void*)(g->player));
  
  parameters = gmerlin_get_parameters(g);

  bg_cfg_section_apply(g->general_section, parameters,
                       gmerlin_set_parameter, (void*)(g));

  parameters = bg_lcdproc_get_parameters(g->lcdproc);
  bg_cfg_section_apply(g->lcdproc_section, parameters,
                       bg_lcdproc_set_parameter, (void*)(g->lcdproc));

  parameters = bg_gtk_log_window_get_parameters(g->log_window);
  bg_cfg_section_apply(g->logwindow_section, parameters,
                       bg_gtk_log_window_set_parameter, (void*)(g->log_window));
  parameters = bg_gtk_info_window_get_parameters(g->info_window);
  bg_cfg_section_apply(g->infowindow_section, parameters,
                       bg_gtk_info_window_set_parameter, (void*)(g->info_window));

  parameters = bg_player_get_visualization_parameters(g->player);
  bg_cfg_section_apply(g->visualization_section, parameters,
                       bg_player_set_visualization_parameter, (void*)(g->player));

  
  }

static void gmerlin_get_config(gmerlin_t * g)
  {
  const bg_parameter_info_t * parameters;
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

  parameters = bg_player_get_audio_filter_parameters(g->player);
  
  bg_cfg_section_apply(g->audio_filter_section, parameters,
                       bg_player_set_audio_filter_parameter, (void*)(g->player));

  parameters = bg_player_get_video_parameters(g->player);
  
  bg_cfg_section_apply(g->video_section, parameters,
                       bg_player_set_video_parameter, (void*)(g->player));

  parameters = bg_player_get_video_filter_parameters(g->player);
  
  bg_cfg_section_apply(g->video_filter_section, parameters,
                       bg_player_set_video_filter_parameter,
                       (void*)(g->player));
  
  parameters = bg_player_get_subtitle_parameters(g->player);
  
  bg_cfg_section_apply(g->subtitle_section, parameters,
                       bg_player_set_subtitle_parameter, (void*)(g->player));
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

static void logwindow_close_callback(bg_gtk_log_window_t * w, void * data)
  {
  gmerlin_t * g;
  g = (gmerlin_t*)data;
  main_menu_set_log_window_item(g->player_window->main_menu, 0);
  g->show_log_window = 0;
  }

static void treewindow_close_callback(bg_gtk_tree_window_t * win,
                               void * data)
  {
  gmerlin_t * g;
  g = (gmerlin_t*)data;
  main_menu_set_tree_window_item(g->player_window->main_menu, 0);
  g->show_tree_window = 0;
  }

static const bg_parameter_info_t input_plugin_parameters[] =
  {
    {
      .name = "input_plugins",
      .long_name = "Input plugins",
    },
    { /* */ },
  };

static const bg_parameter_info_t image_reader_parameters[] =
  {
    {
      .name = "image_readers",
      .long_name = "Image readers",
    },
    { /* */ },
  };

gmerlin_t * gmerlin_create(bg_cfg_registry_t * cfg_reg)
  {
  int remote_port;
  char * remote_env;
  
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
  ret->input_section =
    bg_cfg_registry_find_section(cfg_reg, "Input");
  ret->audio_section =
    bg_cfg_registry_find_section(cfg_reg, "Audio");
  ret->audio_filter_section =
    bg_cfg_registry_find_section(cfg_reg, "AudioFilter");
  ret->video_section =
    bg_cfg_registry_find_section(cfg_reg, "Video");
  ret->video_filter_section =
    bg_cfg_registry_find_section(cfg_reg, "VideoFilter");
  ret->subtitle_section =
    bg_cfg_registry_find_section(cfg_reg, "Subtitles");
  ret->osd_section =
    bg_cfg_registry_find_section(cfg_reg, "OSD");
  ret->lcdproc_section =
    bg_cfg_registry_find_section(cfg_reg, "LCDproc");
  ret->remote_section =
    bg_cfg_registry_find_section(cfg_reg, "Remote");
  ret->logwindow_section =
    bg_cfg_registry_find_section(cfg_reg, "Logwindow");
  ret->infowindow_section =
    bg_cfg_registry_find_section(cfg_reg, "Infowindow");
  ret->visualization_section =
    bg_cfg_registry_find_section(cfg_reg, "Visualization");

  ret->input_plugin_parameters = bg_parameter_info_copy_array(input_plugin_parameters);
  bg_plugin_registry_set_parameter_info_input(ret->plugin_reg,
                                              BG_PLUGIN_INPUT,
                                              BG_PLUGIN_FILE|
                                              BG_PLUGIN_URL|
                                              BG_PLUGIN_REMOVABLE|
                                              BG_PLUGIN_TUNER,
                                              ret->input_plugin_parameters);
  
  ret->image_reader_parameters = bg_parameter_info_copy_array(image_reader_parameters);
  bg_plugin_registry_set_parameter_info_input(ret->plugin_reg,
                                              BG_PLUGIN_IMAGE_READER,
                                              BG_PLUGIN_FILE,
                                              ret->image_reader_parameters);
  
  /* Log window should be created quite early so we can catch messages
     during startup */

  ret->log_window = bg_gtk_log_window_create(logwindow_close_callback, 
                                             ret, TR("Gmerlin player"));

  bg_cfg_section_apply(ret->logwindow_section,
                       bg_gtk_log_window_get_parameters(ret->log_window),
                       bg_gtk_log_window_set_parameter,
                       (void*)ret->log_window);
  
  /* Create player instance */
  
  ret->player = bg_player_create(ret->plugin_reg);
  bg_player_add_accelerators(ret->player, accels);
  
  /* Create media tree */

  tmp_string = bg_search_file_write("player/tree", "tree.xml");
  
  if(!tmp_string)
    {
    goto fail;
    }
  
  ret->tree = bg_media_tree_create(tmp_string, ret->plugin_reg);
  free(tmp_string);
  
  bg_media_tree_set_play_callback(ret->tree, tree_play_callback, ret);
  bg_media_tree_set_userpass_callback(ret->tree, bg_gtk_get_userpass, NULL);

  /* Apply tree config */
  bg_cfg_section_apply(ret->tree_section, bg_media_tree_get_parameters(ret->tree),
                       bg_media_tree_set_parameter, (void*)(ret->tree));

  bg_media_tree_init(ret->tree);
  
  /* Start creating the GUI */

  ret->accel_group = gtk_accel_group_new();
  
  ret->tree_window = bg_gtk_tree_window_create(ret->tree,
                                               treewindow_close_callback,
                                               ret, ret->accel_group);
  
  /* Create player window */
    
  player_window_create(ret);
  
  //  gmerlin_skin_load(&(ret->skin), "Default");
  //  gmerlin_skin_set(ret);

  /* Create subwindows */

  ret->info_window = bg_gtk_info_window_create(ret->player,
                                               infowindow_close_callback, 
                                               ret);

  
  
  ret->lcdproc = bg_lcdproc_create(ret->player);

  remote_port = PLAYER_REMOTE_PORT;
  remote_env = getenv(PLAYER_REMOTE_ENV);
  if(remote_env)
    remote_port = atoi(remote_env);
  
  ret->remote = bg_remote_server_create(remote_port, PLAYER_REMOTE_ID);
  
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
    
  player_window_destroy(g->player_window);

  /* Must destroy the dialogs early, because the
     destructors might reference parameter infos,
     which belong to other modules */
  bg_dialog_destroy(g->cfg_dialog);
  bg_dialog_destroy(g->audio_dialog);
  bg_dialog_destroy(g->audio_filter_dialog);
  bg_dialog_destroy(g->video_dialog);
  bg_dialog_destroy(g->video_filter_dialog);
  bg_dialog_destroy(g->subtitle_dialog);
  bg_dialog_destroy(g->visualization_dialog);
  
  bg_lcdproc_destroy(g->lcdproc);
  bg_remote_server_destroy(g->remote);
    
  bg_player_destroy(g->player);

  
  bg_gtk_tree_window_destroy(g->tree_window);

  bg_parameter_info_destroy_array(g->input_plugin_parameters);
  bg_parameter_info_destroy_array(g->image_reader_parameters);
  
  /* Fetch parameters */
  
  bg_cfg_section_get(g->infowindow_section,
                     bg_gtk_info_window_get_parameters(g->info_window),
                     bg_gtk_info_window_get_parameter,
                     (void*)(g->info_window));
    
  bg_gtk_info_window_destroy(g->info_window);
  
  bg_cfg_section_get(g->logwindow_section,
                     bg_gtk_log_window_get_parameters(g->log_window),
                     bg_gtk_log_window_get_parameter,
                     (void*)(g->log_window));
  

  bg_gtk_log_window_destroy(g->log_window);

  
  bg_media_tree_destroy(g->tree);


  bg_plugin_registry_destroy(g->plugin_reg);

  
  gmerlin_skin_destroy(&(g->skin));
  
  free(g->skin_dir);
  
  free(g);
  
  }

void gmerlin_run(gmerlin_t * g, char ** locations)
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
  if(g->show_log_window)
    {
    bg_gtk_log_window_show(g->log_window);
    main_menu_set_log_window_item(g->player_window->main_menu, 1);
    }
  else
    {
    main_menu_set_log_window_item(g->player_window->main_menu, 0);
    }


  bg_player_run(g->player);
  
  player_window_show(g->player_window);

  if(locations)
    gmerlin_play_locations(g, locations);
  
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

int gmerlin_play(gmerlin_t * g, int flags)
  {
  int track_index;
  bg_plugin_handle_t * handle;
  bg_album_t * album;
  gavl_time_t duration_before;
  gavl_time_t duration_current;
  gavl_time_t duration_after;

  /* Tell the player that we want to change */
  bg_player_change(g->player, flags);
  handle = bg_media_tree_get_current_track(g->tree, &track_index);
  
  if(!handle)
    {
    bg_player_error(g->player);
    return 0;
    }
  
  album = bg_media_tree_get_current_album(g->tree);
  
  bg_album_get_times(album,
                     &duration_before,
                     &duration_current,
                     &duration_after);

  
  display_set_playlist_times(g->player_window->display,
                             duration_before,
                             duration_current,
                             duration_after);

  
  
  bg_player_play(g->player, handle, track_index,
                flags, bg_media_tree_get_current_track_name(g->tree));

  /* Unref the handle, we don't need it any longer here */
  bg_plugin_unref(handle);
    
  return 1;
  }

void gmerlin_pause(gmerlin_t * g)
  {
  if(g->player_state == BG_PLAYER_STATE_STOPPED)
    gmerlin_play(g, BG_PLAY_FLAG_INIT_THEN_PAUSE);
  else
    bg_player_pause(g->player);
  }

void gmerlin_next_track(gmerlin_t * g)
  {
  int result, keep_going, removable;
  bg_album_t * album;

  if(g->playback_flags & PLAYBACK_NOADVANCE)
    {
    bg_player_stop(g->player);
    return;
    }

  album = bg_media_tree_get_current_album(g->tree);
  if(!album)
    return;

  removable = (bg_album_get_type(album) == BG_ALBUM_TYPE_REMOVABLE) ? 1 : 0;

  result = 1;
  keep_going = 1;
  while(keep_going)
    {
    switch(g->repeat_mode)
      {
      case REPEAT_MODE_NONE:
        if(bg_media_tree_next(g->tree, 0, g->shuffle_mode))
          {
          result = gmerlin_play(g, BG_PLAY_FLAG_IGNORE_IF_PLAYING);
          if(result)
            keep_going = 0;
          }
        else
          {
          bg_player_stop(g->player);
          keep_going = 0;
          }
        break;
      case REPEAT_MODE_1:
        result = gmerlin_play(g, BG_PLAY_FLAG_IGNORE_IF_PLAYING);
        if(!result)
          bg_player_stop(g->player);
        keep_going = 0;
        break;
      case REPEAT_MODE_ALL:
        if(!bg_media_tree_next(g->tree, 1, g->shuffle_mode))
          {
          bg_player_stop(g->player);
          keep_going = 0;
          }
        else
          {
          result = gmerlin_play(g, BG_PLAY_FLAG_IGNORE_IF_PLAYING);
          if(result)
            keep_going = 0;
          }
        break;
      case  NUM_REPEAT_MODES:
        break;
      }
    if(!result && (!(g->playback_flags & PLAYBACK_SKIP_ERROR) || removable))
      break;
    }
  
  }

void gmerlin_check_next_track(gmerlin_t * g, int track)
  {
  gavl_time_t duration_before, duration_current, duration_after;
  int result;
  bg_album_t * old_album, *new_album;
  bg_album_entry_t * current_entry;
  
  if(g->playback_flags & PLAYBACK_NOADVANCE)
    {
    bg_player_stop(g->player);
    return;
    }
  switch(g->repeat_mode)
    {
    case REPEAT_MODE_ALL:
    case REPEAT_MODE_NONE:
      old_album = bg_media_tree_get_current_album(g->tree);
      
      if(!bg_media_tree_next(g->tree,
                             (g->repeat_mode == REPEAT_MODE_ALL) ? 1 : 0,
                             g->shuffle_mode))
        {
        bg_player_stop(g->player);
        }

      new_album = bg_media_tree_get_current_album(g->tree);
      
      if(old_album != new_album)
        gmerlin_play(g, 0);
      else
        {
        current_entry = bg_album_get_current_entry(new_album);
        if(current_entry->index != track)
          gmerlin_play(g, 0);
        else
          {
          bg_album_get_times(new_album,
                             &duration_before,
                             &duration_current,
                             &duration_after);
          display_set_playlist_times(g->player_window->display,
                                     duration_before,
                                     duration_current,
                                     duration_after);
          }
        }
      break;
    case REPEAT_MODE_1:
      result = gmerlin_play(g, 0);
      if(!result)
        bg_player_stop(g->player);
      break;
    case NUM_REPEAT_MODES:
      break;
    }
  }


static const bg_parameter_info_t parameters[] =
  {
#if 0
    {
      .name =      "general_options",
      .long_name = TRS("General Options"),
      .type =      BG_PARAMETER_SECTION,
    },
#endif
    {
      .name =      "skip_error_tracks",
      .long_name = TRS("Skip error tracks"),
      .type =      BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("If a track cannot be opened, switch to the next one")
    },
    {
      .name =      "dont_advance",
      .long_name = TRS("Don't advance"),
      .type =      BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
    },
    {
      .name =      "shuffle_mode",
      .long_name = TRS("Shuffle mode"),
      .type =      BG_PARAMETER_STRINGLIST,
      .multi_names = (char const *[]){"off",
                             "current",
                             "all",
                             (char*)0 },
      .multi_labels = (char const *[]){TRS("Off"),
                              TRS("Current album"),
                              TRS("All open albums"),
                              (char*)0 },
      .val_default = { .val_str = "Off" }
    },
    {
      .name =        "show_tooltips",
      .long_name =   TRS("Show tooltips"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name =        "mainwin_x",
      .long_name =   "mainwin_x",
      .type =        BG_PARAMETER_INT,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 10 }
    },
    {
      .name =        "mainwin_y",
      .long_name =   "mainwin_y",
      .type =        BG_PARAMETER_INT,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 10 }
    },
    {
      .name =        "show_tree_window",
      .long_name =   "show_tree_window",
      .type =        BG_PARAMETER_CHECKBUTTON,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 1 }
    },
    {
      .name =        "show_info_window",
      .long_name =   "show_info_window",
      .type =        BG_PARAMETER_CHECKBUTTON,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 0 }
    },
    {
      .name =        "show_log_window",
      .long_name =   "show_log_window",
      .type =        BG_PARAMETER_CHECKBUTTON,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 0 }
    },
    {
      .name =        "volume",
      .long_name =   "Volume",
      .type =        BG_PARAMETER_FLOAT,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_min =     { .val_f = BG_PLAYER_VOLUME_MIN },
      .val_max =     { .val_f = 0.0 },
      .val_default = { .val_f = 0.0 },
    },
    {
      .name =        "skin_dir",
      .long_name =   "Skin Directory",
      .type =        BG_PARAMETER_DIRECTORY,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_str = GMERLIN_DATA_DIR"/skins/Default" },
    },
    { /* End of Parameters */ }
  };

const bg_parameter_info_t * gmerlin_get_parameters(gmerlin_t * g)
  {
  return parameters;
  }

void gmerlin_set_parameter(void * data, const char * name,
                           const bg_parameter_value_t * val)
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
  else if(!strcmp(name, "show_log_window"))
    {
    g->show_log_window = val->val_i;
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
    if(!strcmp(val->val_str, "off"))
      {
      g->shuffle_mode = BG_SHUFFLE_MODE_OFF;
      }
    else if(!strcmp(val->val_str, "current"))
      {
      g->shuffle_mode = BG_SHUFFLE_MODE_CURRENT;
      }
    else if(!strcmp(val->val_str, "all"))
      {
      g->shuffle_mode = BG_SHUFFLE_MODE_ALL;
      }
    }
  else if(!strcmp(name, "volume"))
    {
    bg_player_set_volume(g->player, val->val_f);
    g->player_window->volume = val->val_f;

    bg_gtk_slider_set_pos(g->player_window->volume_slider,
                          (g->player_window->volume - BG_PLAYER_VOLUME_MIN)/
                          (-BG_PLAYER_VOLUME_MIN));

    }
  else if(!strcmp(name, "show_tooltips"))
    {
    bg_gtk_set_tooltips(val->val_i);
    }
  else if(!strcmp(name, "skin_dir"))
    {
    g->skin_dir = bg_strdup(g->skin_dir, val->val_str);
    g->skin_dir = gmerlin_skin_load(&(g->skin), g->skin_dir);
    gmerlin_skin_set(g);
    }
  }

int gmerlin_get_parameter(void * data, const char * name, bg_parameter_value_t * val)
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
  else if(!strcmp(name, "show_log_window"))
    {
    val->val_i = g->show_log_window;
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
 

void gmerlin_add_locations(gmerlin_t * g, char ** locations)
  {
  int was_open;
  int i;
  bg_album_t * incoming;
  
  i = 0;
  while(locations[i])
    {
    i++;
    }
  
  incoming = bg_media_tree_get_incoming(g->tree);

  if(bg_album_is_open(incoming))
    {
    was_open = 1;
    }
  else
    {
    was_open = 0;
    bg_album_open(incoming);
    }

  bg_album_insert_urls_before(incoming,
                              locations,
                              (const char *)0,
                              (bg_album_entry_t *)0);

  if(!was_open)
    bg_album_close(incoming);
  }

void gmerlin_play_locations(gmerlin_t * g, char ** locations)
  {
  int old_num_tracks, new_num_tracks;
  int i;
  bg_album_t * incoming;
  bg_album_entry_t * entry;

  i = 0;
  while(locations[i])
    {
    i++;
    }

  bg_gtk_tree_window_open_incoming(g->tree_window);

  incoming = bg_media_tree_get_incoming(g->tree);

  old_num_tracks = bg_album_get_num_entries(incoming);

  gmerlin_add_locations(g, locations);

  new_num_tracks = bg_album_get_num_entries(incoming);

  if(new_num_tracks > old_num_tracks)
    {
    entry = bg_album_get_entry(incoming, old_num_tracks);
    bg_album_set_current(incoming, entry);
    bg_album_play(incoming);
    }
  }

static bg_album_t * open_device(gmerlin_t * g, char * device)
  {
  bg_album_t * album;
  album = bg_media_tree_get_device_album(g->tree, device);

  if(!album)
    return album;
  
  if(!bg_album_is_open(album))
    {
    bg_album_open(album);
    bg_gtk_tree_window_update(g->tree_window, 1);
    }
  return album;
  }


void gmerlin_open_device(gmerlin_t * g, char * device)
  {
  open_device(g, device);
  }

void gmerlin_play_device(gmerlin_t * g, char * device)
  {
  bg_album_t * album;
  bg_album_entry_t * entry;
  album = open_device(g, device);

  if(!album)
    return;
  
  entry = bg_album_get_entry(album, 0);

  if(!entry)
    return;
  
  bg_album_set_current(album, entry);
  bg_album_play(album);
  }
