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
  lame_common_t com; // Must be first!!
  bg_shout_t * shout;
  } b_lame_t;

static void * create_lame()
  {
  b_lame_t * ret;
  ret = calloc(1, sizeof(*ret));
  bg_lame_init(&ret->com);
  ret->shout = bg_shout_create(SHOUT_FORMAT_MP3);
  return ret;
  }

static void destroy_lame(void * priv)
  {
  b_lame_t * lame;
  lame = priv;
  bg_lame_close(&lame->com);
  if(lame->shout)
    bg_shout_destroy(lame->shout);
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

static int write_callback(void * priv, uint8_t * data, int len)
  {
  return bg_shout_write(priv, data, len);
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
                     const bg_chapter_list_t * chapter_list)
  {
  b_lame_t * lame;

  lame = data;

  bg_lame_open(&lame->com);

  if(!bg_shout_open(lame->shout))
    return 0;

  lame->com.write_callback = write_callback;
  lame->com.write_priv = lame->shout;
  
  return 1;
  }

static int close_lame(void * data, int do_delete)
  {
  int ret = 1;
  b_lame_t * lame;
  lame = data;

  /* 1. Flush the buffer */
  
  if(lame->com.samples_read)
    {
    if(!bg_lame_flush(&lame->com))
      ret = 0;
    }

  bg_shout_destroy(lame->shout);
  lame->shout = NULL;
  
  return ret;
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
    // .writes_compressed_audio = writes_compressed_audio_lame,
    .get_audio_parameters =    get_audio_parameters_lame,

    .add_audio_stream =        bg_lame_add_audio_stream,
    // .add_audio_stream_compressed =        add_audio_stream_compressed_lame,
    
    .set_audio_parameter =     bg_lame_set_audio_parameter,

    .get_audio_format =        bg_lame_get_audio_format,
    
    .write_audio_frame =   bg_lame_write_audio_frame,

    .update_metadata   =   update_metadata,
    
    // .write_audio_packet =   write_audio_packet_lame,
    .close =               close_lame
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
