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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include <config.h>
#include <gmerlin/translation.h>

#include "cdaudio.h"
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "i_cdaudio"

#include <gavl/metatags.h>

#define FRAME_SAMPLES 588

typedef struct
  {
  bg_parameter_info_t * parameters;
  char * device_name;
  bg_track_info_t * track_info;

  void * ripper;

  char disc_id[DISCID_SIZE];

  //  int fd;

  CdIo_t *cdio;
  
  bg_cdaudio_index_t * index;

  char * trackname_template;
  int use_cdtext;
  int use_local;

  /* We initialize ripping on demand to speed up CD loading in the
     transcoder */
  int rip_initialized;
  
  /* Configuration stuff */

#ifdef HAVE_MUSICBRAINZ
  int use_musicbrainz;
  char * musicbrainz_host;
  int    musicbrainz_port;
  char * musicbrainz_proxy_host;
  int    musicbrainz_proxy_port;
#endif

#ifdef HAVE_LIBCDDB
  int    use_cddb;
  char * cddb_host;
  int    cddb_port;
  char * cddb_path;
  char * cddb_proxy_host;
  int    cddb_proxy_port;
  char * cddb_proxy_user;
  char * cddb_proxy_pass;
  int cddb_timeout;
#endif
  
  int current_track;
  int current_sector; /* For ripping only */
      
  int first_sector;
  
  bg_input_callbacks_t * callbacks;

  int old_seconds;
  bg_cdaudio_status_t status;

  int paused;
    
  const char * disc_name;

  gavl_audio_source_t * src;
  
  } cdaudio_t;

static void destroy_cd_data(cdaudio_t* cd)
  {
  int i;
  if(cd->track_info && cd->index)
    {
    for(i = 0; i < cd->index->num_audio_tracks; i++)
      bg_track_info_free(&cd->track_info[i]);
    free(cd->track_info);
    cd->track_info = NULL;
    }
  if(cd->index)
    {
    bg_cdaudio_index_destroy(cd->index);
    cd->index = NULL;
    }

  }


static const char * get_disc_name_cdaudio(void* priv)
  {
  cdaudio_t * cd;
  cd = priv;
  return cd->disc_name;
  }

static void close_cdaudio(void * priv);

static void * create_cdaudio()
  {
  cdaudio_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->ripper = bg_cdaudio_rip_create();
  return ret;
  }

static void set_callbacks_cdaudio(void * data,
                           bg_input_callbacks_t * callbacks)
  {
  cdaudio_t * cd;
  cd = data;
  cd->callbacks = callbacks;
  }

static void destroy_cdaudio(void * data)
  {
  cdaudio_t * cd;
  cd = data;

  destroy_cd_data(cd);
  
  if(cd->device_name)
    free(cd->device_name);

  if(cd->ripper)
    bg_cdaudio_rip_destroy(cd->ripper);

  if(cd->parameters)
    bg_parameter_info_destroy_array(cd->parameters);
  
  free(data);
  }

