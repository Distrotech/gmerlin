#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pluginregistry.h>
#include <tree.h>
#include <transcoder_track.h>
#include <utils.h>

static void set_track(bg_transcoder_track_t * track,
                      bg_track_info_t * track_info,
                      bg_plugin_handle_t * audio_encoder,
                      bg_plugin_handle_t * video_encoder)
  {
  int i;

  bg_encoder_plugin_t * plugin;
  void * priv;
  
  if(track_info->num_audio_streams)
    {
    track->num_audio_streams = track_info->num_audio_streams; 
    track->audio_streams = calloc(track_info->num_audio_streams,
                                  sizeof(*(track->audio_streams)));
    
    if(audio_encoder)
      {
      plugin = (bg_encoder_plugin_t*)(audio_encoder->plugin);
      priv = audio_encoder->priv;
      }
    else
      {
      plugin = (bg_encoder_plugin_t*)(video_encoder->plugin);
      priv = video_encoder->priv;
      }

    for(i = 0; i < track_info->num_audio_streams; i++)
      {
      gavl_audio_format_copy(&(track->audio_streams[i].input_format),
                             &(track_info->audio_streams[i].format));
      gavl_audio_format_copy(&(track->audio_streams[i].output_format),
                             &(track_info->audio_streams[i].format));

      if(plugin->get_audio_parameters)
        track->audio_streams[i].encoder_parameters =
          bg_parameter_info_copy_array(plugin->get_audio_parameters(priv));
      else if(plugin->common.get_parameters)
        track->audio_streams[i].encoder_parameters =
          bg_parameter_info_copy_array(plugin->common.get_parameters(priv));
      }
    }
  
  if(track_info->num_video_streams)
    {
    track->num_video_streams = track_info->num_video_streams; 
    track->video_streams = calloc(track_info->num_video_streams,
                                  sizeof(*(track->video_streams)));

    plugin = (bg_encoder_plugin_t*)(video_encoder->plugin);
    priv = video_encoder->priv;
    
    for(i = 0; i < track_info->num_video_streams; i++)
      {
      gavl_video_format_copy(&(track->video_streams[i].input_format),
                             &(track_info->video_streams[i].format));
      gavl_video_format_copy(&(track->video_streams[i].output_format),
                             &(track_info->video_streams[i].format));

      if(plugin->get_video_parameters)
        track->video_streams[i].encoder_parameters =
          bg_parameter_info_copy_array(plugin->get_video_parameters(priv));
      else if(plugin->common.get_parameters)
        track->video_streams[i].encoder_parameters =
          bg_parameter_info_copy_array(plugin->common.get_parameters(priv));
      }
    }
  track->duration = track_info->duration;
  bg_metadata_copy(&(track->metadata), &(track_info->metadata));
  track->name = bg_strdup(track->name, track_info->name);
  //  fprintf(stderr, "Track name: %s\n", track->name);
  }

static void enable_streams(bg_input_plugin_t * plugin, void * priv, 
                           int track,
                           int num_audio_streams, int num_video_streams)
  {
  int i;

  if(plugin->set_track)
    plugin->set_track(priv, track);

  if(plugin->set_audio_stream)
    {
    for(i = 0; i < num_audio_streams; i++)
      {
      plugin->set_audio_stream(priv, i, BG_STREAM_ACTION_DECODE);
      }
    }

  if(plugin->set_video_stream)
    {
    for(i = 0; i < num_video_streams; i++)
      {
      plugin->set_video_stream(priv, i, BG_STREAM_ACTION_DECODE);
      }
    }

  if(plugin->start)
    plugin->start(priv);

  }

