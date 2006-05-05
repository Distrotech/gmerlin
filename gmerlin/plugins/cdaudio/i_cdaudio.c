#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include "cdaudio.h"
#include <utils.h>
#include <log.h>

#define LOG_DOMAIN "i_cdaudio"

typedef struct
  {
  bg_parameter_info_t * parameters;
  char * device_name;
  bg_track_info_t * track_info;

  void * ripper;
  gavl_audio_frame_t * frame;
  int last_read_samples;
  int read_sectors; /* Sectors to read at once */

  char disc_id[DISCID_SIZE];

  //  int fd;

  CdIo_t *cdio;
  
  bg_cdaudio_index_t * index;

  char * trackname_template;
  int use_cdtext;
  int use_local;
  
  /* Configuration stuff */

#ifdef HAVE_MUSICBRAINZ
  int use_musicbrainz;
  char * musicbrainz_host;
  int    musicbrainz_port;
  char * musicbrainz_proxy_host;
  int    musicbrainz_proxy_port;
#endif

#ifdef HAVE_CDDB
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
  
  int do_bypass;

  bg_input_callbacks_t * callbacks;

  int old_seconds;
  bg_cdaudio_status_t status;

  uint32_t samples_written;

  int paused;
  
  char * error_msg;
  } cdaudio_t;

static void destroy_cd_data(cdaudio_t* cd)
  {
  int i;
  if(cd->track_info && cd->index)
    {
    for(i = 0; i < cd->index->num_audio_tracks; i++)
      bg_track_info_free(&(cd->track_info[i]));
    free(cd->track_info);
    cd->track_info = (bg_track_info_t*)0;
    }
  if(cd->index)
    {
    bg_cdaudio_index_destroy(cd->index);
    cd->index = (bg_cdaudio_index_t*)0;
    }

  if(cd->error_msg)
    free(cd->error_msg);
  }

const char * get_error_cdaudio(void* priv)
  {
  cdaudio_t * cd;
  cd = (cdaudio_t *)priv;
  return cd->error_msg;
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
  cd = (cdaudio_t *)data;
  cd->callbacks = callbacks;
  }

static void destroy_cdaudio(void * data)
  {
  cdaudio_t * cd;
  cd = (cdaudio_t *)data;

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
    
  cdaudio_t * cd = (cdaudio_t*)data;

  /* Destroy data from previous open */
  destroy_cd_data(cd);
  
  cd->device_name = bg_strdup(cd->device_name, arg);

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
      cd->track_info[j].audio_streams[0].description = bg_strdup(NULL, "CD audio");
      
      gavl_set_channel_setup(&(cd->track_info[j].audio_streams[0].format));
      
      cd->track_info[j].duration =
        ((int64_t)(cd->index->tracks[i].last_sector -
          cd->index->tracks[i].first_sector + 1) * GAVL_TIME_SCALE) / 75;
      cd->track_info[j].description = bg_strdup(NULL, "CD audio track");
      //      cd->track_info[j].name = bg_sprintf("Audio CD track %02d", j+1);
      cd->track_info[j].metadata.track = j+1;
      cd->track_info[j].seekable = 1;
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

#ifdef HAVE_CDDB
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
          cd->track_info[j].name = bg_sprintf("Audio CD track %02d", j+1);
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
          cd->track_info[j].name = bg_create_track_name(&(cd->track_info[j].metadata),
                                                        cd->trackname_template);
        }
      }
    }

  /* We close it again, so other apps won't cry */

  close_cdaudio(cd);
  
  return 1;
  }

static int get_num_tracks_cdaudio(void * data)
  {
  cdaudio_t * cd = (cdaudio_t*)data;
  return cd->index->num_audio_tracks;
  }

static bg_track_info_t * get_track_info_cdaudio(void * data, int track)
  {
  cdaudio_t * cd = (cdaudio_t*)data;
  return &(cd->track_info[track]);
  }

static int set_track_cdaudio(void * data, int track)
  {
  int i;
  cdaudio_t * cd = (cdaudio_t*)data;

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
  cdaudio_t * cd = (cdaudio_t*)priv;
  if(action == BG_STREAM_ACTION_BYPASS)
    cd->do_bypass = 1;
  else
    cd->do_bypass = 0;
  return 1;
  }

