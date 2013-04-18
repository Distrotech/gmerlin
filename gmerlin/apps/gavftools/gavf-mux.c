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

#define LOG_DOMAIN "gavf-mux"

static bg_plug_t ** in_plugs = NULL;
static bg_plug_t * out_plug = NULL;

static char ** infiles = NULL;
static int num_infiles = 0;

static void
opt_i(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -i requires an argument\n");
    exit(-1);
    }
  
  infiles = realloc(infiles, sizeof(*infiles) * (num_infiles+1));
  infiles[num_infiles] = (*_argv)[arg];
  num_infiles++;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-i",
      .help_arg =    "<location>",
      .help_string = TRS("Input file or location. Use this option multiple times to add more inputs."),
      .callback    = &opt_i,
    },
    GAVFTOOLS_OUTPLUG_OPTIONS,
    GAVFTOOLS_LOG_OPTIONS,
    { /* End */ },
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-mux",
    .long_name = TRS("Multiplex multiple gavf stream into one"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf tee\n"),
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

static void set_stream_actions(bg_plug_t * in_plug, gavf_stream_type_t type)
  {
  gavf_t * g;
  int num, i;
  const gavf_stream_header_t * sh;

  g = bg_plug_get_gavf(in_plug);

  num = gavf_get_num_streams(g, type);

  for(i = 0; i < num; i++)
    {
    sh = gavf_get_stream(g, i, type);
    bg_plug_set_stream_action(in_plug, sh, BG_STREAM_ACTION_READRAW);
    }
  }

int main(int argc, char ** argv)
  {
  int ret = EXIT_FAILURE;
  int i;
  bg_mediaconnector_t conn;
  
  gavftools_init();
  
  gavftools_block_sigpipe();
  bg_mediaconnector_init(&conn);

  gavftools_set_cmdline_parameters(global_options);

  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  if(!bg_cmdline_check_unsupported(argc, argv))
    return -1;

  if(!num_infiles)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Need at least one input file");
    goto fail;
    }

  out_plug = gavftools_create_out_plug();
  
  in_plugs = calloc(num_infiles, sizeof(*in_plugs));
  for(i = 0; i < num_infiles; i++)
    {
    in_plugs[i] = gavftools_create_in_plug();

    if(!bg_plug_open_location(in_plugs[i], infiles[i], NULL, NULL))
      goto fail;

    set_stream_actions(in_plugs[i], GAVF_STREAM_AUDIO);
    set_stream_actions(in_plugs[i], GAVF_STREAM_VIDEO);
    set_stream_actions(in_plugs[i], GAVF_STREAM_TEXT);
    set_stream_actions(in_plugs[i], GAVF_STREAM_OVERLAY);

    if(!bg_plug_start(in_plugs[i]))
      goto fail;

    if(!bg_plug_setup_reader(in_plugs[i], &conn))
      goto fail;

    /* Copy metadata and so on from first source */
    if(!i)
      {
      if(!gavftools_open_out_plug_from_in_plug(out_plug, NULL,
                                               in_plugs[i]))
        goto fail;
      }
    }

  bg_mediaconnector_create_conn(&conn);
  
  if(!bg_plug_setup_writer(out_plug, &conn))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Setting up plug writer failed");
    goto fail;
    }

  /* Fire up connector */

  bg_mediaconnector_start(&conn);

  /* Run */

  while(1)
    {
    for(i = 0; i < num_infiles; i++)
      {
      if(bg_plug_got_error(in_plugs[i]))
        {
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Error reading from %s",
               infiles[i]);
        goto fail;
        }
      }
    
    if(gavftools_stop() ||
       !bg_mediaconnector_iteration(&conn))
      break;
    }
  
  ret = EXIT_SUCCESS;
  fail:

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Cleaning up");

  bg_mediaconnector_free(&conn);
  
  if(out_plug)
    bg_plug_destroy(out_plug);
  
  for(i = 0; i < num_infiles; i++)
    {
    if(in_plugs[i])
      bg_plug_destroy(in_plugs[i]);
    }

  if(infiles)
    free(infiles);
  if(in_plugs)
    free(in_plugs);
  
  gavftools_cleanup();
  return ret;
  }
