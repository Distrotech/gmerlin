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

#include <config.h>
#include <gmerlin/recorder.h>
#include <gmerlin/pluginregistry.h>
#include <gavl/gavl.h>
#include <gmerlin/utils.h>
#include <gmerlin/cmdline.h>
#include <gmerlin/log.h>
#include <gmerlin/translation.h>

static bg_recorder_t * recorder;

static bg_cfg_section_t * audio_section;
static bg_cfg_section_t * video_section;
static bg_cfg_section_t * metadata_section;
static bg_cfg_section_t * encoder_section;
static bg_cfg_section_t * af_section;
static bg_cfg_section_t * vf_section;
static bg_cfg_section_t * vm_section;
static bg_cfg_section_t * output_section;

const bg_parameter_info_t * audio_parameters;
const bg_parameter_info_t * video_parameters;
const bg_parameter_info_t * af_parameters;
const bg_parameter_info_t * vf_parameters;
const bg_parameter_info_t * vm_parameters;
const bg_parameter_info_t * output_parameters;

const bg_parameter_info_t * metadata_parameters;
const bg_parameter_info_t * encoder_parameters;


static bg_plugin_registry_t * plugin_reg;
static bg_cfg_registry_t * cfg_reg;

static int do_record = 0;

static int do_syslog = 0;

#define DELAY_TIME 50
#define PING_INTERVAL 20

