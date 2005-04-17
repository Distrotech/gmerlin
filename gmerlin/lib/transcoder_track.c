/*****************************************************************
  
  transcoder_track.c
  
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
  
  http://gmerlin.sourceforge.net
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
  
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pluginregistry.h>
#include <tree.h>
#include <transcoder_track.h>
#include <utils.h>

static void create_encoder_sections(bg_transcoder_track_t * t,
                                    bg_cfg_section_t * audio_encoder_section,
                                    bg_cfg_section_t * video_encoder_section,
                                    bg_plugin_handle_t * audio_encoder,
                                    bg_plugin_handle_t * video_encoder)
  {
  int i;

  bg_cfg_section_t * encoder_section;

  char * encoder_label;
  
  if(t->audio_encoder_parameters)
    {
    encoder_label = bg_sprintf("%s options", audio_encoder->info->long_name);
    t->audio_encoder_section = bg_cfg_section_copy(audio_encoder_section);
    bg_cfg_section_set_name(t->audio_encoder_section, encoder_label);

    if(bg_cfg_section_has_subsection(t->audio_encoder_section, "$audio"))
      {
      bg_cfg_section_delete_subsection(t->audio_encoder_section,
                                       bg_cfg_section_find_subsection(t->audio_encoder_section, "$audio"));
      }
    
    free(encoder_label);
    }

  if(t->video_encoder_parameters)
    {
    encoder_label = bg_sprintf("%s options", video_encoder->info->long_name);
    t->video_encoder_section = bg_cfg_section_copy(video_encoder_section);
    bg_cfg_section_set_name(t->video_encoder_section, encoder_label);
        
    if(bg_cfg_section_has_subsection(t->video_encoder_section, "$audio"))
      {
      bg_cfg_section_delete_subsection(t->video_encoder_section,
                                       bg_cfg_section_find_subsection(t->video_encoder_section,
                                                                      "$audio"));
      }

    if(bg_cfg_section_has_subsection(t->video_encoder_section, "$video"))
      {
      bg_cfg_section_delete_subsection(t->video_encoder_section,
                                       bg_cfg_section_find_subsection(t->video_encoder_section,
                                                                      "$video"));
      }
    
    free(encoder_label);
    }
  
  if(t->num_audio_streams)
    {
    if(bg_cfg_section_has_subsection(audio_encoder_section, "$audio"))
      {
      encoder_section = bg_cfg_section_find_subsection(audio_encoder_section, "$audio");
      //      encoder_label;
      }
    else
      {
      encoder_section = (bg_cfg_section_t*)0;
      }
    
    for(i = 0; i < t->num_audio_streams; i++)
      {
      if(encoder_section)
        {
        encoder_label = bg_sprintf("%s audio options",
                                   audio_encoder ? audio_encoder->info->long_name :
                                   video_encoder->info->long_name);
        
        t->audio_streams[i].encoder_section =
          bg_cfg_section_copy(encoder_section);
        bg_cfg_section_set_name(t->audio_streams[i].encoder_section, encoder_label);
        free(encoder_label);
        }
      }
    
    }

  if(t->num_video_streams)
    {
    if(bg_cfg_section_has_subsection(video_encoder_section, "$video"))
      {
      encoder_section =
        bg_cfg_section_find_subsection(video_encoder_section, "$video");
      }
    else
      encoder_section = (bg_cfg_section_t*)0;
    
    for(i = 0; i < t->num_video_streams; i++)
      {
      if(encoder_section)
        {
        encoder_label = bg_sprintf("%s video options", video_encoder->info->long_name);
        
        t->video_streams[i].encoder_section =
          bg_cfg_section_copy(encoder_section);
        bg_cfg_section_set_name(t->video_streams[i].encoder_section, encoder_label);
        free(encoder_label);
        }
      }
    }
  
  }


static void create_sections(bg_transcoder_track_t * t,
                            bg_cfg_section_t * track_defaults_section,
                            bg_cfg_section_t * audio_encoder_section,
                            bg_cfg_section_t * video_encoder_section,
                            bg_plugin_handle_t * audio_encoder,
                            bg_plugin_handle_t * video_encoder)
  {
  int i;

  bg_cfg_section_t * general_section;

  
  t->general_section =
    bg_cfg_section_create_from_parameters("General", t->general_parameters);

  /* Stop here for redirectors */

  if(t->url)
    return;
  
  t->metadata_section =
    bg_cfg_section_create_from_parameters("Metadata", t->metadata_parameters);
    
  if(t->num_audio_streams)
    {
    general_section =
      bg_cfg_section_find_subsection(track_defaults_section, "audio");
    
    for(i = 0; i < t->num_audio_streams; i++)
      {
      t->audio_streams[i].general_section = bg_cfg_section_copy(general_section);
      }
    
    }

  if(t->num_video_streams)
    {
    general_section = bg_cfg_section_find_subsection(track_defaults_section,
                                                     "video");
    
    for(i = 0; i < t->num_video_streams; i++)
      {
      t->video_streams[i].general_section = bg_cfg_section_copy(general_section);
      }
    }
  
  create_encoder_sections(t, audio_encoder_section,
                          video_encoder_section, audio_encoder,
                          video_encoder);

  }

