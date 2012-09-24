/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

/*
 *  Really simple video player
 *  change plugin names to test other plugins
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

typedef struct
  {
  gavl_timer_t * timer;
  const gavl_video_format_t * fmt;
  } process_data_t;

static void process_frame(void * priv, gavl_video_frame_t * frame)
  {
  gavl_time_t diff_time;
  process_data_t * d = priv;
  
  diff_time = gavl_time_unscale(d->fmt->timescale, frame->timestamp) - gavl_timer_get(d->timer);

  if(diff_time > 0)
    gavl_time_delay(&diff_time);
  
  }

int main(int argc, char ** argv)
  {
  /* Plugins */
  bg_input_plugin_t * input_plugin;
  bg_ov_plugin_t * output_plugin;
  const bg_plugin_info_t * plugin_info;
  bg_track_info_t * info;
  char * tmp_path;
  
  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg;

  gavl_video_connector_t * conn;
  
  /* Plugin handles */
  
  bg_plugin_handle_t * input_handle = NULL;
  bg_plugin_handle_t * output_handle = NULL;
    
  /* Output format */
  
  gavl_video_format_t video_format;
  
  process_data_t d;
  
  bg_log_set_verbose(BG_LOG_DEBUG|BG_LOG_WARNING|BG_LOG_ERROR|BG_LOG_INFO);
  if(argc == 1)
    {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return -1;
    }
  
  /* Create registries */

  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Load input plugin */
  input_handle = NULL;
  if(!bg_input_plugin_load(plugin_reg,
                           argv[1],
                           NULL,
                           &input_handle,
                           NULL, 0))
    {
    fprintf(stderr, "Cannot open %s\n", argv[1]);
    return -1;
    }
  input_plugin = (bg_input_plugin_t*)(input_handle->plugin);

  /* Load output plugin */

  plugin_info =
    bg_plugin_registry_get_default(plugin_reg,
                                   BG_PLUGIN_OUTPUT_VIDEO,
                                   BG_PLUGIN_PLAYBACK);
  
  if(!plugin_info)
    {
    fprintf(stderr, "Output plugin not found\n");
    return -1;
    }
  output_handle = bg_plugin_load(plugin_reg, plugin_info);
  output_plugin = (bg_ov_plugin_t*)(output_handle->plugin);
  
  info = input_plugin->get_track_info(input_handle->priv, 0);

  /* Select track */
  if(input_plugin->set_track)
    input_plugin->set_track(input_handle->priv, 0);
  
  if(!info->num_video_streams)
    {
    fprintf(stderr, "File %s has no video\n", argv[1]);
    return -1;
    }

  /* Select first stream */
  input_plugin->set_video_stream(input_handle->priv, 0,
                                 BG_STREAM_ACTION_DECODE);
  
  /* Start playback */
  if(input_plugin->start)
    input_plugin->start(input_handle->priv);
  
  /* Get video format */

  gavl_video_format_copy(&video_format, &info->video_streams[0].format);
  
  /* Initialize output plugin */
    
  if(!output_plugin->open(output_handle->priv, &video_format, 1))
    {
    fprintf(stderr, "Cannot open output plugin %s\n",
            output_handle->info->name);
    return -1;
    }
  output_plugin->show_window(output_handle->priv, 1);
  output_plugin->set_window_title(output_handle->priv, "Video output");
  
  /* Initialize video connector */

  conn =
    gavl_video_connector_create(input_plugin->get_video_source(input_handle->priv, 0));
  
  gavl_video_connector_connect(conn,
                               output_plugin->get_sink(output_handle->priv));

  gavl_video_connector_start(conn);
  
  /* Allocate timer */

  d.timer = gavl_timer_create();
  d.fmt = gavl_video_connector_get_process_format(conn);

  gavl_video_connector_set_process_func(conn, process_frame, &d);
  
  /* Playback until we are done */

  gavl_timer_start(d.timer);
  
  while(gavl_video_connector_process(conn))
    {
    if(output_plugin->handle_events)
      output_plugin->handle_events(output_handle->priv);
    }
    
  /* Clean up */
  
  if(input_plugin->stop)
    input_plugin->stop(input_handle->priv);

  input_plugin->close(input_handle->priv);
  output_plugin->close(output_handle->priv);
  
  bg_plugin_unref(input_handle);
  bg_plugin_unref(output_handle);

  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  
  gavl_timer_destroy(d.timer);
  gavl_video_connector_destroy(conn);
  
  return 0;
  }
