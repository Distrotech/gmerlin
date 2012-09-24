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
 *  Really simple audio player
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>

int main(int argc, char ** argv)
  {
  bg_track_info_t * info;
  char * tmp_path;
  const bg_plugin_info_t * plugin_info;
  bg_cfg_section_t * cfg_section;
  
  gavl_audio_connector_t * conn;
  
  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t    * cfg_reg;
  
  bg_plugin_handle_t * input_handle = NULL;
  bg_plugin_handle_t * output_handle = NULL;
  
  /* Plugins */
  
  bg_input_plugin_t * input_plugin = NULL;
  bg_oa_plugin_t    * output_plugin = NULL;
    
  /* Output format */
  
  gavl_audio_format_t audio_format;
  
  if(argc == 1)
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);

  /* Create registries */

  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Load input plugin */

  if(!bg_input_plugin_load(plugin_reg,
                           argv[1],
                           NULL, // const bg_plugin_info_t * info,
                           &input_handle, // bg_plugin_handle_t ** ret,
                           NULL, // bg_input_callbacks_t * callbacks,
                           0 // int prefer_edl
                           ))
    return -1;
  
  input_plugin = (bg_input_plugin_t*)(input_handle->plugin);
  
  /* Load output plugin */
  
  plugin_info =
    bg_plugin_registry_get_default(plugin_reg,
                                   BG_PLUGIN_OUTPUT_AUDIO,
                                   BG_PLUGIN_PLAYBACK);
  
  if(!plugin_info)
    {
    fprintf(stderr, "Output plugin not found\n");
    return -1;
    }
  output_handle = bg_plugin_load(plugin_reg, plugin_info);
  output_plugin = (bg_oa_plugin_t*)(output_handle->plugin);
  
  /* Select the first track */
  
  if(input_plugin->set_track)
    input_plugin->set_track(input_handle->priv, 0);

  info = input_plugin->get_track_info(input_handle->priv, 0);
  
  if(!info->num_audio_streams)
    {
    fprintf(stderr, "File %s has no audio\n", argv[1]);
    return -1;
    }

  /* Select first stream */

  input_plugin->set_audio_stream(input_handle->priv, 0,
                                 BG_STREAM_ACTION_DECODE);
  
  /* Start playback */
  if(input_plugin->start && !input_plugin->start(input_handle->priv))
    {
    fprintf(stderr, "Starting %s failed\n", argv[1]);
    return -1;
    }
  
  /* Get audio format */

  gavl_audio_format_copy(&audio_format, &info->audio_streams[0].format);
  
  /* Initialize output plugin */
  
  output_plugin->open(output_handle->priv, &audio_format);

  /* Create connector */
  conn =
    gavl_audio_connector_create(input_plugin->get_audio_source(input_handle->priv, 0));

  gavl_audio_connector_connect(conn,
                               output_plugin->get_sink(output_handle->priv));

  gavl_audio_connector_start(conn);
  
  while(gavl_audio_connector_process(conn))
    ;
  
  /* Clean up */
  
  if(input_plugin->stop)
    input_plugin->stop(input_handle->priv);

  input_plugin->close(input_handle->priv);
  output_plugin->close(output_handle->priv);

  bg_plugin_unref(input_handle);
  bg_plugin_unref(output_handle);
    
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);

  gavl_audio_connector_destroy(conn);
  
  return 0;
  }
