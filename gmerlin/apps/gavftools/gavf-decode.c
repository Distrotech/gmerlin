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

#include <string.h>
#include <signal.h>


#include "gavf-decode.h"

#define LOG_DOMAIN "gavf-decode"

static char * input_file   = "-";

char * album_file   = NULL;

static char * input_plugin = NULL;
static bg_plug_t * out_plug = NULL;
static int use_edl = 0;
static int selected_track = 0;

int shuffle = 0;
int loop    = 0;

bg_stream_action_t * audio_actions = NULL;
bg_stream_action_t * video_actions = NULL;
bg_stream_action_t * text_actions = NULL;
bg_stream_action_t * overlay_actions = NULL;

static void opt_t(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -t requires an argument\n");
    exit(-1);
    }
  selected_track = atoi((*_argv)[arg]) - 1;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_shuffle(void * data, int * argc, char *** _argv, int arg)
  {
  shuffle = 1;
  }

static void opt_loop(void * data, int * argc, char *** _argv, int arg)
  {
  loop = 1;
  }

static void opt_edl(void * data, int * argc, char *** _argv, int arg)
  {
  use_edl = 1;
  }

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-i",
      .help_arg =    "<location>",
      .help_string = TRS("Set input file or location"),
      .argv     =    &input_file,
    },
    {
      .arg =         "-a",
      .help_arg =    "<album>",
      .help_string = TRS("Album file"),
      .argv     =    &album_file,
    },
    {
      .arg =         "-loop",
      .help_string = TRS("Loop album"),
      .callback =    opt_loop,
    },
    {
      .arg =         "-shuffle",
      .help_string = TRS("Shuffle"),
      .callback =    opt_shuffle,
    },
    {
      .arg =         "-p",
      .help_arg =    "<plugin>",
      .help_string = TRS("Set input file plugin to use"),
      .argv   =       &input_plugin,
      
    },
    {
      .arg =         "-t",
      .help_arg =    "<track>",
      .help_string = TRS("Track (starting with 1)"),
      .callback =    opt_t,
    },
    {
      .arg =         "-edl",
      .help_string = TRS("Use EDL"),
      .callback =    opt_edl,
    },
    GAVFTOOLS_AUDIO_STREAM_OPTIONS,
    GAVFTOOLS_VIDEO_STREAM_OPTIONS,
    GAVFTOOLS_TEXT_STREAM_OPTIONS,
    GAVFTOOLS_OVERLAY_STREAM_OPTIONS,
    GAVFTOOLS_OUTPLUG_OPTIONS,
    GAVFTOOLS_LOG_OPTIONS,
    {
      /* End */
    }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-decode",
    .long_name = TRS("Decode a media source and output a gavf stream"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf decoder\n"),
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

typedef struct
  {
  bg_plug_t * out_plug;
  } cb_data_t;

static void update_metadata(void * priv, const gavl_metadata_t * m)
  {
  cb_data_t * d = priv;
  gavf_t * g = bg_plug_get_gavf(d->out_plug);
  gavf_update_metadata(g, m);
  }

/* Main */

int main(int argc, char ** argv)
  {
  int ret = EXIT_FAILURE;
  bg_mediaconnector_t * conn;

  bg_mediaconnector_t file_conn;

  
  bg_plugin_handle_t * h = NULL;
  bg_input_callbacks_t cb;
  cb_data_t cb_data;
  gavl_chapter_list_t * cl = NULL; 
  gavl_metadata_t m;
  album_t album;
  int is_album = 0;
  
  gavl_metadata_init(&m); 
  album_init(&album);  

  memset(&cb, 0, sizeof(cb));
  memset(&cb_data, 0, sizeof(cb_data));
  
  gavftools_block_sigpipe();
  bg_mediaconnector_init(&file_conn);  

  gavftools_init();

  gavftools_set_cmdline_parameters(global_options);
  
  /* Handle commandline options */
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  if(!bg_cmdline_check_unsupported(argc, argv))
    return -1;
  
  /* Create out plug */

  out_plug = gavftools_create_out_plug();

  cb_data.out_plug = out_plug;
  cb.data = &cb_data;
  cb.metadata_changed = update_metadata;
  
  
  /* Open location */
  if(album_file)
    {
    if(input_file && strcmp(input_file, "-"))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "-i <location> and -a <album> cannot be used together");
      return ret;
      }
    /* Initialize from album */
    if(!init_decode_album(&album))
      return ret;
    
    conn = &album.out_conn;

    is_album = 1;
    album.out_plug = out_plug;
    }
  else
    {  
    if(!input_file)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No input file or source given");
      return ret;
      }
    if(shuffle)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Shuffle only works with -a <album>");
      return ret;
      }
    if(loop)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Looping only works with -a <album>");
      return ret;
      }


    if(!load_input_file(input_file, input_plugin,
                        selected_track, &file_conn,
                        &cb, &h, &m, &cl, use_edl, 0))
      return ret;

    conn = &file_conn;
    }

  bg_mediaconnector_create_conn(conn);
  
  /* Open output plug */
  if(!bg_plug_open_location(out_plug, gavftools_out_file,
                            &m, cl))
    goto fail;

  gavl_metadata_free(&m);
  
  /* Initialize output plug */
  if(!bg_plug_setup_writer(out_plug, conn))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Setting up plug writer failed");
    goto fail;
    }

  

  bg_mediaconnector_start(conn);

  if(is_album)
    {
    gavf_t * g = bg_plug_get_gavf(out_plug);
    gavf_update_metadata(g, &album.m);
    }
  
  while(1)
    {
    if(gavftools_stop() ||
       bg_plug_got_error(out_plug) ||
       !bg_mediaconnector_iteration(conn))
      break;
    }
  ret = EXIT_SUCCESS;
  fail:

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Cleaning up");
  
  /* Cleanup */
  if(audio_actions)
    free(audio_actions);
  if(video_actions)
    free(video_actions);
  if(text_actions)
    free(text_actions);
  if(overlay_actions)
    free(overlay_actions);
  
  bg_mediaconnector_free(&file_conn);
  bg_plug_destroy(out_plug);
  album_free(&album);
  
  if(h)
    bg_plugin_unref(h);
  
  gavftools_cleanup();
  
  return ret;
  }