static void opt_aud(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -aud requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(audio_section,
                               bg_recorder_set_audio_parameter,
                               recorder,
                               audio_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_af(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -af requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(audio_section,
                               bg_recorder_set_audio_filter_parameter,
                               recorder,
                               af_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_vid(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vid requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(video_section,
                               bg_recorder_set_video_parameter,
                               recorder,
                               video_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_vf(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vf requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(video_section,
                               bg_recorder_set_video_filter_parameter,
                               recorder,
                               vf_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_vm(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vm requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(video_section,
                               bg_recorder_set_video_monitor_parameter,
                               recorder,
                               vm_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_m(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -m requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(metadata_section,
                               bg_recorder_set_metadata_parameter,
                               recorder,
                               metadata_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_r(void * data, int * argc, char *** _argv, int arg)
  {
  do_record = 1;
  }

static void opt_enc(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -enc requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(encoder_section,
                               /* bg_recorder_set_metadata_parameter */ NULL,
                               /* recorder */ NULL,
                               encoder_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_o(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -o requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(encoder_section,
                               bg_recorder_set_output_parameter,
                               recorder,
                               output_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_syslog(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -syslog requires an argument\n");
    exit(-1);
    }
  bg_log_syslog_init((*_argv)[arg]);
  do_syslog = 1;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-aud",
      .help_arg =    "<audio_options>",
      .help_string = "Set audio recording options",
      .callback =    opt_aud,
    },
    {
      .arg =         "-vid",
      .help_arg =    "<video_options>",
      .help_string = "Set video recording options",
      .callback =    opt_vid,
    },
    {
      .arg =         "-af",
      .help_arg =    "<audio filter options>",
      .help_string = "Set audio filter options",
      .callback =    opt_af,
    },
    {
      .arg =         "-vf",
      .help_arg =    "<video filter options>",
      .help_string = "Set video filter options",
      .callback =    opt_vf,
    },
    {
      .arg =         "-vm",
      .help_arg =    "<video monitor options>",
      .help_string = "Set video monitor options",
      .callback =    opt_vm,
    },
    {
      .arg =         "-m",
      .help_arg =    "<metadata_options>",
      .help_string = "Set metadata options",
      .callback =    opt_m,
    },
    {
      .arg =         "-enc",
      .help_arg =    "<encoding_options>",
      .help_string = "Set encoding options",
      .callback =    opt_enc,
      
    },
    {
      .arg =         "-o",
      .help_arg =    "<output_options>",
      .help_string = "Set output options",
      .callback =    opt_o,
      
    },
    {
      .arg =         "-r",
      .help_string = "Record to file",
      .callback =    opt_r,
    },
    {
      .arg =         "-syslog",
      .help_arg =    "<name>",
      .help_string = "Set log messages to syslog",
      .callback =    opt_syslog,
    },
    {
      /* End */
    }
  };

static void update_global_options()
  {
  global_options[0].parameters = audio_parameters;
  global_options[1].parameters = video_parameters;
  global_options[2].parameters = af_parameters;
  global_options[3].parameters = vf_parameters;
  global_options[4].parameters = vm_parameters;
  global_options[5].parameters = metadata_parameters;
  global_options[6].parameters = encoder_parameters;
  global_options[7].parameters = output_parameters;
  }

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin-record",
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("Commandline recorder\n"),
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
  bg_cfg_section_t * cfg_section;
  char * tmp_path;
  int timeout_counter = 0;

  gavl_time_t delay_time = DELAY_TIME * 1000;

  /* We must initialize the random number generator if we want the
     Vorbis encoder to work */
  srand(time(NULL));
  
  /* Create plugin regsitry */
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  recorder = bg_recorder_create(plugin_reg);
  
  /* Create config sections */
  
  audio_parameters = bg_recorder_get_audio_parameters(recorder);
  audio_section =
    bg_cfg_section_create_from_parameters("audio", audio_parameters);

  video_parameters = bg_recorder_get_video_parameters(recorder);
  video_section =
    bg_cfg_section_create_from_parameters("video", video_parameters);

  af_parameters = bg_recorder_get_audio_filter_parameters(recorder);
  af_section =
    bg_cfg_section_create_from_parameters("audio_filters", af_parameters);

  vf_parameters = bg_recorder_get_video_filter_parameters(recorder);
  vf_section =
    bg_cfg_section_create_from_parameters("video_filters", vf_parameters);

  vm_parameters = bg_recorder_get_video_monitor_parameters(recorder);
  vm_section =
    bg_cfg_section_create_from_parameters("video_monitor", vm_parameters);

  metadata_parameters = bg_recorder_get_metadata_parameters(recorder);
  metadata_section =
    bg_cfg_section_create_from_parameters("metadata", metadata_parameters);

  encoder_parameters = bg_recorder_get_encoder_parameters(recorder);
  
  encoder_section =
    bg_cfg_section_create_from_parameters("encoder", encoder_parameters);

  output_parameters = bg_recorder_get_output_parameters(recorder);
  output_section =
    bg_cfg_section_create_from_parameters("output", output_parameters);
  
  update_global_options();

  /* Apply default options */
  
  bg_cfg_section_apply(audio_section, audio_parameters,
                       bg_recorder_set_audio_parameter, recorder);
  bg_cfg_section_apply(video_section, video_parameters,
                       bg_recorder_set_video_parameter, recorder);
  bg_cfg_section_apply(af_section, af_parameters,
                       bg_recorder_set_audio_filter_parameter, recorder);
  bg_cfg_section_apply(vf_section, vf_parameters,
                       bg_recorder_set_video_filter_parameter, recorder);
  bg_cfg_section_apply(vm_section, vm_parameters,
                       bg_recorder_set_video_monitor_parameter, recorder);
  bg_cfg_section_apply(metadata_section, metadata_parameters,
                       bg_recorder_set_metadata_parameter, recorder);
  bg_cfg_section_apply(output_section, output_parameters,
                       bg_recorder_set_output_parameter, recorder);
  
  /* Get commandline options */
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  /* Set encoding options */
  bg_recorder_set_encoder_section(recorder, encoder_section);
  
  bg_recorder_record(recorder, do_record);
  bg_recorder_run(recorder);
  
  while(1)
    {
    if(!timeout_counter)
      {
      if(!bg_recorder_ping(recorder))
        break;
      }
    timeout_counter++;
    if(timeout_counter >= PING_INTERVAL)
      timeout_counter = 0;

    if(do_syslog)
      bg_log_syslog_flush();
    
    gavl_time_delay(&delay_time);
    }
  
  bg_recorder_destroy(recorder);
  return 0;
  }