static int open_cdaudio(void * data, const char * arg)
  {
  int have_local_metadata = 0;
  int have_metadata = 0;
  int i, j;

  char * tmp_filename;
    
  cdaudio_t * cd = data;

  /* Destroy data from previous open */
  destroy_cd_data(cd);
  
  cd->device_name = gavl_strrep(cd->device_name, arg);

  cd->cdio = bg_cdaudio_open(cd->device_name);
  if(!cd->cdio)
    return 0;

  cd->index = bg_cdaudio_get_index(cd->cdio);
  if(!cd->index)
    return 0;

  //  bg_cdaudio_index_dump(cd->index);
  
  /* Create track infos */

  cd->track_info = calloc(cd->index->num_audio_tracks, sizeof(*(cd->track_info)));
  
  for(i = 0; i < cd->index->num_tracks; i++)
    {
    if(cd->index->tracks[i].is_audio)
      {
      j = cd->index->tracks[i].index;
      
      cd->track_info[j].num_audio_streams = 1;
      cd->track_info[j].audio_streams =
        calloc(1, sizeof(*(cd->track_info[j].audio_streams)));
      
      cd->track_info[j].audio_streams[0].format.samplerate = 44100;
      cd->track_info[j].audio_streams[0].format.num_channels = 2;
      cd->track_info[j].audio_streams[0].format.sample_format = GAVL_SAMPLE_S16;
      cd->track_info[j].audio_streams[0].format.interleave_mode = GAVL_INTERLEAVE_ALL;
      cd->track_info[j].audio_streams[0].format.samples_per_frame = FRAME_SAMPLES;
      gavl_metadata_set(&cd->track_info[j].audio_streams[0].m, GAVL_META_FORMAT,
                        "CD Audio");
      
      gavl_set_channel_setup(&cd->track_info[j].audio_streams[0].format);

      gavl_metadata_set_long(&cd->track_info[j].metadata,
                             GAVL_META_APPROX_DURATION,
                             ((int64_t)(cd->index->tracks[i].last_sector -
                                        cd->index->tracks[i].first_sector + 1) *
                              GAVL_TIME_SCALE) / 75);

      gavl_metadata_set(&cd->track_info[j].metadata, GAVL_META_FORMAT,
                        "CD Audio");
      gavl_metadata_set_int(&cd->track_info[j].metadata, GAVL_META_TRACKNUMBER,
                            j+1);
      cd->track_info[j].flags = BG_TRACK_SEEKABLE | BG_TRACK_PAUSABLE;
      }
    }

  /* Create the disc ID */

  bg_cdaudio_get_disc_id(cd->index, cd->disc_id);
  
  /* Now, try to get the metadata */

  /* 1st try: Check for cdtext */

  if(cd->use_cdtext)
    {
    if(bg_cdaudio_get_metadata_cdtext(cd->cdio,
                                      cd->track_info,
                                      cd->index))
      {
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got metadata from CD-Text");
      have_metadata = 1;
      have_local_metadata = 1; /* We never save cdtext infos */
      }
    }

  /* 2nd try: Local file */

  if(!have_metadata && cd->use_local)
    {
    tmp_filename = bg_search_file_read("cdaudio_metadata", cd->disc_id);
    if(tmp_filename)
      {
      if(bg_cdaudio_load(cd->track_info, tmp_filename))
        {
        have_metadata = 1;
        have_local_metadata = 1;
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got metadata from gmerlin cache (%s)", tmp_filename);
        }
      free(tmp_filename);
      }
    }
  
#ifdef HAVE_MUSICBRAINZ
  if(cd->use_musicbrainz && !have_metadata)
    {
    if(bg_cdaudio_get_metadata_musicbrainz(cd->index, cd->track_info,
                                           cd->disc_id,
                                           cd->musicbrainz_host,
                                           cd->musicbrainz_port,
                                           cd->musicbrainz_proxy_host,
                                           cd->musicbrainz_proxy_port))
      {
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got metadata from musicbrainz (%s)", cd->musicbrainz_host);
      have_metadata = 1;
      }
    }
#endif

#ifdef HAVE_LIBCDDB
  if(cd->use_cddb && !have_metadata)
    {
    if(bg_cdaudio_get_metadata_cddb(cd->index, cd->track_info,
                                    cd->cddb_host,
                                    cd->cddb_port,
                                    cd->cddb_path,
                                    cd->cddb_proxy_host,
                                    cd->cddb_proxy_port,
                                    cd->cddb_proxy_user,
                                    cd->cddb_proxy_user,
                                    cd->cddb_timeout))
      {
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got metadata from CDDB (%s)", cd->cddb_host);
      have_metadata = 1;
      /* Disable gmerlin caching */
      have_local_metadata = 1;
      }
    }
#endif
  
  if(have_metadata && !have_local_metadata)
    {
    tmp_filename = bg_search_file_write("cdaudio_metadata", cd->disc_id);
    if(tmp_filename)
      {
      bg_cdaudio_save(cd->track_info, cd->index->num_audio_tracks, tmp_filename);
      free(tmp_filename);
      }
    }
  
  if(!have_metadata)
    {
    for(i = 0; i < cd->index->num_tracks; i++)
      {
      if(cd->index->tracks[i].is_audio)
        {
        j = cd->index->tracks[i].index;
        if(cd->index->tracks[i].is_audio)
          gavl_metadata_set_nocpy(&cd->track_info[j].metadata,
                                  GAVL_META_LABEL,
                                  bg_sprintf(TR("Audio CD track %02d"), j+1));
        }
      }
    }
  else
    {
    for(i = 0; i < cd->index->num_tracks; i++)
      {
      if(cd->index->tracks[i].is_audio)
        {
        j = cd->index->tracks[i].index;
        if(cd->index->tracks[i].is_audio)
          gavl_metadata_set_nocpy(&cd->track_info[j].metadata,
                                  GAVL_META_LABEL,
                                  bg_create_track_name(&cd->track_info[j].metadata,
                                                       cd->trackname_template));
        }
      }
    cd->disc_name = gavl_metadata_get(&cd->track_info[0].metadata, GAVL_META_ALBUM);
    }

  /* We close it again, so other apps won't cry */

  close_cdaudio(cd);
  
  return 1;
  }