bg_transcoder_track_t *
bg_transcoder_track_create(const char * url,
                           const bg_plugin_info_t * plugin_info,
                           int track, bg_plugin_registry_t * plugin_reg,
                           bg_cfg_section_t * section,
                           bg_plugin_handle_t * audio_encoder,
                           bg_plugin_handle_t * video_encoder)
  {
  int i;
  bg_transcoder_track_t * new_track = (bg_transcoder_track_t *)0;
  bg_transcoder_track_t * end_track;
  
  bg_input_plugin_t      * input;
  bg_track_info_t        * track_info;
  bg_plugin_handle_t     * plugin_handle = (bg_plugin_handle_t*)0;
  int num_tracks;

  /* Load the plugin */
  
  if(!bg_input_plugin_load(plugin_reg, url,
                           plugin_info, &plugin_handle))
    {
    return (bg_transcoder_track_t*)0;
    }

  input = (bg_input_plugin_t*)(plugin_handle->plugin);
  
  /* Decide what to load */
  
  if(track >= 0)
    {
    /* Load single track */
    track_info = input->get_track_info(plugin_handle->priv, track);
    new_track = calloc(1, sizeof(*new_track));
    
    enable_streams(input, plugin_handle->priv, 
                   track,
                   track_info->num_audio_streams,
                   track_info->num_video_streams);

    set_track(new_track, track_info, audio_encoder, video_encoder);
    }
  else
    {
    /* Load all tracks */

    num_tracks = input->get_num_tracks ? 
      input->get_num_tracks(plugin_handle->priv) : 1;

    for(i = 0; i < num_tracks; i++)
      {
      track_info = input->get_track_info(plugin_handle->priv, i);

      if(new_track)
        {
        end_track->next = calloc(1, sizeof(*new_track));
        end_track = end_track->next;
        }
      else
        {
        new_track = calloc(1, sizeof(*new_track));
        end_track = new_track;
        }
      
      enable_streams(input, plugin_handle->priv, 
                     i,
                     track_info->num_audio_streams,
                     track_info->num_video_streams);
      
      set_track(end_track, track_info, audio_encoder, video_encoder);
      }
    }
  bg_plugin_unref(plugin_handle);
  return new_track;
  }


bg_transcoder_track_t *
bg_transcoder_track_create_from_urilist(const char * list,
                                        int len,
                                        bg_plugin_registry_t * plugin_reg,
                                        bg_cfg_section_t * section,
                                        bg_plugin_handle_t * audio_encoder,
                                        bg_plugin_handle_t * video_encoder)
  {
  int i;
  char ** uri_list;
  bg_transcoder_track_t * ret_last;
  bg_transcoder_track_t * ret = (bg_transcoder_track_t*)0;
  
  uri_list = bg_urilist_decode(list, len);

  if(!uri_list)
    return (bg_transcoder_track_t*)0;

  i = 0;

  while(uri_list[i])
    {
    if(!ret)
      {
      ret = bg_transcoder_track_create(uri_list[i],
                                       (const bg_plugin_info_t*)0,
                                       -1,
                                       plugin_reg,
                                       section,
                                       audio_encoder,
                                       video_encoder);
      if(ret)
        {
        ret_last = ret;
        while(ret_last->next)
          ret_last = ret_last->next;
        }
      }
    else
      {
      ret_last->next = bg_transcoder_track_create(uri_list[i],
                                                  (const bg_plugin_info_t*)0,
                                                  -1,
                                                  plugin_reg,
                                                  section,
                                                  audio_encoder,
                                                  video_encoder);
      if(ret)
        {
        while(ret_last->next)
          ret_last = ret_last->next;
        }
      }
    i++;
    }
  bg_urilist_free(uri_list);
  return ret;
  }

bg_transcoder_track_t *
bg_transcoder_track_create_from_albumentries(const char * xml_string,
                                             int len,
                                             bg_plugin_registry_t * plugin_reg,
                                             bg_cfg_section_t * section,
                                             bg_plugin_handle_t * audio_encoder,
                                             bg_plugin_handle_t * video_encoder)
  {
  bg_album_entry_t * new_entries, *entry;
  bg_transcoder_track_t * ret_last;
  bg_transcoder_track_t * ret = (bg_transcoder_track_t*)0;
  const bg_plugin_info_t * plugin_info;

  new_entries = bg_album_entries_new_from_xml(xml_string, len);

  entry = new_entries;

  while(entry)
    {
    if(entry->plugin)
      plugin_info = bg_plugin_find_by_name(plugin_reg, entry->plugin);
    else
      plugin_info = (const bg_plugin_info_t*)0;
    if(!ret)
      {
        
      //      fprintf(stderr, "bg_transcoder_track_create %s %s %d\n",
      //              entry->location, entry->plugin, entry->index);
      ret = bg_transcoder_track_create(entry->location,
                                       plugin_info,
                                       entry->index,
                                       plugin_reg,
                                       section, audio_encoder, video_encoder);
      ret_last = ret;
      }
    else
      {
      ret_last->next = bg_transcoder_track_create(entry->location,
                                                  plugin_info,
                                                  entry->index,
                                                  plugin_reg,
                                                  section,
                                                  audio_encoder, video_encoder);
      ret_last = ret_last->next;
      }
    ret_last->name = bg_strdup(ret_last->name, entry->name);
    entry = entry->next;
    }
  bg_album_entries_destroy(new_entries);
  
  return ret;
  }



