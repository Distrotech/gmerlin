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
  bg_track_info_t track_info;
  bgav_t * dec;
  
  int connect_timeout;
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
  
  avdec = (avdec_priv*)(priv);

  if(!(avdec->dec = bgav_open((const char *) location,
                              avdec->connect_timeout)))
    return 0;

  avdec->track_info.num_audio_streams = bgav_num_audio_streams(avdec->dec);
  avdec->track_info.num_video_streams = bgav_num_video_streams(avdec->dec);
  avdec->track_info.seekable = bgav_can_seek(avdec->dec);

  if(avdec->track_info.num_audio_streams)
    {
    avdec->track_info.audio_streams =
      calloc(avdec->track_info.num_audio_streams,
             sizeof(*avdec->track_info.audio_streams));
    }

  if(avdec->track_info.num_video_streams)
    {
    avdec->track_info.video_streams =
      calloc(avdec->track_info.num_video_streams,
             sizeof(*avdec->track_info.video_streams));
    }
  avdec->track_info.duration = bgav_get_duration(avdec->dec);
  
  /* Get metadata */

  str = bgav_get_author(avdec->dec);
  if(str)
    avdec->track_info.metadata.author = bg_strdup(avdec->track_info.metadata.author, str);

  str = bgav_get_title(avdec->dec);
  if(str)
    avdec->track_info.metadata.title = bg_strdup(avdec->track_info.metadata.title, str);

  str = bgav_get_copyright(avdec->dec);
  if(str)
    avdec->track_info.metadata.copyright = bg_strdup(avdec->track_info.metadata.copyright, str);

  str = bgav_get_comment(avdec->dec);
  if(str)
    avdec->track_info.metadata.comment = bg_strdup(avdec->track_info.metadata.comment, str);

  str = bgav_get_description(avdec->dec);
  if(str)
    avdec->track_info.description = bg_strdup(avdec->track_info.metadata.comment, str);
  
  return 1;
  }

static void close_avdec(void * priv)
  {
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  if(avdec->dec)
    {
    bgav_close(avdec->dec);
    avdec->dec = (bgav_t*)0;
    }
  bg_track_info_free(&(avdec->track_info));
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
  return &(avdec->track_info);
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
  bgav_start_decoders(avdec->dec);
  for(i = 0; i < avdec->track_info.num_video_streams; i++)
    {
    gavl_video_format_copy(&(avdec->track_info.video_streams[i].format),
                           bgav_get_video_format(avdec->dec, i));

    str = bgav_get_video_description(avdec->dec, i);
    if(str)
      avdec->track_info.video_streams[i].description = bg_strdup(NULL, str);
    }
  for(i = 0; i < avdec->track_info.num_audio_streams; i++)
    {
    gavl_audio_format_copy(&(avdec->track_info.audio_streams[i].format),
                           bgav_get_audio_format(avdec->dec, i));
    str = bgav_get_audio_description(avdec->dec, i);
    if(str)
      avdec->track_info.audio_streams[i].description = bg_strdup(NULL, str);
    }
  bgav_dump(avdec->dec);
  }

static void seek_bgav(void * priv, float percentage)
  {
  gavl_time_t pos;
  avdec_priv * avdec;
  avdec = (avdec_priv*)(priv);
  pos = (gavl_time_t)(percentage * (double)avdec->track_info.duration + 0.5);
  //  fprintf(stderr, "seek_bgav: percentage: %f, duration: %f, pos: %f\n",
  //          percentage, gavl_time_to_seconds(avdec->track_info.duration),
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
  if(!strcmp(name, "realcodec_path"))
    {
    bgav_set_dll_path_real(val->val_str);
    }
  if(!strcmp(name, "xanimcodec_path"))
    {
    bgav_set_dll_path_xanim(val->val_str);
    }
  if(!strcmp(name, "connect_timeout"))
    {
    avdec->connect_timeout = val->val_i;
    }
  }

bg_input_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_avdec",
      long_name:     "AVDecoder plugin",
      mimetypes:     "video/x-ms-asf",
      extensions:    "avi asf wmv rm mov wav mp4 3gp qt au aiff aif ra",
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
    get_num_tracks: NULL,
  /* Return track information */
    get_track_info: get_track_info_avdec,
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