static bg_parameter_info_t parameters_general[] =
  {
    {
      name:      "name",
      long_name: "Name",
      type:      BG_PARAMETER_STRING,
    },
    {
      name:      "location",
      long_name: "Location",
      type:      BG_PARAMETER_STRING,
      flags:     BG_PARAMETER_HIDE_DIALOG,
    },
    {
      name:      "plugin",
      long_name: "Plugin",
      type:      BG_PARAMETER_STRING,
      flags:     BG_PARAMETER_HIDE_DIALOG,
    },
    {
      name:      "track",
      long_name: "Track",
      type:      BG_PARAMETER_INT,
      flags:     BG_PARAMETER_HIDE_DIALOG,
    },
    {
      name:      "duration",
      long_name: "Duration",
      type:      BG_PARAMETER_TIME,
      flags:     BG_PARAMETER_HIDE_DIALOG,
    },
    {
      name:      "seekable",
      long_name: "Seekable",
      type:      BG_PARAMETER_CHECKBUTTON,
      flags:     BG_PARAMETER_HIDE_DIALOG,
    },
    {
      name:      "audio_encoder",
      long_name: "Audio encoder",
      type:      BG_PARAMETER_STRING,
      flags:     BG_PARAMETER_HIDE_DIALOG,
    },
    {
      name:      "video_encoder",
      long_name: "Video encoder",
      type:      BG_PARAMETER_STRING,
      flags:     BG_PARAMETER_HIDE_DIALOG,
    },
    {
      name:      "set_start_time",
      long_name: "Set start time",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 }
    },
    {
      name:      "start_time",
      long_name: "Start time",
      type:      BG_PARAMETER_TIME,
      val_default: { val_time: 0 }
    },
    {
      name:      "set_end_time",
      long_name: "Set end time",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 }
    },
    {
      name:      "end_time",
      long_name: "End time",
      type:      BG_PARAMETER_TIME,
      val_default: { val_time: 0 }
    },
    { /* End of parameters */ }
  };

static void
create_encoder_parameters(bg_transcoder_track_t * track,
                          bg_plugin_handle_t * audio_encoder,
                          bg_plugin_handle_t * video_encoder)
  {
  int i;
  bg_encoder_plugin_t * plugin;
  void * priv;
  
  /* Audio streams */
  
  if(track->num_audio_streams)
    {
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
    
    for(i = 0; i < track->num_audio_streams; i++)
      {
      if(plugin->get_audio_parameters)
        {
        track->audio_streams[i].encoder_parameters =
          bg_parameter_info_copy_array(plugin->get_audio_parameters(priv));
        }
      }

    if(plugin->common.get_parameters)
      {
      if(audio_encoder)
        {
        track->audio_encoder_parameters =
          bg_parameter_info_copy_array(plugin->common.get_parameters(priv));
        }
      else
        {
        track->video_encoder_parameters =
          bg_parameter_info_copy_array(plugin->common.get_parameters(priv));
        }

      }
    
    }
  
  /* Video streams */
  
  if(track->num_video_streams)
    {
    plugin = (bg_encoder_plugin_t*)(video_encoder->plugin);
    priv = video_encoder->priv;
    
    for(i = 0; i < track->num_video_streams; i++)
      {
      if(plugin->get_video_parameters)
        {
        track->video_streams[i].encoder_parameters =
          bg_parameter_info_copy_array(plugin->get_video_parameters(priv));
        }
      }

    if(!track->video_encoder_parameters && plugin->common.get_parameters)
      {
      track->video_encoder_parameters =
        bg_parameter_info_copy_array(plugin->common.get_parameters(priv));
      }
    }
  
  }

