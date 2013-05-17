/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <gmerlin/upnp/device.h>
#include <gmerlin/utils.h>

#include <fnmatch.h>
#include <string.h>

/* dlna stuff (taken from libdlna) */

/* DLNA.ORG_PS: play speed parameter (integer)
 *     0 invalid play speed
 *     1 normal play speed
 */
typedef enum {
  DLNA_ORG_PLAY_SPEED_INVALID = 0,
  DLNA_ORG_PLAY_SPEED_NORMAL = 1,
} dlna_org_play_speed_t;

/* DLNA.ORG_CI: conversion indicator parameter (integer)
 *     0 not transcoded
 *     1 transcoded
 */
typedef enum {
  DLNA_ORG_CONVERSION_NONE = 0,
  DLNA_ORG_CONVERSION_TRANSCODED = 1,
} dlna_org_conversion_t;

/* DLNA.ORG_OP: operations parameter (string)
 *     "00" (or "0") neither time seek range nor range supported
 *     "01" range supported
 *     "10" time seek range supported
 *     "11" both time seek range and range supported
 */
typedef enum {
  DLNA_ORG_OPERATION_NONE                  = 0x00,
  DLNA_ORG_OPERATION_RANGE                 = 0x01,
  DLNA_ORG_OPERATION_TIMESEEK              = 0x10,
} dlna_org_operation_t;

/* DLNA.ORG_FLAGS, padded with 24 trailing 0s
 *     80000000  31  senderPaced
 *     40000000  30  lsopTimeBasedSeekSupported
 *     20000000  29  lsopByteBasedSeekSupported
 *     10000000  28  playcontainerSupported
 *      8000000  27  s0IncreasingSupported
 *      4000000  26  sNIncreasingSupported
 *      2000000  25  rtspPauseSupported
 *      1000000  24  streamingTransferModeSupported
 *       800000  23  interactiveTransferModeSupported
 *       400000  22  backgroundTransferModeSupported
 *       200000  21  connectionStallingSupported
 *       100000  20  dlnaVersion15Supported
 *
 *     Example: (1 << 24) | (1 << 22) | (1 << 21) | (1 << 20)
 *       DLNA.ORG_FLAGS=01700000[000000000000000000000000] // [] show padding
 */
typedef enum {
  DLNA_ORG_FLAG_SENDER_PACED               = (1 << 31),
  DLNA_ORG_FLAG_TIME_BASED_SEEK            = (1 << 30),
  DLNA_ORG_FLAG_BYTE_BASED_SEEK            = (1 << 29),
  DLNA_ORG_FLAG_PLAY_CONTAINER             = (1 << 28),
  DLNA_ORG_FLAG_S0_INCREASE                = (1 << 27),
  DLNA_ORG_FLAG_SN_INCREASE                = (1 << 26),
  DLNA_ORG_FLAG_RTSP_PAUSE                 = (1 << 25),
  DLNA_ORG_FLAG_STREAMING_TRANSFER_MODE    = (1 << 24),
  DLNA_ORG_FLAG_INTERACTIVE_TRANSFER_MODE  = (1 << 23),
  DLNA_ORG_FLAG_BACKGROUND_TRANSFER_MODE   = (1 << 22),
  DLNA_ORG_FLAG_CONNECTION_STALL           = (1 << 21),
  DLNA_ORG_FLAG_DLNA_V15                   = (1 << 20),
} dlna_org_flags_t;

static char * create_dlna_info(dlna_org_conversion_t ci,
                               dlna_org_operation_t op,
                               dlna_org_flags_t flags, const char * type)
  {
  return
    bg_sprintf("DLNA.ORG_CI=%d;DLNA.ORG_OP=%.2x;DLNA.ORG_PN=%s;DLNA.ORG_FLAGS=%.8x%.24x",
               ci, op, type, flags, 0);
  }


/* LPCM */

static int is_supported_lpcm(bg_plugin_registry_t * plugin_reg)
  {
  return 1;
  }

static char * make_command_lpcm(const char * source_file)
  {
  return bg_sprintf("gavf-decode -vs m -ts m -os m -as d -i \"%s\" -oopt t=120000 | "
                    "gavf-encode -enc \"a2v=0:ae=e_wav{format=raw}\" "
                    "-ac \"bits=16:be=1\" -o -", source_file);
  }

static char * make_protocol_info_lpcm(bg_db_object_t * obj)
  {
  bg_db_audio_file_t * af;
  char * ret;
  char * dlna;
  if(bg_db_object_get_type(obj) != BG_DB_OBJECT_AUDIO_FILE)
    return NULL;

  af = (bg_db_audio_file_t *)obj;

  dlna = create_dlna_info(DLNA_ORG_CONVERSION_TRANSCODED,
                          DLNA_ORG_OPERATION_NONE,
                          DLNA_ORG_FLAG_STREAMING_TRANSFER_MODE |
                          DLNA_ORG_FLAG_BACKGROUND_TRANSFER_MODE |
                          DLNA_ORG_FLAG_CONNECTION_STALL |
                          DLNA_ORG_FLAG_DLNA_V15,
                          "LPCM");

  ret =  bg_sprintf("http-get:*:audio/L16:rate=%d;channels=%d;%s", af->samplerate,
                    af->channels, dlna);
  free(dlna);
  return ret;
  }

