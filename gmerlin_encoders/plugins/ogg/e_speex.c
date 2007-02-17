/*****************************************************************
 
  e_speex.c
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <stdio.h>
#include <stdlib.h>
#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin_encoders.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <ogg/ogg.h>
#include "ogg_common.h"

extern bg_ogg_codec_t bg_speex_codec;

static bg_parameter_info_t * get_audio_parameters_speex(void * data)
  {
  return bg_speex_codec.get_parameters();
  }

static char * speex_extension = ".spx";

static const char * get_extension_speex(void * data)
  {
  return speex_extension;
  }

static int add_audio_stream_speex(void * data, const char * language,
                                  gavl_audio_format_t * format)
  {
  int ret;
  ret = bg_ogg_encoder_add_audio_stream(data, format);
  bg_ogg_encoder_init_audio_stream(data, ret, &bg_speex_codec);
  return ret;
  }



bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      BG_LOCALE,
      name:            "e_speex",       /* Unique short name */
      long_name:       TRS("Speex encoder"),
      description:     TRS("Encoder for Speex files"),
      mimetypes:       NULL,
      extensions:      "ogg",
      type:            BG_PLUGIN_ENCODER_AUDIO,
      flags:           BG_PLUGIN_FILE,
      priority:        5,
      create:            bg_ogg_encoder_create,
      destroy:           bg_ogg_encoder_destroy,
#if 0
      get_parameters:    get_parameters_speex,
      set_parameter:     set_parameter_speex,
#endif
    },
    max_audio_streams:   1,
    max_video_streams:   0,
    
    get_extension:       get_extension_speex,
    
    open:                bg_ogg_encoder_open,
    
    get_audio_parameters:    get_audio_parameters_speex,

    add_audio_stream:        add_audio_stream_speex,
    
    set_audio_parameter:     bg_ogg_encoder_set_audio_parameter,

    start:                  bg_ogg_encoder_start,

    get_audio_format:        bg_ogg_encoder_get_audio_format,
    
    write_audio_frame:   bg_ogg_encoder_write_audio_frame,
    close:               bg_ogg_encoder_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