static int start_cdaudio(void * priv)
  {
  int i, last_sector;
  cdaudio_t * cd = (cdaudio_t*)priv;

  //  fprintf(stderr, "start_cdaudio %d\n", cd->do_bypass);

  if(!cd->cdio)
    {
    cd->cdio = bg_cdaudio_open(cd->device_name);
    if(!cd->cdio)
      return 0;
    }
  
  if(cd->do_bypass)
    {

    last_sector = cd->index->tracks[cd->current_track].last_sector;

    for(i = cd->current_track; i < cd->index->num_tracks; i++)
      {
      if((i == cd->index->num_tracks - 1) || !cd->index->tracks[i+1].is_audio)
        last_sector = cd->index->tracks[i].last_sector;
      }
    if(!bg_cdaudio_play(cd->cdio, cd->first_sector, last_sector))
      {
      cd->error_msg = bg_sprintf("Play command failed. Disk missing?");
      return 0;
      }
    cd->status.sector = cd->first_sector;
    cd->status.track  = cd->current_track;

    for(i = 0; i < cd->index->num_audio_tracks; i++)
      {
      cd->track_info[i].audio_streams[0].format.samples_per_frame = 588;
      }
    }
  else
    {
    /* Rip */
    bg_cdaudio_rip_init(cd->ripper, cd->cdio,
                        cd->first_sector,
                        cd->first_sector - cd->index->tracks[0].first_sector,
                        &(cd->read_sectors));

    for(i = 0; i < cd->index->num_audio_tracks; i++)
      {
      cd->track_info[i].audio_streams[0].format.samples_per_frame =
        cd->read_sectors * 588;
      }
    
    
    cd->frame =
      gavl_audio_frame_create(&(cd->track_info[0].audio_streams[0].format));
    cd->current_sector = cd->first_sector;
    cd->samples_written = 0;
    }
  return 1;
  }

static void stop_cdaudio(void * priv)
  {
  cdaudio_t * cd = (cdaudio_t*)priv;
  if(cd->do_bypass)
    {
    //    fprintf(stderr, "stop_cdaudio\n");
    bg_cdaudio_stop(cd->cdio);
    close_cdaudio(cd);
    }
  else
    {
    bg_cdaudio_rip_close(cd->ripper);
    //    fprintf(stderr, "Processed %d samples\n", cd->samples_written);
    }
  cd->cdio = (CdIo_t*)0;
  }

static void read_frame(cdaudio_t * cd)
  {
  bg_cdaudio_rip_rip(cd->ripper, cd->frame);

  if(cd->current_sector + cd->read_sectors >
     cd->index->tracks[cd->current_track].last_sector)
    {
    cd->frame->valid_samples =
      (cd->index->tracks[cd->current_track].last_sector - 
       cd->current_sector + 1) * 588;
    }
  else
    cd->frame->valid_samples = cd->read_sectors * 588;
  cd->last_read_samples = cd->frame->valid_samples;
  cd->current_sector += cd->read_sectors;
  }

static int read_audio_cdaudio(void * priv,
                              gavl_audio_frame_t * frame, int stream,
                              int num_samples)
  {
  int samples_read = 0, samples_copied;
  cdaudio_t * cd = (cdaudio_t*)priv;


  //  fprintf(stderr, "Sector: %d %d\n", cd->current_sector,
  //          cd->index->tracks[cd->current_track].last_sector);
  
  if(cd->current_sector > cd->index->tracks[cd->current_track].last_sector)
    {
    //    fprintf(stderr, "EOF: %d %d\n", cd->current_sector,
    //            cd->index->tracks[cd->current_track].last_sector);
    return 0;
    }
  while(samples_read < num_samples)
    {
    if(cd->current_sector > cd->index->tracks[cd->current_track].last_sector)
      break;
    
    if(!cd->frame->valid_samples)
      read_frame(cd);

    samples_copied = gavl_audio_frame_copy(&(cd->track_info[0].audio_streams[0].format),
                                           frame,
                                           cd->frame,
                                           samples_read, /* out_pos */
                                           cd->last_read_samples - cd->frame->valid_samples,  /* in_pos */
                                           num_samples - samples_read, /* out_size, */
                                           cd->frame->valid_samples /* in_size */);
    cd->frame->valid_samples -= samples_copied;
    samples_read += samples_copied;
    
    //    fprintf(stderr, "cd->frame->valid_samples: %d\n", cd->frame->valid_samples);
    }
  if(frame)
    frame->valid_samples = samples_read;
  cd->samples_written += samples_read;
  return samples_read;
  }

