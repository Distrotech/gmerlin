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

// #define INFO_WINDOW

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include <config.h>
#include <gmerlin/player.h>
#include <gmerlin/pluginregistry.h>
#include <gavl/gavl.h>
#include <gavl/metatags.h>

#include <gmerlin/utils.h>
#include <gmerlin/cmdline.h>
#include <gmerlin/log.h>
#include <gmerlin/translation.h>

#ifdef INFO_WINDOW
#include <gtk/gtk.h>
#include <gui_gtk/infowindow.h>
#endif // INFO_WINDOW

#define LOG_DOMAIN "gmerlin_player"

/* In commandline apps, global variables are allowed :) */

static int time_active = 0;         // Whether to display the time
static gavl_time_t total_time  = 0; // Total time of the current track

int num_tracks;
int current_track = -1;

bg_player_t * player;

bg_plugin_registry_t * plugin_reg;
bg_cfg_registry_t * cfg_reg;

bg_plugin_handle_t * input_handle = NULL;
int display_time = 1;

bg_plugin_handle_t * oa_handle = NULL;
bg_plugin_handle_t * ov_handle = NULL;

char ** gmls = NULL;
int gml_index = 0;

/* Sections from the plugin registry */
bg_cfg_section_t * oa_section = NULL;
bg_cfg_section_t * ov_section = NULL;
bg_cfg_section_t * i_section = NULL;

/* Sections from the player */
bg_cfg_section_t * audio_section;
bg_cfg_section_t * video_section;
bg_cfg_section_t * osd_section;
bg_cfg_section_t * input_section;

char * input_plugin_name = NULL;

const bg_plugin_info_t * ov_info = NULL;
char * window_id = NULL;