static int get_num_tracks_cdaudio(void * data)
  {
  cdaudio_t * cd = data;
  return cd->index->num_audio_tracks;
  }

static bg_track_info_t * get_track_info_cdaudio(void * data, int track)
  {
  cdaudio_t * cd = data;
  return &cd->track_info[track];
  }

static int set_track_cdaudio(void * data, int track)
  {
  int i;
  cdaudio_t * cd = data;

  if(cd->src)
    {
    gavl_audio_source_destroy(cd->src);
    cd->src = NULL;
    }
  
  for(i = 0; i < cd->index->num_tracks; i++)
    {
    if(cd->index->tracks[i].is_audio && (cd->index->tracks[i].index == track))
      {
      cd->current_track = i;
      break;
      }
    }

  cd->first_sector = cd->index->tracks[cd->current_track].first_sector;
  
  return 1;
  }

static int set_audio_stream_cdaudio(void * priv, int stream,
                                    bg_stream_action_t action)
  {
  return 1;
  }

static gavl_source_status_t read_frame(void * priv, gavl_audio_frame_t ** fp)
  {
  gavl_audio_frame_t * f = *fp;
  cdaudio_t * cd = priv;

  if(cd->current_sector > cd->index->tracks[cd->current_track].last_sector)
    return GAVL_SOURCE_EOF;
  
  if(!cd->rip_initialized)
    {
    bg_cdaudio_rip_init(cd->ripper, cd->cdio,
                        cd->first_sector);
    cd->rip_initialized = 1;
    }
  bg_cdaudio_rip_rip(cd->ripper, f);
  
  f->valid_samples = FRAME_SAMPLES;
  f->timestamp =
    (cd->current_sector - cd->index->tracks[cd->current_track].first_sector) *
    FRAME_SAMPLES;
  
  cd->current_sector++;
  return GAVL_SOURCE_OK;
  }

static int start_cdaudio(void * priv)
  {
  cdaudio_t * cd = priv;
  if(!cd->cdio)
    {
    cd->cdio = bg_cdaudio_open(cd->device_name);
    if(!cd->cdio)
      return 0;
    }
  cd->src = gavl_audio_source_create(read_frame, cd, 0,
                                     /* Format is the same for all tracks */
                                     &cd->track_info[0].audio_streams[0].format);
  
  cd->current_sector = cd->first_sector;
  return 1;
  }

static gavl_audio_source_t *
get_audio_source_cdaudio(void * priv, int stream)
  {
  cdaudio_t * cd = priv;
  return cd->src;
  }