static void set_header_lpcm(bg_db_object_t * obj,
                            const gavl_metadata_t * req,
                            gavl_metadata_t * res)
  {
  char * tmp_string;
  bg_db_audio_file_t * af;
  //  int64_t content_length;

  if(bg_db_object_get_type(obj) != BG_DB_OBJECT_AUDIO_FILE)
    return;
  af = (bg_db_audio_file_t *)obj;

  
  tmp_string = bg_sprintf("audio/L16;rate=%d;channels=%d",
                          af->samplerate, af->channels); 


  gavl_metadata_set(res, "Content-Type", tmp_string);
  free(tmp_string);
#if 0
  content_length = gavl_time_scale(af->samplerate, obj->duration);
  content_length *= 2 * af->channels;
  content_length += 1024 * af->channels;
  tmp_string = bg_sprintf("%"PRId64, content_length); 
  gavl_metadata_set(res, "Content-Length", tmp_string);
  free(tmp_string);
#endif
  gavl_metadata_set(res, "transferMode.dlna.org", "Streaming");
  
  tmp_string = create_dlna_info(DLNA_ORG_CONVERSION_TRANSCODED,
                                DLNA_ORG_OPERATION_NONE,
                                DLNA_ORG_FLAG_STREAMING_TRANSFER_MODE |
                                DLNA_ORG_FLAG_BACKGROUND_TRANSFER_MODE |
                                DLNA_ORG_FLAG_CONNECTION_STALL |
                                DLNA_ORG_FLAG_DLNA_V15,
                                "LPCM");
  gavl_metadata_set(res, "contentFeatures.dlna.org", tmp_string);
  
  free(tmp_string);
  }

static int get_bitrate_lpcm(bg_db_object_t * obj)
  {
  bg_db_audio_file_t * af;
  af = (bg_db_audio_file_t *)obj;
  return af->samplerate * af->channels * 2 * 8;
  }

/* mp3 */

static int is_supported_mp3(bg_plugin_registry_t * plugin_reg)
  {
  return 1;
  }

static char * make_command_mp3(const char * source_file)
  {
  return bg_sprintf("gavf-decode -vs m -ts m -os m -as d -i \"%s\" | "
                    "gavf-encode -enc \"a2v=0:ae=e_lame{do_id3v1=0:do_id3v2=0}\" "
                    "-ac cbr_bitrate=320 -o -", source_file);
  }

static char * make_protocol_info_mp3(bg_db_object_t * obj)
  {
  return bg_sprintf("http-get:*:audio/mpeg:*");
  }
                    

static void set_header_mp3(bg_db_object_t * obj,
                           const gavl_metadata_t * req,
                           gavl_metadata_t * res)
  {
  gavl_metadata_set(res, "Content-Type", "audio/mpeg");
  }

static int get_bitrate_mp3(bg_db_object_t * obj)
  {
  return 320000;
  }

static bg_upnp_transcoder_t transcoders[] =
  {
#if 0
    {
      .is_supported = is_supported_lpcm,
      .make_command = make_command_lpcm,
      .make_protocol_info = make_protocol_info_lpcm,
      .set_header = set_header_lpcm,
      .get_bitrate = get_bitrate_lpcm,
      .name         = "lpcm-dlna",
      .in_mimetype  = "audio/*",
      .out_mimetype = "audio/L16",
      
    },
#endif
    {
      .is_supported = is_supported_mp3,
      .make_command = make_command_mp3,
      .make_protocol_info = make_protocol_info_mp3,
      .set_header = set_header_mp3,
      .get_bitrate = get_bitrate_mp3,
      .name         = "mp3-320",
      .in_mimetype  = "audio/*",
      .out_mimetype = "audio/mpeg",
      
    },
    { /* */ }
  };

const bg_upnp_transcoder_t *
bg_upnp_transcoder_find(const char ** mimetypes_supp, const char * in_mimetype)
  {
  int i = 0;
  while(transcoders[i].is_supported)
    {
    if(!fnmatch(transcoders[i].in_mimetype, in_mimetype, 0))
      {
      int j = 0;
      while(mimetypes_supp[j])
        {
        if(!strcmp(mimetypes_supp[j], transcoders[i].out_mimetype))
          return &transcoders[i];
        j++;
        }
      }
    i++;
    }
  return NULL;
  }

const bg_upnp_transcoder_t *
bg_upnp_transcoder_by_name(const char * name)
  {
  int i = 0;
  while(transcoders[i].is_supported)
    {
    if(!strcmp(name, transcoders[i].name))
      return &transcoders[i];
    i++;
    }
  return NULL;
  }

int64_t bg_upnp_transcoder_get_size(const bg_upnp_transcoder_t * t,
                                    bg_db_object_t * obj)
  {
  int bitrate = t->get_bitrate(obj);
  return (int64_t)(bitrate / 8 * gavl_time_to_seconds(obj->duration) + 0.5);
  }
