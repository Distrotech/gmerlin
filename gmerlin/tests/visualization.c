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
#include <string.h>

#include <config.h>

#include <utils.h>
#include <pluginregistry.h>

#include <visualize.h>
#include <translation.h>

#include <cmdline.h>
#include <log.h>
#define LOG_DOMAIN "visualization"

static bg_visualizer_t * visualizer = (bg_visualizer_t*)0;

static bg_cfg_section_t * vis_section = (bg_cfg_section_t*)0;
static bg_parameter_info_t const * vis_parameters = (bg_parameter_info_t*)0;


static void opt_visualizer(void * data, int * argc,
                           char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vis requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(vis_section,
                               bg_visualizer_set_parameter,
                               visualizer,
                               vis_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t visualizer_options[] =
  {
    {
      .arg =         "-vis",
      .help_string = "options",
      .callback =    opt_visualizer,
    },
    { /* End of options */ }
    
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "visualization",
    .synopsis = TRS("[<options>]\n"),
    .help_before = TRS("Visualize test\n"),
    .args = (const bg_cmdline_arg_array_t[]) { { TRS("Options"), visualizer_options },
                                               {  } },
  };


int main(int argc, char ** argv)
  {
  const bg_plugin_info_t * info;
  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  bg_plugin_registry_t * plugin_reg;
  char * tmp_path;
  
  gavl_audio_format_t format;
  gavl_audio_frame_t * frame;
  
  bg_plugin_handle_t * input_handle;
  bg_recorder_plugin_t     * input_plugin;
  
  bg_plugin_handle_t * ov_handle;
  
  /* Load config registry */
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  vis_section = bg_cfg_registry_find_section(cfg_reg,
                                             "visualization");
  
  /* Load plugin registry */
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Create visualizer */

  visualizer = bg_visualizer_create(plugin_reg);

  vis_parameters = bg_visualizer_get_parameters(visualizer);
  visualizer_options[0].parameters = vis_parameters;
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(visualizer_options, &argc, &argv, NULL);
  
  /* Load input */

  info = bg_plugin_registry_get_default(plugin_reg, BG_PLUGIN_RECORDER_AUDIO);
  if(!info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No input plugin defined");
    return -1;
    }

  input_handle = bg_plugin_load(plugin_reg, info);
  if(!input_handle)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Input plugin could not be loaded");
    return -1;
    }
  
  input_plugin = (bg_recorder_plugin_t*)(input_handle->plugin);
  
  /* Load output */
  info = bg_plugin_registry_get_default(plugin_reg, BG_PLUGIN_OUTPUT_VIDEO);
  if(!info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No output plugin defined");
    return -1;
    }

  ov_handle = bg_plugin_load(plugin_reg, info);
  if(!ov_handle)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Output plugin could not be loaded");
    return -1;
    }
  
  /* Open input */

  memset(&format, 0, sizeof(format));

  format.samplerate = 44100;
  format.sample_format = GAVL_SAMPLE_S16;
  format.samples_per_frame = 2048;
  format.interleave_mode = GAVL_INTERLEAVE_ALL;
  format.num_channels   = 2;
  gavl_set_channel_setup(&format);

  if(!input_plugin->open(input_handle->priv, &format, NULL))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Opening input plugin failed");
    return -1;
    }

  /* Create frame */
  
  frame = gavl_audio_frame_create(&format);
  gavl_audio_frame_mute(frame, &format);
  
  /* Start */
  
  bg_visualizer_open_plugin(visualizer, &format, ov_handle);
  
  while(1)
    {
    input_plugin->read_audio(input_handle->priv, frame, 0, format.samples_per_frame);
    bg_visualizer_update(visualizer, frame);
    }
  
  bg_visualizer_close(visualizer);
  return 0;
  }