static void stop_cdaudio(void * priv)
  {
  cdaudio_t * cd = priv;
  if(cd->rip_initialized)
    {
    bg_cdaudio_rip_close(cd->ripper);
    cd->rip_initialized = 0;
    }
  
  cd->cdio = NULL;
  }


static void seek_cdaudio(void * priv, int64_t * time, int scale)
  {
  /* We seek with frame accuracy (1/75 seconds) */

  uint32_t sample_position, samples_to_skip;
  
  cdaudio_t * cd = priv;
  
  if(!cd->rip_initialized)
    {
    bg_cdaudio_rip_init(cd->ripper, cd->cdio,
                        cd->first_sector);
    cd->rip_initialized = 1;
    }
  
  sample_position = gavl_time_rescale(scale, 44100, *time);
  
  cd->current_sector =
    sample_position / FRAME_SAMPLES + cd->index->tracks[cd->current_track].first_sector;
  samples_to_skip = sample_position % FRAME_SAMPLES;
  
  /* Seek to the point */
  
  bg_cdaudio_rip_seek(cd->ripper, cd->current_sector);

  /* Set skipped samples */

  gavl_audio_source_reset(cd->src);
  gavl_audio_source_skip_src(cd->src, samples_to_skip);
  }

static void close_cdaudio(void * priv)
  {
  cdaudio_t * cd = priv;
  if(cd->cdio)
    {
    bg_cdaudio_close(cd->cdio);
    cd->cdio = NULL;
    }
  if(cd->src)
    {
    gavl_audio_source_destroy(cd->src);
    cd->src = NULL;
    }
  }

