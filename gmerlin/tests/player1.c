// #define INFO_WINDOW

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <player.h>
#include <pluginregistry.h>
#include <gavl/gavl.h>
#include <utils.h>
#include <cmdline.h>
#include <log.h>

#ifdef INFO_WINDOW
#include <gtk/gtk.h>
#include <gui_gtk/infowindow.h>
#endif // INFO_WINDOW

#define LOG_DOMAIN "gmerlin_player"

bg_player_t * player;

bg_plugin_registry_t * plugin_reg;
bg_cfg_registry_t * cfg_reg;

bg_plugin_handle_t * input_handle = (bg_plugin_handle_t*)0;
int display_time = 1;

char * audio_output_name = (char*)0;
char * video_output_name = (char*)0;

char ** gmls = (char **)0;
int gml_index = 0;

bg_cfg_section_t * audio_section;
bg_cfg_section_t * video_section;
bg_cfg_section_t * osd_section;

/*
 *  Commandline options stuff
 */

static void opt_oa(void * data, int * argc, char *** _argv, int arg)
  {
  char ** argv = *_argv;
  
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -oa requires an argument\n");
    exit(-1);
    }
  audio_output_name = bg_strdup(audio_output_name, argv[arg]);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_ov(void * data, int * argc, char *** _argv, int arg)
  {
  char ** argv = *_argv;
  
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -ov requires an argument\n");
    exit(-1);
    }
  video_output_name = bg_strdup(video_output_name, argv[arg]);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_nt(void * data, int * argc, char *** _argv, int arg)
  {
  display_time = 0;
  }

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


static void opt_help(void * data, int * argc, char *** argv, int arg);

