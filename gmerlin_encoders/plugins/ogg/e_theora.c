/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>

#include <theora/theora.h>

#include "ogg_common.h"

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
  };

static int num_audio_codecs = sizeof(audio_codecs) / sizeof(audio_codecs[0]);

static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name =      "codec",
      .long_name = TRS("Codec"),
      .type =      BG_PARAMETER_MULTI_MENU,
      .val_default = { .val_str = "vorbis" },
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_audio_parameters_theora(void * data)
  {
  int i;
  bg_ogg_encoder_t * e = (bg_ogg_encoder_t *)data;

  /* Create audio parameters */
  if(!e->audio_parameters)
    {
    e->audio_parameters = bg_parameter_info_copy_array(audio_parameters);
    e->audio_parameters[0].multi_names_nc =
      calloc(num_audio_codecs+1, sizeof(*e->audio_parameters[0].multi_names));
    e->audio_parameters[0].multi_labels_nc =
      calloc(num_audio_codecs+1, sizeof(*e->audio_parameters[0].multi_labels));
    e->audio_parameters[0].multi_parameters_nc =
      calloc(num_audio_codecs+1, sizeof(*e->audio_parameters[0].multi_parameters));
    for(i = 0; i < num_audio_codecs; i++)
      {
      e->audio_parameters[0].multi_names_nc[i]  = bg_strdup((char*)0, audio_codecs[i]->name);
      e->audio_parameters[0].multi_labels_nc[i] = bg_strdup((char*)0, audio_codecs[i]->long_name);

      if(audio_codecs[i]->get_parameters)
        e->audio_parameters[0].multi_parameters_nc[i] =
          bg_parameter_info_copy_array(audio_codecs[i]->get_parameters());
      }
    bg_parameter_info_set_const_ptrs(&e->audio_parameters[0]);
    }
  return e->audio_parameters;
  }

static const bg_parameter_info_t * get_video_parameters_theora(void * data)
  {
  return bg_theora_codec.get_parameters();
  }

static char * theora_extension = ".ogg";

static const char * get_extension_theora(void * data)
  {
  return theora_extension;
  }

static int add_audio_stream_theora(void * data,
                                   const char * language,
                                   gavl_audio_format_t * format)
  {
  int ret;
  ret = bg_ogg_encoder_add_audio_stream(data, format);
  bg_ogg_encoder_init_audio_stream(data, ret, &bg_theora_codec);
  return ret;
  }

static int add_video_stream_theora(void * data, gavl_video_format_t * format)
  {
  int ret;
  ret = bg_ogg_encoder_add_video_stream(data, format);
  bg_ogg_encoder_init_video_stream(data, ret, &bg_theora_codec);
  return ret;
  }

static void set_audio_parameter_theora(void * data, int stream,
                                       const char * name, const bg_parameter_value_t * val)
  {
  int i;
  if(!name)
    return;
  if(!strcmp(name, "codec"))
    {
    for(i = 0; i < num_audio_codecs; i++)
      {
      if(!strcmp(audio_codecs[i]->name, val->val_str))
        bg_ogg_encoder_init_audio_stream(data, stream, audio_codecs[i]);
      }
    }
  else
    bg_ogg_encoder_set_audio_parameter(data, stream, name, val);
  }


const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "e_theora",       /* Unique short name */
      .long_name =       TRS("Theora encoder"),
      .description =     TRS("Encoder for Theora files. Audio can be Vorbis, Flac or Speex."),
      .mimetypes =       NULL,
      .extensions =      "ogg",
      .type =            BG_PLUGIN_ENCODER,
      .flags =           BG_PLUGIN_FILE,
      .priority =        5,
      .create =            bg_ogg_encoder_create,
      .destroy =           bg_ogg_encoder_destroy,
#if 0
      .get_parameters =    get_parameters_theora,
      .set_parameter =     set_parameter_theora,
#endif
    },
    .max_audio_streams =   -1,
    .max_video_streams =   -1,
    
    .get_extension =       get_extension_theora,
    
    .open =                bg_ogg_encoder_open,
    
    .get_audio_parameters =    get_audio_parameters_theora,
    .get_video_parameters =    get_video_parameters_theora,

    .add_audio_stream =        add_audio_stream_theora,
    .add_video_stream =        add_video_stream_theora,
    
    .set_audio_parameter =     set_audio_parameter_theora,
    .set_video_parameter =     bg_ogg_encoder_set_video_parameter,
    
    .start =                  bg_ogg_encoder_start,

    .get_audio_format =        bg_ogg_encoder_get_audio_format,
    .get_video_format =        bg_ogg_encoder_get_video_format,
    
    .write_audio_frame =   bg_ogg_encoder_write_audio_frame,
    .write_video_frame =   bg_ogg_encoder_write_video_frame,
    .close =               bg_ogg_encoder_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
