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

/* Stat */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "gavftools.h"

#include <gavl/metatags.h>

#include <gmerlin/tree.h>

#define LOG_DOMAIN "gavf-decode"

static char * input_file   = "-";

static char * album_file   = NULL;

static char * input_plugin = NULL;
static bg_plug_t * out_plug = NULL;
static int use_edl = 0;
static int selected_track = 0;

static int shuffle = 0;
static int loop    = 0;

static bg_stream_action_t * audio_actions = NULL;
static bg_stream_action_t * video_actions = NULL;
static bg_stream_action_t * text_actions = NULL;
static bg_stream_action_t * overlay_actions = NULL;

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
  shuffle = 1;
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
    GAVFTOOLS_AUDIO_COMPRESSOR_OPTIONS,
    GAVFTOOLS_VIDEO_COMPRESSOR_OPTIONS,
    GAVFTOOLS_OVERLAY_COMPRESSOR_OPTIONS,
    GAVFTOOLS_OUTPLUG_OPTIONS,
    GAVFTOOLS_VERBOSE_OPTIONS,
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

static int load_input_file(const char * file, const char * plugin_name,
                           int track, bg_mediaconnector_t * conn,
                           bg_input_callbacks_t * cb,
                           bg_plugin_handle_t ** hp, 
                           gavl_metadata_t * m, gavl_chapter_list_t ** cl,
                           int edl, int force_decompress)
  {
  gavl_audio_source_t * asrc;
  gavl_video_source_t * vsrc;
  gavl_packet_source_t * psrc;
  bg_track_info_t * track_info;
  const bg_plugin_info_t * plugin_info;
  bg_input_plugin_t * plugin;
  void * priv;
  int i;
  const gavl_metadata_t * stream_metadata;
  
  if(plugin_name)
    {
    plugin_info = bg_plugin_find_by_name(plugin_reg, plugin_name);
    if(!plugin_info)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Input plugin %s not found", plugin_name);
      goto fail;
      }
    }
  else
    plugin_info = NULL;

  if(!bg_input_plugin_load(plugin_reg,
                           file,
                           plugin_info,
                           hp, cb, edl))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't load %s", file);
    goto fail;
    }
  
  plugin = (bg_input_plugin_t*)((*hp)->plugin);
  priv = (*hp)->priv;
  
  /* Select track and get track info */
  if((track < 0) ||
     (track >= plugin->get_num_tracks(priv)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No such track %d", track);
    goto fail;
    }

  if(plugin->set_track)
    plugin->set_track(priv, track);
  
  track_info = plugin->get_track_info(priv, track);

  /* Extract metadata */
  gavl_metadata_free(m);
  gavl_metadata_init(m);
  gavl_metadata_copy(m, &track_info->metadata);

  if(!gavl_metadata_get(m, GAVL_META_LABEL))
    {
    if(strcmp(file, "-"))
      {
      gavl_metadata_set_nocpy(m, GAVL_META_LABEL,
                              bg_get_track_name_default(file,
                                                        track,
                                                        plugin->get_num_tracks(priv)));
      }
    }
  if(cl)
    *cl = track_info->chapter_list;

  /* Select A/V streams */
  if(!audio_actions)  
    audio_actions = gavftools_get_stream_actions(track_info->num_audio_streams,
                                                 GAVF_STREAM_AUDIO);
  if(!video_actions)
    video_actions = gavftools_get_stream_actions(track_info->num_video_streams,
                                                 GAVF_STREAM_VIDEO);
  if(!text_actions)
    text_actions = gavftools_get_stream_actions(track_info->num_text_streams,
                                                GAVF_STREAM_TEXT);
  if(!overlay_actions)
    overlay_actions = gavftools_get_stream_actions(track_info->num_overlay_streams,
                                                   GAVF_STREAM_OVERLAY);

  /* Check for compressed reading */

  for(i = 0; i < track_info->num_audio_streams; i++)
    {
    if((audio_actions[i] == BG_STREAM_ACTION_READRAW) &&
       (force_decompress || !plugin->get_audio_compression_info ||
        !plugin->get_audio_compression_info(priv, i, NULL)))
      {
      audio_actions[i] = BG_STREAM_ACTION_DECODE;
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Audio stream %d cannot be read compressed",
             i+1);
      }
    }

  for(i = 0; i < track_info->num_video_streams; i++)
    {
    if((video_actions[i] == BG_STREAM_ACTION_READRAW) &&
       (force_decompress || !plugin->get_video_compression_info ||
        !plugin->get_video_compression_info(priv, i, NULL)))
      {
      video_actions[i] = BG_STREAM_ACTION_DECODE;
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Video stream %d cannot be read compressed",
             i+1);
      }
    }

  for(i = 0; i < track_info->num_overlay_streams; i++)
    {
    if((overlay_actions[i] == BG_STREAM_ACTION_READRAW) &&
       (force_decompress || !plugin->get_overlay_compression_info ||
        !plugin->get_overlay_compression_info(priv, i, NULL)))
      {
      overlay_actions[i] = BG_STREAM_ACTION_DECODE;
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Overlay stream %d cannot be read compressed",
             i+1);
      }
    }

  /* Enable streams */
  
  if(plugin->set_audio_stream)
    {
    for(i = 0; i < track_info->num_audio_streams; i++)
      plugin->set_audio_stream(priv, i, audio_actions[i]);
    }
  if(plugin->set_video_stream)
    {
    for(i = 0; i < track_info->num_video_streams; i++)
      plugin->set_video_stream(priv, i, video_actions[i]);
    }
  if(plugin->set_text_stream)
    {
    for(i = 0; i < track_info->num_text_streams; i++)
      plugin->set_text_stream(priv, i, text_actions[i]);
    }
  if(plugin->set_overlay_stream)
    {
    for(i = 0; i < track_info->num_overlay_streams; i++)
      plugin->set_overlay_stream(priv, i, overlay_actions[i]);
    }
  
  /* Start plugin */
  if(plugin->start && !plugin->start(priv))
    goto fail;
  
  for(i = 0; i < track_info->num_audio_streams; i++)
    {
    stream_metadata = &track_info->audio_streams[i].m;
    
    switch(audio_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        asrc = NULL;
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
        asrc = plugin->get_audio_source(priv, i);
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_READRAW:
        psrc = plugin->get_audio_packet_source(priv, i);
        asrc = NULL;
        break;
      }

    if(asrc || psrc)
      bg_mediaconnector_add_audio_stream(conn, stream_metadata, asrc, psrc,
                                         gavftools_ac_section());
    }
  
  for(i = 0; i < track_info->num_video_streams; i++)
    {
    stream_metadata = &track_info->video_streams[i].m;
    switch(video_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        vsrc = NULL;
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
        vsrc = plugin->get_video_source(priv, i);
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_READRAW:
        psrc = plugin->get_video_packet_source(priv, i);
        vsrc = NULL;
        break;
      }
    if(vsrc || psrc)
      bg_mediaconnector_add_video_stream(conn, stream_metadata, vsrc, psrc,
                                         gavftools_vc_section());
    }
  
  for(i = 0; i < track_info->num_text_streams; i++)
    {
    stream_metadata = &track_info->text_streams[i].m;
    switch(text_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
      case BG_STREAM_ACTION_READRAW:
        psrc = plugin->get_text_source(priv, i);
        break;
      }
    if(psrc)
      bg_mediaconnector_add_text_stream(conn, stream_metadata, psrc,
                                        track_info->text_streams[i].timescale);
    }
  
  for(i = 0; i < track_info->num_overlay_streams; i++)
    {
    stream_metadata = &track_info->overlay_streams[i].m;
    switch(text_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        vsrc = NULL;
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
        vsrc = plugin->get_overlay_source(priv, i);
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_READRAW:
        vsrc = NULL;
        psrc = plugin->get_overlay_packet_source(priv, i);
        break;
      }
    if(vsrc || psrc)
      bg_mediaconnector_add_overlay_stream(conn, stream_metadata, vsrc, psrc,
                                           gavftools_oc_section());
    }

  return 1;
  fail:
  
  return 0;
  }