void
bg_transcoder_track_create_parameters(bg_transcoder_track_t * track,
                                      bg_plugin_handle_t * audio_encoder,
                                      bg_plugin_handle_t * video_encoder)
  {
  gavl_time_t duration = GAVL_TIME_UNDEFINED;
  int i;
  int seekable = 0;
  
  track->general_parameters = bg_parameter_info_copy_array(parameters_general);

  bg_cfg_section_get_parameter_time(track->general_section,
                                    "duration", &duration);
  bg_cfg_section_get_parameter_int(track->general_section,
                                   "seekable", &seekable);

  if((duration != GAVL_TIME_UNDEFINED) && seekable)
    {
    i = 0;

    while(track->general_parameters[i].name)
      {
      if(!strcmp(track->general_parameters[i].name, "start_time") ||
         !strcmp(track->general_parameters[i].name, "end_time"))
        track->general_parameters[i].val_max.val_time = duration;
      i++;
      }
    }
  else
    {
    i = 0;

    while(track->general_parameters[i].name)
      {
      if(!strcmp(track->general_parameters[i].name, "start_time") ||
         !strcmp(track->general_parameters[i].name, "set_start_time"))
        track->general_parameters[i].flags |= BG_PARAMETER_HIDE_DIALOG;
      i++;
      }
    }
  
  track->metadata_parameters = bg_metadata_get_parameters((bg_metadata_t*)0);

  create_encoder_parameters(track, audio_encoder, video_encoder);
    
#if 0
  fprintf(stderr, "Encoder parameters: %p %p %p %p\n",
          audio_encoder,
          video_encoder,
          track->audio_encoder_parameters,
          track->video_encoder_parameters);
#endif
  }

static void set_track(bg_transcoder_track_t * track,
                      bg_track_info_t * track_info,
                      bg_plugin_handle_t * input_plugin,
                      const char * location,
                      int track_index,
                      bg_plugin_handle_t * audio_encoder,
                      bg_plugin_handle_t * video_encoder)
  {
  int i;

  bg_encoder_plugin_t * plugin;
  void * priv;

  //  fprintf(stderr, "set_track, Encoders: %p %p\n",
  //          audio_encoder, video_encoder);

  /* General parameters */

  track->general_parameters = bg_parameter_info_copy_array(parameters_general);

  i = 0;
  while(track->general_parameters[i].name)
    {
    if(!strcmp(track->general_parameters[i].name, "name"))
      {
      if(track_info->name)
        track->general_parameters[i].val_default.val_str = bg_strdup((char*)0,
                                                                     track_info->name);
      else
        track->general_parameters[i].val_default.val_str = bg_get_track_name_default(location);
      }
    else if(!strcmp(track->general_parameters[i].name, "audio_encoder"))
      {
      if(audio_encoder)
        track->general_parameters[i].val_default.val_str = bg_strdup((char*)0,
                                                                     audio_encoder->info->name);
      else
        track->general_parameters[i].val_default.val_str = bg_strdup((char*)0,
                                                                     video_encoder->info->name);
      }

    else if(!strcmp(track->general_parameters[i].name, "video_encoder"))
      track->general_parameters[i].val_default.val_str = bg_strdup((char*)0,
                                                                   video_encoder->info->name);
    
    else if(!strcmp(track->general_parameters[i].name, "duration"))
      track->general_parameters[i].val_default.val_time = track_info->duration;
    else if(!strcmp(track->general_parameters[i].name, "seekable"))
      track->general_parameters[i].val_default.val_i = track_info->seekable;
    else if(!strcmp(track->general_parameters[i].name, "location"))
      track->general_parameters[i].val_default.val_str = bg_strdup((char*)0, location);

    else if(!strcmp(track->general_parameters[i].name, "plugin"))
      track->general_parameters[i].val_default.val_str = bg_strdup((char*)0, input_plugin->info->name);

    else if(!strcmp(track->general_parameters[i].name, "track"))
      track->general_parameters[i].val_default.val_i = track_index;

    else if(!strcmp(track->general_parameters[i].name, "set_start_time"))
      {
      if(!track_info->seekable)
        {
        track->general_parameters[i].flags |= BG_PARAMETER_HIDE_DIALOG;
        }
      }
    else if(!strcmp(track->general_parameters[i].name, "start_time"))
      {
      if(!track_info->seekable)
        {
        track->general_parameters[i].flags |= BG_PARAMETER_HIDE_DIALOG;
        }
      else
        {
        track->general_parameters[i].val_max.val_time = track_info->duration;
        }
      }
    else if(!strcmp(track->general_parameters[i].name, "end_time"))
      {
      if(track_info->duration != GAVL_TIME_UNDEFINED)
        {
        track->general_parameters[i].val_max.val_time = track_info->duration;
        track->general_parameters[i].val_default.val_time = track_info->duration;
        }
      }
    i++;
    }

  /* Stop here for redirectors */

  if(track->url)
    return;
  
  /* Metadata */
    
  track->metadata_parameters = bg_metadata_get_parameters(&(track_info->metadata));

  /* Audio streams */
  
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

    if(plugin->common.get_parameters)
      {
      if(audio_encoder)
        track->audio_encoder_parameters =
          bg_parameter_info_copy_array(plugin->common.get_parameters(priv));
      else
        track->video_encoder_parameters =
          bg_parameter_info_copy_array(plugin->common.get_parameters(priv));
      }
    
    for(i = 0; i < track_info->num_audio_streams; i++)
      {
      if(plugin->get_audio_parameters)
        {
        track->audio_streams[i].encoder_parameters =
          bg_parameter_info_copy_array(plugin->get_audio_parameters(priv));

        if(audio_encoder && !track->audio_encoder_parameters &&
           plugin->common.get_parameters)
          {
          track->audio_encoder_parameters =
            bg_parameter_info_copy_array(plugin->common.get_parameters(priv));
          }
        }
      }
    }

  plugin = (bg_encoder_plugin_t*)(video_encoder->plugin);
  priv = video_encoder->priv;
  
  /* Video streams */
  
  if(track_info->num_video_streams)
    {
    if(plugin->common.get_parameters && !track->video_encoder_parameters)
      track->video_encoder_parameters =
        bg_parameter_info_copy_array(plugin->common.get_parameters(priv));

    track->num_video_streams = track_info->num_video_streams; 
    track->video_streams = calloc(track_info->num_video_streams,
                                  sizeof(*(track->video_streams)));
    
    for(i = 0; i < track_info->num_video_streams; i++)
      {
      if(plugin->get_video_parameters)
        {
        track->video_streams[i].encoder_parameters =
          bg_parameter_info_copy_array(plugin->get_video_parameters(priv));

        if(!track->video_encoder_parameters &&
           plugin->common.get_parameters)
          {
          track->video_encoder_parameters =
            bg_parameter_info_copy_array(plugin->common.get_parameters(priv));
          }

        }
      }
    }
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

