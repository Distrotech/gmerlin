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

#include "avdec_common.h"

static int open_avdec(void * priv, const char * location)
  {
  int i, result;
  const char * str;
  avdec_priv * avdec;

  avdec = (avdec_priv*)(priv);

  avdec->dec = bgav_create();

  /* Set parameters */
  
  bgav_set_connect_timeout(avdec->dec, avdec->connect_timeout);
  bgav_set_read_timeout(avdec->dec, avdec->read_timeout);
  bgav_set_network_bandwidth(avdec->dec, avdec->network_bandwidth);
  bgav_set_network_buffer_size(avdec->dec, avdec->network_buffer_size * 1024);
  
  bgav_set_http_shoutcast_metadata(avdec->dec, avdec->http_shoutcast_metadata);
    
  if(!bgav_open(avdec->dec, location))
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
  result = bg_avdec_init(avdec);

  /* Set default track name */
  
  if(result && (avdec->num_tracks == 1) && !avdec->track_info->name)
    {
    bg_set_track_name_default(avdec->track_info, location);
    }
  
  return result;
  }


static bg_parameter_info_t parameters[] =
  {
    {
      name:       "codec_paths",
      long_name:  "Codec Paths",
      type:       BG_PARAMETER_SECTION,
    },
    {
      name:        "codec_path_real",
      long_name:   "Real DLLs",
      type:        BG_PARAMETER_DIRECTORY,
      val_default: { val_str: (char*)0 }
    },
    {
      name:        "codec_path_win32",
      long_name:   "Win32 DLLs",
      type:        BG_PARAMETER_DIRECTORY,
      val_default: { val_str: (char*)0 }
    },
    {
      name:        "codec_path_xanim",
      long_name:   "Xanim DLLs",
      type:        BG_PARAMETER_DIRECTORY,
      val_default: { val_str: (char*)0 }
    },
    {
      name:       "network_options",
      long_name:  "Network options",
      type:       BG_PARAMETER_SECTION
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
      name:        "network_buffer_size",
      long_name:   "Network buffer size (kB)",
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 32 },
      val_min:     { val_i: 1 },
      val_max:     { val_i: 1000 },
    },
    {
      name:        "network_bandwidth",
      long_name:   "Bandwidth",
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
    {
      name:       "http_options",
      long_name:  "HTTP Options",
      type:       BG_PARAMETER_SECTION
    },
    {
      name:        "http_shoutcast_metadata",
      long_name:   "Enable shoutcast title streaming",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_avdec(void * priv)
  {
  return parameters;
  }



bg_input_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_avdec",
      long_name:     "AVDecoder plugin",
      mimetypes:     "video/x-ms-asf audio/x-pn-realaudio-plugin video/x-pn-realvideo-plugin audio/x-pn-realaudio video/x-pn-realvideo audio/x-mpegurl audio/mpegurl audio/x-scpls audio/scpls audio/m3u",
      extensions:    "avi asf asx wmv rm ra ram mov wav mp4 m4a 3gp qt au aiff aif mp3 mpg mpeg vob m3u pls ogg flac aac",
      type:          BG_PLUGIN_INPUT,
      flags:         BG_PLUGIN_FILE|BG_PLUGIN_URL,
      create:         bg_avdec_create,
      destroy:        bg_avdec_destroy,
      get_parameters: get_parameters_avdec,
      set_parameter:  bg_avdec_set_parameter
    },
  /* Open file/device */
    open: open_avdec,
    set_callbacks: bg_avdec_set_callbacks,
  /* For file and network plugins, this can be NULL */
    get_num_tracks: bg_avdec_get_num_tracks,
    /* Return track information */
    get_track_info: bg_avdec_get_track_info,

    /* Set track */
    set_track:             bg_avdec_set_track,
    /* Set streams */
    set_audio_stream:      bg_avdec_set_audio_stream,
    set_video_stream:      bg_avdec_set_video_stream,
    set_subpicture_stream: NULL,

    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    start:                 bg_avdec_start,
    /* Read one audio frame (returns FALSE on EOF) */
    read_audio_samples:    bg_avdec_read_audio,
    /* Read one video frame (returns FALSE on EOF) */
    read_video_frame:      bg_avdec_read_video,
    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    seek: bg_avdec_seek,
    /* Stop playback, close all decoders */
    stop: NULL,
    close: bg_avdec_close,
  };
