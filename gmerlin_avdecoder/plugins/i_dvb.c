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

#include "avdec_common.h"

static int open_dvb(void * priv, const char * location)
  {
  bgav_options_t * opt;
  avdec_priv * avdec = priv;

  avdec->dec = bgav_create();
  opt = bgav_get_options(avdec->dec);
  
  bgav_options_copy(opt, avdec->opt);
  
  if(!bgav_open_dvb(avdec->dec, location))
    return 0;
  return bg_avdec_init(avdec);
  }

static bg_device_info_t * find_devices_dvb()
  {
  bg_device_info_t * ret;
  bgav_device_info_t * dev;
  dev = bgav_find_devices_dvb();
  ret = bg_avdec_get_devices(dev);
  bgav_device_info_destroy(dev);
  return ret;
  }

static int check_device_dvb(const char * device, char ** name)
  {
  return bgav_check_device_dvb(device, name);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "dvb_channels_file",
      .long_name =   TRS("Channel file"),
      .type =        BG_PARAMETER_FILE,
      .help_string = TRS("The channels file must have the format of the dvb-utils\
 programs (like szap, tzap). If you don't set this file,\
 several locations like $HOME/.tzap/channels.conf will be\
 searched."),
      .gettext_domain =    PACKAGE,
      .gettext_directory = LOCALE_DIR,
    },
    PARAM_DYNRANGE,
    {
      .name =        "read_timeout",
      .long_name =   TRS("Read timeout (milliseconds)"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 100 },
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 2000000 },
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_dvb(void * priv)
  {
  return parameters;
  }


static char const * const protocols = "dvb";

static const char * get_protocols(void * priv)
  {
  return protocols;
  }

const bg_input_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_dvb",
      .long_name =     TRS("DVB Player"),
      .description =   TRS("Plugin for playing DVB streams from a Linux-DVB compatible card. Based on Gmerlin avdecoder."),
      .type =          BG_PLUGIN_INPUT,
      .flags =         BG_PLUGIN_TUNER,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        bg_avdec_create,
      .destroy =       bg_avdec_destroy,
      .get_parameters = get_parameters_dvb,
      .set_parameter =  bg_avdec_set_parameter,
      .find_devices = find_devices_dvb,
      .check_device = check_device_dvb,
    },
    .get_protocols = get_protocols,
    
    .set_callbacks = bg_avdec_set_callbacks,
    /* Open file/device */
    .open = open_dvb,

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
    //    .set_subtitle_stream =   bg_avdec_set_subtitle_stream,

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
    .get_video_source = bg_avdec_get_video_source,
    .get_audio_source = bg_avdec_get_audio_source,

    .get_video_packet_source = bg_avdec_get_video_packet_source,
    .get_audio_packet_source = bg_avdec_get_audio_packet_source,
    .get_text_source = bg_avdec_get_text_packet_source,
    .get_overlay_source = bg_avdec_get_overlay_source,
    .get_overlay_packet_source = bg_avdec_get_overlay_packet_source,
    
    .read_audio_packet = bg_avdec_read_audio_packet,
    .read_video_packet = bg_avdec_read_video_packet,
    
    /* Stop playback, close all decoders */
    .stop =         NULL,
    .close =        bg_avdec_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
