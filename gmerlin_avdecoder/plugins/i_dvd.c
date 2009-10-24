/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <avdec.h>

#include "avdec_common.h"

#include <cdio/cdio.h> // Version

static int open_dvd(void * priv, const char * location)
  {
  avdec_priv * avdec;
  bgav_options_t * opt;
  avdec = (avdec_priv*)(priv);

  avdec->dec = bgav_create();
  opt = bgav_get_options(avdec->dec);
  
  bgav_options_copy(opt, avdec->opt);
  
  if(!bgav_open_dvd(avdec->dec, location))
    return 0;
  return bg_avdec_init(avdec);
  }

static bg_device_info_t * find_devices_dvd()
  {
  bg_device_info_t * ret;
  bgav_device_info_t * dev;
  dev = bgav_find_devices_dvd();
  ret = bg_avdec_get_devices(dev);
  bgav_device_info_destroy(dev);
  return ret;
  }

static int check_device_dvd(const char * device, char ** name)
  {
  return bgav_check_device_dvd(device, name);
  }

static const bg_parameter_info_t parameters[] =
  {
#if 0
    {
      .name =       "dvd_chapters_as_tracks",
      .long_name =  TRS("Handle chapters as tracks"),
      .type =       BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .gettext_domain =    PACKAGE,
      .gettext_directory = LOCALE_DIR,
    },
#endif
    PARAM_DYNRANGE,
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_dvd(void * priv)
  {
  return parameters;
  }

static char const * const protocols = "dvd";

static const char * get_protocols(void * priv)
  {
  return protocols;
  }


const bg_input_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_dvd",
      .long_name =     TRS("DVD Player"),
      .description =   TRS("Plugin for playing DVDs. Based on Gmerlin avdecoder."),
      .type =          BG_PLUGIN_INPUT,
      .flags =         BG_PLUGIN_REMOVABLE,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        bg_avdec_create,
      .destroy =       bg_avdec_destroy,
      .get_parameters = get_parameters_dvd,
      .set_parameter =  bg_avdec_set_parameter,
      .find_devices = find_devices_dvd,
      .check_device = check_device_dvd,
    },
    .get_protocols = get_protocols,
    .set_callbacks = bg_avdec_set_callbacks,
    /* Open file/device */
    .open = open_dvd,
    .get_disc_name = bg_avdec_get_disc_name,
#if LIBCDIO_VERSION_NUM >= 78
    .eject_disc = bgav_eject_disc,
#endif
    //    .set_callbacks = set_callbacks_avdec,
  /* For file and network plugins, this can be NULL */
    .get_num_tracks = bg_avdec_get_num_tracks,
    /* Return track information */
    .get_track_info = bg_avdec_get_track_info,

    /* Set track */
    .set_track =             bg_avdec_set_track,
    /* Set streams */
    .set_audio_stream =      bg_avdec_set_audio_stream,
    .set_video_stream =      bg_avdec_set_video_stream,
    .set_subtitle_stream =   bg_avdec_set_subtitle_stream,

    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    .start =                 bg_avdec_start,
    /* Read one audio frame (returns FALSE on EOF) */
    .read_audio =    bg_avdec_read_audio,

    .has_still  =      bg_avdec_has_still,
    /* Read one video frame (returns FALSE on EOF) */
    .read_video =      bg_avdec_read_video,
    .skip_video =      bg_avdec_skip_video,

    .has_subtitle =          bg_avdec_has_subtitle,

    .read_subtitle_overlay = bg_avdec_read_subtitle_overlay,
    
    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    .seek =         bg_avdec_seek,
    /* Stop playback, close all decoders */
    .stop =         NULL,
    .close =        bg_avdec_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