static int bypass_cdaudio(void * priv)
  {
  int j;
  int seconds;
  
  cdaudio_t * cd = (cdaudio_t*)priv;

  if(!bg_cdaudio_get_status(cd->cdio, &(cd->status)))
    {
    //    fprintf(stderr, "bg_cdaudio_get_status returned 0\n");
    return 0;
    }
  if((cd->status.track < cd->current_track) ||
     (cd->status.track > cd->current_track+1))
    {
    //    fprintf(stderr, "bg_cdaudio_get_status returned bullshit\n");
    cd->status.track = cd->current_track;
    return 1;
    }
  if(cd->status.track == cd->current_track + 1)
    {
    //    fprintf(stderr, "Track changed, old_track: %d, new_track: %d\n",
    //            cd->current_track, cd->status.track);
    cd->current_track = cd->status.track;

    j = cd->index->tracks[cd->current_track].index;
    
    if(cd->callbacks)
      {
      if(cd->callbacks->track_changed)
        cd->callbacks->track_changed(cd->callbacks->data,
                                     cd->current_track);

      if(cd->callbacks->name_changed)
        cd->callbacks->name_changed(cd->callbacks->data,
                                    cd->track_info[j].name);

      if(cd->callbacks->duration_changed)
        cd->callbacks->duration_changed(cd->callbacks->data,
                                        cd->track_info[j].duration);

      if(cd->callbacks->metadata_changed)
        cd->callbacks->metadata_changed(cd->callbacks->data,
                                        &(cd->track_info[j].metadata));
      

      }
    cd->first_sector = cd->index->tracks[cd->current_track].first_sector;
    }

  seconds = (cd->status.sector - cd->first_sector) / 75;

  if(cd->old_seconds != seconds)
    {
    cd->old_seconds = seconds;

    if((cd->callbacks) && (cd->callbacks->time_changed))
      {
      cd->callbacks->time_changed(cd->callbacks->data,
                                  ((gavl_time_t)seconds) * GAVL_TIME_SCALE);
      }
    }
  
  return 1;
  }

static void seek_cdaudio(void * priv, gavl_time_t * time)
  {
  /* We seek with frame accuracy (1/75 seconds) */

  int i, last_sector;
  int sector;
  uint32_t sample_position, samples_to_skip;
  
  cdaudio_t * cd = (cdaudio_t*)priv;
  //  fprintf(stderr, "Seek cdaudio\n");
  
  if(cd->do_bypass)
    {
    sector = cd->index->tracks[cd->current_track].first_sector +
      (*time * 75) / GAVL_TIME_SCALE;
    
    last_sector = cd->index->tracks[cd->current_track].last_sector;

    for(i = cd->current_track; i < cd->index->num_tracks; i++)
      {
      if((i == cd->index->num_tracks - 1) || !cd->index->tracks[i+1].is_audio)
        last_sector = cd->index->tracks[i].last_sector;
      }
    *time = ((int64_t)sector * GAVL_TIME_SCALE) / 75;
    if(!bg_cdaudio_play(cd->cdio, sector, last_sector))
      return;
    
    if(cd->paused)
      {
      bg_cdaudio_set_pause(cd->cdio, 1);
      }
    }
  else /* TODO */
    {
    sample_position = gavl_time_to_samples(44100, *time);
        
    cd->current_sector =
      sample_position / 588 + cd->index->tracks[cd->current_track].first_sector;
    samples_to_skip = sample_position % 588;

    /* Seek to the point */

    bg_cdaudio_rip_seek(cd->ripper, cd->current_sector,
                        cd->current_sector - cd->index->tracks[0].first_sector);

    /* Read one frame os samples (can be more than one sector) */
    read_frame(cd);

    /* Set skipped samples */
    
    cd->frame->valid_samples -= samples_to_skip;
    }
  }

void bypass_set_pause_cdaudio(void * priv, int pause)
  {
  cdaudio_t * cd = (cdaudio_t*)priv;
  bg_cdaudio_set_pause(cd->cdio, pause);
  cd->paused = pause;
  }

void bypass_set_volume_cdaudio(void * priv, float volume)
  {
  cdaudio_t * cd = (cdaudio_t*)priv;
  bg_cdaudio_set_volume(cd->cdio, volume);
  }

