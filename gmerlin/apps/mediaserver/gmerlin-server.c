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

#include "server.h"

#include <signal.h>

#define LOG_DOMAIN "gmerlin-server"

static int got_sigint = 0;
struct sigaction old_int_sigaction;
struct sigaction old_term_sigaction;

const bg_parameter_info_t * s_params;
bg_cfg_section_t * s_section;

static void sigint_handler(int sig)
  {
  got_sigint = 1;
  sigaction(SIGINT, &old_int_sigaction, 0);
  sigaction(SIGTERM, &old_term_sigaction, 0);
  
  switch(sig)
    {
    case SIGINT:
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Caught SIGINT, terminating");
      break;
    case SIGTERM:
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Caught SIGTERM, terminating");
      break;
    }
  }

static void set_sigint_handler()
  {
  struct sigaction sa;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = sigint_handler;
  if (sigaction(SIGINT, &sa, &old_int_sigaction) == -1)
    fprintf(stderr, "sigaction failed\n");
  if (sigaction(SIGTERM, &sa, &old_term_sigaction) == -1)
    fprintf(stderr, "sigaction failed\n");
  }

static void opt_s(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -s requires an argument\n");
    exit(-1);
    }

  if(!bg_cmdline_apply_options(s_section,
                               NULL,
                               NULL,
                               s_params,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-s",
      .help_arg =    "<options>",
      .help_string = "Sever options",
      .callback =    opt_s,
    },
    { /* End */ }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin-server",
    .long_name = TRS("Gmerlin media server"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gmerlin db editor\n"),
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
  server_t s;
  int ret = EXIT_FAILURE;

  set_sigint_handler();
  signal(SIGPIPE, SIG_IGN);

  server_init(&s);

  s_params = server_get_parameters();
  s_section = bg_cfg_section_create_from_parameters("server", s_params);
  
  bg_cmdline_arg_set_parameters(global_options, "-s",
                                s_params);
 
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  bg_cfg_section_apply(s_section,
                       s_params,
                       server_set_parameter,
                       &s);
 
  if(!server_start(&s))
    goto fail;
  
  while(server_iteration(&s))
    {
    if(got_sigint)
      break;
    }

  ret = EXIT_SUCCESS;  
  fail:
  server_cleanup(&s);
  return 0;
  }