static void disable_streams(bg_input_plugin_t * plugin, void * priv)
  {
  if(plugin->stop)
    plugin->stop(priv);
  }

bg_transcoder_track_t *
bg_transcoder_track_create(const char * url,
                           const bg_plugin_info_t * input_info,
                           int track, bg_plugin_registry_t * plugin_reg,
                           bg_cfg_section_t * track_defaults_section,
                           char * name)
  {
  char * error_msg = (char*)0;
  int i;

  bg_transcoder_track_t * ret = (bg_transcoder_track_t *)0;
  bg_transcoder_track_t * new_track = (bg_transcoder_track_t *)0;
  bg_transcoder_track_t * end_track = (bg_transcoder_track_t *)0;
  
  bg_input_plugin_t      * input;
  bg_track_info_t        * track_info;
  bg_plugin_handle_t     * plugin_handle = (bg_plugin_handle_t*)0;
  int num_tracks;
  int streams_enabled = 0;
    
  bg_plugin_handle_t * audio_encoder;
  bg_plugin_handle_t * video_encoder;

  bg_cfg_section_t * audio_encoder_section;
  bg_cfg_section_t * video_encoder_section;
  
  const bg_plugin_info_t * encoder_info;
    
  /* Load video encoder */
  
  encoder_info = bg_plugin_registry_get_default(plugin_reg,
                                                BG_PLUGIN_ENCODER | BG_PLUGIN_ENCODER_VIDEO);

  if(encoder_info)
    video_encoder = bg_plugin_load(plugin_reg, encoder_info);
  else
    {
    fprintf(stderr, "No video encoder found, check installation\n");
    return (bg_transcoder_track_t*)0;
    }

  /* Load audio encoder */
  
  if((encoder_info->type == BG_PLUGIN_ENCODER_VIDEO) ||
     !bg_plugin_registry_get_encode_audio_to_video(plugin_reg))
    {
    encoder_info = bg_plugin_registry_get_default(plugin_reg,
                                                  BG_PLUGIN_ENCODER_AUDIO);

    if(encoder_info)
      audio_encoder = bg_plugin_load(plugin_reg, encoder_info);
    else
      {
      fprintf(stderr, "No audio encoder found, check installation\n");
      return (bg_transcoder_track_t*)0;
      }
    }
  else
    {
    audio_encoder = (bg_plugin_handle_t*)0;
    }

  video_encoder_section =
    bg_plugin_registry_get_section(plugin_reg,
                                   video_encoder->info->name);

  if(audio_encoder)
    audio_encoder_section =
      bg_plugin_registry_get_section(plugin_reg,
                                     audio_encoder->info->name);
  else
    audio_encoder_section = video_encoder_section;
  
  /* Load the plugin */
  
  if(!bg_input_plugin_load(plugin_reg, url,
                           input_info, &plugin_handle, &error_msg))
    {
    fprintf(stderr, __FILE__": Loading %s failed: %s\n", url, error_msg);
    free(error_msg);
    return (bg_transcoder_track_t*)0;
    }

  input = (bg_input_plugin_t*)(plugin_handle->plugin);
  
  /* Decide what to load */
  
  if(track >= 0)
    {
    /* Load single track */
    track_info = input->get_track_info(plugin_handle->priv, track);

    if(name)
      track_info->name = bg_strdup(track_info->name, name);
        
    new_track = calloc(1, sizeof(*new_track));
    ret = new_track;
    
    if(track_info->url)
      {
      new_track->url = bg_strdup(new_track->url, track_info->url);
      //      fprintf(stderr, "Track: %d, URL: %s\n", track, new_track->url);
      }
    else
      {
      enable_streams(input, plugin_handle->priv, 
                     track,
                     track_info->num_audio_streams,
                     track_info->num_video_streams);
      streams_enabled = 1;
      }
    
    set_track(new_track, track_info, plugin_handle, url, track,
              audio_encoder, video_encoder);
    create_sections(new_track, track_defaults_section,
                    audio_encoder_section, video_encoder_section,
                    audio_encoder, video_encoder);
    if(streams_enabled)
      disable_streams(input, plugin_handle->priv);
    }
  else
    {
    /* Load all tracks */

    num_tracks = input->get_num_tracks ? 
      input->get_num_tracks(plugin_handle->priv) : 1;

    for(i = 0; i < num_tracks; i++)
      {
      track_info = input->get_track_info(plugin_handle->priv, i);

      if(name)
        track_info->name = bg_strdup(track_info->name, name);
      
      new_track = calloc(1, sizeof(*new_track));
      
      if(ret)
        {
        end_track->next = new_track;
        end_track = end_track->next;
        }
      else
        {
        ret = new_track;
        end_track = new_track;
        }

      if(track_info->url)
        {
        new_track->url = bg_strdup(new_track->url, track_info->url);
        //        fprintf(stderr, "Track: %d, URL: %s\n", i, new_track->url);
        }
      else
        {
        enable_streams(input, plugin_handle->priv, 
                       i,
                       track_info->num_audio_streams,
                       track_info->num_video_streams);
        streams_enabled = 1;
        }
      
      set_track(new_track, track_info, plugin_handle, url, i,
                audio_encoder, video_encoder);
      create_sections(new_track, track_defaults_section,
                      audio_encoder_section, video_encoder_section,
                      audio_encoder, video_encoder);
      if(streams_enabled)
        {
        disable_streams(input, plugin_handle->priv);
        streams_enabled = 0;
        }
      }
    }
  bg_plugin_unref(plugin_handle);

  bg_plugin_unref(video_encoder);

  if(audio_encoder)
    bg_plugin_unref(audio_encoder);
  
  return ret;
  }