static void close_cdaudio(void * priv)
  {
  cdaudio_t * cd = (cdaudio_t*)priv;
  if(cd->cdio)
    {
    //    fprintf(stderr, "Closing CD device, read %d samples", cd->samples_written);
    bg_cdaudio_close(cd->cdio);
    //    fprintf(stderr, "done\n");
    }
  cd->cdio = (CdIo_t*)0;
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:      "general",
      long_name: "General",
      type:      BG_PARAMETER_SECTION
    },
    {
      name:        "trackname_template",
      long_name:   "Trackname template",
      type:        BG_PARAMETER_STRING,
      val_default: { val_str: "%p - %t" },
      help_string: "Template for track name generation from metadata\n\
%p:    Artist\n\
%a:    Album\n\
%g:    Genre\n\
%t:    Track name\n\
%<d>n: Track number (d = number of digits, 1-9)\n\
%y:    Year\n\
%c:    Comment\n"
    },
    {
      name:        "use_cdtext",
      long_name:   "Use CD-Text",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
      help_string: "Try to get CD metadata from CD-Text",
    },
    {
      name:        "use_local",
      long_name:   "Use locally saved metadata",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
      help_string: "Whenever we obtain CD metadata from the internet, we save them into \
$HOME/.gmerlin/cdaudio_metadata. If you got wrong metadata for a CD,\
 disabling this option will retrieve the metadata again and overwrite the saved data.",
    },
#ifdef HAVE_MUSICBRAINZ
    {
      name:      "musicbrainz",
      long_name: "Musicbrainz",
      type:      BG_PARAMETER_SECTION
    },
    {
      name:        "use_musicbrainz",
      long_name:   "Use Musicbrainz",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "musicbrainz_host",
      long_name:   "Server",
      type:        BG_PARAMETER_STRING,
      val_default: { val_str: "mm.musicbrainz.org" }
    },
    {
      name:        "musicbrainz_port",
      long_name:   "Port",
      type:         BG_PARAMETER_INT,
      val_min:      { val_i: 1 },
      val_max:      { val_i: 65535 },
      val_default:  { val_i: 80 }
    },
    {
      name:        "musicbrainz_proxy_host",
      long_name:   "Proxy",
      type:        BG_PARAMETER_STRING,
      help_string: "Proxy server (leave empty for direct connection)"
    },
    {
      name:        "musicbrainz_proxy_port",
      long_name:   "Proxy Port",
      type:         BG_PARAMETER_INT,
      val_min:      { val_i: 1 },
      val_max:      { val_i: 65535 },
      val_default:  { val_i: 80 },
      help_string: "Proxy port"
    },
#endif
#ifdef HAVE_CDDB
    {
      name:      "cddb",
      long_name: "Cddb",
      type:      BG_PARAMETER_SECTION
    },
    {
      name:        "use_cddb",
      long_name:   "Use Cddb",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "cddb_host",
      long_name:   "Server",
      type:        BG_PARAMETER_STRING,
      val_default: { val_str: "freedb.org" }
    },
    {
      name:        "cddb_port",
      long_name:   "Port",
      type:         BG_PARAMETER_INT,
      val_min:      { val_i: 1 },
      val_max:      { val_i: 65535 },
      val_default:  { val_i: 80 }
    },
    {
      name:        "cddb_path",
      long_name:   "Path",
      type:        BG_PARAMETER_STRING,
      val_default: { val_str: "/~cddb/cddb.cgi" }
    },
    {
      name:        "cddb_proxy_host",
      long_name:   "Proxy",
      type:        BG_PARAMETER_STRING,
      help_string: "Proxy server (leave empty for direct connection)"
    },
    {
      name:        "cddb_proxy_port",
      long_name:   "Proxy Port",
      type:         BG_PARAMETER_INT,
      val_min:      { val_i: 1 },
      val_max:      { val_i: 65535 },
      val_default:  { val_i: 80 },
      help_string: "Proxy port"
    },
    {
      name:        "cddb_proxy_user",
      long_name:   "Proxy username",
      type:        BG_PARAMETER_STRING,
      help_string: "User name for proxy (leave empty for poxies, which don't require authentication)"
    },
    {
      name:        "cddb_proxy_pass",
      long_name:   "Proxy password",
      type:        BG_PARAMETER_STRING_HIDDEN,
      help_string: "Password for proxy"
    },
    {
      name:        "cddb_timeout",
      long_name:   "Timeout",
      type:         BG_PARAMETER_INT,
      val_min:      { val_i: 0 },
      val_max:      { val_i: 1000 },
      val_default:  { val_i: 10 },
      help_string: "Timeout (in seconds) for connections to the CDDB server"
    },
#endif
    { /* End of parmeters */ }
  };

static bg_parameter_info_t * get_parameters_cdaudio(void * data)
  {
  cdaudio_t * cd = (cdaudio_t*)data;
  bg_parameter_info_t * srcs[3];

  if(!cd->parameters)
    {
    srcs[0] = parameters;
    srcs[1] = bg_cdaudio_rip_get_parameters();
    srcs[2] = (bg_parameter_info_t*)0;
    cd->parameters = bg_parameter_info_merge_arrays(srcs);
    }
    
  return cd->parameters;
  }

