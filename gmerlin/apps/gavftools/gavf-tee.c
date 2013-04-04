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

#define LOG_DOMAIN "gavf-tee"

static bg_plug_t * in_plug = NULL;
static bg_plug_t ** out_plugs = NULL;

static char ** outfiles = NULL;
static int num_outfiles = 0;

static void
opt_o(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -o requires an argument\n");
    exit(-1);
    }
  
  outfiles = realloc(outfiles, sizeof(*outfiles) * (num_outfiles+1));
  outfiles[num_outfiles] = (*_argv)[arg];
  num_outfiles++;
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
      .arg =         "-o",
      .help_arg =    "<output>",
      .help_string = TRS("Output file or location. Use this option multiple times to add more outputs."),
      .callback    = &opt_o,
    },
    GAVFTOOLS_OOPT_OPTIONS,
    GAVFTOOLS_LOG_OPTIONS,
    { /* End */ },
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-tee",
    .long_name = TRS("Send a gavf stream to one or more destinations"),
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

  bg_stream_action_t * actions = NULL;
  g = bg_plug_get_gavf(in_plug);

  num = gavf_get_num_streams(g, type);
  actions = gavftools_get_stream_actions(num, type);

  for(i = 0; i < num; i++)
    {
    sh = gavf_get_stream(g, i, type);
    bg_plug_set_stream_action(in_plug, sh, actions[i]);
    }
  
  if(actions)
    free(actions);
  }

int main(int argc, char ** argv)
  {
  int ret = 1;
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

  in_plug = gavftools_create_in_plug();

  if(!bg_plug_open_location(in_plug, gavftools_in_file, NULL, NULL))
    goto fail;
  
  set_stream_actions(in_plug, GAVF_STREAM_AUDIO);
  set_stream_actions(in_plug, GAVF_STREAM_VIDEO);
  set_stream_actions(in_plug, GAVF_STREAM_TEXT);
  set_stream_actions(in_plug, GAVF_STREAM_OVERLAY);

  if(!bg_plug_start(in_plug))
    goto fail;

  if(!num_outfiles)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Need at least one outout file");
    goto fail;
    }

  if(!bg_plug_setup_reader(in_plug, &conn))
    goto fail;

  bg_mediaconnector_create_conn(&conn);
  
  out_plugs = calloc(num_outfiles, sizeof(*out_plugs));

  for(i = 0; i < num_outfiles; i++)
    {
    out_plugs[i] = gavftools_create_out_plug();

    if(!gavftools_open_out_plug_from_in_plug(out_plugs[i], outfiles[i],
                                             in_plug))
      goto fail;
    
    if(!bg_plug_setup_writer(out_plugs[i], &conn))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Setting up plug writer failed");
      goto fail;
      }
    }
    /* Fire up connector */

  bg_mediaconnector_start(&conn);

  /* Run */

  while(1)
    {
    if(bg_plug_got_error(in_plug))
      break;

    if(gavftools_stop() ||
       !bg_mediaconnector_iteration(&conn))
      break;
    }
  
  ret = 0;
  fail:

  bg_mediaconnector_free(&conn);
  bg_plug_destroy(in_plug);
  
  for(i = 0; i < num_outfiles; i++)
    bg_plug_destroy(out_plugs[i]);

  gavftools_cleanup();
  

  
  return ret;
  }
