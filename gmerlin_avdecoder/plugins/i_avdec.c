/*****************************************************************
 
  i_avdec.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <avdec.h>

typedef struct
  {
  bg_track_info_t * track_info;
  bg_track_info_t * current_track;
  int num_tracks;

  bgav_t * dec;

  int connect_timeout;
  int read_timeout;
  int network_bandwidth;
  } avdec_priv;

static void * create_avdec()
  {
  avdec_priv * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static int open_avdec(void * priv, const void * location)
  {
  avdec_priv * avdec;
  const char * str;
  int i;

  avdec = (avdec_priv*)(priv);

  avdec->dec = bgav_create();

  /* Set parameters */
  
  bgav_set_connect_timeout(avdec->dec, avdec->connect_timeout);
  bgav_set_read_timeout(avdec->dec, avdec->read_timeout);
  bgav_set_network_bandwidth(avdec->dec, avdec->network_bandwidth);
  
  if(!bgav_open(avdec->dec, (const char *) location))
    return 0;
  
  if(bgav_is_redirector(avdec->dec))
    {
    avdec->num_tracks = bgav_redirector_get_num_urls(avdec->dec);
    avdec->track_info = calloc(avdec->num_tracks, sizeof(*(avdec->track_info)));
    
    for(i = 0; i < avdec->num_tracks; i++)
      {
      str = bgav_redirector_get_url(avdec->dec, i);

      avdec->track_info[i].url = bg_strdup(avdec->track_info[i].url, str);
      
      str = bgav_redirector_get_name(avdec->dec, i);

      if(str)
        avdec->track_info[i].name = bg_strdup(avdec->track_info[i].name, str);
      else
        avdec->track_info[i].name = bg_strdup(avdec->track_info[i].name,
                                              avdec->track_info[i].url);

      avdec->track_info[i].plugin = bg_strdup(avdec->track_info[i].plugin, "i_avdec");
      }
    return 1;
    }

  avdec->num_tracks = bgav_num_tracks(avdec->dec);
  avdec->track_info = calloc(avdec->num_tracks, sizeof(*(avdec->track_info)));

  for(i = 0; i < avdec->num_tracks; i++)
    {
    avdec->track_info[i].num_audio_streams = bgav_num_audio_streams(avdec->dec, i);
    avdec->track_info[i].num_video_streams = bgav_num_video_streams(avdec->dec, i);
    avdec->track_info[i].seekable = bgav_can_seek(avdec->dec);
    
    if(avdec->track_info[i].num_audio_streams)
      {
      avdec->track_info[i].audio_streams =
        calloc(avdec->track_info[i].num_audio_streams,
               sizeof(*avdec->track_info[i].audio_streams));
      }
    
    if(avdec->track_info[i].num_video_streams)
      {
      avdec->track_info[i].video_streams =
        calloc(avdec->track_info[i].num_video_streams,
               sizeof(*avdec->track_info[i].video_streams));
      }
    avdec->track_info[i].duration = bgav_get_duration(avdec->dec, i);
    avdec->track_info[i].name =
      bg_strdup(avdec->track_info[i].name,
                bgav_get_track_name(avdec->dec, i));

    /* Get metadata */
    
    str = bgav_get_author(avdec->dec, i);
    if(str)
      avdec->track_info[i].metadata.author = bg_strdup(avdec->track_info[i].metadata.author, str);

    str = bgav_get_artist(avdec->dec, i);
    if(str)
      avdec->track_info[i].metadata.artist = bg_strdup(avdec->track_info[i].metadata.artist, str);

    str = bgav_get_artist(avdec->dec, i);
    if(str)
      avdec->track_info[i].metadata.artist = bg_strdup(avdec->track_info[i].metadata.artist, str);

    str = bgav_get_album(avdec->dec, i);
    if(str)
      avdec->track_info[i].metadata.album = bg_strdup(avdec->track_info[i].metadata.album, str);

    str = bgav_get_genre(avdec->dec, i);
    if(str)
      avdec->track_info[i].metadata.genre = bg_strdup(avdec->track_info[i].metadata.genre, str);
    
    str = bgav_get_title(avdec->dec, i);
    //    fprintf(stderr, "Title: %s\n", str);
    if(str)
      avdec->track_info[i].metadata.title = bg_strdup(avdec->track_info[i].metadata.title, str);
    
    str = bgav_get_copyright(avdec->dec, i);
    if(str)
      avdec->track_info[i].metadata.copyright = bg_strdup(avdec->track_info[i].metadata.copyright, str);
    
    str = bgav_get_comment(avdec->dec, i);
    if(str)
      avdec->track_info[i].metadata.comment = bg_strdup(avdec->track_info[i].metadata.comment, str);

    str = bgav_get_date(avdec->dec, i);
    if(str)
      avdec->track_info[i].metadata.date = bg_strdup(avdec->track_info[i].metadata.date, str);
    avdec->track_info[i].metadata.track = bgav_get_track(avdec->dec, i);
    }
  return 1;
  }

