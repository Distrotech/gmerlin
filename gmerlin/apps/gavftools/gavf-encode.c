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

static bg_plug_t * in_plug = NULL;

#define LOG_DOMAIN "gavf-play"

static const char * input_file   = NULL;

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

static void opt_in(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -i requires an argument\n");
    exit(-1);
    }
  input_file = (*_argv)[arg];
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-i",
      .help_arg =    "<location>",
      .help_string = "Set input file or location",
      .callback =    opt_in,
    },
    {
      .arg =         "-v",
      .help_arg =    "level",
      .help_string = "Set verbosity level (0..4)",
      .callback =    opt_v,
    },

    { /* End */ },
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-encode",
    .long_name = TRS("Encode a gavf stream to another format"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf encoder\n"),
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

  bg_mediaconnector_init(&conn);
  gavftools_init_registries();
  
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  in_plug = bg_plug_create_reader(plugin_reg);

  /* Open */

  if(!bg_plug_open_location(in_plug, input_file, NULL, NULL))
    return ret;

  /* Check for stream copying */

  /* Select streams */
  
  /* Set up encoder */

  /* Main loop */

  /* Cleanup */

  return 0;
  }
