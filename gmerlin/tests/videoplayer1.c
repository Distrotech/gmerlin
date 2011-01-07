/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

static char * output_plugin_name = "ov_x11";

int main(int argc, char ** argv)
  {
  /* Plugins */
  bg_input_plugin_t * input_plugin;
  bg_ov_plugin_t * output_plugin;
  const bg_plugin_info_t * plugin_info;
  bg_track_info_t * info;
  int do_convert;
  char * tmp_path;


  gavl_timer_t * timer;
  int64_t frames_written;
  gavl_time_t diff_time;

  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg;

  /* Plugin handles */
  
  bg_plugin_handle_t * input_handle = NULL;
  bg_plugin_handle_t * output_handle = NULL;
    
  /* Output format */
  
  gavl_video_format_t video_format;

  /* Frames */
  
  gavl_video_frame_t * input_frame = (gavl_video_frame_t *)0;
  gavl_video_frame_t * output_frame = (gavl_video_frame_t *)0;

  /* Converter */

  gavl_video_converter_t * video_converter;

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
  input_handle = (bg_plugin_handle_t*)0;
  if(!bg_input_plugin_load(plugin_reg,
                           argv[1],
                           (const bg_plugin_info_t*)0,
                           &input_handle,
                           (bg_input_callbacks_t*)0, 0))
    {
    fprintf(stderr, "Cannot open %s\n", argv[1]);
    return -1;
    }
  input_plugin = (bg_input_plugin_t*)(input_handle->plugin);

  /* Load output plugin */
    
  plugin_info = bg_plugin_find_by_name(plugin_reg, output_plugin_name);
  if(!plugin_info)
    {
    fprintf(stderr, "Output plugin %s not found\n", output_plugin_name);
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

  memcpy(&video_format, &info->video_streams[0].format,
         sizeof(gavl_video_format_t));

  /* Initialize output plugin */
    
  if(!output_plugin->open(output_handle->priv, &video_format, 1))
    {
    fprintf(stderr, "Cannot open output plugin %s\n",
            output_handle->info->name);
    return -1;
    }
  output_plugin->show_window(output_handle->priv, 1);
  output_plugin->set_window_title(output_handle->priv, "Video output");
  
  /* Initialize video converter */

  video_converter = gavl_video_converter_create();

  
  do_convert = gavl_video_converter_init(video_converter,
                                         &info->video_streams[0].format,
                                         &video_format);
  
  if(do_convert)
    fprintf(stderr, "Doing Video Conversion\n");
  else
    fprintf(stderr, "No Video conversion\n");

  fprintf(stderr, "Input format:\n");
  gavl_video_format_dump(&info->video_streams[0].format);
  fprintf(stderr, "Output format:\n");
  gavl_video_format_dump(&video_format);
  
  /* Allocate frames */

  if(do_convert)
    input_frame = gavl_video_frame_create(&info->video_streams[0].format);
  
  if(output_plugin->create_frame)
    output_frame = output_plugin->create_frame(output_handle->priv);
  else
    input_frame = gavl_video_frame_create(&video_format);

  /* Allocate timer */

  timer = gavl_timer_create();
  
  /* Playback until we are done */

  frames_written = 0;

  gavl_timer_start(timer);
  
  if(do_convert)
    {
    while(1)
      {
      if(!input_plugin->read_video(input_handle->priv, input_frame, 0))
        break;
      gavl_video_convert(video_converter, input_frame, output_frame);
            
      diff_time = gavl_time_unscale(info->video_streams[0].format.timescale, output_frame->timestamp) - gavl_timer_get(timer);

      if(diff_time > 0)
        {
        gavl_time_delay(&diff_time);
        }
      
      output_plugin->put_video(output_handle->priv, output_frame);
      output_plugin->handle_events(output_handle->priv);
      frames_written++;
      }
    }
  else
    {
    while(1)
      {
      if(!input_plugin->read_video(input_handle->priv, output_frame, 0))
        break;
            
      diff_time = gavl_time_unscale(info->video_streams[0].format.timescale, output_frame->timestamp) - gavl_timer_get(timer);
      if(diff_time > 0)
        gavl_time_delay(&diff_time);
      output_plugin->put_video(output_handle->priv, output_frame);
      frames_written++;
      }
    }
  
  /* Clean up */

  gavl_video_converter_destroy(video_converter);

  if(do_convert)
    gavl_video_frame_destroy(input_frame);

  if(output_plugin->destroy_frame)
    output_plugin->destroy_frame(output_handle->priv, output_frame);
  else
    gavl_video_frame_destroy(output_frame);
  
  if(input_plugin->stop)
    input_plugin->stop(input_handle->priv);

  input_plugin->close(input_handle->priv);
  output_plugin->close(output_handle->priv);
  
  bg_plugin_unref(input_handle);
  bg_plugin_unref(output_handle);

  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  
  gavl_timer_destroy(timer);
    
  return 0;
  }
