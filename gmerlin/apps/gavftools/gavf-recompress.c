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

#include "gavftools.h"

static bg_plug_t * in_plug = NULL;
static bg_plug_t * out_plug = NULL;

#define LOG_DOMAIN "gavf-recompress"

static gavl_metadata_t ac_options;
static gavl_metadata_t vc_options;
static gavl_metadata_t oc_options;

static bg_parameter_info_t * ac_parameters = NULL;
static bg_parameter_info_t * vc_parameters = NULL;
static bg_parameter_info_t * oc_parameters = NULL;

static void opt_ac(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -ac requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_set_stream_options(&ac_options,(*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_vc(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vc requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_set_stream_options(&vc_options,(*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_oc(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -oc requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_set_stream_options(&oc_options,(*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    GAVFTOOLS_INPLUG_OPTIONS,
    GAVFTOOLS_OUTPLUG_OPTIONS,
    GAVFTOOLS_AUDIO_STREAM_OPTIONS,
    GAVFTOOLS_VIDEO_STREAM_OPTIONS,
    GAVFTOOLS_TEXT_STREAM_OPTIONS,
    GAVFTOOLS_OVERLAY_STREAM_OPTIONS,
    {
      .arg =         "-ac",
      .help_arg =    "<options>",
      .help_string = "Audio compression options",
      .callback =    opt_ac,
    },
    {
      .arg =         "-vc",
      .help_arg =    "<options>",
      .help_string = "Video compression options",
      .callback =    opt_vc,
    },
    {
      .arg =         "-oc",
      .help_arg =    "<options>",
      .help_string = "Overlay compression options",
      .callback =    opt_oc,
    },
    GAVFTOOLS_LOG_OPTIONS,
    { /* End */ },
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-recompress",
    .long_name = TRS("Recompress streams of a gavf stream"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf recompressor\n"),
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
  int ret = 1;
  bg_mediaconnector_t conn;
  
  gavl_metadata_init(&ac_options);
  gavl_metadata_init(&vc_options);
  gavl_metadata_init(&oc_options);
  
  bg_mediaconnector_init(&conn);
  gavftools_init();

  ac_parameters =
    bg_plugin_registry_create_compressor_parameters(plugin_reg,
                                                    BG_PLUGIN_AUDIO_COMPRESSOR);
  vc_parameters =
    bg_plugin_registry_create_compressor_parameters(plugin_reg,
                                                    BG_PLUGIN_VIDEO_COMPRESSOR);
  oc_parameters =
    bg_plugin_registry_create_compressor_parameters(plugin_reg,
                                                    BG_PLUGIN_OVERLAY_COMPRESSOR);

  
  bg_cmdline_arg_set_parameters(global_options, "-ac", ac_parameters);
  bg_cmdline_arg_set_parameters(global_options, "-vc", vc_parameters);
  bg_cmdline_arg_set_parameters(global_options, "-oc", oc_parameters);

  /* Handle commandline options */
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  if(!bg_cmdline_check_unsupported(argc, argv))
    return -1;

  return ret;
  }
