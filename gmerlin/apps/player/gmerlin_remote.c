/*****************************************************************
 
  gmerlin_remote.c
 
  Copyright (c) 2003-2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>

#include <config.h>
#include <cfg_registry.h>
#include <cmdline.h>
#include <utils.h>
#include <remote.h>
#include "player_remote.h"

#include <log.h>
#define LOG_DOMAIN "gmerlin_remote"

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
  FILE * out = stderr;
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  remote = (bg_remote_client_t *)data;

  
  if(arg >= *argc)
    {
    fprintf(out, "Option -addplay requires an argument\n");
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
  FILE * out = stderr;
  bg_msg_t * msg;
  bg_remote_client_t * remote;
  char ** argv = *_argv;
  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    fprintf(out, "Option -add requires an argument\n");
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
  FILE * out = stderr;

  remote = (bg_remote_client_t *)data;
  
  if(arg >= *argc)
    {
    fprintf(out, "Option -openplay requires an argument\n");
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
  FILE * out = stderr;
  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    fprintf(out, "Option -open requires an argument\n");
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
  FILE * out = stderr;
  float vol;
  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    fprintf(out, "Option -volume requires an argument\n");
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
  FILE * out = stderr;

  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    fprintf(out, "Option -volume_rel requires an argument\n");
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
  FILE * out = stderr;

  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    fprintf(out, "Option -seek_rel requires an argument\n");
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
  FILE * out = stderr;

  remote = (bg_remote_client_t *)data;

  if(arg >= *argc)
    {
    fprintf(out, "Option -chapter requires an argument\n");
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
      help_string: "Switch to previous track",
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
      arg:         "-mute",
      help_string: "Toggle mute",
      callback:    cmd_toggle_mute,
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
      arg:         "-open",
      help_arg:    "<device>",
      help_string: "Open album for <device>. Device must be a GML (e.g. dvd:///dev/hdd).",
      callback:    cmd_open,
    },
    {
      arg:         "-openplay",
      help_arg:    "<device>",
      help_string: "Open album for <device> and play first track. Device must be a GML (e.g. dvd:///dev/hdd).",
      callback:    cmd_openplay,
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
    {
      arg:         "-seek_rel",
      help_arg:    "<diff>",
      help_string: "Seek relative. <diff> is in seconds.",
      callback:    cmd_seek_rel,
    },
    {
      arg:         "-chapter",
      help_arg:    "[num|+|-]>",
      help_string: "Go to the specified chapter. Use '+' and '-' to go to the next or previous chapter respectively",
      callback:    cmd_chapter,
    },
    
    { /* End of options */ }
  };

char * host = (char *)0;
int port;
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
      arg:         "-launch",
      help_string: "Launch new player if necessary",
      callback:    opt_launch,
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
  FILE * out = stderr;
  
  fprintf(out, "Usage: %s [options] command\n\n", (*argv)[0]);
  fprintf(out, "Options:\n\n");
  bg_cmdline_print_help(global_options);
  fprintf(out, "\ncommand is of the following:\n\n");
  bg_cmdline_print_help(commands);
  exit(0);
  }


int main(int argc, char ** argv)
  {
  char * env;
  int i;
  gavl_time_t delay_time = GAVL_TIME_SCALE / 50;
  bg_remote_client_t * remote;
  
  if(argc < 2)
    opt_help(NULL, &argc, &argv, 0);

  port = PLAYER_REMOTE_PORT;
  env = getenv(PLAYER_REMOTE_ENV);
  if(env)
    port = atoi(env);
  
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
