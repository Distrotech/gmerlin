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

#include <string.h>

#include <config.h>
#include <gmerlin/cfg_registry.h>
#include <gmerlin/cmdline.h>
#include <gmerlin/utils.h>
#include <gmerlin/remote.h>
#include <gmerlin/translation.h>
#include "player_remote.h"

#include <gmerlin/log.h>
#define LOG_DOMAIN "gmerlin_remote"

static void cmd_get_name(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char * str;
  
  remote = (bg_remote_client_t *)data;
  msg = bg_remote_client_get_msg_write(remote);
  bg_msg_set_id(msg, PLAYER_COMMAND_GET_NAME);
  bg_remote_client_done_msg_write(remote);

  msg = bg_remote_client_get_msg_read(remote);
  if(!msg)
    return;

  str = bg_msg_get_arg_string(msg, 0);
  if(str)
    {
    printf("Name: %s\n", str);
    free(str);
    }
  }

static void cmd_get_metadata(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  msg = bg_remote_client_get_msg_write(remote);
  bg_msg_set_id(msg, PLAYER_COMMAND_GET_METADATA);
  bg_remote_client_done_msg_write(remote);
  
  }

static void cmd_get_time(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  msg = bg_remote_client_get_msg_write(remote);
  bg_msg_set_id(msg, PLAYER_COMMAND_GET_METADATA);
  bg_remote_client_done_msg_write(remote);
  
  }

static void cmd_play(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  
  msg = bg_remote_client_get_msg_write(remote);
  bg_msg_set_id(msg, PLAYER_COMMAND_PLAY);
  bg_remote_client_done_msg_write(remote);
  }

static void cmd_next(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_NEXT);
  

  bg_remote_client_done_msg_write(remote);
  }

static void cmd_prev(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_PREV);
  

  bg_remote_client_done_msg_write(remote);
  }

static void cmd_stop(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_STOP);


  bg_remote_client_done_msg_write(remote);
  }

static void cmd_toggle_mute(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_TOGGLE_MUTE);


  bg_remote_client_done_msg_write(remote);
  }


static void cmd_pause(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_PAUSE);

  bg_remote_client_done_msg_write(remote);
  }

static void cmd_addplay(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  remote = (bg_remote_client_t *)data;

  
  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -addplay requires an argument\n");
    exit(-1);
    }
  
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_PLAY_LOCATION);
  bg_msg_set_arg_string(msg, 0, argv[arg]);
  bg_cmdline_remove_arg(argc, _argv, arg);

  bg_remote_client_done_msg_write(remote);
  }

static void cmd_add(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -add requires an argument\n");
    exit(-1);
    }
  
  msg = bg_remote_client_get_msg_write(remote);
  
  bg_msg_set_id(msg, PLAYER_COMMAND_ADD_LOCATION);
  bg_msg_set_arg_string(msg, 0, argv[arg]);
  bg_cmdline_remove_arg(argc, _argv, arg);

  bg_remote_client_done_msg_write(remote);
  }

static void cmd_openplay(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;

  remote = (bg_remote_client_t *)data;
  
  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -openplay requires an argument\n");
    exit(-1);
    }
  
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_PLAY_DEVICE);
  bg_msg_set_arg_string(msg, 0, argv[arg]);
  bg_cmdline_remove_arg(argc, _argv, arg);

  bg_remote_client_done_msg_write(remote);
  }

static void cmd_open(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -open requires an argument\n");
    exit(-1);
    }
  
  msg = bg_remote_client_get_msg_write(remote);
  
  bg_msg_set_id(msg, PLAYER_COMMAND_OPEN_DEVICE);
  bg_msg_set_arg_string(msg, 0, argv[arg]);
  bg_cmdline_remove_arg(argc, _argv, arg);

  bg_remote_client_done_msg_write(remote);
  }

static void cmd_volume(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  float vol;
  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -volume requires an argument\n");
    exit(-1);
    }
  vol = strtod(argv[arg], NULL);

  msg = bg_remote_client_get_msg_write(remote);
  bg_msg_set_id(msg, PLAYER_COMMAND_SET_VOLUME);
  bg_msg_set_arg_float(msg, 0, vol);
  
  bg_remote_client_done_msg_write(remote);
  }

static void cmd_volume_rel(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  float vol;

  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -volume_rel requires an argument\n");
    exit(-1);
    }

  vol = strtod(argv[arg], NULL);
  
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_SET_VOLUME_REL);
  bg_msg_set_arg_float(msg, 0, vol);
  
  bg_remote_client_done_msg_write(remote);
  }

static void cmd_seek_rel(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  float diff;

  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -seek_rel requires an argument\n");
    exit(-1);
    }

  diff = strtod(argv[arg], NULL);
  
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_SEEK_REL);
  bg_msg_set_arg_time(msg, 0, gavl_seconds_to_time(diff));
  
  bg_remote_client_done_msg_write(remote);
  }

static void cmd_chapter(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  int index;

  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -chapter requires an argument\n");
    exit(-1);
    }

  msg = bg_remote_client_get_msg_write(remote);

  if(!strcmp(argv[arg], "+"))
    bg_msg_set_id(msg, PLAYER_COMMAND_NEXT_CHAPTER);
  else if(!strcmp(argv[arg], "-"))
    bg_msg_set_id(msg, PLAYER_COMMAND_PREV_CHAPTER);
  else
    {
    index = atoi(argv[arg]);
    bg_msg_set_id(msg, PLAYER_COMMAND_SET_CHAPTER);
    bg_msg_set_arg_int(msg, 0, index);
    }
  bg_remote_client_done_msg_write(remote);
  }



