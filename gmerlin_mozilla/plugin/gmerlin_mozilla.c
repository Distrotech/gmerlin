/*****************************************************************
 * gmerlin-mozilla - A gmerlin based mozilla plugin
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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
#include <locale.h>

#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>

#define LOG_DOMAIN "gmerlin_mozilla"
#include <gmerlin/log.h>

static void infowindow_close_callback(bg_gtk_info_window_t * w, void * data)
  {
  bg_mozilla_t * m = data;
  gtk_widget_set_sensitive(m->widget->menu.url_menu.info, 1);
  }

bg_mozilla_t * gmerlin_mozilla_create()
  {
  bg_mozilla_t * ret;
  bg_cfg_section_t * cfg_section;
  char * tmp_path;
  char * old_locale;
  ret = calloc(1, sizeof(*ret));
  pthread_mutex_init(&ret->start_finished_mutex, NULL);
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
  bg_player_set_visualization(ret->player, 1);
  
  ret->plugin_window =
    bg_mozilla_plugin_window_create(ret);

  ret->msg_queue = bg_msg_queue_create();
  
  bg_player_add_message_queue(ret->player,
                              ret->msg_queue);
  
  ret->info_window = bg_gtk_info_window_create(ret->player,
                                               infowindow_close_callback, 
                                               ret);

  ret->widget = bg_mozilla_widget_create(ret);
  
  /* Create config sections */
  ret->gui_section =
    bg_cfg_registry_find_section(ret->cfg_reg, "GUI");
  ret->infowindow_section =
    bg_cfg_registry_find_section(ret->cfg_reg, "infowindow");
  ret->visualization_section =
    bg_cfg_registry_find_section(ret->cfg_reg, "visualize");
  ret->osd_section =
    bg_cfg_registry_find_section(ret->cfg_reg, "osd");

  ret->general_section =
    bg_cfg_registry_find_section(ret->cfg_reg, "general");
  ret->input_section =
    bg_cfg_registry_find_section(ret->cfg_reg, "input");
  
  /* Create config dialog */
  gmerlin_mozilla_create_dialog(ret);

  ret->player_state = BG_PLAYER_STATE_STOPPED;
  
  
  bg_player_run(ret->player);
  
  return ret;
  }

void gmerlin_mozilla_destroy(bg_mozilla_t* m)
  {
  const bg_parameter_info_t * parameters;
  char * tmp_path;
  char * old_locale;
  /* If the start thread is running, wait until it exits */
  if(m->state == STATE_STARTING)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Waiting for start thread...");
    pthread_join(m->start_thread, (void**)0);
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Waiting for start thread done");
    }
  /* Shutdown player */
  bg_player_quit(m->player);
  bg_player_destroy(m->player);
  if(m->ov_handle) bg_plugin_unref(m->ov_handle);
  if(m->oa_handle) bg_plugin_unref(m->oa_handle);

  if(m->input_handle) bg_plugin_unref(m->input_handle);
  
  /* Set the LC_NUMERIC locale to "C" so we read floats right */
  old_locale = setlocale(LC_NUMERIC, "C");
  
  bg_plugin_registry_destroy(m->plugin_reg);

  /* Get parameters */
  parameters = gmerlin_mozilla_get_parameters(m);
  
  bg_cfg_section_get(m->general_section, parameters,
                     gmerlin_mozilla_get_parameter, m);
  
  parameters = bg_gtk_info_window_get_parameters(m->info_window);
  bg_cfg_section_get(m->infowindow_section, parameters,
                     bg_gtk_info_window_get_parameter,
                     (void*)(m->info_window));

  
  /* Save configuration */
  tmp_path =  bg_search_file_write("mozilla-plugin", "config.xml");

  bg_cfg_registry_save(m->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  bg_cfg_registry_destroy(m->cfg_reg);

  setlocale(LC_NUMERIC, old_locale);
  
  /* Free strings */
  if(m->display_string) free(m->display_string);
  if(m->url)         free(m->url);
  if(m->uri)         free(m->uri);
  if(m->current_url) free(m->current_url);
  if(m->clipboard) bg_album_entries_destroy(m->clipboard);
  
  bg_gtk_info_window_destroy(m->info_window);
  
  bg_mozilla_widget_destroy(m->widget);
  bg_mozilla_embed_info_free(&m->ei);
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
  }

void gmerlin_mozilla_set_vis_plugin(bg_mozilla_t* m,
                                   const bg_plugin_info_t * info)
  {
  bg_player_set_visualization_plugin(m->player, info);
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
    bg_player_set_ov_plugin(m->player, m->ov_handle);
    }
  }


int gmerlin_mozilla_set_stream(bg_mozilla_t * m,
                               const char * url,
                               const char * mimetype, int64_t total_bytes)
  {
  if(m->player_state != BG_PLAYER_STATE_CHANGING &&
     m->player_state != BG_PLAYER_STATE_STOPPED)
    bg_mozilla_stop(m);
  
  //  if(m->url && !strcmp(url, m->url))
  //    return 0;
  
  m->url      = bg_strdup(m->url, url); 
  m->mimetype = bg_strdup(m->mimetype, mimetype);
  m->total_bytes = total_bytes;
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got URL: %s %s", m->url, m->mimetype);
  
  if(!strncmp(url, "file://", 7) || (url[0] == '/'))
    {
    m->url_mode = URL_MODE_LOCAL;
    }
  else
    {
    m->url_mode = URL_MODE_STREAM;
    m->buffer = bg_mozilla_buffer_create();
    }

  if(m->state == STATE_PLAYING)
    m->state = STATE_IDLE; /* Ready to be started again */
  
  return 1;
  }