static bg_transcoder_track_t * remove_redirectors(bg_transcoder_track_t * entries,
                                                  bg_plugin_registry_t * plugin_reg,
                                                  bg_cfg_section_t * track_defaults_section)
  {
  bg_transcoder_track_t * before, * e;
  //  bg_album_entry_t * ret_end = (bg_album_entry_t*)0;
  bg_transcoder_track_t * new_entry, * end_entry;
  int done = 0;
  const char * plugin_name = (const char*)0;
  const bg_plugin_info_t * info;
  
  done = 1;
  e = entries;

  fprintf(stderr, "Remove redirectors\n");
  
  while(e)
    {
    if(e->url)
      {
      bg_cfg_section_get_parameter_string(e->general_section, "plugin", &plugin_name);
      
      if(plugin_name)
        info = bg_plugin_find_by_name(plugin_reg, plugin_name);
      else
        info = (const bg_plugin_info_t*)0;

      /* Load "real" url */
      
      new_entry = bg_transcoder_track_create(e->url,
                                             info,
                                             -1, plugin_reg,
                                             track_defaults_section, (char*)0);
      
      if(new_entry)
        {
        /* Insert new entries into list */
        if(e != entries)
          {
          before = entries;
          while(before->next != e)
            before = before->next;
          before->next = new_entry;
          }
        else
          {
          entries = new_entry;
          }
      
        end_entry = new_entry;
        while(end_entry->next)
          end_entry = end_entry->next;
        end_entry->next = e->next;
      
        bg_transcoder_track_destroy(e);
        e = new_entry;
        }
      else
        {
        /* Remove e from list */
        if(e != entries)
          {
          before = entries;
          while(before->next != e)
            before = before->next;
          before->next = e->next;
          }
        else
          {
          entries = e->next;
          before = (bg_transcoder_track_t*)0;
          }
        bg_transcoder_track_destroy(e);
        e = (before) ? before->next : entries;
        }
      }
    else
      {
      /* Leave e as it is */
      e = e->next;
      }
    }
  return entries;
  }