static bg_cmdline_arg_t global_options[] =
  {
    {
      arg:         "-oa",
      help_string: "Set audio plugin",
      callback:    opt_oa,
    },
    {
      arg:         "-ov",
      help_string: "Set video plugin",
      callback:    opt_ov,
    },
    {
      arg:         "-nt",
      help_string: "Disable time display",
      callback:    opt_nt,
    },
    {
      arg:         "-v",
      help_string: "Set verbosity level",
      callback:    opt_v,
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
  fprintf(stderr, "Usage: %s [options] gml...\n\n", (*argv)[0]);
  fprintf(stderr, "gml is a gmerlin media location (e.g. filename or URL)\n");
  fprintf(stderr, "Options:\n\n");
  bg_cmdline_print_help(global_options);
  exit(0);
  }



/* Input plugin stuff */

static void play_track(bg_player_t * player, const char * gml, int track,
                       const char * plugin_name)
  {
  const bg_plugin_info_t * info = (const bg_plugin_info_t *)0;
  bg_input_plugin_t * plugin;
  bg_track_info_t * track_info;
  int num_tracks;
  char * error_msg = (char*)0;
  if(plugin_name)
    info = bg_plugin_find_by_name(plugin_reg,
                                  plugin_name);
  
  if(!bg_input_plugin_load(plugin_reg, gml, info,
                           &input_handle, &error_msg,
                           (bg_input_callbacks_t*)0))
    {
    fprintf(stderr, "Cannot open %s\n", gml);
    if(error_msg)
      free(error_msg);
    return;
    }
  
  plugin = (bg_input_plugin_t*)(input_handle->plugin);

  num_tracks = plugin->get_num_tracks(input_handle->priv);

  if((track < 0) || (track >= num_tracks))
    {
    fprintf(stderr, "No such track %d\n", track);
    return;
    }
  
  track_info = plugin->get_track_info(input_handle->priv, track);
  
  if(!track_info)
    {
    return;
    }

  if(!track_info->name)
    bg_set_track_name_default(track_info, gml);
  bg_player_play(player, input_handle, track, 0, track_info->name);
  }

static int time_active = 0;
static gavl_time_t total_time  = 0;

static void print_time(gavl_time_t time)
  {
  char str[GAVL_TIME_STRING_LEN];
  int i;
  int len;
  int total_len;
  if(!display_time)
    return;
  total_len = 2*GAVL_TIME_STRING_LEN + 7;
  
  if(time_active)
    putc('\r', stderr);
  
  gavl_time_prettyprint(time, str);

  fprintf(stderr, "[ ");
  len = strlen(str);

  for(i = 0; i < GAVL_TIME_STRING_LEN - len - 1; i++)
    {
    fprintf(stderr, " ");
    }
  fprintf(stderr, "%s ]/", str);

  gavl_time_prettyprint(total_time, str);

  fprintf(stderr, "[ ");
  len = strlen(str);

  for(i = 0; i < GAVL_TIME_STRING_LEN - len - 1; i++)
    {
    fprintf(stderr, " ");
    }
  fprintf(stderr, "%s ]", str);
  
  time_active = 1;
  }

#define DO_PRINT(f, s) if(s)bg_log(BG_LOG_INFO, LOG_DOMAIN, f, s);

static void dump_metadata(bg_metadata_t * m)
  {
  DO_PRINT("Artist:    %s", m->artist);
  DO_PRINT("Author:    %s", m->author);
  DO_PRINT("Title:     %s", m->title);
  DO_PRINT("Album:     %s", m->album);
  DO_PRINT("Genre:     %s", m->genre);
  DO_PRINT("Copyright: %s", m->copyright);
  DO_PRINT("Date:      %s", m->date);
  DO_PRINT("Track:     %d", m->track);
  DO_PRINT("Comment:   %s", m->comment);
  }
#undef DO_PRINT

static int handle_message(bg_player_t * player,
                          bg_msg_t * message)
  {
  int arg_i1;
  char * arg_str1;
  gavl_time_t t;
  gavl_audio_format_t audio_format;
  gavl_video_format_t video_format;
  bg_metadata_t metadata;

  char * tmp_string_1;
  char * tmp_string_2;
  
  switch(bg_msg_get_id(message))
    {
    case BG_PLAYER_MSG_TIME_CHANGED:
      t = bg_msg_get_arg_time(message, 0);
      print_time(t);
      break;
    case BG_PLAYER_MSG_TRACK_DURATION:
      total_time = bg_msg_get_arg_time(message, 0);
      break;
    case BG_PLAYER_MSG_TRACK_NAME:
      arg_str1 = bg_msg_get_arg_string(message, 0);
      if(arg_str1)
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Name: %s", arg_str1);
      break;
    case BG_PLAYER_MSG_TRACK_NUM_STREAMS:
      arg_i1 = bg_msg_get_arg_int(message, 0);
      if(arg_i1)
        {
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Audio streams %d", arg_i1);
        }
      arg_i1 = bg_msg_get_arg_int(message, 1);
      if(arg_i1)
        {
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Video streams %d", arg_i1);
        }
      arg_i1 = bg_msg_get_arg_int(message, 2);
      if(arg_i1)
        {
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Subpicture streams %d", arg_i1);
        }
      break;
    case BG_PLAYER_MSG_AUDIO_STREAM:
      arg_i1 = bg_msg_get_arg_int(message, 0);

      bg_msg_get_arg_audio_format(message, 1, &audio_format);
      tmp_string_1 = bg_audio_format_to_string(&audio_format, 0);

      bg_msg_get_arg_audio_format(message, 2, &audio_format);
      tmp_string_2 = bg_audio_format_to_string(&audio_format, 0);
  
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Playing audio stream %d\nInput format:\n%s\nOutput format:\n%s",
             arg_i1+1, tmp_string_1, tmp_string_2);
      free(tmp_string_1);
      free(tmp_string_2);
      
      break;
    case BG_PLAYER_MSG_VIDEO_STREAM:
      arg_i1 = bg_msg_get_arg_int(message, 0);

      bg_msg_get_arg_video_format(message, 1, &video_format);
      tmp_string_1 = bg_video_format_to_string(&video_format, 0);

      bg_msg_get_arg_video_format(message, 2, &video_format);
      tmp_string_2 = bg_video_format_to_string(&video_format, 0);
  
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Playing video stream %d\nInput format:\n%s\nOutput format:\n%s",
             arg_i1+1, tmp_string_1, tmp_string_2);
      free(tmp_string_1);
      free(tmp_string_2);
      break;
    case BG_PLAYER_MSG_STREAM_DESCRIPTION:
      arg_str1 = bg_msg_get_arg_string(message, 0);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Format: %s", arg_str1);
      break;
    case BG_PLAYER_MSG_AUDIO_DESCRIPTION:
      arg_str1 = bg_msg_get_arg_string(message, 0);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Audio stream: %s", arg_str1);
      break;
    case BG_PLAYER_MSG_VIDEO_DESCRIPTION:
      arg_str1 = bg_msg_get_arg_string(message, 0);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Video stream: %s", arg_str1);
      break;
    case BG_PLAYER_MSG_SUBTITLE_DESCRIPTION:
      arg_str1 = bg_msg_get_arg_string(message, 0);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Subtitle stream: %s", arg_str1);
      free(arg_str1);
      break;
      /* Metadata */
    case BG_PLAYER_MSG_METADATA:
      memset(&metadata, 0, sizeof(metadata));
      bg_msg_get_arg_metadata(message, 0, &metadata);
      dump_metadata(&metadata);
      bg_metadata_free(&metadata);
      break;
    case BG_PLAYER_MSG_STATE_CHANGED:
      arg_i1 = bg_msg_get_arg_int(message, 0);
      switch(arg_i1)
        {
        case BG_PLAYER_STATE_STOPPED:
          break;
        case BG_PLAYER_STATE_PLAYING:
          fprintf(stderr, "Playing\n");
          break;
        case BG_PLAYER_STATE_SEEKING:
          break;
        case BG_PLAYER_STATE_CHANGING:
          fprintf(stderr, "Changing\n");
          gml_index++;
          if(!gmls[gml_index])
            return 0;
          else
            play_track(player, gmls[gml_index], 0, (const char*)0);
          break;
        case BG_PLAYER_STATE_BUFFERING:
          break;
        case BG_PLAYER_STATE_PAUSED:
          break;
        case BG_PLAYER_STATE_FINISHING:
          fprintf(stderr, "Finishing\n");
          break;
        }
      break;
    case BG_PLAYER_MSG_TRACK_CHANGED:
      break;
    }
  return 1;
  }

