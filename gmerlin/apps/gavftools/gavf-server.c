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

#include "gavf-server.h"

#define LOG_DOMAIN "gavf-server"

static int num_listen_addresses = 0;
static char ** listen_addresses;

static void
opt_l(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -l requires an argument\n");
    exit(-1);
    }
  
  listen_addresses = realloc(listen_addresses,
                             sizeof(*listen_addresses) * (num_listen_addresses+1));
  listen_addresses[num_listen_addresses] = (*_argv)[arg];
  num_listen_addresses++;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-l",
      .help_arg =    "<address>",
      .help_string = "Listen address (e.g. tcp://0.0.0.0:5555 or unix://socket). Use this option more than once to specify multiple addresses.",
      .callback =    opt_l,
    },
    { /* End */ }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-server",
    .long_name = TRS("Server for gavf streams"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf server\n"),
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
  gavl_time_t delay_time = GAVL_TIME_SCALE / 100;
  int ret = 1;
  server_t * s = NULL;
    
  gavftools_init();

  gavftools_set_cmdline_parameters(global_options);
  
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  if(!bg_cmdline_check_unsupported(argc, argv))
    return -1;

  if(!num_listen_addresses)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No listen addresses specified");
    goto fail;
    }

  if(!(s = server_create(listen_addresses, num_listen_addresses)))
    goto fail;
  
  while(1)
    {
    if(gavftools_stop() ||
       !server_iteration(s))
      break;

    gavl_time_delay(&delay_time);
    }
  
  ret = 0;
  fail:

  if(s)
    server_destroy(s);
    
  return ret;
  
  }