bg_transcoder_track_t *
bg_transcoder_track_create_from_urilist(const char * list,
                                        int len,
                                        bg_plugin_registry_t * plugin_reg,
                                        bg_cfg_section_t * track_defaults_section)
  {
  int i;
  char ** uri_list;
  bg_transcoder_track_t * ret_last = (bg_transcoder_track_t*)0;
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
                                       track_defaults_section, (char*)0);
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
                                                  track_defaults_section, (char*)0);
      if(ret)
        {
        while(ret_last->next)
          ret_last = ret_last->next;
        }
      }
    i++;
    }
  bg_urilist_free(uri_list);

  ret = remove_redirectors(ret,
                           plugin_reg,
                           track_defaults_section);
  

  return ret;
  }

bg_transcoder_track_t *
bg_transcoder_track_create_from_albumentries(const char * xml_string,
                                             int len,
                                             bg_plugin_registry_t * plugin_reg,
                                             bg_cfg_section_t * track_defaults_section)
  {
  bg_album_entry_t * new_entries, *entry;
  bg_transcoder_track_t * ret_last = (bg_transcoder_track_t*)0;
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
                                       track_defaults_section, entry->name);
      ret_last = ret;
      }
    else
      {
      ret_last->next = bg_transcoder_track_create(entry->location,
                                                  plugin_info,
                                                  entry->index,
                                                  plugin_reg,
                                                  track_defaults_section,
                                                  entry->name);
      ret_last = ret_last->next;
      }
    entry = entry->next;
    }
  bg_album_entries_destroy(new_entries);

  ret = remove_redirectors(ret,
                           plugin_reg,
                           track_defaults_section);
  
  return ret;
  }

static void free_encoders(bg_transcoder_track_t * track)
  {
  int i;
  /* Free all encoder related data */

  if(track->audio_encoder_parameters)
    {
    bg_parameter_info_destroy_array(track->audio_encoder_parameters);
    track->audio_encoder_parameters = NULL;
    }
  
  if(track->video_encoder_parameters)
    {
    bg_parameter_info_destroy_array(track->video_encoder_parameters);
    track->video_encoder_parameters = NULL;
    }
  
  if(track->audio_encoder_section)
    {
    bg_cfg_section_destroy(track->audio_encoder_section);
    track->audio_encoder_section = NULL;
    }
  
  if(track->video_encoder_section)
    {
    bg_cfg_section_destroy(track->video_encoder_section);
    track->video_encoder_section = NULL;
    }
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    if(track->audio_streams[i].encoder_parameters)
      {
      bg_parameter_info_destroy_array(track->audio_streams[i].encoder_parameters);
      track->audio_streams[i].encoder_parameters = NULL;
      }
    if(track->audio_streams[i].encoder_section)
      {
      bg_cfg_section_destroy(track->audio_streams[i].encoder_section);
      track->audio_streams[i].encoder_section = NULL;
      }
    }
  
  for(i = 0; i < track->num_video_streams; i++)
    {
    if(track->video_streams[i].encoder_parameters)
      {
      bg_parameter_info_destroy_array(track->video_streams[i].encoder_parameters);
      track->video_streams[i].encoder_parameters = NULL;
      }
    if(track->video_streams[i].encoder_section)
      {
      bg_cfg_section_destroy(track->video_streams[i].encoder_section);
      track->video_streams[i].encoder_section = NULL;
      }
    }

  }
                          

