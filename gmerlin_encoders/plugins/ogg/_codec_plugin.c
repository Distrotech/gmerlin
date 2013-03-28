/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

typedef struct
  {
  void * priv;
  const bg_ogg_codec_t * codec;
  } stream_codec_t;

static void * create_codec()
  {
  stream_codec_t * ret = calloc(1, sizeof(*ret));

  ret->codec = &CODEC;
  ret->priv = ret->codec->create();
  
  return ret;
  }

static void destroy_codec(void * priv)
  {
  stream_codec_t * c = priv;
  c->codec->close(c->priv);
  free(c);
  }

static const bg_parameter_info_t * get_parameters(void * priv)
  {
  stream_codec_t * c = priv;
  return c->codec->get_parameters();
  }

static void set_parameter(void * priv, const char * name, const bg_parameter_value_t * val)
  {
  stream_codec_t * c = priv;
  c->codec->set_parameter(c->priv, name, val);
  }

#ifdef IS_AUDIO
static gavl_audio_sink_t * open_audio(void * priv,
                                      gavl_compression_info_t * ci,
                                      gavl_audio_format_t * fmt,
                                      gavl_metadata_t * m)
  {
  stream_codec_t * c = priv;
  return c->codec->init_audio(c->priv, ci, fmt, m);
  }
#else
static gavl_video_sink_t * open_video(void * priv,
                                      gavl_compression_info_t * ci,
                                      gavl_video_format_t * fmt,
                                      gavl_metadata_t * m)
  {
  stream_codec_t * c = priv;
  return c->codec->init_video(c->priv, ci, fmt, m);
  }
#endif

static void set_packet_sink(void * priv, gavl_packet_sink_t * s)
  {
  stream_codec_t * c = priv;
  return c->codec->set_packet_sink(c->priv, s);
  }

gavl_codec_id_t compressions[] =
  {
    COMPRESSION,
    GAVL_CODEC_ID_NONE
  };

static const gavl_codec_id_t * get_compressions(void * priv)
  {
  return compressions;
  }


const bg_codec_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            CODEC_NAME,       /* Unique short name */
      .long_name =       CODEC_LONG_NAME,
      .description =     CODEC_DESC,
      .type =            BG_PLUGIN_CODEC,
#ifdef IS_AUDIO
      .flags =           BG_PLUGIN_AUDIO_COMPRESSOR,
#else
      .flags =           BG_PLUGIN_VIDEO_COMPRESSOR,
#endif
      .priority =        5,
      .create =          create_codec,
      .destroy =         destroy_codec,
      .get_parameters =    get_parameters,
      .set_parameter =     set_parameter,
    },
#ifdef IS_AUDIO
    .open_encode_audio = open_audio,
#else 
    .open_encode_video = open_video,
#endif
    .set_packet_sink = set_packet_sink,
    .get_compressions = get_compressions,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