static void set_parameter_cdaudio(void * data, char * name, bg_parameter_value_t * val)
  {
  cdaudio_t * cd = (cdaudio_t*)data;

  if(!name)
    return;

  if(bg_cdaudio_rip_set_parameter(cd->ripper, name, val))
    return;

  if(!strcmp(name, "trackname_template"))
    cd->trackname_template = bg_strdup(cd->trackname_template, val->val_str);

  if(!strcmp(name, "use_cdtext"))
    cd->use_cdtext = val->val_i;
  if(!strcmp(name, "use_local"))
    cd->use_local = val->val_i;
  
#ifdef HAVE_MUSICBRAINZ
  if(!strcmp(name, "use_musicbrainz"))
    cd->use_musicbrainz = val->val_i;
  if(!strcmp(name, "musicbrainz_host"))
    cd->musicbrainz_host = bg_strdup(cd->musicbrainz_host, val->val_str);
  if(!strcmp(name, "musicbrainz_port"))
    cd->musicbrainz_port = val->val_i;
  if(!strcmp(name, "musicbrainz_proxy_host"))
    cd->musicbrainz_proxy_host = bg_strdup(cd->musicbrainz_proxy_host, val->val_str);
  if(!strcmp(name, "musicbrainz_proxy_port"))
    cd->musicbrainz_proxy_port = val->val_i;
#endif

#ifdef HAVE_CDDB
  if(!strcmp(name, "use_cddb"))
    cd->use_cddb = val->val_i;
  if(!strcmp(name, "cddb_host"))
    cd->cddb_host = bg_strdup(cd->cddb_host, val->val_str);
  if(!strcmp(name, "cddb_port"))
    cd->cddb_port = val->val_i;
  if(!strcmp(name, "cddb_path"))
    cd->cddb_path = bg_strdup(cd->cddb_path, val->val_str);
  if(!strcmp(name, "cddb_proxy_host"))
    cd->cddb_proxy_host = bg_strdup(cd->cddb_proxy_host, val->val_str);
  if(!strcmp(name, "cddb_proxy_port"))
    cd->cddb_proxy_port = val->val_i;
  if(!strcmp(name, "cddb_proxy_user"))
    cd->cddb_proxy_user = bg_strdup(cd->cddb_proxy_user, val->val_str);
  if(!strcmp(name, "cddb_proxy_pass"))
    cd->cddb_proxy_pass = bg_strdup(cd->cddb_proxy_pass, val->val_str);
  if(!strcmp(name, "cddb_timeout"))
    cd->cddb_timeout = val->val_i;
#endif
  }

bg_input_plugin_t the_plugin =
  {
    common:
    {
      name:          "i_cdaudio",
      long_name:     "Audio CD player/ripper",
      type:          BG_PLUGIN_INPUT,

      flags:         BG_PLUGIN_REMOVABLE |
                     BG_PLUGIN_BYPASS |
                     BG_PLUGIN_KEEP_RUNNING |
                     BG_PLUGIN_INPUT_HAS_SYNC,
      
      priority:      BG_PLUGIN_PRIORITY_MAX,
      create:        create_cdaudio,
      destroy:       destroy_cdaudio,
      get_parameters: get_parameters_cdaudio,
      set_parameter:  set_parameter_cdaudio,
      find_devices: bg_cdaudio_find_devices,
      check_device: bg_cdaudio_check_device,
      get_error:    get_error_cdaudio,
    },
  /* Open file/device */
    open: open_cdaudio,
    set_callbacks: set_callbacks_cdaudio,
  /* For file and network plugins, this can be NULL */
    get_num_tracks: get_num_tracks_cdaudio,
    /* Return track information */
    get_track_info: get_track_info_cdaudio,
    
    /* Set track */
    set_track:             set_track_cdaudio,
    /* Set streams */
    set_audio_stream:      set_audio_stream_cdaudio,
    set_video_stream:      NULL,
    set_subtitle_stream:   NULL,

    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    start:                 start_cdaudio,
    /* Read one audio frame (returns FALSE on EOF) */
    read_audio_samples:    read_audio_cdaudio,
    /* Read one video frame (returns FALSE on EOF) */
    read_video_frame:      NULL,

    bypass:                bypass_cdaudio,
    bypass_set_pause:      bypass_set_pause_cdaudio,
    bypass_set_volume:     bypass_set_volume_cdaudio,

    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    seek:         seek_cdaudio,
    /* Stop playback, close all decoders */
    stop:         stop_cdaudio,
    close:        close_cdaudio,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