static void close_avdec(void * priv)
  {
  avdec_priv * avdec;
  int i;
  avdec = (avdec_priv*)(priv);
  if(avdec->dec)
    {
    bgav_close(avdec->dec);
      avdec->dec = (bgav_t*)0;
    }
  if(avdec->track_info)
    {
    for(i = 0; i < avdec->num_tracks; i++)
      bg_track_info_free(&(avdec->track_info[i]));
    free(avdec->track_info);
    avdec->track_info = (bg_track_info_t*)0;
    }
  }

static void destroy_avdec(void * priv)
  {
  avdec_priv * avdec;
  close_avdec(priv);

  avdec = (avdec_priv*)(priv);
  free(avdec);
  }

static bg_track_info_t * get_track_info_avdec(void * priv, int track)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  return &(avdec->track_info[track]);
  }

static int read_video_avdec(void * priv,
                            gavl_video_frame_t * frame,
                            int stream)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  return bgav_read_video(avdec->dec, frame, stream);
  }

static int read_audio_avdec(void * priv,
                            gavl_audio_frame_t * frame,
                            int stream,
                            int num_samples)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  return bgav_read_audio(avdec->dec, frame, stream);
  }

static int get_stream_action(bg_stream_action_t action)
  {
  switch(action)
    {
    case BG_STREAM_ACTION_OFF:
      return BGAV_STREAM_MUTE;
      break;
    case BG_STREAM_ACTION_DECODE:
      return BGAV_STREAM_DECODE;
      break;
    case BG_STREAM_ACTION_STANDBY:
      return BGAV_STREAM_SYNC;
      break;
    }
  return -1;
  }

static int set_audio_stream_avdec(void * priv,
                                  int stream,
                                  bg_stream_action_t action)
  {
  int act;
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  act = get_stream_action(action);
  return bgav_set_audio_stream(avdec->dec, stream, act);
  }

static int set_video_stream_avdec(void * priv,
                                  int stream,
                                  bg_stream_action_t action)
  {
  avdec_priv * avdec;
  int act;
  avdec = (avdec_priv*)(priv);
  act = get_stream_action(action);
  return bgav_set_video_stream(avdec->dec, stream, act);
  }

static void start_bgav(void * priv)
  {
  int i;
  const char * str;
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  //  fprintf(stderr, "**** START BGAV *****\n");
  bgav_start(avdec->dec);
  for(i = 0; i < avdec->current_track->num_video_streams; i++)
    {
    gavl_video_format_copy(&(avdec->current_track->video_streams[i].format),
                           bgav_get_video_format(avdec->dec, i));

    str = bgav_get_video_description(avdec->dec, i);
    if(str)
      avdec->current_track->video_streams[i].description = bg_strdup(NULL, str);
    }
  for(i = 0; i < avdec->current_track->num_audio_streams; i++)
    {
    gavl_audio_format_copy(&(avdec->current_track->audio_streams[i].format),
                           bgav_get_audio_format(avdec->dec, i));
    str = bgav_get_audio_description(avdec->dec, i);
    if(str)
      avdec->current_track->audio_streams[i].description = bg_strdup(NULL, str);
    }
  bgav_dump(avdec->dec);
  }

static void seek_bgav(void * priv, float percentage)
  {
  gavl_time_t pos;
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  pos = (gavl_time_t)(percentage * (double)avdec->current_track->duration + 0.5);
  //  fprintf(stderr, "seek_bgav: percentage: %f, duration: %f, pos: %f\n",
  //          percentage, gavl_time_to_seconds(avdec->track_info->duration),
  //          gavl_time_to_seconds(pos));
  bgav_seek(avdec->dec, pos);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "realcodec_path",
      long_name:   "Real codec path",
      type:        BG_PARAMETER_DIRECTORY,
      val_default: { val_str: "/usr/lib/RealPlayer8/Codecs/" }
    },
    {
      name:        "xanimcodec_path",
      long_name:   "Xanim codec path",
      type:        BG_PARAMETER_DIRECTORY,
      val_default: { val_str: "/usr/lib/xanim/" }
    },
    {
      name:        "connect_timeout",
      long_name:   "Connect timeout (milliseconds)",
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 5000 },
      val_min:     { val_i: 0 },
      val_max:     { val_i: 2000000 },
    },
    {
      name:        "read_timeout",
      long_name:   "Read timeout (milliseconds)",
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 5000 },
      val_min:     { val_i: 0 },
      val_max:     { val_i: 2000000 },
    },
    {
      name:        "network_bandwidth",
      long_name:   "Network bandwidth",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str:  "524.3 Kbps (Cable/DSL)" },
      options:     (char*[]){ "14.4 Kbps (Modem)",
                              "19.2 Kbps (Modem)",
                              "28.8 Kbps (Modem)",
                              "33.6 Kbps (Modem)",
                              "34.4 Kbps (Modem)",
                              "57.6 Kbps (Modem)",
                              "115.2 Kbps (ISDN)",
                              "262.2 Kbps (Cable/DSL)",
                              "393.2 Kbps (Cable/DSL)",
                              "524.3 Kbps (Cable/DSL)",
                              "1.5 Mbps (T1)",
                              "10.5 Mbps (LAN)", NULL },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_avdec(void * priv)
  {
  return parameters;
  }

