#include <cmdline.h>
#include <utils.h>
#include <remote.h>
#include "player_remote.h"

/*
 *  Options:
 *  -host <hostname>
 *  -port <port>
 *  -spawn
 *
 *  Commands:
 *  -p --play <location>: Load location into incoming and play it
 *  -a --add <location>:  Load location into incoming
 *  -P:                   Play current track
 *  -v+
 */

#if 0
typedef struct
  {
  char * arg;
  char * help_string;
  void (*callback)(void * data, int * argc, char *** argv, int arg);
  } bg_cmdline_arg_t;
#endif

static void cmd_play(void * data, int * argc, char *** _argv, int arg)
  {
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  remote = (bg_remote_client_t *)data;
  
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_PLAY);

  //  fprintf(stderr, "Remote -> server: PLAYER_COMMAND_PLAY\n");
  
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

  fprintf(stderr, "cmd_addplay\n");
  
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -addplay requires an argument\n");
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

  fprintf(stderr, "cmd_add\n");
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -add requires an argument\n");
    exit(-1);
    }
  
  msg = bg_remote_client_get_msg_write(remote);
  
  bg_msg_set_id(msg, PLAYER_COMMAND_ADD_LOCATION);
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
    fprintf(stderr, "Option -volume requires an argument\n");
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
    fprintf(stderr, "Option -volume_rel requires an argument\n");
    exit(-1);
    }

  vol = strtod(argv[arg], NULL);
  
  msg = bg_remote_client_get_msg_write(remote);

  bg_msg_set_id(msg, PLAYER_COMMAND_SET_VOLUME_REL);
  bg_msg_set_arg_float(msg, 0, vol);
  
  bg_remote_client_done_msg_write(remote);
  }
  
bg_cmdline_arg_t commands[] =
  {
    {
      arg:         "-play",
      help_string: "Play current track",
      callback:    cmd_play,
    },
    {
      arg:         "-next",
      help_string: "Switch to next track",
      callback:    cmd_next,
    },
    {
      arg:         "-prev",
      help_string: "Switch to previous track track",
      callback:    cmd_prev,
    },
    {
      arg:         "-stop",
      help_string: "Stop playback",
      callback:    cmd_stop,
    },
    {
      arg:         "-pause",
      help_string: "Pause playback",
      callback:    cmd_pause,
    },
    {
      arg:         "-add",
      help_arg:    "<location>",
      help_string: "Add <location> to the incoming album",
      callback:    cmd_add,
    },
    {
      arg:         "-addplay",
      help_arg:    "<location>",
      help_string: "Add <location> to the incoming album and play it",
      callback:    cmd_addplay,
    },
    {
      arg:         "-volume",
      help_arg:    "<volume>",
      help_string: "Set player volume. <volume> is in dB, 0.0 is max",
      callback:    cmd_volume,
    },
    {
      arg:         "-volume_rel",
      help_arg:    "<diff>",
      help_string: "In- or decrease player volume. <diff> is in dB",
      callback:    cmd_volume_rel,
    },
    
    { /* End of options */ }
  };

char * host = (char *)0;
int port = PLAYER_REMOTE_PORT;
int spawn = 0;

static void opt_host(void * data, int * argc, char *** argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -host requires an argument\n");
    exit(-1);
    }
  host = bg_strdup(host, (*argv)[arg]);
  bg_cmdline_remove_arg(argc, argv, arg);
  }

static void opt_port(void * data, int * argc, char *** argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -port requires an argument\n");
    exit(-1);
    }
  port = atoi((*argv)[arg]);
  bg_cmdline_remove_arg(argc, argv, arg);
  }

static void opt_spawn(void * data, int * argc, char *** argv, int arg)
  {
  spawn = 1;
  }

static void opt_help(void * data, int * argc, char *** argv, int arg);

static bg_cmdline_arg_t global_options[] =
  {
    {
      arg:         "-host",
      help_arg:    "<hostname>",
      help_string: "Host to connect to, default is localhost",
      callback:    opt_host,
    },
    {
      arg:         "-port",
      help_arg:    "<port>",
      help_string: "Port to connect to",
      callback:    opt_port,
    },
    {
      arg:         "-spawn",
      help_string: "Spawn new player is necessary",
      callback:    opt_spawn,
    },
    {
      arg:         "-help",
      help_string: "Print this help message and exit",
      callback:    opt_help,
    },
    { /* End of options */ }
  };

static void opt_help(void * data, int * argc, char *** argv, int arg)
  {
  fprintf(stderr, "Usage: %s [options] [command]\n\n", (*argv)[0]);
  fprintf(stderr, "Options:\n\n");
  bg_cmdline_print_help(global_options);
  fprintf(stderr, "\nCommand is of the following:\n\n");
  bg_cmdline_print_help(commands);
  exit(0);
  }


int main(int argc, char ** argv)
  {
  int i;
  gavl_time_t delay_time = GAVL_TIME_SCALE / 50;
  bg_remote_client_t * remote;

  if(argc < 2)
    opt_help(NULL, &argc, &argv, 0);
  
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  remote = bg_remote_client_create(PLAYER_REMOTE_ID, 0);

  if(!host)
    host = bg_strdup(host, "localhost");

  if(!bg_remote_client_init(remote, host, port, 1000))
    {
    //    fprintf(stderr, "Initializing remote failed\n");
    if(spawn)
      {
      if(system("gmerlin &"))
        {
        fprintf(stderr, "Cannot spawn gmerlin process\n");
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