bg_cmdline_arg_t commands[] =
  {
    {
      .arg =         "-play",
      .help_string = TRS("Play current track"),
      .callback =    cmd_play,
    },
    {
      .arg =         "-next",
      .help_string = TRS("Switch to next track"),
      .callback =    cmd_next,
    },
    {
      .arg =         "-prev",
      .help_string = TRS("Switch to previous track"),
      .callback =    cmd_prev,
    },
    {
      .arg =         "-stop",
      .help_string = TRS("Stop playback"),
      .callback =    cmd_stop,
    },
    {
      .arg =         "-pause",
      .help_string = TRS("Pause playback"),
      .callback =    cmd_pause,
    },
    {
      .arg =         "-mute",
      .help_string = TRS("Toggle mute"),
      .callback =    cmd_toggle_mute,
    },
    {
      .arg =         "-add",
      .help_arg =    TRS("<gml>"),
      .help_string = TRS("Add <gml> to the incoming album"),
      .callback =    cmd_add,
    },
    {
      .arg =         "-addplay",
      .help_arg =    TRS("<gml>"),
      .help_string = TRS("Add <gml> to the incoming album and play it"),
      .callback =    cmd_addplay,
    },
    {
      .arg =         "-open",
      .help_arg =    TRS("<device>"),
      .help_string = TRS("Open album for <device>. Device must be a GML (e.g. dvd:///dev/hdd)."),
      .callback =    cmd_open,
    },
    {
      .arg =         "-openplay",
      .help_arg =    TRS("<device>"),
      .help_string = TRS("Open album for <device> and play first track. Device must be a GML (e.g. dvd:///dev/hdd)."),
      .callback =    cmd_openplay,
    },
    {
      .arg =         "-volume",
      .help_arg =    TRS("<volume>"),
      .help_string = TRS("Set player volume. <volume> is in dB, 0.0 is max"),
      .callback =    cmd_volume,
    },
    {
      .arg =         "-volume-rel",
      .help_arg =    TRS("<diff>"),
      .help_string = TRS("In- or decrease player volume. <diff> is in dB"),
      .callback =    cmd_volume_rel,
    },
    {
      .arg =         "-seek-rel",
      .help_arg =    TRS("<diff>"),
      .help_string = TRS("Seek relative. <diff> is in seconds."),
      .callback =    cmd_seek_rel,
    },
    {
      .arg =         "-chapter",
      .help_arg =    "[num|+|-]",
      .help_string = TRS("Go to the specified chapter. Use '+' and '-' to go to the next or previous chapter respectively"),
      .callback =    cmd_chapter,
    },
    {
      .arg =         "-get-name",
      .help_string = TRS("Print name of the current track"),
      .callback =    cmd_get_name,
    },
    {
      .arg =         "-get-metadata",
      .help_string = TRS("Print metadata of the current track"),
      .callback =    cmd_get_metadata,
    },
    {
      .arg =         "-get-time",
      .help_string = TRS("Print time of the current track"),
      .callback =    cmd_get_time,
    },
    { /* End of options */ }
  };

char * host = NULL;
int port;
int launch = 0;

static void opt_host(void * data, int * argc, char *** argv, int arg)
  {
  
  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -host requires an argument");
    exit(-1);
    }
  host = bg_strdup(host, (*argv)[arg]);
  bg_cmdline_remove_arg(argc, argv, arg);
  }

static void opt_port(void * data, int * argc, char *** argv, int arg)
  {
  
  if(arg >= *argc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Option -port requires an argument");
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
      .help_string = TRS("Host to connect to, default is localhost"),
      .callback =    opt_host,
    },
    {
      .arg =         "-port",
      .help_arg =    "<port>",
      .help_string = TRS("Port to connect to"),
      .callback =    opt_port,
    },
    {
      .arg =         "-launch",
      .help_string = TRS("Launch new player if necessary"),
      .callback =    opt_launch,
    },
    { /* End of options */ }
  };

#if 0
static void opt_help(void * data, int * argc, char *** argv, int arg)
  {
  FILE * out = stderr;
  
  fprintf(out, "Usage: %s [options] command\n\n", (*argv)[0]);
  fprintf(out, "Options:\n\n");
  bg_cmdline_print_help(global_options);
  fprintf(out, "\ncommand is of the following:\n\n");
  bg_cmdline_print_help(commands);
  exit(0);
  }
#endif



const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin_remote",
    .synopsis = TRS("[options] command\n"),
    .help_before = TRS("Remote control command for the Gmerlin GUI Player\n"),
    .args = (bg_cmdline_arg_array_t[]) { { TRS("Global options"), global_options },
                                       { TRS("Commands"),       commands       },
                                       {  } },
    .env = (bg_cmdline_ext_doc_t[])
    { { PLAYER_REMOTE_ENV,
        TRS("Default port for the remote control") },
      { /* End */ }
    },

  };

int main(int argc, char ** argv)
  {
  char * env;
  int i;
  gavl_time_t delay_time = GAVL_TIME_SCALE / 50;
  bg_remote_client_t * remote;

  bg_cmdline_init(&app_data);
  
  if(argc < 2)
    bg_cmdline_print_help(argv[0], 0);
  
  port = PLAYER_REMOTE_PORT;
  env = getenv(PLAYER_REMOTE_ENV);
  if(env)
    port = atoi(env);

  bg_cmdline_init(&app_data);

  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  remote = bg_remote_client_create(PLAYER_REMOTE_ID, 0);

  if(!host)
    host = bg_strdup(host, "localhost");

  if(!bg_remote_client_init(remote, host, port, 1000))
    {
    if(launch)
      {
      if(system("gmerlin &"))
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot launch gmerlin process");
        return -1;
        }
      
      for(i = 0; i < 50; i++)
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