/* Stuff for loading albums */

static int load_album_entry(bg_album_entry_t * entry,
                            bg_mediaconnector_t * conn,
                            bg_plugin_handle_t ** hp, 
                            gavl_metadata_t * m)
  {
  int ret = load_input_file(entry->location, entry->plugin,
                            entry->index, conn,
                            NULL, hp, m, NULL,
                            !!(entry->flags & BG_ALBUM_ENTRY_EDL), 1);
  if(!ret)
    return ret;
  
  gavl_metadata_set_nocpy(m, GAVL_META_LABEL, entry->name);
  return ret;
  }

typedef struct
  {
  int num_entries;
  int current_entry;
  bg_album_entry_t ** entries;
  bg_album_entry_t * first;

  time_t mtime;
 
  bg_plugin_handle_t * h;
  gavl_metadata_t m;

  bg_mediaconnector_t out_conn;

  int num_streams;
  int active_streams;

  int eof; // Album is done
  } album_t;

typedef struct
  {
  bg_mediaconnector_stream_t * in_stream;

  gavl_audio_source_t * asrc;
  gavl_video_source_t * vsrc;
  gavl_packet_source_t * psrc;

  gavl_audio_frame_t * mute_aframe;
  gavl_video_frame_t * mute_vframe;

  gavl_audio_frame_t * next_aframe;
  gavl_video_frame_t * next_vframe;

  gavl_audio_format_t afmt;
  gavl_video_format_t vfmt;

  int64_t pts_offset;
  int64_t pts;

  /* Time to mute the output (Audio or video) so we can keep A/V sync across multiple files */
  int64_t mute_time;

  int timescale;

  album_t * a;
  } stream_t;

