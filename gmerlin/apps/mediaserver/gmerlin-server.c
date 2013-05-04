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


static bg_cmdline_arg_t global_options[] =
  {
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
  
  if(!server_init(&s))
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
