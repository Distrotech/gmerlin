/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <avdec.h>

static const gavl_codec_id_t * get_compressions(void * priv)
  {
  
  }

static gavl_video_source_t * connect_decode_video(void * priv,
                                                  gavl_packet_source_t * src,
                                                  const gavl_compression_info_t * ci,
                                                  const gavl_video_format_t * fmt,
                                                  const gavl_metadata_t * m)
  {
  
  }

static const bg_parameter_info_t * get_parameters(void * priv)
  {
  return NULL;
  }

const bg_codec_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "c_avdec_audio",
      .long_name =      TRS("AVDecoder audio decompressor"),
      .description =    TRS("Audio decompressor based on the Gmerlin avdecoder library."),
      .type =           BG_PLUGIN_CODEC,
      .flags =          BG_PLUGIN_AUDIO_DECOMPRESSOR,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .create =         bg_avdec_codec_create,
      .destroy =        bg_avdec_codec_destroy,
      .get_parameters = get_parameters,
      .set_parameter =  bg_avdec_codec_set_parameter,
    },
    .get_compressions     = get_compressions,
    .connect_decode_audio = connect_decode_audio,
    .get_metadata         = get_metadata,
    .reset                = reset,
    .skip                 = skip,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
