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

#include <string.h>

#include <config.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#include <gmerlin/filters.h>
#include <gmerlin/cmdline.h>
#include <gmerlin/translation.h>

bg_video_filter_chain_t * fc;

const bg_parameter_info_t * fv_parameters;
bg_cfg_section_t * fv_section = (bg_cfg_section_t*)0;
bg_cfg_section_t * opt_section = (bg_cfg_section_t*)0;

bg_gavl_video_options_t opt;

int frameno = 0;
int dump_format = 0;

static void opt_frame(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -frame requires an argument\n");
    exit(-1);
    }
  frameno = atoi((*_argv)[arg]);  
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_df(void * data, int * argc, char *** _argv, int arg)
  {
  dump_format = 1;
  }

static void opt_v(void * data, int * argc, char *** _argv, int arg)
  {
  int val, verbose = 0;

  if(arg >= *argc)
    {
    fprintf(stderr, "Option -v requires an argument\n");
    exit(-1);
    }
  val = atoi((*_argv)[arg]);  
  
  if(val > 0)
    verbose |= BG_LOG_ERROR;
  if(val > 1)
    verbose |= BG_LOG_WARNING;
  if(val > 2)
    verbose |= BG_LOG_INFO;
  if(val > 3)
    verbose |= BG_LOG_DEBUG;
  bg_log_set_verbose(verbose);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_fv(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -fv requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(fv_section,
                               bg_video_filter_chain_set_parameter,
                               fc,
                               fv_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_parameter_info_t opt_parameters[] =
  {
    BG_GAVL_PARAM_THREADS,
    { /* */ },
  };

static void opt_set_param(void * data, const char * name,
                   const bg_parameter_value_t * val)
  {
  bg_gavl_video_set_parameter(data, name, val);
  }


static void opt_opt(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -opt requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(opt_section,
                               opt_set_param,
                               &opt,
                               opt_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-fv",
      .help_arg =    "<filter options>",
      .help_string = "Set filter options",
      .callback =    opt_fv,
    },
    {
      .arg =         "-opt",
      .help_arg =    "<video options>",
      .help_string = "Set video options",
      .callback =    opt_opt,
      .parameters =  opt_parameters,
    },
    {
      .arg =         "-v",
      .help_arg =    "level",
      .help_string = "Set verbosity level (0..4)",
      .callback =    opt_v,
    },
    {
      .arg =         "-frame",
      .help_arg =    "<number>",
      .help_string = "Output that frame (default 0)",
      .callback =    opt_frame,
    },
    {
      .arg =         "-df",
      .help_string = "Dump format",
      .callback =    opt_df,
    },
    { /* End of options */ }
  };

static void update_global_options()
  {
  global_options[0].parameters = fv_parameters;
  }

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "fvtest",
    .synopsis = TRS("[options] infile outimage\n"),
    .help_before = TRS("Test tool for video filters\n"),
    .args = (bg_cmdline_arg_array_t[]) { { TRS("Options"), global_options },
                                       {  } },
    .files = (bg_cmdline_ext_doc_t[])
    { { "~/.gmerlin/plugins.xml",
        TRS("Cache of the plugin registry (shared by all applicatons)") },
      { "~/.gmerlin/generic/config.xml",
        TRS("Default plugin parameters are read from there. Use gmerlin_plugincfg to change them.") },
      { /* End */ }
    },
    
  };


int main(int argc, char ** argv)
  {
  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;
  int i;
  char * tmp_path;
  bg_track_info_t * info;

  /* Plugins */
  bg_input_plugin_t * input_plugin;

  /* Plugin handles */
  bg_plugin_handle_t * input_handle = NULL;

  /* Frames */
  gavl_video_frame_t * frame = (gavl_video_frame_t *)0;
  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  
  /* Filter chain */
  /* Create registries */
  
  char ** gmls = (char **)0;

  bg_read_video_func_t read_func;
  void * read_priv;
  int read_stream;
  gavl_timer_t * timer = gavl_timer_create();
  
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Create filter chain */
  memset(&opt, 0, sizeof(opt));
  bg_gavl_video_options_init(&opt);
  fc = bg_video_filter_chain_create(&opt, plugin_reg);
  fv_parameters = bg_video_filter_chain_get_parameters(fc);
  fv_section =
    bg_cfg_section_create_from_parameters("fv", fv_parameters);
  opt_section =
    bg_cfg_section_create_from_parameters("opt", opt_parameters);
  
  /* Get commandline options */
  bg_cmdline_init(&app_data);

  update_global_options();
  
  bg_cmdline_parse(global_options, &argc, &argv, NULL);
  gmls = bg_cmdline_get_locations_from_args(&argc, &argv);

  if(!gmls || !gmls[0])
    {
    fprintf(stderr, "No input file given\n");
    return 0;
    }
  if(!gmls[1])
    {
    fprintf(stderr, "No output file given\n");
    return 0;
    }
  if(gmls[2])
    {
    fprintf(stderr, "Unknown argument %s\n", gmls[2]);
    }
  
  /* Load input plugin */
  if(!bg_input_plugin_load(plugin_reg,
                           gmls[0],
                           (const bg_plugin_info_t*)0,
                           &input_handle,
                           (bg_input_callbacks_t*)0, 0))
    {
    fprintf(stderr, "Cannot open %s\n", gmls[0]);
    return -1;
    }
  input_plugin = (bg_input_plugin_t*)(input_handle->plugin);

  info = input_plugin->get_track_info(input_handle->priv, 0);
  
  /* Select track */
  if(input_plugin->set_track)
    input_plugin->set_track(input_handle->priv, 0);
  
  if(!info->num_video_streams)
    {
    fprintf(stderr, "File %s has no video\n", gmls[0]);
    return -1;
    }

  /* Select first stream */
  input_plugin->set_video_stream(input_handle->priv, 0,
                                 BG_STREAM_ACTION_DECODE);
  
  /* Start playback */
  if(input_plugin->start)
    input_plugin->start(input_handle->priv);

  gavl_video_format_copy(&in_format, &info->video_streams[0].format);
  
  /* Initialize filter chain */

  read_func = input_plugin->read_video;
  read_priv = input_handle->priv;
  read_stream = 0;
  
  bg_video_filter_chain_connect_input(fc,
                                      input_plugin->read_video,
                                      input_handle->priv,
                                      0);

  if(bg_video_filter_chain_init(fc, &in_format, &out_format))
    {
    read_func = bg_video_filter_chain_read;
    read_priv = fc;
    read_stream = 0;
    }
  else
    gavl_video_format_copy(&out_format, &in_format);

  frame = gavl_video_frame_create(&out_format);
  
  if(frameno >= 0)
    {
    gavl_timer_start(timer);
    for(i = 0; i < frameno+1; i++)
      {
      if(!read_func(read_priv, frame, read_stream))
        {
        fprintf(stderr, "Unexpected EOF\n");
        return -1;
        }
      }
    gavl_timer_stop(timer);
    }
  else
    {
    gavl_timer_start(timer);
    while(1)
      {
      if(!read_func(read_priv, frame, read_stream))
        {
        break;
        }
      }
    gavl_timer_stop(timer);
    }

  fprintf(stderr, "Processing took %f seconds\n", gavl_time_to_seconds(gavl_timer_get(timer)));
  
  bg_plugin_registry_save_image(plugin_reg, gmls[1], frame, &out_format, NULL);

  /* Destroy everything */
  bg_plugin_unref(input_handle);
  bg_video_filter_chain_destroy(fc);
  bg_gavl_video_options_free(&opt);
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  gavl_video_frame_destroy(frame);
  gavl_timer_destroy(timer);
  return 0;
  }
