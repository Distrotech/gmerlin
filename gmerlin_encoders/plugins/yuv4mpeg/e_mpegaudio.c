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

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/subprocess.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "e_mpegaudio"


#include <gmerlin_encoders.h>
#include "mpa_common.h"

typedef struct
  {
  char * filename;

  bg_mpa_common_t com;
  } e_mpa_t;

static void * create_mpa()
  {
  e_mpa_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_mpa(void * priv)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)priv;

  free(mpa);
  }


static void set_parameter_mpa(void * data, const char * name,
                              const bg_parameter_value_t * v)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;
  if(!name)
    {
    return;
    }
  bg_mpa_set_parameter(&(mpa->com), name, v);
  }

static int open_mpa(void * data, const char * filename,
                    bg_metadata_t * metadata,
                    bg_chapter_list_t * chapter_list)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;

  mpa->filename = bg_strdup(mpa->filename, filename);
  
  return 1;
  }

static int add_audio_stream_mpa(void * data,
                                const char * language,
                                gavl_audio_format_t * format)
  {
  e_mpa_t * mpa;

  mpa = (e_mpa_t*)data;
  bg_mpa_set_format(&mpa->com, format);
  return 0;
  }

static int start_mpa(void * data)
  {
  int result;
  e_mpa_t * e = (e_mpa_t*)data;
  result = bg_mpa_start(&e->com, e->filename);
  bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot find mp2enc executable");
  return result;
  }



static int write_audio_frame_mpa(void * data, gavl_audio_frame_t * frame,
                                  int stream)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;
  return bg_mpa_write_audio_frame(&mpa->com, frame);
  }

static void get_audio_format_mpa(void * data, int stream,
                                 gavl_audio_format_t * ret)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;
  bg_mpa_get_format(&mpa->com, ret);
  
  }

static int close_mpa(void * data, int do_delete)
  {
  int ret = 1;
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;

  if(!bg_mpa_close(&mpa->com))
    ret = 0;

  if(do_delete)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing %s", mpa->filename);
    remove(mpa->filename);
    }
  if(mpa->filename)
    free(mpa->filename);
  return ret;
  }

static const char * get_extension_mpa(void * data)
  {
  e_mpa_t * mpa;
  mpa = (e_mpa_t*)data;
  return bg_mpa_get_extension(&mpa->com);
  }

static const bg_parameter_info_t * get_parameters_mpa(void * data)
  {
  return bg_mpa_get_parameters();
  }


const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "e_mpegaudio",       /* Unique short name */
      .long_name =       TRS("MPEG-1 layer 1/2 audio encoder"),
      .description =     TRS("Encoder for elementary MPEG-1 layer 1/2 audio streams.\
 Based on mjpegtools (http://mjpeg.sourceforge.net)."),
      .type =            BG_PLUGIN_ENCODER_AUDIO,
      .flags =           BG_PLUGIN_FILE,
      .priority =        5,
      .create =            create_mpa,
      .destroy =           destroy_mpa,
      .get_parameters =    get_parameters_mpa,
      .set_parameter =     set_parameter_mpa,
    },
    .max_audio_streams =   1,
    .max_video_streams =   0,

    .get_extension =       get_extension_mpa,

    .open =                open_mpa,
    .add_audio_stream =    add_audio_stream_mpa,
    .get_audio_format =    get_audio_format_mpa,

    .start =               start_mpa,
    .write_audio_frame =   write_audio_frame_mpa,
    .close =               close_mpa
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