static gavl_source_status_t read_audio(void * priv, gavl_audio_frame_t ** ret)
  {
  stream_t * s = priv;

  if(s->mute_time)
    {
    const gavl_audio_format_t * fmt = gavl_audio_source_get_src_format(s->asrc);
    if(!s->mute_aframe)
      s->mute_aframe = gavl_audio_frame_create(fmt);
    gavl_audio_frame_mute(s->mute_aframe, fmt);
    s->mute_aframe->valid_samples = s->mute_time;
    if(s->mute_aframe->valid_samples > fmt->samples_per_frame)
      s->mute_aframe->valid_samples = fmt->samples_per_frame;
    *ret = s->mute_aframe;
    s->mute_time -= s->mute_aframe->valid_samples;
    return GAVL_SOURCE_OK;
    }
  else if(s->next_aframe)
    {
    *ret = s->next_aframe;
    s->next_aframe = NULL;
    return GAVL_SOURCE_OK;
    }
  return GAVL_SOURCE_EOF;
  }

static gavl_source_status_t read_video(void * priv, gavl_video_frame_t ** ret)
  {
  stream_t * s = priv;
  return GAVL_SOURCE_EOF;
  }

static gavl_source_status_t read_overlay(void * priv, gavl_video_frame_t ** ret)
  {
  stream_t * s = priv;
  return GAVL_SOURCE_EOF;
  }

static gavl_source_status_t read_text(void * priv, gavl_packet_t ** ret)
  {
  stream_t * s = priv;
  return GAVL_SOURCE_EOF;
  }

static void stream_destroy(bg_mediaconnector_stream_t * ms)
  {
  stream_t * s = ms->priv;
  if(s->asrc)
    gavl_audio_source_destroy(s->asrc);
  if(s->vsrc)
    gavl_video_source_destroy(s->vsrc);
  if(s->psrc)
    gavl_packet_source_destroy(s->psrc);
  if(s->mute_aframe)
    gavl_audio_frame_destroy(s->mute_aframe);
  if(s->mute_vframe)
    gavl_video_frame_destroy(s->mute_vframe);
  free(s);
  }