void bg_transcoder_track_destroy(bg_transcoder_track_t * t)
  {
  
  }

static bg_parameter_info_t parameters_general[] =
  {
    {
      name:      "name",
      long_name: "Name",
      type:      BG_PARAMETER_STRING,
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t parameters_video[] =
  {
    {
      name:      "fixed_framerate",
      long_name: "Fixed framerate",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:      "timescale",
      long_name: "Timescale",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 25 }
    },
    {
      name:      "frame_duration",
      long_name: "Frame duration",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 1 }
    },
  };

static bg_parameter_info_t parameters_audio[] =
  {
    {
      name:      "fixed_samplerate",
      long_name: "Fixed samplerate",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:        "samplerate",
      long_name:   "Samplerate",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 8000 },
      val_max:     { val_i: 192000 },
      val_default: { val_i: 44100 },
    },
    {
      name:      "fixed_channel_setup",
      long_name: "Fixed channel setup",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:        "channel_setup",
      long_name:   "Channel setup",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Stereo" },
      options:     (char*[]){ "Mono",
                              "Stereo",
                              "3 Front",
                              "2 Front 1 Rear",
                              "3 Front 1 Rear",
                              "2 Front 2 Rear",
                              "3 Front 2 Rear",
                              (char*)0 },
    },
    {
      name:        "front_to_rear",
      long_name:   "Front to rear mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Copy" },
      options:     (char*[]){ "Mute",
                              "Copy",
                              "Diff",
                              (char*)0 },
    },
    {
      name:        "stereo_to_mono",
      long_name:   "Stereo to mono mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Mix" },
      options:     (char*[]){ "Choose left",
                              "Choose right",
                              "Mix",
                              (char*)0 },
    },
  };

bg_parameter_info_t *
bg_transcoder_track_get_parameters_general(bg_transcoder_track_t * t)
  {
  int i;
  bg_parameter_info_t * ret;

  ret = bg_parameter_info_copy_array(parameters_general);

  i = 0;
  while(ret[i].name)
    {
    if(!strcmp(ret[i].name, "name"))
      ret[i].val_default.val_str = bg_strdup(ret[i].val_default.val_str,
                                             t->name); 
    i++;
    }
  return ret;
  }

/* Audio stream parameters */

bg_parameter_info_t *
bg_transcoder_audio_stream_get_parameters(bg_transcoder_audio_stream_t * t)
  {
  return parameters_audio;
  }

void
bg_transcoder_audio_stream_set_format_parameter(void * data, char * name,
                                                bg_parameter_value_t * val)
  {
  
  }

void
bg_transcoder_audio_stream_set_encoder_parameter(void * data, char * name,
                                                 bg_parameter_value_t * val)
  {
  
  }


/* Video stream parameters */

bg_parameter_info_t *
bg_transcoder_video_stream_get_parameters(bg_transcoder_video_stream_t * t)
  {
  return parameters_video;
  }


void
bg_transcoder_video_stream_set_format_parameter(void * data, char * name,
                                                bg_parameter_value_t * val)
  {
  
  }

void
bg_transcoder_video_stream_set_encoder_parameter(void * data, char * name,
                                                 bg_parameter_value_t * val)
  {
  
  }

/* General parameters */

void bg_transcoder_track_set_parameter_general(void * data, char * name,
                                               bg_parameter_value_t * val)
  {
  
  }

