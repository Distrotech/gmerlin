/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <stdlib.h>
#include <config.h>
#include <gmerlin_encoders.h>

#include <bgshout.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>

#include <theora/theora.h>

#include "ogg_common.h"

#undef HAVE_FLAC /* Switch off flac */

extern const bg_ogg_codec_t bg_theora_codec;
extern const bg_ogg_codec_t bg_vorbis_codec;

#ifdef HAVE_SPEEX
extern const bg_ogg_codec_t bg_speex_codec;
#endif

#ifdef HAVE_FLAC
extern const bg_ogg_codec_t bg_flacogg_codec;
#endif

static bg_ogg_codec_t const * const audio_codecs[] =
  {
    &bg_vorbis_codec,
#ifdef HAVE_SPEEX
    &bg_speex_codec,
#endif
#ifdef HAVE_FLAC
    &bg_flacogg_codec,
#endif
    NULL,
  };

static const bg_parameter_info_t * get_audio_parameters_b_ogg(void * data)
  {
  return bg_ogg_encoder_get_audio_parameters(data, audio_codecs);
  }

static const bg_parameter_info_t * get_video_parameters_b_ogg(void * data)
  {
  return bg_theora_codec.get_parameters();
  }

static int add_audio_stream_b_ogg(void * data,
                                   const char * language,
                                   const gavl_audio_format_t * format)
  {
  int ret;
  ret = bg_ogg_encoder_add_audio_stream(data, format);
  return ret;
  }

static int add_video_stream_b_ogg(void * data,
                                  const gavl_video_format_t * format)
  {
  int ret;
  ret = bg_ogg_encoder_add_video_stream(data, format);
  bg_ogg_encoder_init_video_stream(data, ret, &bg_theora_codec);
  return ret;
  }

static void set_audio_parameter_b_ogg(void * data, int stream,
                                      const char * name,
                                      const bg_parameter_value_t * val)
  {
  int i;
  if(!name)
    return;
  if(!strcmp(name, "codec"))
    {
    i = 0;
    while(audio_codecs[i])
      {
      if(!strcmp(audio_codecs[i]->name, val->val_str))
        {
        bg_ogg_encoder_init_audio_stream(data, stream, audio_codecs[i]);
        break;
        }
      i++;
      }
    }
  else
    bg_ogg_encoder_set_audio_parameter(data, stream, name, val);
  }

static int write_callback(void * priv, const uint8_t * data, int len)
  {
  return bg_shout_write(priv, data, len);
  }

static void close_callback(void * data)
  {
  bg_shout_destroy(data);
  }

static int open_callback(void * data)
  {
  return bg_shout_open(data);
  }

static void update_metadata(void * data,
                            const char * name,
                            const bg_metadata_t * m)
  {
  bg_ogg_encoder_t * enc = data;
  bg_shout_update_metadata(enc->write_callback_data, name, m);
  }

static void * create_b_ogg()
  {
  bg_ogg_encoder_t * ret = bg_ogg_encoder_create();
  ret->write_callback_data = bg_shout_create(SHOUT_FORMAT_OGG);
  ret->open_callback = open_callback;
  ret->write_callback = write_callback;
  ret->close_callback = close_callback;
  return ret;
  }

static const bg_parameter_info_t * get_parameters_b_ogg(void * data)
  {
  bg_ogg_encoder_t * enc = data;
  return bg_shout_get_parameters(enc->write_callback_data);
  }

static void set_parameter_b_ogg(void * data, const char * name,
                                const bg_parameter_value_t * val)
  {
  bg_ogg_encoder_t * enc = data;
  bg_shout_set_parameter(enc->write_callback_data, name, val);
  }

static int
open_b_ogg(void * data, const char * file,
           const bg_metadata_t * metadata,
           const bg_chapter_list_t * chapter_list)
  {
  bg_ogg_encoder_t * enc = data;
  if(!bg_ogg_encoder_open(enc, NULL, metadata, chapter_list,
                          NULL))
    return 0;
  if(metadata)
    bg_shout_update_metadata(enc->write_callback_data, NULL, metadata);
  return 1;
  }

const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "b_ogg",       /* Unique short name */
      .long_name =       TRS("Ogg Broadcaster"),
      .description =     TRS("Broadcaster for Ogg streams using libshout. Supports vorbis, theora and speex."),
      .type =            BG_PLUGIN_ENCODER,
      .flags =           BG_PLUGIN_BROADCAST,
      .priority =        5,
      .create =            create_b_ogg,
      .destroy =           bg_ogg_encoder_destroy,

      .get_parameters =    get_parameters_b_ogg,
      .set_parameter =     set_parameter_b_ogg,
    },
    .max_audio_streams =   -1,
    .max_video_streams =   -1,
    
    .set_callbacks =       bg_ogg_encoder_set_callbacks,
    .open =                open_b_ogg,
    
    .get_audio_parameters =    get_audio_parameters_b_ogg,
    .get_video_parameters =    get_video_parameters_b_ogg,

    .add_audio_stream =        add_audio_stream_b_ogg,
    .add_video_stream =        add_video_stream_b_ogg,
    
    .set_audio_parameter =     set_audio_parameter_b_ogg,
    .set_video_parameter =     bg_ogg_encoder_set_video_parameter,
    
    .start =                  bg_ogg_encoder_start,

    .get_audio_format =        bg_ogg_encoder_get_audio_format,
    .get_video_format =        bg_ogg_encoder_get_video_format,

    .update_metadata  =        update_metadata,
    
    .write_audio_frame =   bg_ogg_encoder_write_audio_frame,
    .write_video_frame =   bg_ogg_encoder_write_video_frame,
    .close =               bg_ogg_encoder_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