void bg_transcoder_track_destroy(bg_transcoder_track_t * t)
  {
  int i;

  free_encoders(t);
  
  /* Shredder everything */

  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->audio_streams[i].general_section)
      bg_cfg_section_destroy(t->audio_streams[i].general_section);
    }  
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].general_section)
      bg_cfg_section_destroy(t->video_streams[i].general_section);
    }

  if(t->audio_streams)
    free(t->audio_streams);
  if(t->video_streams)
    free(t->video_streams);

  if(t->general_section)
    bg_cfg_section_destroy(t->general_section);
  if(t->metadata_section)
    bg_cfg_section_destroy(t->metadata_section);

  if(t->general_parameters)
    bg_parameter_info_destroy_array(t->general_parameters);
  if(t->metadata_parameters)
    bg_parameter_info_destroy_array(t->metadata_parameters);

  if(t->url)
    free(t->url);
  free(t);
  }

static bg_parameter_info_t general_parameters_video[] =
  {
    {
      name:        "action",
      long_name:   "Action",
      type:        BG_PARAMETER_STRINGLIST,
      multi_names: (char*[]){ "transcode", "forget", (char*)0 },
      multi_labels:  (char*[]){ "Transcode", "Forget", (char*)0 },
      val_default: { val_str: "transcode" },
    },
    {
      name:        "conversion_quality",
      long_name:   "Conversion Quality",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: GAVL_QUALITY_FASTEST },
      val_max:     { val_i: GAVL_QUALITY_BEST    },
      val_default: { val_i: GAVL_QUALITY_DEFAULT }
    },
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
    {
      name:      "crop_left",
      long_name: "Crop left",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 0 },
      help_string: "Cut this many pixels from the left border"
    },
    {
      name:      "crop_right",
      long_name: "Crop right",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 0 },
      help_string: "Cut this many pixels from the right border"
    },
    {
      name:      "crop_top",
      long_name: "Crop top",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 0 },
      help_string: "Cut this many pixels from the top border"
    },
    {
      name:      "crop_bottom",
      long_name: "Crop bottom",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 0 },
      help_string: "Cut this many pixels from the bottom border"
    },
    {
      name:        "frame_size",
      long_name:   "Frame Size",
      type:        BG_PARAMETER_STRINGLIST,
      multi_names: (char*[]){ "from_input",
                              "user_defined",
                              "dvd_pal_d1",
                              "dvd_pal",
                              "dvd_ntsc_d1",
                              "dvd_ntsc",
                              "vcd_pal",
                              "vcd_ntsc",
                              (char*)0 },
      
      multi_labels:  (char*[]){ "From Input",
                                "User defined",
                                "DVD PAL D1 (720 x 576)",
                                "DVD PAL (704 x 576)",
                                "DVD NTSC D1 (720 x 480)",
                                "DVD NTSC (704 x 480)",
                                "VCD PAL (352 x 288)",
                                "VCD NTSC (352 x 240)",
                                (char*)0 },
      val_default: { val_str: "from_input" },
      help_string: "Set the output frame size. For a user defined size, enter the width and height below",
    },
    {
      name:      "user_width",
      long_name: "User defined width",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 640 },
      help_string: "User defined width in pixels. Only meaningful if you selected \"User defined\" for the framesize",
    },
    {
      name:      "user_height",
      long_name: "User defined height",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 1 },
      val_max:     { val_i: 100000 },
      val_default: { val_i: 480 },
      help_string: "User defined height in pixels. Only meaningful if you selected \"User defined\" for the framesize",
    },
    {
      name:      "maintain_aspect",
      long_name: "Maintain aspect ratio",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
      help_string: "Let the aspect ratio appear the same as in the source, probably resulting in additional black borders"
    },
    
    { /* End of parameters */ }
  };

static bg_parameter_info_t general_parameters_audio[] =
  {
    {
      name:        "action",
      long_name:   "Action",
      type:        BG_PARAMETER_STRINGLIST,
      multi_names: (char*[]){ "transcode", "forget", (char*)0 },
      multi_labels:  (char*[]){ "Transcode", "Forget", (char*)0 },
      val_default: { val_str: "transcode" },
    },
    {
      name:        "conversion_quality",
      long_name:   "Conversion Quality",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: GAVL_QUALITY_FASTEST },
      val_max:     { val_i: GAVL_QUALITY_BEST    },
      val_default: { val_i: GAVL_QUALITY_DEFAULT }
    },
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
      multi_names: (char*[]){ "Mono",
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
      multi_names:  (char*[]){ "Mute",
                              "Copy",
                              "Diff",
                              (char*)0 },
    },
    {
      name:        "stereo_to_mono",
      long_name:   "Stereo to mono mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Mix" },
      multi_names:  (char*[]){ "Choose left",
                              "Choose right",
                              "Mix",
                              (char*)0 },
    },
    { /* End of parameters */ }
  };

