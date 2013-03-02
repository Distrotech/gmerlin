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

#include <gmerlin/encoder.h>

#define LOG_DOMAIN "gavf-filter"

static gavl_metadata_t af_options;
static gavl_metadata_t vf_options;

static bg_plug_t * in_plug = NULL;
static bg_plug_t * out_plug = NULL;

static void opt_af(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -af requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_set_stream_options(&af_options,(*_argv)[arg]))
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
  if(!bg_cmdline_set_stream_options(&vf_options,(*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    GAVFTOOLS_INPLUG_OPTIONS,
    GAVFTOOLS_AUDIO_STREAM_OPTIONS,
    GAVFTOOLS_VIDEO_STREAM_OPTIONS,
    GAVFTOOLS_TEXT_STREAM_OPTIONS,
    GAVFTOOLS_OVERLAY_STREAM_OPTIONS,
    {
      .arg =         "-af",
      .help_arg =    "<options>",
      .help_string = "Audio compression options",
      .callback =    opt_af,
    },
    {
      .arg =         "-vf",
      .help_arg =    "<options>",
      .help_string = "Video compression options",
      .callback =    opt_vf,
    },
    GAVFTOOLS_OUTPLUG_OPTIONS,
    GAVFTOOLS_VERBOSE_OPTIONS,
    { /* End */ },
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-filter",
    .long_name = TRS("Apply A/V filter to a gavf stream"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf filter\n"),
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

  gavftools_block_sigpipe();
  bg_mediaconnector_init(&conn);
  
  gavftools_init();

  gavl_metadata_init(&af_options);
  gavl_metadata_init(&vf_options);

  bg_cmdline_arg_set_parameters(global_options, "-iopt",
                                bg_plug_get_input_parameters());

  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  in_plug = gavftools_create_in_plug();
  
  
  }