/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "general",
      .long_name = TRS("General"),
      .type =      BG_PARAMETER_SECTION
    },
    {
      .name =        "trackname_template",
      .long_name =   TRS("Trackname template"),
      .type =        BG_PARAMETER_STRING,
      .val_default = { .val_str = "%p - %t" },
      .help_string = TRS("Template for track name generation from metadata\n\
%p:    Artist\n\
%a:    Album\n\
%g:    Genre\n\
%t:    Track name\n\
%<d>n: Track number (d = number of digits, 1-9)\n\
%y:    Year\n\
%c:    Comment")
    },
    {
      .name =        "use_cdtext",
      .long_name =   TRS("Use CD-Text"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Try to get CD metadata from CD-Text"),
    },
    {
      .name =        "use_local",
      .long_name =   TRS("Use locally saved metadata"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Whenever we obtain CD metadata from the internet, we save them into \
$HOME/.gmerlin/cdaudio_metadata. If you got wrong metadata for a CD,\
 disabling this option will retrieve the metadata again and overwrite the saved data."),
    },
#ifdef HAVE_MUSICBRAINZ
    {
      .name =      "musicbrainz",
      .long_name = TRS("Musicbrainz"),
      .type =      BG_PARAMETER_SECTION
    },
    {
      .name =        "use_musicbrainz",
      .long_name =   TRS("Use Musicbrainz"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 }
    },
    {
      .name =        "musicbrainz_host",
      .long_name =   TRS("Server"),
      .type =        BG_PARAMETER_STRING,
      .val_default = { .val_str = "mm.musicbrainz.org" }
    },
    {
      .name =        "musicbrainz_port",
      .long_name =   TRS("Port"),
      .type =         BG_PARAMETER_INT,
      .val_min =      { .val_i = 1 },
      .val_max =      { .val_i = 65535 },
      .val_default =  { .val_i = 80 }
    },
    {
      .name =        "musicbrainz_proxy_host",
      .long_name =   TRS("Proxy"),
      .type =        BG_PARAMETER_STRING,
      .help_string = TRS("Proxy server (leave empty for direct connection)")
    },
    {
      .name =        "musicbrainz_proxy_port",
      .long_name =   TRS("Proxy port"),
      .type =         BG_PARAMETER_INT,
      .val_min =      { .val_i = 1 },
      .val_max =      { .val_i = 65535 },
      .val_default =  { .val_i = 80 },
      .help_string = TRS("Proxy port")
    },
#endif
#ifdef HAVE_LIBCDDB
    {
      .name =      "cddb",
      .long_name = TRS("Cddb"),
      .type =      BG_PARAMETER_SECTION
    },
    {
      .name =        "use_cddb",
      .long_name =   TRS("Use Cddb"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 }
    },
    {
      .name =        "cddb_host",
      .long_name =   TRS("Server"),
      .type =        BG_PARAMETER_STRING,
      .val_default = { .val_str = "freedb.org" }
    },
    {
      .name =        "cddb_port",
      .long_name =   TRS("Port"),
      .type =         BG_PARAMETER_INT,
      .val_min =      { .val_i = 1 },
      .val_max =      { .val_i = 65535 },
      .val_default =  { .val_i = 80 }
    },
    {
      .name =        "cddb_path",
      .long_name =   TRS("Path"),
      .type =        BG_PARAMETER_STRING,
      .val_default = { .val_str = "/~cddb/cddb.cgi" }
    },
    {
      .name =        "cddb_proxy_host",
      .long_name =   TRS("Proxy"),
      .type =        BG_PARAMETER_STRING,
      .help_string = TRS("Proxy server (leave empty for direct connection)")
    },
    {
      .name =        "cddb_proxy_port",
      .long_name =   TRS("Proxy port"),
      .type =         BG_PARAMETER_INT,
      .val_min =      { .val_i = 1 },
      .val_max =      { .val_i = 65535 },
      .val_default =  { .val_i = 80 },
      .help_string = TRS("Proxy port")
    },
    {
      .name =        "cddb_proxy_user",
      .long_name =   TRS("Proxy username"),
      .type =        BG_PARAMETER_STRING,
      .help_string = TRS("User name for proxy (leave empty for poxies, which don't require authentication)")
    },
    {
      .name =        "cddb_proxy_pass",
      .long_name =   TRS("Proxy password"),
      .type =        BG_PARAMETER_STRING_HIDDEN,
      .help_string = TRS("Password for proxy")
    },
    {
      .name =        "cddb_timeout",
      .long_name =   TRS("Timeout"),
      .type =         BG_PARAMETER_INT,
      .val_min =      { .val_i = 0 },
      .val_max =      { .val_i = 1000 },
      .val_default =  { .val_i = 10 },
      .help_string = TRS("Timeout (in seconds) for connections to the CDDB server")
    },
#endif
    { /* End of parmeters */ }
  };

static const bg_parameter_info_t * get_parameters_cdaudio(void * data)
  {
  cdaudio_t * cd = data;
  bg_parameter_info_t const * srcs[3];

  if(!cd->parameters)
    {
    srcs[0] = parameters;
    srcs[1] = bg_cdaudio_rip_get_parameters();
    srcs[2] = NULL;
    cd->parameters = bg_parameter_info_concat_arrays(srcs);
    }
    
  return cd->parameters;
  }

static void set_parameter_cdaudio(void * data, const char * name,
                                  const bg_parameter_value_t * val)
  {
  cdaudio_t * cd = data;

  if(!name)
    return;

  if(bg_cdaudio_rip_set_parameter(cd->ripper, name, val))
    return;

  if(!strcmp(name, "trackname_template"))
    cd->trackname_template = gavl_strrep(cd->trackname_template, val->val_str);

  if(!strcmp(name, "use_cdtext"))
    cd->use_cdtext = val->val_i;
  if(!strcmp(name, "use_local"))
    cd->use_local = val->val_i;
  
#ifdef HAVE_MUSICBRAINZ
  if(!strcmp(name, "use_musicbrainz"))
    cd->use_musicbrainz = val->val_i;
  if(!strcmp(name, "musicbrainz_host"))
    cd->musicbrainz_host = gavl_strrep(cd->musicbrainz_host, val->val_str);
  if(!strcmp(name, "musicbrainz_port"))
    cd->musicbrainz_port = val->val_i;
  if(!strcmp(name, "musicbrainz_proxy_host"))
    cd->musicbrainz_proxy_host = gavl_strrep(cd->musicbrainz_proxy_host, val->val_str);
  if(!strcmp(name, "musicbrainz_proxy_port"))
    cd->musicbrainz_proxy_port = val->val_i;
#endif

#ifdef HAVE_LIBCDDB
  if(!strcmp(name, "use_cddb"))
    cd->use_cddb = val->val_i;
  if(!strcmp(name, "cddb_host"))
    cd->cddb_host = gavl_strrep(cd->cddb_host, val->val_str);
  if(!strcmp(name, "cddb_port"))
    cd->cddb_port = val->val_i;
  if(!strcmp(name, "cddb_path"))
    cd->cddb_path = gavl_strrep(cd->cddb_path, val->val_str);
  if(!strcmp(name, "cddb_proxy_host"))
    cd->cddb_proxy_host = gavl_strrep(cd->cddb_proxy_host, val->val_str);
  if(!strcmp(name, "cddb_proxy_port"))
    cd->cddb_proxy_port = val->val_i;
  if(!strcmp(name, "cddb_proxy_user"))
    cd->cddb_proxy_user = gavl_strrep(cd->cddb_proxy_user, val->val_str);
  if(!strcmp(name, "cddb_proxy_pass"))
    cd->cddb_proxy_pass = gavl_strrep(cd->cddb_proxy_pass, val->val_str);
  if(!strcmp(name, "cddb_timeout"))
    cd->cddb_timeout = val->val_i;
#endif
  }

static int eject_disc_cdaudio(const char * device)
  {
#if LIBCDIO_VERSION_NUM >= 78
  
  driver_return_code_t err;
  if((err = cdio_eject_media_drive(device)) != DRIVER_OP_SUCCESS)
    {
#if LIBCDIO_VERSION_NUM >= 77
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Ejecting disk failed: %s", cdio_driver_errmsg(err));
#else
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Ejecting disk failed");
#endif
    return 0;
    }
  else
    return 1;
#else
  return 0;
#endif
  }

char const * const protocols = "cda";

static const char * get_protocols(void * priv)
  {
  return protocols;
  }

const bg_input_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "i_cdaudio",
      .long_name =     TRS("Audio CD player/ripper"),
      .description =   TRS("Plugin for audio CDs. Supports both playing with direct connection from the CD-drive to the souncard and ripping with cdparanoia. Metadata are obtained from Musicbrainz, freedb or CD-text. Metadata are cached in $HOME/.gmerlin/cdaudio_metadata."),
      .type =          BG_PLUGIN_INPUT,

      .flags =         BG_PLUGIN_REMOVABLE,      
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        create_cdaudio,
      .destroy =       destroy_cdaudio,
      .get_parameters = get_parameters_cdaudio,
      .set_parameter =  set_parameter_cdaudio,
      .find_devices = bg_cdaudio_find_devices,
      .check_device = bg_cdaudio_check_device,
    },
    .get_protocols = get_protocols,
    /* Open file/device */
    .open = open_cdaudio,
    .get_disc_name = get_disc_name_cdaudio,
#if LIBCDIO_VERSION_NUM >= 78
    .eject_disc = eject_disc_cdaudio,
#endif
    .set_callbacks = set_callbacks_cdaudio,
  /* For file and network plugins, this can be NULL */
    .get_num_tracks = get_num_tracks_cdaudio,
    /* Return track information */
    .get_track_info = get_track_info_cdaudio,
    
    /* Set track */
    .set_track =             set_track_cdaudio,
    /* Set streams */
    .set_audio_stream =      set_audio_stream_cdaudio,
    .set_video_stream =      NULL,

    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    .start =                 start_cdaudio,
    
    .get_audio_source = get_audio_source_cdaudio,
    
    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    .seek =         seek_cdaudio,
    /* Stop playback, close all decoders */
    .stop =         stop_cdaudio,
    .close =        close_cdaudio,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
