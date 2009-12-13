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

#include <yuv4mpeg.h>

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/subprocess.h>
#include <gmerlin/log.h>
#include <gmerlin_encoders.h>
#include "mpv_common.h"

#define LOG_DOMAIN "e_mpegvideo"

typedef struct
  {
  bg_mpv_common_t mpv;
  char * filename;

  bg_encoder_callbacks_t * cb;
  
  } e_mpv_t;

static void * create_mpv()
  {
  e_mpv_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void set_callbacks_mpv(void * data, bg_encoder_callbacks_t * cb)
  {
  e_mpv_t * mpv = data;
  mpv->cb = cb;
  }

static int open_mpv(void * data, const char * filename,
                    const bg_metadata_t * metadata,
                    const bg_chapter_list_t * chapter_list)
  {
  e_mpv_t * e = data;
  e->filename =
    bg_filename_ensure_extension(filename, bg_mpv_get_extension(&(e->mpv)));

  if(!bg_encoder_cb_create_output_file(e->cb, e->filename))
    return 0;
  
  return bg_mpv_open(&e->mpv, e->filename);
  }

static int add_video_stream_mpv(void * data, const gavl_video_format_t* format)
  {
  e_mpv_t * e = data;
  bg_mpv_set_format(&e->mpv, format);
  return 0;
  }

static void get_video_format_mpv(void * data, int stream,
                                 gavl_video_format_t * ret)
  {
  e_mpv_t * e = data;
  gavl_video_format_copy(ret, &(e->mpv.y4m.format));
  }

static int start_mpv(void * data)
  {
  e_mpv_t * e = data;
  return bg_mpv_start(&e->mpv);
  }

static int write_video_frame_mpv(void * data,
                                  gavl_video_frame_t* frame,
                                  int stream)
  {
  e_mpv_t * e = data;
  return bg_mpv_write_video_frame(&(e->mpv), frame);
  }

static int close_mpv(void * data, int do_delete)
  {
  int ret;
  e_mpv_t * e = data;
  ret = bg_mpv_close(&e->mpv);
  if(do_delete)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing %s", e->filename);
    remove(e->filename);
    }
  return ret;
  }

static void destroy_mpv(void * data)
  {
  e_mpv_t * e = data;
  free(e);
  }

/* Per stream parameters */

static const bg_parameter_info_t * get_parameters_mpv(void * data)
  {
  return bg_mpv_get_parameters();
  }

static void set_parameter_mpv(void * data, const char * name,
                              const bg_parameter_value_t * val)
  {
  e_mpv_t * e = data;
  bg_mpv_set_parameter(&e->mpv, name, val);
  }

#undef SET_ENUM

const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "e_mpegvideo",       /* Unique short name */
      .long_name =      TRS("MPEG-1/2 video encoder"),
      .description =     TRS("Encoder for elementary MPEG-1/2 video streams.\
 Based on mjpegtools (http://mjpeg.sourceforge.net)."),
      .type =           BG_PLUGIN_ENCODER_VIDEO,
      .flags =          BG_PLUGIN_FILE,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .create =         create_mpv,
      .destroy =        destroy_mpv,
      .get_parameters = get_parameters_mpv,
      .set_parameter =  set_parameter_mpv,
    },

    .max_audio_streams =  0,
    .max_video_streams =  1,

    .set_callbacks =        set_callbacks_mpv,

    .open =                 open_mpv,

    .add_video_stream =     add_video_stream_mpv,

    //    .set_video_parameter =  set_video_parameter_mpv,

    .get_video_format =     get_video_format_mpv,

    .start =                start_mpv,

    .write_video_frame = write_video_frame_mpv,
    .close =             close_mpv,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