/* Audio stream parameters */

bg_parameter_info_t *
bg_transcoder_track_audio_get_general_parameters()
  {
  return general_parameters_audio;
  }

/* Video stream parameters */

bg_parameter_info_t *
bg_transcoder_track_video_get_general_parameters()
  {
  return general_parameters_video;
  }

char * bg_transcoder_track_get_name(bg_transcoder_track_t * t)
  {
  bg_parameter_value_t val;
  bg_parameter_info_t info;

  memset(&val, 0, sizeof(val));
  memset(&info, 0, sizeof(info));
  info.name = "name";
    
  bg_cfg_section_get_parameter(t->general_section, &info, &val);
  return val.val_str;
  }

char * bg_transcoder_track_get_audio_encoder(bg_transcoder_track_t * t)
  {
  bg_parameter_value_t val;
  bg_parameter_info_t info;

  memset(&val, 0, sizeof(val));
  memset(&info, 0, sizeof(info));
  info.name = "audio_encoder";
    
  bg_cfg_section_get_parameter(t->general_section, &info, &val);
  return val.val_str;
  }

char * bg_transcoder_track_get_video_encoder(bg_transcoder_track_t * t)
  {
  bg_parameter_value_t val;
  bg_parameter_info_t info;

  memset(&val, 0, sizeof(val));
  memset(&info, 0, sizeof(info));
  info.name = "video_encoder";
    
  bg_cfg_section_get_parameter(t->general_section, &info, &val);
  return val.val_str;
  }

void bg_transcoder_track_get_duration(bg_transcoder_track_t * t, gavl_time_t * ret,
                                      gavl_time_t * ret_total)
  {
  gavl_time_t start_time = 0, end_time = 0, duration_total = 0;
  int set_start_time = 0, set_end_time = 0;
  
  bg_cfg_section_get_parameter_int(t->general_section,  "set_start_time", &set_start_time);
  bg_cfg_section_get_parameter_int(t->general_section,  "set_end_time", &set_end_time);

  bg_cfg_section_get_parameter_time(t->general_section, "duration",   &duration_total);
  bg_cfg_section_get_parameter_time(t->general_section, "start_time", &start_time);
  bg_cfg_section_get_parameter_time(t->general_section, "end_time",   &end_time);

  *ret_total = duration_total;

  if(duration_total == GAVL_TIME_UNDEFINED)
    {
    if(set_end_time)
      *ret = end_time;
    else
      *ret = duration_total;
    }
  else
    {
    if(set_start_time)
      {
      if(set_end_time) /* Start and end */
        {
        *ret = end_time - start_time;
        if(*ret < 0)
          *ret = 0;
        }
      else /* Start only */
        {
        *ret = duration_total - start_time;
        if(*ret < 0)
          *ret = 0;
        }
      }
    else
      {
      if(set_end_time) /* End only */
        {
        *ret = end_time;
        }
      else
        {
        *ret = duration_total;
        }
      }
    }
  
  return;
  }

void
bg_transcoder_track_set_encoders(bg_transcoder_track_t * track,
                                 bg_plugin_registry_t * plugin_reg,
                                 bg_plugin_handle_t * audio_encoder,
                                 bg_plugin_handle_t * video_encoder)
  {
  bg_cfg_section_t * audio_encoder_section;
  bg_cfg_section_t * video_encoder_section;
    
  free_encoders(track);
 
  /* Update the plugin entries in the general section */

  bg_cfg_section_set_parameter_string(track->general_section,
                                      "audio_encoder",
                                      (audio_encoder) ? audio_encoder->info->name : video_encoder->info->name);

  bg_cfg_section_set_parameter_string(track->general_section,
                                      "video_encoder", video_encoder->info->name);

  /* Create parameters */

  create_encoder_parameters(track, audio_encoder, video_encoder);

  /* Create sections */
    
  video_encoder_section =
    bg_plugin_registry_get_section(plugin_reg,
                                   video_encoder->info->name);

  if(audio_encoder)
    audio_encoder_section =
      bg_plugin_registry_get_section(plugin_reg,
                                     audio_encoder->info->name);
  else
    audio_encoder_section = video_encoder_section;

  
  create_encoder_sections(track, audio_encoder_section, video_encoder_section,
                          audio_encoder, video_encoder);
  
  }