#ifdef INFO_WINDOW
static gboolean idle_callback(gpointer data)
  {
  bg_msg_t * msg;
  bg_msg_queue_t * q = (bg_msg_queue_t *)data;
  
  msg = bg_msg_queue_try_lock_read(q);
  if(!msg)
    return TRUE;
  
  if(!handle_message(player, msg))
    gtk_main_quit();
  bg_msg_queue_unlock_read(q);
  return TRUE;
  }

static void info_close_callback(bg_gtk_info_window_t * info_window,
                                void * data)
  {
  fprintf(stderr, "Info window now closed\n");
  }
#endif

int main(int argc, char ** argv)
  {
  
  bg_msg_t * message;
  char * tmp_path;
#ifdef INFO_WINDOW
  bg_gtk_info_window_t * info_window;
#endif
  const bg_plugin_info_t * info;
  bg_cfg_section_t * cfg_section;
  bg_plugin_handle_t * handle;

  bg_msg_queue_t * message_queue;
#ifdef INFO_WINDOW
  bg_gtk_init(&argc, &argv);
#endif
  
  player = bg_player_create();
  
  bg_player_set_volume(player, 0.0);
  
  /* Create config section */

  audio_section =
    bg_cfg_section_create_from_parameters("audio", bg_player_get_audio_parameters(player));
  video_section =
    bg_cfg_section_create_from_parameters("video", bg_player_get_video_parameters(player));
  osd_section =
    bg_cfg_section_create_from_parameters("osd", bg_player_get_osd_parameters(player));

  bg_cfg_section_apply(audio_section, bg_player_get_audio_parameters(player),
                       bg_player_set_audio_parameter, player);
  bg_cfg_section_apply(video_section, bg_player_get_video_parameters(player),
                       bg_player_set_video_parameter, player);

  bg_cfg_section_apply(video_section, bg_player_get_osd_parameters(player),
                       bg_player_set_osd_parameter, player);
  
  /* Create plugin regsitry */
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Create Plugin registry */
#ifdef INFO_WINDOW
  info_window =
    bg_gtk_info_window_create(player, info_close_callback, NULL);
  bg_gtk_info_window_show(info_window);
#endif

    
  /* Set message queue */

  message_queue = bg_msg_queue_create();

  bg_player_add_message_queue(player,
                              message_queue);


  /* Get commandline options */

  bg_cmdline_parse(global_options, &argc, &argv, NULL);
  gmls = bg_cmdline_get_locations_from_args(&argc, &argv);

  if(!gmls)
    {
    fprintf(stderr, "No input files given");
    return 0;
    }
  
  /* Start the player thread */

  bg_player_run(player);

  /* Load audio output */

  if(!audio_output_name)
    info = bg_plugin_registry_get_default(plugin_reg,
                                          BG_PLUGIN_OUTPUT_AUDIO);
  else
    info = bg_plugin_find_by_name(plugin_reg,
                                  audio_output_name);
  
  handle = bg_plugin_load(plugin_reg, info);
  bg_player_set_oa_plugin(player, handle);
  
  /* Load video output */

  if(!video_output_name)
    info = bg_plugin_registry_get_default(plugin_reg,
                                          BG_PLUGIN_OUTPUT_VIDEO);
  else
    info = bg_plugin_find_by_name(plugin_reg,
                                  video_output_name);
  handle = bg_plugin_load(plugin_reg, info);
  bg_player_set_ov_plugin(player, handle);

  /* Play first track */

  play_track(player, gmls[gml_index], 0, (const char*)0);
  
  /* Main loop */
  
#ifndef INFO_WINDOW
  while(1)
    {
    message = bg_msg_queue_lock_read(message_queue);
    if(!handle_message(player, message))
      {
      bg_msg_queue_unlock_read(message_queue);
      fprintf(stderr, "Finishing\n");
      break;
      }
    bg_msg_queue_unlock_read(message_queue);
    }
#else
  g_timeout_add(10, idle_callback, message_queue);
  gtk_main();
#endif // INFO_WINDOW

  bg_player_quit(player);
  bg_player_destroy(player);

  bg_msg_queue_destroy(message_queue);

  bg_cfg_registry_destroy(cfg_reg);
  bg_plugin_registry_destroy(plugin_reg);


  return 0;
  }
