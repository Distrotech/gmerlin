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
#include <gmerlin/cfg_registry.h>
#include <gmerlin/cmdline.h>
#include <gmerlin/utils.h>
#include <gmerlin/remote.h>
#include <gmerlin/translation.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "gmerlin_transcoder_remote"

#include "transcoder_remote.h"

static void cmd_addalbum(void * data, int * argc, char *** _argv, int arg)
  {
  FILE * out = stderr;
  char * xml_string;

  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  remote = (bg_remote_client_t *)data;
  
  if(arg >= *argc)
    {
    fprintf(out, "Option -addalbum requires an argument\n");
    exit(-1);
    }

  /* Load the entire xml file into a string */

  xml_string = bg_read_file(argv[arg], NULL);
  if(!xml_string)
    return;
  
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, TRANSCODER_REMOTE_ADD_ALBUM);

  bg_msg_set_arg_string(msg, 0, xml_string);
  bg_cmdline_remove_arg(argc, _argv, arg);
  
  bg_remote_client_done_msg_write(remote);

  free(xml_string);
  }

static void cmd_add(void * data, int * argc, char *** _argv, int arg)
  {
  FILE * out = stderr;

  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  
  if(arg >= *argc)
    {
    fprintf(out, "Option -add requires an argument\n");
    exit(-1);
    }
  
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, TRANSCODER_REMOTE_ADD_FILE);

  bg_msg_set_arg_string(msg, 0, (*_argv)[arg]);
  bg_cmdline_remove_arg(argc, _argv, arg);
  
  bg_remote_client_done_msg_write(remote);
  }

bg_cmdline_arg_t commands[] =
  {
    {
      .arg =         "-addalbum",
      .help_arg =    "<album_file>",
      .help_string = "Add album to track list",
      .callback =    cmd_addalbum,
    },
    {
      .arg =         "-add",
      .help_arg =    "<location>",
      .help_string = "Add <location> to the track list",
      .callback =    cmd_add,
    },
    { /* End of options */ }
  };

char * host = NULL;
int port = TRANSCODER_REMOTE_PORT;
int launch = 0;


static void opt_host(void * data, int * argc, char *** argv, int arg)
  {
  FILE * out = stderr;
  if(arg >= *argc)
    {
    fprintf(out, "Option -host requires an argument\n");
    exit(-1);
    }
  host = bg_strdup(host, (*argv)[arg]);
  bg_cmdline_remove_arg(argc, argv, arg);
  }

static void opt_port(void * data, int * argc, char *** argv, int arg)
  {
  FILE * out = stderr;
  if(arg >= *argc)
    {
    fprintf(out, "Option -port requires an argument\n");
    exit(-1);
    }
  port = atoi((*argv)[arg]);
  bg_cmdline_remove_arg(argc, argv, arg);
  }

static void opt_launch(void * data, int * argc, char *** argv, int arg)
  {
  launch = 1;
  }


static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-host",
      .help_arg =    "<hostname>",
      .help_string = "Host to connect to, default is localhost",
      .callback =    opt_host,
    },
    {
      .arg =         "-port",
      .help_arg =    "<port>",
      .help_string = "Port to connect to",
      .callback =    opt_port,
    },
    {
      .arg =         "-launch",
      .help_string = "Launch new transcoder if necessary",
      .callback =    opt_launch,
    },
    { /* End of options */ }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin_transcoder_remote",
    .synopsis = TRS("[options] command\n"),
    .help_before = TRS("Remote control command for the Gmerlin GUI transcoder\n"),
    .args = (bg_cmdline_arg_array_t[]) { { TRS("Global options"), global_options },
                                       { TRS("Commands"),       commands       },
                                       {  } },
    .env = (bg_cmdline_ext_doc_t[])
    { { TRANSCODER_REMOTE_ENV,
        TRS("Default port for the remote control") },
      { /* End */ }
    },
  };

int main(int argc, char ** argv)
  {
  int i;
  gavl_time_t delay_time = GAVL_TIME_SCALE / 50;
  bg_remote_client_t * remote;
  char * env;
  bg_cmdline_init(&app_data);
  
  if(argc < 2)
    bg_cmdline_print_help(argv[0], 0);
  
  port = TRANSCODER_REMOTE_PORT;
  env = getenv(TRANSCODER_REMOTE_ENV);
  if(env)
    port = atoi(env);

  
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  remote = bg_remote_client_create(TRANSCODER_REMOTE_ID, 0);

  if(!host)
    host = bg_strdup(host, "localhost");

  if(!bg_remote_client_init(remote, host, port, 1000))
    {
    if(launch)
      {
      if(system("gmerlin_transcoder &"))
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot launch transcoder process");
        return -1;
        }

      for(i = 0; i < 500; i++)
        {
        if(bg_remote_client_init(remote, host, port, 1000))
          break;
        gavl_time_delay(&delay_time);
        }
      }
    else
      return -1;
    }
  bg_cmdline_parse(commands, &argc, &argv, remote);

  bg_remote_client_destroy(remote);
  return 0;
  }