static void
set_parameter_avdec(void * p, char * name,
                    bg_parameter_value_t * val)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(p);
  if(!name)
    return;
  else if(!strcmp(name, "realcodec_path"))
    {
    bgav_set_dll_path_real(val->val_str);
    }
  else if(!strcmp(name, "xanimcodec_path"))
    {
    bgav_set_dll_path_xanim(val->val_str);
    }
  else if(!strcmp(name, "connect_timeout"))
    {
    avdec->connect_timeout = val->val_i;
    }
  else if(!strcmp(name, "read_timeout"))
    {
    avdec->read_timeout = val->val_i;
    }
  else if(!strcmp(name, "network_bandwidth"))
    {
    if(!strcmp(val->val_str, "14.4 Kbps (Modem)"))
      avdec->network_bandwidth = 14400;
    else if(!strcmp(val->val_str, "19.2 Kbps (Modem)"))
      avdec->network_bandwidth = 19200;
    else if(!strcmp(val->val_str, "28.8 Kbps (Modem)"))
      avdec->network_bandwidth = 28800;
    else if(!strcmp(val->val_str, "33.6 Kbps (Modem)"))
      avdec->network_bandwidth = 33600;
    else if(!strcmp(val->val_str, "34.4 Kbps (Modem)"))
      avdec->network_bandwidth = 34430;
    else if(!strcmp(val->val_str, "57.6 Kbps (Modem)"))
      avdec->network_bandwidth = 57600;
    else if(!strcmp(val->val_str, "115.2 Kbps (ISDN)"))
      avdec->network_bandwidth = 115200;
    else if(!strcmp(val->val_str, "262.2 Kbps (Cable/DSL)"))
      avdec->network_bandwidth = 262200;
    else if(!strcmp(val->val_str, "393.2 Kbps (Cable/DSL)"))
      avdec->network_bandwidth = 393216;
    else if(!strcmp(val->val_str, "524.3 Kbps (Cable/DSL)"))
      avdec->network_bandwidth = 524300;
    else if(!strcmp(val->val_str, "1.5 Mbps (T1)"))
      avdec->network_bandwidth = 1544000;
    else if(!strcmp(val->val_str, "10.5 Mbps (LAN)"))
      avdec->network_bandwidth = 10485800;
    }
  
  }

static int get_num_tracks_avdec(void * p)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(p);
  return avdec->num_tracks;
  }

int set_track_avdec(void * priv, int track)
  {
  const char * str;
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  bgav_select_track(avdec->dec, track);
  avdec->current_track = &(avdec->track_info[track]);
  
  str = bgav_get_description(avdec->dec);
  if(str)
    avdec->track_info[track].description = bg_strdup(avdec->track_info[track].description, str);


  return 1;
  }


bg_input_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_avdec",
      long_name:     "AVDecoder plugin",
      mimetypes:     "video/x-ms-asf audio/x-pn-realaudio-plugin video/x-pn-realvideo-plugin audio/x-pn-realaudio video/x-pn-realvideo",
      extensions:    "avi asf asx wmv rm ra ram mov wav mp4 3gp qt au aiff aif mp3",
      type:          BG_PLUGIN_INPUT,
      flags:         BG_PLUGIN_FILE|BG_PLUGIN_URL,
      create:        create_avdec,
      destroy:       destroy_avdec,
      get_parameters: get_parameters_avdec,
      set_parameter:  set_parameter_avdec
    },
  /* Open file/device */
    open: open_avdec,
    //    set_callbacks: set_callbacks_avdec,
  /* For file and network plugins, this can be NULL */
    get_num_tracks: get_num_tracks_avdec,
    /* Return track information */
    get_track_info: get_track_info_avdec,

    /* Set track */
    set_track: set_track_avdec,
    /* Set streams */
    set_audio_stream:      set_audio_stream_avdec,
    set_video_stream:      set_video_stream_avdec,
    set_subpicture_stream: NULL,

    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    start: start_bgav,
    /* Read one audio frame (returns FALSE on EOF) */
    read_audio_samples: read_audio_avdec,
    /* Read one video frame (returns FALSE on EOF) */
    read_video_frame:   read_video_avdec,
    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    seek: seek_bgav,
    /* Stop playback, close all decoders */
    stop: NULL,
    close: close_avdec,
  };
