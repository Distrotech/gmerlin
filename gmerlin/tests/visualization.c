#include <stdio.h>
#include <string.h>

#include <config.h>

#include <utils.h>
#include <pluginregistry.h>

#include <visualize.h>

#include <cmdline.h>
#include <log.h>
#define LOG_DOMAIN "visualization"

static bg_visualizer_t * visualizer = (bg_visualizer_t*)0;

static bg_cfg_section_t * vis_section = (bg_cfg_section_t*)0;
static bg_parameter_info_t * vis_parameters = (bg_parameter_info_t*)0;

static void opt_help(void * data, int * argc, char *** argv, int arg);

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
      arg:         "-vis",
      help_string: "options",
      callback:    opt_visualizer,
    },
    {
      arg:         "-help",
      help_string: "Print this help message and exit",
      callback:    opt_help,
    },
    { /* End of options */ }
    
  };

static void opt_help(void * data, int * argc, char *** argv, int arg)
  {
  fprintf(stderr, "Usage: %s [options]\n\n", (*argv)[0]);
  fprintf(stderr, "Options:\n\n");
  bg_cmdline_print_help(visualizer_options);
  exit(0);
  }


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
  bg_ra_plugin_t     * input_plugin;
  
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
  
  input_plugin = (bg_ra_plugin_t*)(input_handle->plugin);
  
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

  if(!input_plugin->open(input_handle->priv, &format))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Opening input plugin failed");
    return -1;
    }

  /* Create frame */
  
  frame = gavl_audio_frame_create(&format);
  gavl_audio_frame_mute(frame, &format);
  
  /* Start */
  
  bg_visualizer_open(visualizer, &format, ov_handle);
  
  while(1)
    {
    input_plugin->read_frame(input_handle->priv, frame, format.samples_per_frame);
    bg_visualizer_update(visualizer, frame);
    }
  
  bg_visualizer_close(visualizer);
  return 0;
  }