/* Set up by registry */
static bg_parameter_info_t oa_parameters[] =
  {
    {
      .name =      "plugin",
      .long_name = "Audio output plugin",
      .opt =       "p",
      .type =      BG_PARAMETER_MULTI_MENU,
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t ov_parameters[] =
  {
    {
      .name =      "plugin",
      .long_name = "Video output plugin",
      .opt =       "p",
      .type =      BG_PARAMETER_MULTI_MENU,
    },
    {
      .name =      "window",
      .long_name = "Window ID",
      .opt =       "w",
      .type =      BG_PARAMETER_STRING,
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t i_parameters[] =
  {
    {
      .name =      "plugin",
      .long_name = "input plugin",
      .opt =       "p",
      .type =      BG_PARAMETER_MULTI_MENU,
    },
    { /* End of parameters */ }
  };


/* Set from player */
const bg_parameter_info_t * osd_parameters;
const bg_parameter_info_t * audio_parameters;
const bg_parameter_info_t * video_parameters;
const bg_parameter_info_t * input_parameters;


char * track_spec = NULL;
char * track_spec_ptr;

/*
 *  Commandline options stuff
 */

static void set_oa_parameter(void * data, const char * name,
                             const bg_parameter_value_t * val)
  {
  const bg_plugin_info_t * info;

  //  fprintf(stderr, "set_oa_parameter: %s\n", name);

  if(name && !strcmp(name, "plugin"))
    {
    //    fprintf(stderr, "set_oa_parameter: plugin: %s\n", val->val_str);

    info =  bg_plugin_find_by_name(plugin_reg, val->val_str);
    oa_handle = bg_plugin_load(plugin_reg, info);
    }
  else
    {
    if(oa_handle && oa_handle->plugin && oa_handle->plugin->set_parameter)
      oa_handle->plugin->set_parameter(oa_handle->priv, name, val);
    }
  }

static void opt_oa(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -oa requires an argument\n");
    exit(-1);
    }
  
  if(!bg_cmdline_apply_options(oa_section,
                               set_oa_parameter,
                               NULL,
                               oa_parameters,
                               (*_argv)[arg]))
    exit(-1);
    
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void set_ov_parameter(void * data, const char * name,
                             const bg_parameter_value_t * val)
  {
  if(name && !strcmp(name, "plugin"))
    {
    ov_info =  bg_plugin_find_by_name(plugin_reg, val->val_str);
    }
  else if(name && !strcmp(name, "window"))
    {
    window_id = gavl_strrep(window_id, val->val_str);
    }
#if 0
  else
    {
    if(ov_handle && ov_handle->plugin && ov_handle->plugin->set_parameter)
      ov_handle->plugin->set_parameter(ov_handle->priv, name, val);
    }
#endif
  }

static void set_i_parameter(void * data, const char * name,
                            const bg_parameter_value_t * val)
  {
  if(name && !strcmp(name, "plugin"))
    input_plugin_name = gavl_strrep(input_plugin_name,
                                  val->val_str);
  }

static void opt_ov(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -ov requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(ov_section,
                               set_ov_parameter,
                               NULL,
                               ov_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_i(void * data, int * argc, char *** _argv, int arg)
  {
  bg_cfg_section_t * cmdline_section;
  bg_cfg_section_t * registry_section;
  
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -i requires an argument\n");
    exit(-1);
    }

  if(!i_section)
    i_section =
      bg_cfg_section_create_from_parameters("i", i_parameters);
  
  if(!bg_cmdline_apply_options(i_section,
                               set_i_parameter,
                               NULL,
                               i_parameters,
                               (*_argv)[arg]))
    exit(-1);

  /* Put the parameters into the registry */

  if(input_plugin_name)
    {
    cmdline_section = bg_cfg_section_find_subsection(i_section, "plugin");
    cmdline_section = bg_cfg_section_find_subsection(cmdline_section, input_plugin_name);

    registry_section = bg_plugin_registry_get_section(plugin_reg, input_plugin_name);
    bg_cfg_section_transfer(cmdline_section, registry_section);
    bg_cfg_registry_save(cfg_reg, "test.xml");
    }
  
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


static void opt_osd(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -osd requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(osd_section,
                               bg_player_set_osd_parameter,
                               player,
                               osd_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


static void opt_aud(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -aud requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(audio_section,
                               bg_player_set_audio_parameter,
                               player,
                               audio_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_vid(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vid requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(video_section,
                               bg_player_set_video_parameter,
                               player,
                               video_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_inopt(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -inopt requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(input_section,
                               bg_player_set_input_parameter,
                               player,
                               input_parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_nt(void * data, int * argc, char *** _argv, int arg)
  {
  display_time = 0;
  }

static void opt_vol(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vol requires an argument\n");
    exit(-1);
    }
  bg_player_set_volume(player, strtod((*_argv[arg]), NULL));
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_tracks(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -tracks requires an argument\n");
    exit(-1);
    }
  track_spec = gavl_strrep(track_spec, (*_argv)[arg]);
  track_spec_ptr = track_spec;
  bg_cmdline_remove_arg(argc, _argv, arg);
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


// static void opt_help(void * data, int * argc, char *** argv, int arg);

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-oa",
      .help_arg =    "<audio_output_options>",
      .help_string = "Set audio output options",
      .callback =    opt_oa,
      .parameters =  oa_parameters,
    },
    {
      .arg =         "-ov",
      .help_arg =    "<video_output_options>",
      .help_string = "Set video output options",
      .callback =    opt_ov,
      .parameters =  ov_parameters,
    },
    {
      .arg =         "-i",
      .help_arg =    "<input_plugin>",
      .help_string = "Set and configure input plugin",
      .callback =    opt_i,
      .parameters =  i_parameters,
    },
    {
      .arg =         "-aud",
      .help_arg =    "<audio_options>",
      .help_string = "Set audio processing options",
      .callback =    opt_aud,
    },
    {
      .arg =         "-vid",
      .help_arg =    "<video_options>",
      .help_string = "Set video processing options",
      .callback =    opt_vid,
    },
    {
      .arg =         "-inopt",
      .help_arg =    "<input_options>",
      .help_string = "Set generic input options",
      .callback =    opt_inopt,
    },
    {
      .arg =         "-osd",
      .help_arg =    "<osd_options>",
      .help_string = "Set OSD options",
      .callback =    opt_osd,
    },
    {
      .arg =         "-nt",
      .help_string = "Disable time display",
      .callback =    opt_nt,
    },
    {
      .arg =         "-vol",
      .help_arg =    "<volume>",
      .help_string = "Set volume in dB (max: 0.0)",
      .callback =    opt_vol,
    },
    {
      .arg =         "-v",
      .help_arg =    "level",
      .help_string = "Set verbosity level (0..4)",
      .callback =    opt_v,
    },
    {
      .arg =         "-tracks",
      .help_arg =    "<track_spec>",
      .help_string = "<track_spec> can be a ranges mixed with comma separated tracks",
      .callback =    opt_tracks,
    },
    { /* End of options */ }
  };

static void update_global_options()
  {
  global_options[3].parameters = audio_parameters;
  global_options[4].parameters = video_parameters;
  global_options[5].parameters = input_parameters;
  global_options[6].parameters = osd_parameters;
  }

static int set_track_from_spec()
  {
  char * rest;
  if(*track_spec_ptr == '\0')
    return 0;
  
  /* Beginning */
  if(track_spec_ptr == track_spec)
    {
    current_track = strtol(track_spec_ptr, &rest, 10)-1;
    track_spec_ptr = rest;
    }
  else
    {
    if(*track_spec_ptr == '\0')
      return 0;
    
    else if(*track_spec_ptr == '-')
      {
      current_track++;
      if(current_track >= num_tracks)
        return 0;
      
      /* Open range */
      if(!isdigit(*(track_spec_ptr+1)))
        return 1;
      
      if(current_track+1 == atoi(track_spec_ptr+1))
        {
        /* End of range reached, advance pointer */
        track_spec_ptr++;
        // FIXME ???
        strtol(track_spec_ptr, &rest, 10);
        track_spec_ptr = rest;
        }
      }
    else if(*track_spec_ptr == ',')
      {
      track_spec_ptr++;
     
      if(*track_spec_ptr == '\0')
        return 0;
     
      current_track = strtol(track_spec_ptr, &rest, 10)-1;
      track_spec_ptr = rest;
      }
    }
  return 1;
  }

/* Input plugin stuff */

static int play_track(bg_player_t * player, const char * gml,
                      const char * plugin_name)
  {
  const bg_plugin_info_t * info = NULL;
  bg_input_plugin_t * plugin;
  bg_track_info_t * track_info;

  int result;
  char * redir_url;
  char * redir_plugin;
  
  if(plugin_name)
    info = bg_plugin_find_by_name(plugin_reg,
                                  plugin_name);
  
  if(input_handle &&
     (input_handle->info->flags & BG_PLUGIN_REMOVABLE))
    {
    plugin = (bg_input_plugin_t*)(input_handle->plugin);
    }
  else
    {
    if(input_handle)
      {
      bg_plugin_unref(input_handle);
      input_handle = NULL;
      }
    if(!bg_input_plugin_load(plugin_reg, gml, info,
                             &input_handle,
                             NULL, 0))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s", gml);
      return 0;
      }
    plugin = (bg_input_plugin_t*)(input_handle->plugin);
    if(plugin->get_num_tracks)
      num_tracks = plugin->get_num_tracks(input_handle->priv);
    else
      num_tracks = 1;
    }

  if(num_tracks == 1)
    {
    if(current_track > 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "No such track %d", current_track+1);
      return 0;
      }
    current_track = 0;
    }
  else
    {
    if(!track_spec)
      {
      current_track++;
      if(current_track >= num_tracks)
        return 0;
      }
    else
      {
#if 0      
      if(track_spec == track_spec_ptr)
        {
        while(set_track_from_spec())
          {
          fprintf(stderr, "Track %d\n", current_track + 1);
          }
        track_spec_ptr = track_spec;
        current_track = -1;
        }
#endif
      
      if(!set_track_from_spec())
        return 0;
      }
    }
  
  track_info = plugin->get_track_info(input_handle->priv, current_track);
  
  if(!track_info)
    return 0;
    
  if(track_info->url)
    {
    redir_url    = gavl_strdup(track_info->url);
    redir_plugin = gavl_strdup(input_handle->info->name);

    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Got redirector %s (%d/%d)",
            redir_url, current_track+1, num_tracks);

    
    bg_plugin_unref(input_handle);
    input_handle = NULL;
    
    result = play_track(player, redir_url, redir_plugin);
    free(redir_url);
    free(redir_plugin);
    return result;
    }
  
  bg_set_track_name_default(track_info, gml);
  bg_player_play(player, input_handle, current_track, 0, gavl_metadata_get(&track_info->metadata,
                                                                           GAVL_META_LABEL));
  
  return 1;
  }


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


static int handle_message(bg_player_t * player,
                          bg_msg_t * message)
  {
  int arg_i1;
  gavl_time_t t;
  gavl_audio_format_t audio_format;
  gavl_video_format_t video_format;
  gavl_metadata_t metadata;

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
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Subtitle streams %d", arg_i1);
        }
      break;
    case BG_PLAYER_MSG_AUDIO_STREAM:
      arg_i1 = bg_msg_get_arg_int(message, 0);

      bg_msg_get_arg_audio_format(message, 1, &audio_format, NULL);
      tmp_string_1 = bg_audio_format_to_string(&audio_format, 0);

      bg_msg_get_arg_audio_format(message, 2, &audio_format, NULL);
      tmp_string_2 = bg_audio_format_to_string(&audio_format, 0);
  
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Playing audio stream %d\nInput format:\n%s\nOutput format:\n%s",
             arg_i1+1, tmp_string_1, tmp_string_2);
      free(tmp_string_1);
      free(tmp_string_2);
      
      break;
    case BG_PLAYER_MSG_VIDEO_STREAM:
      arg_i1 = bg_msg_get_arg_int(message, 0);

      bg_msg_get_arg_video_format(message, 1, &video_format, NULL);
      tmp_string_1 = bg_video_format_to_string(&video_format, 0);

      bg_msg_get_arg_video_format(message, 2, &video_format, NULL);
      tmp_string_2 = bg_video_format_to_string(&video_format, 0);
  
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Playing video stream %d\nInput format:\n%s\nOutput format:\n%s",
             arg_i1+1, tmp_string_1, tmp_string_2);
      free(tmp_string_1);
      free(tmp_string_2);
      break;
    case BG_PLAYER_MSG_AUDIO_STREAM_INFO:
      gavl_metadata_init(&metadata);
      bg_msg_get_arg_metadata(message, 1, &metadata);
      fprintf(stderr, "Audio stream\n");
      gavl_metadata_dump(&metadata, 2);
      gavl_metadata_free(&metadata);
      break;
    case BG_PLAYER_MSG_VIDEO_STREAM_INFO:
      gavl_metadata_init(&metadata);
      bg_msg_get_arg_metadata(message, 1, &metadata);
      fprintf(stderr, "Video stream\n");
      gavl_metadata_dump(&metadata, 2);
      gavl_metadata_free(&metadata);
      break;
    case BG_PLAYER_MSG_SUBTITLE_STREAM_INFO:
      gavl_metadata_init(&metadata);
      bg_msg_get_arg_metadata(message, 1, &metadata);
      fprintf(stderr, "Subtitle stream\n");
      gavl_metadata_dump(&metadata, 2);
      gavl_metadata_free(&metadata);
      break;
    case BG_PLAYER_MSG_CLEANUP:
      bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Player cleaned up");
      break;
    case BG_PLAYER_MSG_METADATA:
      /* Metadata */
      gavl_metadata_init(&metadata);
      bg_msg_get_arg_metadata(message, 0, &metadata);
      fprintf(stderr, "Metadata:\n");
      gavl_metadata_dump(&metadata, 2);
      gavl_metadata_free(&metadata);
      break;
    case BG_PLAYER_MSG_STATE_CHANGED:
      arg_i1 = bg_msg_get_arg_int(message, 0);
      switch(arg_i1)
        {
        case BG_PLAYER_STATE_STOPPED:
          break;
        case BG_PLAYER_STATE_PLAYING:
          bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Player now playing");
          break;
        case BG_PLAYER_STATE_SEEKING:
          if(time_active) { putc('\n', stderr); time_active = 0; }
          bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Player now seeking");
          break;
        case BG_PLAYER_STATE_CHANGING:
        case BG_PLAYER_STATE_EOF:
          bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Player now changing");
          if(time_active) { putc('\n', stderr); time_active = 0; }
          
          if(num_tracks == 1)
            gml_index++;
          
          if(!gmls[gml_index])
            return 0;
          else
            if(!play_track(player, gmls[gml_index], input_plugin_name))
              return 0;
          break;
        case BG_PLAYER_STATE_BUFFERING:
          break;
        case BG_PLAYER_STATE_PAUSED:
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

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin_player",
    .long_name = TRS("Commandline media player"),
    .synopsis = TRS("[options] gml...\n"),
    .help_before = TRS("Commandline Multimedia player\n"),
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
  int i;
  bg_msg_t * message;
  char * tmp_path;
#ifdef INFO_WINDOW
  bg_gtk_info_window_t * info_window;
#endif
  const bg_plugin_info_t * info;
  bg_cfg_section_t * cfg_section;
  bg_msg_queue_t * message_queue;
#ifdef INFO_WINDOW
  bg_gtk_init(&argc, &argv);
#endif

  /* Create plugin regsitry */
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);
  
  player = bg_player_create(plugin_reg);
  
  bg_player_set_volume(player, 0.0);
  
  /* Create config sections */

  audio_parameters = bg_player_get_audio_parameters(player);
  audio_section =
    bg_cfg_section_create_from_parameters("audio", audio_parameters);

  video_parameters = bg_player_get_video_parameters(player);
  video_section =
    bg_cfg_section_create_from_parameters("video", video_parameters);

  osd_parameters = bg_player_get_osd_parameters(player);
  osd_section =
    bg_cfg_section_create_from_parameters("osd", osd_parameters);

  input_parameters = bg_player_get_input_parameters(player);
  input_section =
    bg_cfg_section_create_from_parameters("input",
                                          input_parameters);

  update_global_options();
  
#ifdef INFO_WINDOW
  info_window =
    bg_gtk_info_window_create(player, info_close_callback, NULL);
  bg_gtk_info_window_show(info_window);
#endif
  /* Set the plugin parameter for the commandline */

  bg_plugin_registry_set_parameter_info(plugin_reg,
                                        BG_PLUGIN_OUTPUT_AUDIO,
                                        BG_PLUGIN_PLAYBACK, &oa_parameters[0]);

  bg_plugin_registry_set_parameter_info(plugin_reg,
                                        BG_PLUGIN_OUTPUT_VIDEO,
                                        BG_PLUGIN_PLAYBACK, &ov_parameters[0]);

  bg_plugin_registry_set_parameter_info(plugin_reg,
                                        BG_PLUGIN_INPUT,
                                        BG_PLUGIN_FILE|BG_PLUGIN_URL|BG_PLUGIN_REMOVABLE,
                                        &i_parameters[0]);

  oa_section = bg_cfg_section_create_from_parameters("oa", oa_parameters);
  ov_section = bg_cfg_section_create_from_parameters("oa", ov_parameters);
  
  /* Set message queue */

  message_queue = bg_msg_queue_create();

  bg_player_add_message_queue(player,
                              message_queue);


  /* Apply default options */

  bg_cfg_section_apply(audio_section, audio_parameters,
                       bg_player_set_audio_parameter, player);
  bg_cfg_section_apply(video_section, video_parameters,
                       bg_player_set_video_parameter, player);
  bg_cfg_section_apply(osd_section, osd_parameters,
                       bg_player_set_osd_parameter, player);
  bg_cfg_section_apply(input_section, input_parameters,
                       bg_player_set_input_parameter, player);
    
  /* Get commandline options */
  bg_cmdline_init(&app_data);
  
  bg_cmdline_parse(global_options, &argc, &argv, NULL);
  gmls = bg_cmdline_get_locations_from_args(&argc, &argv);

  if(!gmls)
    {
    fprintf(stderr, "No input files given\n");
    return 0;
    }
  
  /* Start the player thread */

  bg_player_run(player);

  /* Load audio output */

  if(!oa_handle)
    {
    info = bg_plugin_registry_get_default(plugin_reg,
                                          BG_PLUGIN_OUTPUT_AUDIO, BG_PLUGIN_PLAYBACK);

    if(info)
      {
      oa_handle = bg_plugin_load(plugin_reg, info);
      }
    }
  
  bg_player_set_oa_plugin(player, oa_handle);
  
  /* Load video output */

  if(!ov_info)
    {
    ov_info = bg_plugin_registry_get_default(plugin_reg,
                                             BG_PLUGIN_OUTPUT_VIDEO, BG_PLUGIN_PLAYBACK);
    }

  if(ov_info)
    {
    ov_handle = bg_ov_plugin_load(plugin_reg, ov_info, window_id);

    cfg_section = bg_cfg_section_find_subsection(ov_section,
                                                 "plugin");
    cfg_section = bg_cfg_section_find_subsection(cfg_section,
                                                 ov_info->name);
    i = bg_parameter_get_selected(&ov_parameters[0],
                                  ov_info->name);

    bg_cfg_section_apply(cfg_section,
                         ov_parameters[0].multi_parameters[i],
                         ov_handle->plugin->set_parameter,
                         ov_handle->priv);
    }
  
  bg_player_set_ov_plugin(player, ov_handle);
  
  /* Play first track */
  
  play_track(player, gmls[gml_index], input_plugin_name);
  
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

  if(oa_handle)
    bg_plugin_unref(oa_handle);
  if(ov_handle)
    bg_plugin_unref(ov_handle);

  if(input_handle)
    bg_plugin_unref(input_handle);
  
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
    
  bg_cfg_registry_destroy(cfg_reg);
  bg_plugin_registry_destroy(plugin_reg);

  

  return 0;
  }
