#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>

bg_mozilla_t * gmerlin_mozilla_create()
  {
  bg_mozilla_t * ret;
  bg_cfg_section_t * cfg_section;
  char * tmp_path;
  char * old_locale;
  ret = calloc(1, sizeof(*ret));

  /* Set the LC_NUMERIC locale to "C" so we read floats right */
  old_locale = setlocale(LC_NUMERIC, "C");
  
  /* Create plugin regsitry */
  ret->cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("mozilla-plugin", "config.xml");
  bg_cfg_registry_load(ret->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  cfg_section = bg_cfg_registry_find_section(ret->cfg_reg, "plugins");
  ret->plugin_reg = bg_plugin_registry_create(cfg_section);

  setlocale(LC_NUMERIC, old_locale);

  
  /* Create the player before the plugin widget is created */
  
  ret->player = bg_player_create(ret->plugin_reg);

  bg_player_set_volume(ret->player, 0.0);
  
  ret->widget = bg_mozilla_widget_create(ret);
  ret->plugin_window =
    bg_mozilla_plugin_window_create(ret);

  ret->msg_queue = bg_msg_queue_create();
  
  bg_player_add_message_queue(ret->player,
                              ret->msg_queue);
  
  bg_player_run(ret->player);
  
  return ret;
  }

void gmerlin_mozilla_destroy(bg_mozilla_t* m)
  {
  char * tmp_path;
  char * old_locale;
  /* Shutdown player */
  bg_player_quit(m->player);
  bg_player_destroy(m->player);
  if(m->ov_handle) bg_plugin_unref(m->ov_handle);
  if(m->oa_handle) bg_plugin_unref(m->oa_handle);

  /* Set the LC_NUMERIC locale to "C" so we read floats right */
  old_locale = setlocale(LC_NUMERIC, "C");
  
  bg_plugin_registry_destroy(m->plugin_reg);

  
  /* Save configuration */
  tmp_path =  bg_search_file_write("mozilla-plugin", "config.xml");

  bg_cfg_registry_save(m->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  bg_cfg_registry_destroy(m->cfg_reg);

  setlocale(LC_NUMERIC, old_locale);
  
  /* Free strings */
  if(m->display_string) free(m->display_string);
  if(m->orig_url) free(m->orig_url);

  bg_mozilla_widget_destroy(m->widget);
  
  free(m);
  }

void gmerlin_mozilla_set_window(bg_mozilla_t* ret,
                                GdkNativeWindow window_id)
  {
  if(ret->window == window_id)
    return;
  ret->window = window_id;
  
  bg_mozilla_widget_set_window(ret->widget, window_id);
  }

void gmerlin_mozilla_set_oa_plugin(bg_mozilla_t * m,
                                   const bg_plugin_info_t * info)
  {
  if(m->oa_handle)
    bg_plugin_unref(m->oa_handle);
  m->oa_handle = bg_plugin_load(m->plugin_reg, info);
  bg_player_set_oa_plugin(m->player, m->oa_handle);
  fprintf(stderr, "Loaded OA %s\n", m->oa_handle->info->name);
  }

void gmerlin_mozilla_set_ov_plugin(bg_mozilla_t * m,
                                   const bg_plugin_info_t * info)
  {
  if(m->ov_handle)
    {
    bg_plugin_unref(m->ov_handle);
    m->ov_handle = NULL;
    }
  m->ov_info = info;
  
  if(m->display_string)
    {
    m->ov_handle = bg_ov_plugin_load(m->plugin_reg, m->ov_info,
                                     m->display_string);
    fprintf(stderr, "Loaded OV %s\n", m->ov_handle->info->name);
    bg_player_set_ov_plugin(m->player, m->ov_handle);
    }
  }

/* Append URL to list */

static void append_url(bg_mozilla_t * m, const char * url,
                       int depth, int * playing)
  {
  bg_input_plugin_t * input;
  int num_tracks, i;
  bg_track_info_t * ti;
  bg_plugin_handle_t * h = (bg_plugin_handle_t *)0;
  if(!bg_input_plugin_load(m->plugin_reg,
                           url,
                           (const bg_plugin_info_t *)0,
                           &h, (bg_input_callbacks_t*)0, 0))
    {
    fprintf(stderr, "Loading URL failed\n");
    return;
    }
  input = (bg_input_plugin_t *)h->plugin;

  if(!input->get_num_tracks)
    num_tracks = 1;
  else
    num_tracks = input->get_num_tracks(h->priv);
  for(i = 0; i < num_tracks; i++)
    {
    ti = input->get_track_info(h->priv, i);
    
    if(ti->url) /* Redirection */
      {
      fprintf(stderr, "%s is redirector to %s\n", url, ti->url);
      append_url(m, ti->url, depth+1, playing);
      }
    else
      {
      if(!(*playing))
        {
        bg_player_play(m->player, h,
                       0,
                       0, (ti->name ? ti->name : "Livestream"));
        *playing = 1;
        }
      fprintf(stderr, "%s is real stream\n", ti->url);
      }
    }
  bg_plugin_unref(h);
  }

void gmerlin_mozilla_set_url(bg_mozilla_t * m, const char * url)
  {
  int playing = 0;
  if(m->orig_url && !strcmp(m->orig_url, url))
    return;
  m->orig_url = bg_strdup(m->orig_url, url);
  fprintf(stderr, "Set URL: %s\n", m->orig_url);
  
  append_url(m, url, 0, &playing);
  
  }
