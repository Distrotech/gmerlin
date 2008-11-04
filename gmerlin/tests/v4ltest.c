/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <stdio.h>
#include <stdlib.h>
#include <gmerlin/utils.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginregistry.h>

int keep_going = 1;



int main(int argc, char ** argv)
  {
  char * tmp_path;
  bg_cfg_section_t     * section;
  bg_cfg_registry_t    * cfg_reg;
  bg_plugin_registry_t * plugin_reg;
  gavl_video_converter_t * cnv;
  
  int do_convert;

  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  
  bg_plugin_handle_t * input_handle;
  bg_plugin_handle_t * output_handle;

  bg_recorder_plugin_t     * input;
  bg_ov_plugin_t     * output;
  
  const bg_plugin_info_t * info;
  gavl_video_frame_t * frame = (gavl_video_frame_t*)0;
  gavl_video_frame_t * input_frame = (gavl_video_frame_t*)0;

  /* Create config registry */
  
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  /* Create plugin registry */

  section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(section);

  /* Load v4l plugin */

  info = bg_plugin_find_by_name(plugin_reg, "i_v4l");

  if(!info)
    {
    fprintf(stderr, "Video4linux plugin not installed\n");
    return -1;
    }

  input_handle = bg_plugin_load(plugin_reg, info);

  if(!input_handle)
    {
    fprintf(stderr, "Video4linux plugin could not be loaded\n");
    return -1;
    }

  input = (bg_recorder_plugin_t*)input_handle->plugin;

  /* Load output plugin */
  
  info = bg_plugin_registry_get_default(plugin_reg,
                                        BG_PLUGIN_OUTPUT_VIDEO);
  
  if(!info)
    {
    fprintf(stderr, "No video output plugin found\n");
    return -1;
    }
  
  output_handle = bg_plugin_load(plugin_reg, info);

  if(!output_handle)
    {
    fprintf(stderr, "Video output plugin could not be loaded\n");
    return -1;
    }

  output = (bg_ov_plugin_t*)output_handle->plugin;

  /* Open input */

  if(!input->open(input_handle->priv, NULL, &input_format))
    {
    fprintf(stderr, "Opening video device fauled\n");
    return -1;
    }

  fprintf(stderr, "Opened input device, format:\n");
  gavl_video_format_dump(&input_format);
  
  gavl_video_format_copy(&output_format, &input_format);
    
  /* Open output */

  if(!output->open(output_handle->priv, &output_format, 1))
    {
    fprintf(stderr, "Opening video device fauled\n");
    return -1;
    }

  output->set_window_title(output_handle->priv, "Webcam");
  
  if(output->show_window)
    output->show_window(output_handle->priv, 1);
  
  fprintf(stderr, "Opened output plugin, format:\n");
  gavl_video_format_dump(&output_format);
  
  /* Fire up video converter */

  cnv = gavl_video_converter_create();

  do_convert = gavl_video_converter_init(cnv, &input_format, &output_format);

  /* Allocate video image */

  if(output->create_frame)
    frame = output->create_frame(output_handle->priv);
    
  if(do_convert)
    {
    input_frame = gavl_video_frame_create(&input_format);
    }

  while(keep_going)
    {
    if(do_convert)
      {
      if(!input->read_video(input_handle->priv, input_frame, 0))
        {
        fprintf(stderr, "read_frame failed\n");
        return -1;
        }
      gavl_video_convert(cnv, input_frame, frame);
      }
    else
      {
      //      fprintf(stderr, "Get frame\n");
      input->read_video(input_handle->priv, frame, 0);
      }
    //    fprintf(stderr, "done\nPut frame...");
    output->put_video(output_handle->priv, frame);
    //    fprintf(stderr, "done\n");
    }
  return 0;
  }
