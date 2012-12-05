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

#include <config.h>

#include <gmerlin_encoders.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/translation.h>

#include "bglame.h"

#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "b_lame"

#include <bgshout.h>

typedef struct
  {
  bg_lame_t * com;
  bg_shout_t * shout;
  gavl_audio_format_t fmt;
  gavl_packet_sink_t * psink;
  gavl_audio_sink_t * asink;
  int compressed;
  gavl_compression_info_t ci;
  } b_lame_t;

static void * create_lame()
  {
  b_lame_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->com = bg_lame_create();
  ret->shout = bg_shout_create(SHOUT_FORMAT_MP3);
  return ret;
  }

static void destroy_lame(void * priv)
  {
  b_lame_t * lame;
  lame = priv;
  if(lame->shout)
    bg_shout_destroy(lame->shout);
  if(lame->com)
    bg_lame_destroy(lame->com);
  free(lame);
  }

static const bg_parameter_info_t * get_parameters_b_lame(void * data)
  {
  return bg_shout_get_parameters();
  }

static void set_parameter_b_lame(void * data, const char * name,
                                 const bg_parameter_value_t * val)
  {
  b_lame_t * enc = data;
  bg_shout_set_parameter(enc->shout, name, val);
  }

static const bg_parameter_info_t * get_audio_parameters_lame(void * data)
  {
  return audio_parameters;
  }


static void update_metadata(void * data,
                            const char * name,
                            const gavl_metadata_t * m)
  {
  b_lame_t * enc = data;
  bg_shout_update_metadata(enc->shout, name, m);
  }

static int open_lame(void * data, const char * filename,
                     const gavl_metadata_t * metadata,
                     const gavl_chapter_list_t * chapter_list)
  {
  //  b_lame_t * lame;
  //  lame = data;
  
  return 1;
  }

static int
add_audio_stream_lame(void * data,
                      const gavl_metadata_t * m,
                      const gavl_audio_format_t * format)
  {
  b_lame_t * lame = data;
  gavl_audio_format_copy(&lame->fmt, format);
  return 0;
  }

static int
add_audio_stream_compressed_lame(void * data,
                                 const gavl_metadata_t * m,
                                 const gavl_audio_format_t * format,
                                 const gavl_compression_info_t * ci)
  {
  b_lame_t * lame = data;

  add_audio_stream_lame(data, m, format);
  gavl_compression_info_copy(&lame->ci, ci);
  lame->compressed = 1;
  return 0;
  }

static void set_audio_parameter_lame(void * data, int stream, const char * name,
                                     const bg_parameter_value_t * val)
  {
  b_lame_t * lame = data;
  bg_lame_set_parameter(lame->com, name, val);
  }

static gavl_sink_status_t write_callback(void * data, gavl_packet_t * p)
  {
  b_lame_t * lame = data;
  return (bg_shout_write(lame->shout, p->data, p->data_len) == p->data_len) ?
    GAVL_SINK_OK : GAVL_SINK_ERROR;
  }

static int start_lame(void * data)
  {
  b_lame_t * lame = data;

  if(!bg_shout_open(lame->shout))
    return 0;
  
  /* Create sink */
  lame->psink = gavl_packet_sink_create(NULL, write_callback,
                                        lame);
  if(!lame->compressed)
    {
    lame->asink = bg_lame_open(lame->com, NULL, &lame->fmt, NULL);
    bg_lame_set_packet_sink(lame->com, lame->psink);
    }
  
  return 1;
  }

static int write_audio_packet_lame(void * data, gavl_packet_t * p, int stream)
  {
  b_lame_t * lame = data;
  return gavl_packet_sink_put_packet(lame->psink, p) == GAVL_SINK_OK;
  }


static int close_lame(void * data, int do_delete)
  {
  int ret = 1;
  b_lame_t * lame;
  lame = data;

  /* 1. Flush the buffer */

  bg_lame_destroy(lame->com);
  lame->com = NULL;
  
  bg_shout_destroy(lame->shout);
  lame->shout = NULL;
  
  return ret;
  }

static void get_audio_format_lame(void * data, int stream,
                           gavl_audio_format_t * ret)
  {
  b_lame_t * lame = data;
  gavl_audio_format_copy(ret, &lame->fmt);
  }

static gavl_audio_sink_t * get_audio_sink_lame(void * data, int stream)
  {
  b_lame_t * lame = data;
  return lame->asink;
  }

static int write_audio_frame_lame(void * data, gavl_audio_frame_t * frame, int stream)
  {
  b_lame_t * lame = data;
  return gavl_audio_sink_put_frame(lame->asink, frame) == GAVL_SINK_OK;
  }

static int
writes_compressed_audio_lame(void * data, const gavl_audio_format_t * format,
                             const gavl_compression_info_t * ci)
  {
  if((ci->id == GAVL_CODEC_ID_MP3) && (ci->bitrate != GAVL_BITRATE_VBR))
    return 1;
  else
    return 0;
  }


const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "b_lame",       /* Unique short name */
      .long_name =       TRS("Lame mp3 broadcaster"),
      .description =     TRS("mp3 broadcaster for icecast servers. Based on lame (http://www.mp3dev.org) and libshout (http://www.icecast.org)."),
      .type =            BG_PLUGIN_ENCODER_AUDIO,
      .flags =           BG_PLUGIN_BROADCAST,
      .priority =        5,
      .create =            create_lame,
      .destroy =           destroy_lame,
      .get_parameters =    get_parameters_b_lame,
      .set_parameter =     set_parameter_b_lame,
    },
    .max_audio_streams =   1,
    .max_video_streams =   0,
    
    //    .set_callbacks =       set_callbacks_lame,
    
    .open =                open_lame,
    .writes_compressed_audio = writes_compressed_audio_lame,
    .get_audio_parameters =    get_audio_parameters_lame,

    .add_audio_stream =        add_audio_stream_lame,
    .add_audio_stream_compressed =        add_audio_stream_compressed_lame,
    
    .set_audio_parameter =     set_audio_parameter_lame,

    .get_audio_format =      get_audio_format_lame,
    .get_audio_sink =        get_audio_sink_lame,

    .start = start_lame,
    
    .write_audio_frame =   write_audio_frame_lame,

    .update_metadata   =   update_metadata,
    
    .write_audio_packet =   write_audio_packet_lame,
    .close =               close_lame
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