static stream_t * stream_create(bg_mediaconnector_stream_t * in_stream,
                                album_t * a)
  {
  bg_mediaconnector_stream_t * out_stream = NULL;
  stream_t * ret = calloc(1, sizeof(*ret));
  ret->in_stream = in_stream;
  ret->a = a;

  switch(ret->in_stream->type)
    {
    case GAVF_STREAM_AUDIO:
      gavl_audio_format_copy(&ret->afmt, gavl_audio_source_get_src_format(ret->in_stream->asrc));
      ret->timescale = ret->afmt.samplerate;
      ret->asrc = 
        gavl_audio_source_create(read_audio, ret, 
                                 GAVL_SOURCE_SRC_ALLOC | GAVL_SOURCE_SRC_FRAMESIZE_MAX,
                                 &ret->afmt);
      out_stream = bg_mediaconnector_add_audio_stream(&a->out_conn,
                                                      NULL, ret->asrc, NULL, NULL);

      break;
    case GAVF_STREAM_VIDEO:
      gavl_video_format_copy(&ret->vfmt, gavl_video_source_get_src_format(ret->in_stream->vsrc));
      ret->vsrc =
        gavl_video_source_create(read_video, ret, GAVL_SOURCE_SRC_ALLOC, &ret->vfmt);
      out_stream = bg_mediaconnector_add_video_stream(&a->out_conn,
                                                      NULL, ret->vsrc, NULL, NULL);
      break;
    case GAVF_STREAM_TEXT:
      ret->psrc =
        gavl_packet_source_create_text(read_text, ret, GAVL_SOURCE_SRC_ALLOC, ret->in_stream->timescale);
      out_stream = bg_mediaconnector_add_text_stream(&a->out_conn,
                                                     NULL, ret->psrc, ret->in_stream->timescale);
      break;
    case GAVF_STREAM_OVERLAY:
      ret->vsrc =
        gavl_video_source_create(read_overlay, ret, GAVL_SOURCE_SRC_ALLOC,
                                 gavl_video_source_get_src_format(ret->in_stream->vsrc));
      out_stream = bg_mediaconnector_add_overlay_stream(&a->out_conn,
                                                         NULL, ret->vsrc, NULL, NULL);
      break;
    }

  if(out_stream)
    {
    out_stream->priv = ret;
    out_stream->free_priv = stream_destroy;
    }
  return ret;
  }

void stream_replug(stream_t * s, bg_mediaconnector_stream_t * in_stream)
  {
  s->in_stream = in_stream;
 
  switch(s->in_stream->type)
    {
    case GAVF_STREAM_AUDIO:
      break;
    case GAVF_STREAM_VIDEO:
      break;
    case GAVF_STREAM_TEXT:
      break;
    case GAVF_STREAM_OVERLAY:
      break;
    }

  }

static void album_init(album_t * a)
  {
  memset(a, 0, sizeof(*a));
  }

static void album_free(album_t * a)
  {
  if(a->entries)
    free(a->entries);
  if(a->first)
    bg_album_entries_destroy(a->first);
  }

static int get_mtime(const char * file, time_t * ret)
  {
  struct stat st;
  if(stat(file, &st))
    return 0;
  *ret = st.st_mtime;
  return 1;
  }