static void do_play(bg_mozilla_t * m, const char * url, int track, bg_track_info_t * ti,
                    bg_plugin_handle_t * h)
  {
  m->current_url = bg_strdup(m->current_url, url);
  bg_player_play(m->player, h,
                 track,
                 0, (ti->name ? ti->name : "Livestream"));
  m->playing = 1;
  m->input_handle = h;
  bg_plugin_ref(m->input_handle);
  m->ti = ti;
  }

/* Append URL to list */

static int append_url(bg_mozilla_t * m, const char * url,
                       int depth)
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
    bg_log(BG_LOG_ERROR , LOG_DOMAIN, "Loading URL failed");
    return 0;
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
      if(depth > 5)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,
               "Too many redirections");
        bg_plugin_unref(h);
        return 0;
        }
      
      bg_log(BG_LOG_ERROR , LOG_DOMAIN,
             "%s is redirector to %s (depth: %d)",
             url, ti->url, depth);
      if(!append_url(m, ti->url, depth+1))
        {
        bg_plugin_unref(h);
        return 0;
        }
      }
    else
      {
      if(!(m->playing))
        {
        m->url_mode = URL_MODE_REDIRECT;
        do_play(m, url, i, ti, h);
        }
      }
    }
  bg_plugin_unref(h);
  return 1;
  }


static void * start_func(void * priv)
  {
  int num_tracks, i, num;
  const bg_plugin_info_t * info;
  bg_input_plugin_t * input;
  bg_track_info_t * ti;
  bg_mozilla_t * m = priv;
  bg_plugin_handle_t * h = (bg_plugin_handle_t *)0;
  m->playing = 0;

  /* Cleanup earlier playback */
  if(m->input_handle)
    {
    bg_plugin_unref(m->input_handle);
    m->input_handle = NULL;
    }
  m->ti = NULL;
  
  /* TODO: If we have more than one callback capable plugin
     (unlikely), we must to proper selection */

  num = bg_plugin_registry_get_num_plugins(m->plugin_reg,
                                           BG_PLUGIN_INPUT,
                                           BG_PLUGIN_CALLBACKS);
  
  if(!num)
    {
    goto fail;
    }
  info = bg_plugin_find_by_index(m->plugin_reg, 0,
                                 BG_PLUGIN_INPUT,
                                 BG_PLUGIN_CALLBACKS);
  if(!info)
    goto fail;
  
  h = bg_plugin_load(m->plugin_reg, info);
  if(!h)
    goto fail;

  if(!num)
    {
    goto fail;
    }
  
  input = (bg_input_plugin_t *)h->plugin;
  
  if(m->url_mode == URL_MODE_STREAM)
    {
    if(!input->open_callbacks)
      {
      bg_plugin_unref(h);
      goto fail;
      }
    
    if(!input->open_callbacks(h->priv,
                              bg_mozilla_buffer_read,
                              NULL, /* Seek callback */
                              m->buffer, m->url, m->mimetype, m->total_bytes))
      {
      bg_mozilla_widget_set_error(m->widget);
      goto fail;
      }
    h->location = bg_strdup(h->location, m->url);
    }
  else
    {
    if(!input->open(h->priv, m->url))
      {
      bg_mozilla_widget_set_error(m->widget);
      goto fail;
      }
    }
  
  if(!input->get_num_tracks)
    num_tracks = 1;
  else
    num_tracks = input->get_num_tracks(h->priv);

  for(i = 0; i < num_tracks; i++)
    {
    ti = input->get_track_info(h->priv, i);
    if(ti->url) /* Redirection */
      {
      if(!append_url(m, ti->url, 0))
        {
        bg_mozilla_widget_set_error(m->widget);
        goto fail;
        }
      }
    else if(!m->playing)
      do_play(m, m->url, i, ti, h);
    }
  
  fail:
  if(h)
    bg_plugin_unref(h);

  pthread_mutex_lock(&m->start_finished_mutex);
  m->start_finished = 1;
  pthread_mutex_unlock(&m->start_finished_mutex);

  return NULL;
  }

void gmerlin_mozilla_start(bg_mozilla_t * m)
  {
  if(m->url_mode != URL_MODE_STREAM)
    {
    start_func(m);
    m->state = STATE_PLAYING;
    }
  else
    {
    m->state = STATE_STARTING;
    pthread_create(&(m->start_thread), (pthread_attr_t*)0, start_func, m);
    }
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "volume",
      .long_name =   "Volume",
      .type =        BG_PARAMETER_FLOAT,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_min =     { .val_f = BG_PLAYER_VOLUME_MIN },
      .val_max =     { .val_f = 0.0 },
      .val_default = { .val_f = 0.0 },
    },
#if 0
    {
      .name =        "skin_dir",
      .long_name =   "Skin Directory",
      .type =        BG_PARAMETER_DIRECTORY,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_str = GMERLIN_DATA_DIR"/skins/Default" },
    },
#endif
    { /* End of Parameters */ }
  };

const bg_parameter_info_t * gmerlin_mozilla_get_parameters(bg_mozilla_t * g)
  {
  return parameters;
  }

void gmerlin_mozilla_set_parameter(void * data, const char * name,
                                   const bg_parameter_value_t * val)
  {
  bg_mozilla_t * g = data;
  if(!name)
    return;
  else if(!strcmp(name, "volume"))
    {
    bg_player_set_volume(g->player, val->val_f);
    g->volume = val->val_f;
    }
  }

int gmerlin_mozilla_get_parameter(void * data,
                                  const char * name,
                                  bg_parameter_value_t * val)
  {
  bg_mozilla_t * g = data;
  if(!name)
    return 0;
  else if(!strcmp(name, "volume"))
    {
    val->val_f = g->volume;
    }
  return 0;
  }
