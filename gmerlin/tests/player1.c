// #define INFO_WINDOW

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <player.h>
#include <pluginregistry.h>
#include <gavl/gavl.h>
#include <utils.h>

#ifdef INFO_WINDOW
#include <gtk/gtk.h>
#include <gui_gtk/infowindow.h>
#endif // INFO_WINDOW

int     bg_argc;
int     bg_arg_index = 1;
char ** bg_argv;

bg_player_t * player;

bg_plugin_registry_t * plugin_reg;
bg_cfg_registry_t * cfg_reg;

bg_plugin_handle_t * input_handle = (bg_plugin_handle_t*)0;
void * input_priv;

int display_time = 1;

/* Input plugin stuff */

static void play_file(bg_player_t * player)
  {
  bg_msg_t * message;
  bg_msg_queue_t * queue;
  
  const bg_plugin_info_t * info;
  bg_input_plugin_t * plugin;

  /* Open input */
  
  /* Load input plugin */
  
  //  fprintf(stderr, "Searching plugin for %s...", filename);
  
  info = bg_plugin_find_by_filename(plugin_reg,
                                    bg_argv[bg_arg_index],
                                    BG_PLUGIN_INPUT);

  if(!info)
    {
    fprintf(stderr, "No plugin found, skipping file\n");
    return;
    }
  //  else
  //    fprintf(stderr, "Found %s\n", info->long_name);
  
  if(!input_handle ||
     strcmp(input_handle->info->name, info->name))
    {
    if(input_handle)
      bg_plugin_unref(input_handle);

    input_handle = bg_plugin_load(plugin_reg, info);
    }

  plugin = (bg_input_plugin_t*)(input_handle->plugin);
  
  /* Initialize input plugin */

  if(!plugin->open(input_handle->priv, bg_argv[bg_arg_index]))
    {
    fprintf(stderr, "Cannot open %s\n", bg_argv[bg_arg_index]);
    return;
    }
  
  /* Create message */

  queue = bg_player_get_command_queue(player);

  message = bg_msg_queue_lock_write(queue);
  bg_msg_set_id(message, BG_PLAYER_CMD_PLAY);

  bg_msg_set_arg_ptr_nocopy(message, 0, input_handle);
  bg_msg_set_arg_int(message, 1, 0);
  bg_msg_set_arg_int(message, 2, 0);
  bg_msg_queue_unlock_write(queue);
  bg_arg_index++;
  
  }

static int time_active = 0;
static gavl_time_t total_time  = 0;