static int album_load(album_t * a)
  {
  int i;
  char * album_xml;
  bg_album_entry_t * e;
  
  if(!get_mtime(album_file, &a->mtime))
    return 0;
  
  album_xml = bg_read_file(album_file, NULL);

  if(!album_xml)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s could not be opened",
           album_file);
    return 0;
    }

  a->first = bg_album_entries_new_from_xml(album_xml);
  free(album_xml);
  
  if(!a->first)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s contains no entries",
           album_file);
    return 0;
    }

  /* Count entries */
  e = a->first;
  while(e)
    {
    a->num_entries++;
    e = e->next;
    }

  /* Set up array */
  a->entries = calloc(a->num_entries, sizeof(a->entries));
  e = a->first;

  for(i = 0; i < a->num_entries; i++)
    {
    a->entries[i] = e;
    e = e->next;
    }

  /* Shuffle */
  if(shuffle)
    {
    int idx;
    for(i = 0; i < a->num_entries; i++)
      {
      idx = rand() % a->num_entries;
      e = a->entries[i];
      a->entries[i] = a->entries[idx];
      a->entries[idx] = e;
      }
    }
  return 1;
  }

static bg_album_entry_t * album_next(album_t * a)
  {
  time_t mtime;
  bg_album_entry_t * ret;

  if(!get_mtime(album_file, &mtime))
    return NULL;

  if(mtime != a->mtime)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Album file %s changed on disk, reloading",
           album_file);
    album_free(a);
    album_init(a);

    if(!album_load(a))
      return NULL;
    }
  
  if(a->current_entry == a->num_entries)
    {
    if(!loop)
      return NULL;
    else a->current_entry = 0;
    }
  
  ret = a->entries[a->current_entry];
  
  a->current_entry++;
  return ret;
  }

int init_decode_album(album_t * a, bg_mediaconnector_t * conn,
                      bg_mediaconnector_t * conn2)
  {
  int ret = 0;
  bg_album_entry_t * e;

  if(!album_load(a))
    return ret;
  
  e = album_next(a);

  if(!load_album_entry(e, conn, &a->h, &a->m))
    return ret;

  /* Set up the conn2 from conn1 */
  
  
  }


/* Main */

int main(int argc, char ** argv)
  {
  int ret = 1;
  bg_mediaconnector_t conn;
  bg_mediaconnector_t conn2;

  bg_plugin_handle_t * h = NULL;
  bg_input_callbacks_t cb;
  cb_data_t cb_data;
  gavl_chapter_list_t * cl = NULL; 
  gavl_metadata_t m;
  album_t album;

  gavl_metadata_init(&m); 
  album_init(&album);  

  memset(&cb, 0, sizeof(cb));
  memset(&cb_data, 0, sizeof(cb_data));
  
  gavftools_block_sigpipe();
  bg_mediaconnector_init(&conn);  
  bg_mediaconnector_init(&conn2);

  gavftools_init();

  gavftools_set_compresspor_options(global_options);
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
  
  bg_plug_set_compressor_config(out_plug,
                                gavftools_ac_params(),
                                gavftools_vc_params(),
                                gavftools_oc_params());
  
  /* Open location */
  if(album_file)
    {
    if(input_file)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "-i <location> and -a <album> cannot be used together");
      return ret;
      }
    /* Initialize from album */
    if(!init_decode_album(&album, &conn, &conn2))
      return ret;
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
                        selected_track, &conn,
                        &cb, &h, &m, &cl, use_edl, 0))
      return ret;
  
    bg_mediaconnector_create_conn(&conn);
  
    /* Open output plug */
    if(!bg_plug_open_location(out_plug, gavftools_out_file,
                              &m, cl))
      goto fail;

    gavl_metadata_free(&m);
  
    /* Initialize output plug */
    if(!bg_plug_setup_writer(out_plug, &conn))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Setting up plug writer failed");
      goto fail;
      }
    }
  
  ret = 1;

  bg_mediaconnector_start(&conn);

  while(1)
    {
    if(gavftools_stop() ||
       bg_plug_got_error(out_plug) ||
       !bg_mediaconnector_iteration(&conn))
      break;
    }
  ret = 0;
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
  
  bg_mediaconnector_free(&conn);
  bg_plug_destroy(out_plug);

  bg_plugin_unref(h);
  
  gavftools_cleanup();
  
  return ret;
  }
