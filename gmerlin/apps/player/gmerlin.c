#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

static void tree_play_callback(bg_media_tree_t * t, void * data)
  {
  gmerlin_t * g = (gmerlin_t*)data;
  gmerlin_play(g, 0);
  }

static void tree_error_callback(bg_media_tree_t * t, void * data, const char * message)
  {
  gmerlin_t * g = (gmerlin_t*)data;

  bg_player_error(g->player, message);
  }

gmerlin_t * gmerlin_create(bg_cfg_registry_t * cfg_reg)
  {
  char * tmp_string;
  
  gmerlin_t * ret;
  bg_cfg_section_t * cfg_section;
  ret = calloc(1, sizeof(*ret));
  
  ret->cfg_reg = cfg_reg;
  
  /* Create plugin registry */
  cfg_section     = bg_cfg_registry_find_section(cfg_reg, "plugins");
  ret->plugin_reg = bg_plugin_registry_create(cfg_section);

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
                                               gmerlin_tree_close_callback,
                                               ret);

  /* Create player window */
    
  ret->player_window = player_window_create(ret);
  
  gmerlin_skin_load(&(ret->skin), "Default");
  gmerlin_skin_set(ret);

  gmerlin_create_dialog(ret);
  
  return ret;
  fail:
  gmerlin_destroy(ret);
  return (gmerlin_t*)0;
  }

void gmerlin_destroy(gmerlin_t * g)
  {
  plugin_window_destroy(g->plugin_window);
  player_window_destroy(g->player_window);
  
  bg_gtk_tree_window_destroy(g->tree_window);
  
  bg_media_tree_destroy(g->tree);
  bg_plugin_registry_destroy(g->plugin_reg);
  gmerlin_skin_destroy(&(g->skin));
  
  free(g);
  }

void gmerlin_run(gmerlin_t * g)
  {
  bg_player_run(g->player);
  
  bg_gtk_tree_window_show(g->tree_window);
  player_window_show(g->player_window);
  gtk_main();
  bg_player_quit(g->player);
  }

void gmerlin_skin_destroy(gmerlin_skin_t * s)
  {
  if(s->directory)
    free(s->directory);
  player_window_skin_destroy(&(s->playerwindow));
  
  }

void gmerlin_play(gmerlin_t * g, int ignore_flags)
  {
  int track_index;
  bg_plugin_handle_t * handle;
  
  handle = bg_media_tree_get_current_track(g->tree, &track_index);
  
  if(!handle)
    return;
  
  if(handle)
    bg_player_set_track_name(g->player,
                             bg_media_tree_get_current_track_name(g->tree));
  
  bg_player_play(g->player, handle, track_index,
                 ignore_flags);

  }

void gmerlin_next_track(gmerlin_t * g)
  {
  switch(g->repeat_mode)
    {
    case REPEAT_MODE_NONE:
      //      fprintf(stderr, "REPEAT_MODE_NONE\n");
      if(bg_media_tree_next(g->tree, 0))
        gmerlin_play(g, BG_PLAYER_IGNORE_IF_PLAYING);
      else
        {
        //        fprintf(stderr, "End of album, stopping\n");
        bg_player_stop(g->player);
        }
      break;
    case REPEAT_MODE_1:
      //      fprintf(stderr, "REPEAT_MODE_1\n");
      gmerlin_play(g, BG_PLAYER_IGNORE_IF_PLAYING);
      break;
    case REPEAT_MODE_ALL:
      //      fprintf(stderr, "REPEAT_MODE_ALL\n");
      bg_media_tree_next(g->tree, 1);
      gmerlin_play(g, BG_PLAYER_IGNORE_IF_PLAYING);
      break;
    case  NUM_REPEAT_MODES:
      break;
    }
  }


static bg_parameter_info_t parameters[] =
  {
    {
      name:      "playback_options",
      long_name: "Playback Options",
      type:      BG_PARAMETER_SECTION,
    },
    {
      name:      "mark_error_tracks",
      long_name: "Mark error tracks",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
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

  if(!strcmp(name, "mark_error_tracks"))
    {
    if(val->val_i)
      g->playback_flags |= PLAYBACK_MARK_ERROR;
    else
      g->playback_flags &= ~PLAYBACK_MARK_ERROR;
    }
  else if(!strcmp(name, "skip_error_tracks"))
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
  }