static void print_time(int seconds)
  {
  char str[GAVL_TIME_STRING_LEN];
  int i;
  int len;
  int total_len;
  if(!display_time)
    return;
  total_len = 2*GAVL_TIME_STRING_LEN + 7;
  
  if(time_active)
    {
    for(i = 0; i < total_len; i++)
      {
      putc(0x08, stderr);
      }
    }

  gavl_time_prettyprint_seconds(seconds, str);

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

#define DO_PRINT(f, s) if(s)fprintf(stderr, f, s);

static void dump_metadata(bg_metadata_t * m)
  {
  DO_PRINT("Artist:    %s\n", m->artist);
  DO_PRINT("Author:    %s\n", m->author);
  DO_PRINT("Title:     %s\n", m->title);
  DO_PRINT("Album:     %s\n", m->album);
  DO_PRINT("Genre:     %s\n", m->genre);
  DO_PRINT("Copyright: %s\n", m->copyright);
  DO_PRINT("Date:      %s\n", m->date);
  DO_PRINT("Track:     %d\n", m->track);
  DO_PRINT("Comment:   %s\n", m->comment);
  }

static int handle_message(bg_player_t * player,
                          bg_msg_t * message)
  {
  int arg_i1;
  char * arg_str1;
  gavl_time_t t;
  gavl_audio_format_t audio_format;
  gavl_video_format_t video_format;
  bg_metadata_t metadata;

  switch(bg_msg_get_id(message))
    {
    case BG_PLAYER_MSG_TIME_CHANGED:
      t = bg_msg_get_arg_time(message, 0);
      print_time(t/GAVL_TIME_SCALE);
      break;
    case BG_PLAYER_MSG_TRACK_DURATION:
      total_time = bg_msg_get_arg_time(message, 0);
      break;
    case BG_PLAYER_MSG_TRACK_NAME:
      arg_str1 = bg_msg_get_arg_string(message, 0);
      if(arg_str1)
        {
        fprintf(stderr, "Track: %s\n", arg_str1);
        free(arg_str1);
        }
      break;
    case BG_PLAYER_MSG_TRACK_NUM_STREAMS:
      arg_i1 = bg_msg_get_arg_int(message, 0);
      if(arg_i1)
        {
        fprintf(stderr, "Audio streams %d\n", arg_i1);
        }
      arg_i1 = bg_msg_get_arg_int(message, 1);
      if(arg_i1)
        {
        fprintf(stderr, "Video streams %d\n", arg_i1);
        }
      arg_i1 = bg_msg_get_arg_int(message, 2);
      if(arg_i1)
        {
        fprintf(stderr, "Subpicture streams %d\n", arg_i1);
        }
      break;

    case BG_PLAYER_MSG_AUDIO_STREAM:
      arg_i1 = bg_msg_get_arg_int(message, 0);
      fprintf(stderr, "Playing audio stream %d\n", arg_i1 + 1);
      bg_msg_get_arg_audio_format(message, 1, &audio_format);
      fprintf(stderr, "Input format:\n");
      gavl_audio_format_dump(&audio_format);
      bg_msg_get_arg_audio_format(message, 2, &audio_format);
      fprintf(stderr, "Output format:\n");
      gavl_audio_format_dump(&audio_format);
      break;

    case BG_PLAYER_MSG_VIDEO_STREAM:
      arg_i1 = bg_msg_get_arg_int(message, 0);
      fprintf(stderr, "Playing video stream %d\n", arg_i1 + 1);
      bg_msg_get_arg_video_format(message, 1, &video_format);
      fprintf(stderr, "Input format:\n");
      gavl_video_format_dump(&video_format);
      bg_msg_get_arg_video_format(message, 2, &video_format);
      fprintf(stderr, "Output format:\n");
      gavl_video_format_dump(&video_format);
      break;
    case BG_PLAYER_MSG_STREAM_DESCRIPTION:
    case BG_PLAYER_MSG_AUDIO_DESCRIPTION:
    case BG_PLAYER_MSG_VIDEO_DESCRIPTION:
    case BG_PLAYER_MSG_SUBPICTURE_DESCRIPTION:
      arg_str1 = bg_msg_get_arg_string(message, 0);
      fprintf(stderr, "%s\n", arg_str1);
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
          fprintf(stderr, "Stopped\n");
          if(bg_arg_index >= bg_argc)
            return 0;
          else
            play_file(player);
          break;
        case BG_PLAYER_STATE_PLAYING:
          break;
        case BG_PLAYER_STATE_SEEKING:
          break;
        case BG_PLAYER_STATE_CHANGING:
          fprintf(stderr, "Changing...");
          if(bg_arg_index >= bg_argc)
            {
            fprintf(stderr, "Exiting\n");
            return 0;
            }
          else
            {
            fprintf(stderr, "Next track\n");
            play_file(player);
            }
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
  bg_msg_queue_t * queue;
  char * tmp_path;
  int i;
  int remove_arg;
#ifdef INFO_WINDOW
  bg_gtk_info_window_t * info_window;
#endif
  const bg_plugin_info_t * info;
  bg_cfg_section_t * cfg_section;
  bg_plugin_handle_t * handle;

  bg_msg_queue_t * message_queue;
#ifdef INFO_WINDOW
  gtk_init(&argc, &argv);
#endif
  
  player = bg_player_create();

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

  for(i = 1; i < argc; i++)
    {
    remove_arg = 0;
    if(!strcmp(argv[i], "-nt"))
      {
      display_time = 0;
      remove_arg = 1;
      }
    else if(!strcmp(argv[i], "--"))
      {
      break;
      }
      if(remove_arg)
        {
        if(i < argc-1)
          {
          memcpy(&(argv[i]), &(argv[i+1]), (argc - i)*sizeof(char*));
          i--;
          }
        argc--;
        }
    }
  bg_argc = argc;
  bg_argv = argv;

  /* Start the player thread */

  bg_player_run(player);

  /* Load output plugins */
  queue = bg_player_get_command_queue(player);

  info = bg_plugin_registry_get_default(plugin_reg,
                                        BG_PLUGIN_OUTPUT_AUDIO);

  handle = bg_plugin_load(plugin_reg, info);

  message = bg_msg_queue_lock_write(queue);
  bg_msg_set_id(message, BG_PLAYER_CMD_SET_OA_PLUGIN);
  
  bg_msg_set_arg_ptr_nocopy(message, 0, handle);
  bg_msg_queue_unlock_write(queue);

  
  info = bg_plugin_registry_get_default(plugin_reg,
                                        BG_PLUGIN_OUTPUT_VIDEO);
  handle = bg_plugin_load(plugin_reg, info);

  message = bg_msg_queue_lock_write(queue);
  bg_msg_set_id(message, BG_PLAYER_CMD_SET_OV_PLUGIN);
  
  bg_msg_set_arg_ptr_nocopy(message, 0, handle);
  bg_msg_queue_unlock_write(queue);

  
  /* Main loop */
  
#ifndef INFO_WINDOW
  while(1)
    {
    message = bg_msg_queue_lock_read(message_queue);
    if(!handle_message(player, message))
      {
      bg_msg_queue_unlock_read(message_queue);
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
