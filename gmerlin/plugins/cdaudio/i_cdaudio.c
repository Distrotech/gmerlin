#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <utils.h>

#include "cdaudio.h"

typedef struct
  {
  bg_track_info_t * track_info;

  int start_seconds;
  
  int fd;

  bg_cdaudio_index_t * index;
  
  /* Configuration stuff */

#ifdef HAVE_MUSICBRAINZ
  int use_musicbrainz;
  char * musicbrainz_host;
  int    musicbrainz_port;
  char * musicbrainz_proxy_host;
  int    musicbrainz_proxy_port;
#endif

  int current_track;
  int first_sector;
  
  int do_bypass;

  bg_input_callbacks_t * callbacks;

  int old_seconds;
  bg_cdaudio_status_t status;
  } cdaudio_t;

static void * create_cdaudio()
  {
  cdaudio_t * ret;
  ret = calloc(1, sizeof(*ret));
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
  free(data);
  }

static int open_cdaudio(void * data, const char * arg)
  {
  int have_metadata = 0;
  int i, j;

  cdaudio_t * cd = (cdaudio_t*)data;

  cd->fd = bg_cdaudio_open(arg);
  if(cd->fd < 0)
    return 0;

  cd->index = bg_cdaudio_get_index(cd->fd);
  if(!cd->index)
    return 0;

  bg_cdaudio_index_dump(cd->index);
  
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
      cd->track_info[j].name = bg_sprintf("Audio CD track %d", j+1);
      cd->track_info[j].metadata.track = j+1;
      cd->track_info[j].seekable = 1;
      }
    }

  /* Now, try to get the metadata */
#ifdef HAVE_MUSICBRAINZ
  if(cd->use_musicbrainz)
    {
    if(bg_cdaudio_get_metadata_musicbrainz(cd->index, cd->track_info,
                                           cd->musicbrainz_host,
                                           cd->musicbrainz_port,
                                           cd->musicbrainz_proxy_host,
                                           cd->musicbrainz_proxy_port))
      have_metadata = 1;
    }
#endif

  if(!have_metadata)
    {
    for(i = 0; i < cd->index->num_tracks; i++)
      {
      if(cd->index->tracks[i].is_audio)
        {
        j = cd->index->tracks[i].index;
        if(cd->index->tracks[i].is_audio)
          cd->track_info[j].name = bg_sprintf("Audio CD track %d", j+1);
        }
      }
    }
  
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
    if(cd->index->tracks[i].index == track)
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

static void start_cdaudio(void * priv)
  {
  int i, last_sector;
  cdaudio_t * cd = (cdaudio_t*)priv;

  //  fprintf(stderr, "start_cdaudio %d\n", cd->do_bypass);
  
  if(cd->do_bypass)
    {
    last_sector = cd->index->tracks[cd->current_track].last_sector;

    for(i = cd->current_track; i < cd->index->num_tracks; i++)
      {
      if((i == cd->index->num_tracks - 1) || !cd->index->tracks[i+1].is_audio)
        last_sector = cd->index->tracks[i].last_sector;
      }
    bg_cdaudio_play(cd->fd, cd->first_sector, last_sector);
    cd->status.sector = cd->first_sector;
    cd->status.track  = cd->current_track;
    }
  }

static void stop_cdaudio(void * priv)
  {
  cdaudio_t * cd = (cdaudio_t*)priv;
  if(cd->do_bypass)
    {
    fprintf(stderr, "stop_cdaudio\n");
    bg_cdaudio_stop(cd->fd);
    }
  
  }

static int read_audio_cdaudio(void * priv,
                              gavl_audio_frame_t * frame, int stream,
                              int num_samples)
  {
  return num_samples;
  }

static int bypass_cdaudio(void * priv)
  {
  int j;
  int seconds;
  
  cdaudio_t * cd = (cdaudio_t*)priv;

  if(!bg_cdaudio_get_status(cd->fd, &(cd->status)))
    {
    fprintf(stderr, "bg_cdaudio_get_status returned 0\n");
    return 0;
    }
  if((cd->status.track < cd->current_track) ||
     (cd->status.track > cd->current_track+1))
    {
    fprintf(stderr, "bg_cdaudio_get_status returned bullshit\n");
    cd->status.track = cd->current_track;
    return 1;
    }
  if(cd->status.track == cd->current_track + 1)
    {
    fprintf(stderr, "Track changed, old_track: %d, new_track: %d\n",
            cd->current_track, cd->status.track);
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
    bg_cdaudio_play(cd->fd, sector, last_sector);
    }
  else /* TODO */
    {

    }
  }

void bypass_set_pause_cdaudio(void * priv, int pause)
  {
  cdaudio_t * cd = (cdaudio_t*)priv;
  bg_cdaudio_set_pause(cd->fd, pause);
  }

void bypass_set_volume_cdaudio(void * priv, float volume)
  {
  cdaudio_t * cd = (cdaudio_t*)priv;
  bg_cdaudio_set_volume(cd->fd, volume);
  }

static void close_cdaudio(void * priv)
  {
  cdaudio_t * cd = (cdaudio_t*)priv;
  close(cd->fd);
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
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
      name:        "musicbrainz_server",
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
      name:        "musicbrainz_proxy_server",
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
    {
      name:         "cdparanoia",
      long_name:    "CDparanoia options",
      type:      BG_PARAMETER_SECTION
    },
    {
      name:        "enable_paranoia",
      long_name:   "Enable paranoia",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    { /* End of parmeters */ }
  };

static bg_parameter_info_t * get_parameters_cdaudio(void * data)
  {
  return parameters;
  }

static void set_parameter_cdaudio(void * data, char * name, bg_parameter_value_t * val)
  {
  cdaudio_t * cd = (cdaudio_t*)data;

  if(!name)
    return;

#ifdef HAVE_MUSICBRAINZ
  if(!strcmp(name, "use_musicbrainz"))
    cd->use_musicbrainz = val->val_i;
  if(!strcmp(name, "musicbrainz_server"))
    cd->musicbrainz_host = bg_strdup(cd->musicbrainz_host, val->val_str);
  if(!strcmp(name, "musicbrainz_port"))
    cd->musicbrainz_port = val->val_i;
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
      check_device: bg_cdaudio_check_device

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
    set_subpicture_stream: NULL,

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
API_VERSION;
