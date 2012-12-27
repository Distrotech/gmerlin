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

#include <cdio/cdio.h> // Version

#include "avdec_common.h"

static int open_vcd(void * priv, const char * location)
  {
  bgav_options_t * opt;
  avdec_priv * avdec = priv;

  avdec->dec = bgav_create();
  opt = bgav_get_options(avdec->dec);
  
  bgav_options_copy(opt, avdec->opt);
  
  if(!bgav_open_vcd(avdec->dec, location))
    return 0;
  return bg_avdec_init(avdec);
  }

static bg_device_info_t * find_devices_vcd()
  {
  bg_device_info_t * ret;
  bgav_device_info_t * dev;
  dev = bgav_find_devices_vcd();
  ret = bg_avdec_get_devices(dev);
  bgav_device_info_destroy(dev);
  return ret;
  }

static int check_device_vcd(const char * device, char ** name)
  {
  return bgav_check_device_vcd(device, name);
  }

static char const * const protocols = "vcd";

static const char * get_protocols(void * priv)
  {
  return protocols;
  }

const bg_input_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_vcd",
      .long_name =     TRS("VCD Player"),
      .description =   TRS("Plugin for playing VCDs. Based on Gmerlin avdecoder."),
      .type =          BG_PLUGIN_INPUT,
      .flags =         BG_PLUGIN_REMOVABLE,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        bg_avdec_create,
      .destroy =       bg_avdec_destroy,
      //      .get_parameters = get_parameters_vcd,
      //      .set_parameter =  bg_avdec_set_parameter
      .find_devices = find_devices_vcd,
      .check_device = check_device_vcd,
    },
    .get_protocols = get_protocols,
    
    /* Open file/device */
    .open = open_vcd,
    .set_callbacks = bg_avdec_set_callbacks,
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

    /* Get compression infos */
    .get_audio_compression_info = bg_avdec_get_audio_compression_info,
    .get_video_compression_info = bg_avdec_get_video_compression_info,
    
    
    /* Set streams */
    .set_audio_stream =      bg_avdec_set_audio_stream,
    .set_video_stream =      bg_avdec_set_video_stream,
    
    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    .start =         bg_avdec_start,
    /* Read one audio frame (returns FALSE on EOF) */
    .read_audio =    bg_avdec_read_audio,

    .has_still  =      bg_avdec_has_still,
    /* Read one video frame (returns FALSE on EOF) */
    .read_video =      bg_avdec_read_video,
    .skip_video =      bg_avdec_skip_video,
    .get_video_source = bg_avdec_get_video_source,
    .get_audio_source = bg_avdec_get_audio_source,

    .read_audio_packet = bg_avdec_read_audio_packet,
    .read_video_packet = bg_avdec_read_video_packet,
    
    /*
     *  Do percentage seeking (can be NULL)
     */
    .seek =         bg_avdec_seek,
    /* Stop playback, close all decoders */
    .stop =         NULL,
    .close =        bg_avdec_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
