/*****************************************************************
 
  i_vcd.c
 
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

static int open_vcd(void * priv, const char * location)
  {
  avdec_priv * avdec;

  avdec = (avdec_priv*)(priv);

  avdec->dec = bgav_create();
  
  if(!bgav_open_vcd(avdec->dec, avdec->device))
    return 0;
  return bg_avdec_init(avdec);
  }


static bg_parameter_info_t parameters[] =
  {
    {
      name:        "device",
      long_name:   "Device",
      type:        BG_PARAMETER_DEVICE,
      val_default: { val_str: "/dev/cdrom" }
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_vcd(void * priv)
  {
  return parameters;
  }



bg_input_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_vcd",
      long_name:     "VCD Plugin",
      type:          BG_PLUGIN_INPUT,
      flags:         BG_PLUGIN_REMOVABLE,
      create:         bg_avdec_create,
      destroy:        bg_avdec_destroy,
      get_parameters: get_parameters_vcd,
      set_parameter:  bg_avdec_set_parameter
    },
  /* Open file/device */
    open: open_vcd,
    //    set_callbacks: set_callbacks_avdec,
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
