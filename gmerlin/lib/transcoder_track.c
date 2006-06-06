/*****************************************************************
  
  transcoder_track.c
  
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
  
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

#include <bggavl.h>

#include <textrenderer.h>

void bg_transcoder_track_create_encoder_sections(bg_transcoder_track_t * t,
                                                 bg_transcoder_encoder_info_t * info)
  {
  int i;
  
  /* Audio encoder section */
  
  if(info->audio_encoder_section && !t->audio_encoder_section)
    {
    t->audio_encoder_section = bg_cfg_section_copy(info->audio_encoder_section);
    bg_cfg_section_set_name(t->audio_encoder_section, info->audio_info->long_name);

    if(bg_cfg_section_has_subsection(t->audio_encoder_section, "$audio"))
      {
      bg_cfg_section_delete_subsection(t->audio_encoder_section,
                                       bg_cfg_section_find_subsection(t->audio_encoder_section,
                                                                      "$audio"));
      }
    }
  
  /* Video encoder section */
  if(info->video_encoder_section && !t->video_encoder_section)
    {
    t->video_encoder_section = bg_cfg_section_copy(info->video_encoder_section);
    bg_cfg_section_set_name(t->video_encoder_section, info->video_info->long_name);
        
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
    if(bg_cfg_section_has_subsection(t->video_encoder_section, "$subtitle_text"))
      {
      bg_cfg_section_delete_subsection(t->video_encoder_section,
                                       bg_cfg_section_find_subsection(t->video_encoder_section,
                                                                      "$subtitle_text"));
      }
    if(bg_cfg_section_has_subsection(t->video_encoder_section, "$subtitle_overlay"))
      {
      bg_cfg_section_delete_subsection(t->video_encoder_section,
                                       bg_cfg_section_find_subsection(t->video_encoder_section,
                                                                      "$subtitle_overlay"));
      }
    }

  /* Text subtitle encoder section */
  if(info->subtitle_text_encoder_section && !t->subtitle_text_encoder_section)
    {
    t->subtitle_text_encoder_section = bg_cfg_section_copy(info->subtitle_text_encoder_section);
    //    fprintf(stderr, "Text encoder: %s\n", info->subtitle_text_info->long_name);
    bg_cfg_section_set_name(t->subtitle_text_encoder_section,
                            info->subtitle_text_info->long_name);

    if(bg_cfg_section_has_subsection(t->subtitle_text_encoder_section, "$subtitle_text"))
      {
      bg_cfg_section_delete_subsection(t->subtitle_text_encoder_section,
                                       bg_cfg_section_find_subsection(t->subtitle_text_encoder_section,
                                                                      "$subtitle_text"));
      }
    }

  /* Overlay subtitle encoder section */
  if(info->subtitle_overlay_encoder_section && !t->subtitle_overlay_encoder_section)
    {
    t->subtitle_overlay_encoder_section =
      bg_cfg_section_copy(info->subtitle_overlay_encoder_section);
    bg_cfg_section_set_name(t->subtitle_overlay_encoder_section,
                            info->subtitle_overlay_info->long_name);

    if(bg_cfg_section_has_subsection(t->subtitle_overlay_encoder_section, "$subtitle_overlay"))
      {
      bg_cfg_section_delete_subsection(t->subtitle_overlay_encoder_section,
                                       bg_cfg_section_find_subsection(t->subtitle_overlay_encoder_section,
                                                                      "$subtitle_overlay"));
      }
    }
  
  /* Audio stream sections */
    
  if(t->num_audio_streams)
    {
    if(info->audio_stream_section)
      {
      for(i = 0; i < t->num_audio_streams; i++)
        {
        if(!t->audio_streams[i].encoder_section)
          {
          t->audio_streams[i].encoder_section =
            bg_cfg_section_copy(info->audio_stream_section);
          bg_cfg_section_set_name(t->audio_streams[i].encoder_section, "Encode options");
          }
        }
      }
    }

  /* Video stream sections */

  if(t->num_video_streams)
    {
    if(info->video_stream_section)
      {
      for(i = 0; i < t->num_video_streams; i++)
        {
        if(!t->video_streams[i].encoder_section)
          {
          t->video_streams[i].encoder_section =
            bg_cfg_section_copy(info->video_stream_section);
          bg_cfg_section_set_name(t->video_streams[i].encoder_section, "Encode options");
          }
        }
      }
    }

  /* Subtitle text stream sections */

  if(t->num_subtitle_text_streams)
    {
    for(i = 0; i < t->num_subtitle_text_streams; i++)
      {
      if(info->subtitle_text_stream_section &&
         !t->subtitle_text_streams[i].encoder_section_text)
        {
        t->subtitle_text_streams[i].encoder_section_text =
          bg_cfg_section_copy(info->subtitle_text_stream_section);
        bg_cfg_section_set_name(t->subtitle_text_streams[i].encoder_section_text,
                                "Encode options (Text)");
        }
      if(info->subtitle_overlay_stream_section &&
         !t->subtitle_text_streams[i].encoder_section_overlay)
        {
        t->subtitle_text_streams[i].encoder_section_overlay =
          bg_cfg_section_copy(info->subtitle_overlay_stream_section);
        bg_cfg_section_set_name(t->subtitle_text_streams[i].encoder_section_overlay,
                                "Encode options (Overlay)");
        }
      }
    }
  
  /* Subtitle overlay stream sections */

  if(t->num_subtitle_overlay_streams)
    {
    if(info->subtitle_overlay_stream_section)
      {
      for(i = 0; i < t->num_subtitle_overlay_streams; i++)
        {
        if(!t->subtitle_overlay_streams[i].encoder_section)
          {
          t->subtitle_overlay_streams[i].encoder_section =
            bg_cfg_section_copy(info->subtitle_overlay_stream_section);
          bg_cfg_section_set_name(t->subtitle_overlay_streams[i].encoder_section,
                                  "Encode options");
          }
        }
      }
    }
  }

static void create_sections(bg_transcoder_track_t * t,
                            bg_cfg_section_t * track_defaults_section,
                            bg_cfg_section_t * input_section,
                            bg_transcoder_encoder_info_t * encoder_info)
  {
  int i;

  bg_cfg_section_t * general_section;
  bg_cfg_section_t * textrenderer_section;

#if 0
  t->general_section =
    bg_cfg_section_create_from_parameters("General", t->general_parameters);
#else
  t->input_section = bg_cfg_section_copy(input_section);
  
  general_section = bg_cfg_section_find_subsection(track_defaults_section, "general");
  t->general_section = bg_cfg_section_copy(general_section);
  
  /* The parameters which were initially hidden are not present in the general section.
     Therefore, we will create the missing items now */

  i = 0;
  while(t->general_parameters[i].name)
    {
    bg_cfg_section_get_parameter(t->general_section, &(t->general_parameters[i]),
                                 (bg_parameter_value_t*)0);
    i++;
    }
  
#endif
  
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

  if(t->num_subtitle_text_streams)
    {
    general_section = bg_cfg_section_find_subsection(track_defaults_section,
                                                     "subtitle_text");
    textrenderer_section = bg_cfg_section_find_subsection(track_defaults_section,
                                                          "textrenderer");
    for(i = 0; i < t->num_subtitle_text_streams; i++)
      {
      t->subtitle_text_streams[i].general_section = bg_cfg_section_copy(general_section);
      t->subtitle_text_streams[i].textrenderer_section = bg_cfg_section_copy(textrenderer_section);
      }
    }

  if(t->num_subtitle_overlay_streams)
    {
    general_section = bg_cfg_section_find_subsection(track_defaults_section,
                                                     "subtitle_overlay");
    for(i = 0; i < t->num_subtitle_overlay_streams; i++)
      {
      t->subtitle_overlay_streams[i].general_section = bg_cfg_section_copy(general_section);
      }
    }
  bg_transcoder_track_create_encoder_sections(t, encoder_info);
  
  }

static bg_parameter_info_t parameters_general[] =
  {
    {
      name:      "name",
      long_name: "Name",
      type:      BG_PARAMETER_STRING,
      flags:     BG_PARAMETER_HIDE_DIALOG,
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
      name:      "subtitle_text_encoder",
      long_name: "Subtitle text encoder",
      type:      BG_PARAMETER_STRING,
      flags:     BG_PARAMETER_HIDE_DIALOG,
    },
    {
      name:      "subtitle_overlay_encoder",
      long_name: "Subtitle overlay encoder",
      type:      BG_PARAMETER_STRING,
      flags:     BG_PARAMETER_HIDE_DIALOG,
    },
    {
      name:      "pp_only",
      long_name: "Postprocess only",
      type:      BG_PARAMETER_CHECKBUTTON,
      help_string: "Skip transcoding of this track and send the file directly to the postprocessor.",
    },
    {
      name:      "set_start_time",
      long_name: "Set start time",
      type:      BG_PARAMETER_CHECKBUTTON,
      flags:     BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 0 },
      help_string: "Specify a start time below. This time will be slightly wrong if the input \
format doesn't support sample accurate seeking."
    },
    {
      name:      "start_time",
      long_name: "Start time",
      type:      BG_PARAMETER_TIME,
      flags:     BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_time: 0 }
    },
    {
      name:      "set_end_time",
      long_name: "Set end time",
      type:      BG_PARAMETER_CHECKBUTTON,
      flags:     BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 0 },
      help_string: "Specify an end time below."
    },
    {
      name:      "end_time",
      long_name: "End time",
      type:      BG_PARAMETER_TIME,
      flags:     BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_time: 0 }
    },
    { /* End of parameters */ }
  };

/* Subtitle text parameters */

static bg_parameter_info_t general_parameters_subtitle_text[] =
  {
    {
      name:        "action",
      long_name:   "Action",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "forget" },
      multi_names:  (char*[]){ "forget", "transcode",           "transcode_overlay",    "blend",          (char*)0 },
      multi_labels: (char*[]){ "Forget", "Transcode as text",   "Transcode as overlay", "Blend onto video", (char*)0 },
      help_string: "Select action for this subtitle stream."
    },
    {
      name:        "video_stream",
      long_name:   "Video stream",
      type:        BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 1 },
      val_min:     { val_i: 1 },
      help_string: "Attach subtitle stream to this video stream. For blending, this video stream will\
 get the subtitles. For encoding, take frame dimensions and framerate from this video stream as they are\
 sometimes needed by subtitle encoders."
    },
    { /* End of parameters */ }
  };

/* Subtitle overlay parameters */

static bg_parameter_info_t general_parameters_subtitle_overlay[] =
  {
    {
      name:        "action",
      long_name:   "Action",
      type:        BG_PARAMETER_STRINGLIST,
      multi_names:  (char*[]){ "forget", "transcode", "blend",          (char*)0 },
      multi_labels: (char*[]){ "Forget", "Transcode", "Blend onto video", (char*)0 },
      val_default: { val_str: "forget" },
    },
    {
      name:        "blend_stream",
      long_name:   "Video stream to blend onto",
      type:        BG_PARAMETER_INT,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 1 },
      val_min:     { val_i: 1 },
    },
    { /* End of parameters */ }
  };

/* Create subtitle parameters */

static void create_subtitle_parameters(bg_transcoder_track_t * track)
  {
  int i;
  bg_parameter_info_t * info;

  /* Create subtitle parameters. These depend on the number of video streams */

  for(i = 0; i < track->num_subtitle_text_streams; i++)
    {
    /* Forget, Dump, Blend #1, Blend #2 ... */
    track->subtitle_text_streams[i].general_parameters =
      bg_parameter_info_copy_array(general_parameters_subtitle_text);
    info = track->subtitle_text_streams[i].general_parameters;

    if(track->num_video_streams > 1)
      {
      info[1].val_max.val_i = track->num_video_streams;
      info[1].flags &= ~BG_PARAMETER_HIDE_DIALOG;
      }
    
    }
  for(i = 0; i < track->num_subtitle_overlay_streams; i++)
    {
    /* Forget, Blend #1, Blend #2 ... */

    track->subtitle_overlay_streams[i].general_parameters =
      bg_parameter_info_copy_array(general_parameters_subtitle_overlay);
    info = track->subtitle_overlay_streams[i].general_parameters;

    if(track->num_video_streams > 1)
      {
      info[1].val_max.val_i = track->num_video_streams;
      info[1].flags &= ~BG_PARAMETER_HIDE_DIALOG;
      }
    }
  }

/* Create parameters if the config sections are already there */

void
bg_transcoder_track_create_parameters(bg_transcoder_track_t * track,
                                      const bg_plugin_info_t * audio_info,
                                      const bg_plugin_info_t * video_info)
  {
  gavl_time_t duration = GAVL_TIME_UNDEFINED;
  int i;
  int seekable = 0;
  
  track->general_parameters = bg_parameter_info_copy_array(parameters_general);

  bg_cfg_section_get_parameter_time(track->general_section,
                                    "duration", &duration);
  bg_cfg_section_get_parameter_int(track->general_section,
                                   "seekable", &seekable);

  if(duration != GAVL_TIME_UNDEFINED)
    {
    i = 0;

    while(track->general_parameters[i].name)
      {
      if(!strcmp(track->general_parameters[i].name, "start_time") ||
         !strcmp(track->general_parameters[i].name, "end_time"))
        track->general_parameters[i].val_max.val_time = duration;
      i++;
      }


    if(seekable)
      {
      i = 0;
      while(track->general_parameters[i].name)
        {
        if(!strcmp(track->general_parameters[i].name, "start_time") ||
           !strcmp(track->general_parameters[i].name, "set_start_time"))
          track->general_parameters[i].flags &= ~BG_PARAMETER_HIDE_DIALOG;
        i++;
        }
      }
    }
  
  i = 0;
  
  while(track->general_parameters[i].name)
    {
    if(!strcmp(track->general_parameters[i].name, "name") ||
       !strcmp(track->general_parameters[i].name, "set_end_time") ||
       !strcmp(track->general_parameters[i].name, "end_time"))
      track->general_parameters[i].flags &= ~BG_PARAMETER_HIDE_DIALOG;
    i++;
    }
  
  track->metadata_parameters = bg_metadata_get_parameters((bg_metadata_t*)0);
  
  create_subtitle_parameters(track);
    
#if 0
  fprintf(stderr, "Encoder parameters: %p %p %p %p\n",
          audio_encoder,
          video_encoder,
          track->audio_encoder_parameters,
          track->video_encoder_parameters);
#endif
  }

static char * create_stream_label(const char * info, const char * language)
  {
  if(language && *language && info)
    return bg_sprintf("%s [%s]", info, language);
  else if(language && *language)
    return bg_strdup((char*)0, language);
  else if(info)
    return bg_strdup((char*)0, info);
  else
    return (char*)0;
  }

static void set_track(bg_transcoder_track_t * track,
                      bg_track_info_t * track_info,
                      bg_plugin_handle_t * input_plugin,
                      const char * location,
                      int track_index,
                      bg_transcoder_encoder_info_t * encoder_info)
  {
  int i;
  int subtitle_text_index, subtitle_overlay_index;
  
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
      track->general_parameters[i].flags &= ~BG_PARAMETER_HIDE_DIALOG;
      }
    else if(!strcmp(track->general_parameters[i].name, "audio_encoder"))
      {
      if(encoder_info->audio_info)
        track->general_parameters[i].val_default.val_str =
          bg_strdup((char*)0, encoder_info->audio_info->name);
      else
        track->general_parameters[i].val_default.val_str =
          bg_strdup((char*)0, encoder_info->video_info->name);
      }
    else if(!strcmp(track->general_parameters[i].name, "subtitle_text_encoder"))
      {
      if(encoder_info->subtitle_text_info)
        track->general_parameters[i].val_default.val_str =
          bg_strdup((char*)0, encoder_info->subtitle_text_info->name);
      else
        track->general_parameters[i].val_default.val_str =
          bg_strdup((char*)0, encoder_info->video_info->name);
      }
    else if(!strcmp(track->general_parameters[i].name, "subtitle_overlay_encoder"))
      {
      if(encoder_info->subtitle_overlay_info)
        track->general_parameters[i].val_default.val_str =
          bg_strdup((char*)0, encoder_info->subtitle_overlay_info->name);
      else
        track->general_parameters[i].val_default.val_str =
          bg_strdup((char*)0, encoder_info->video_info->name);
      }

    
    else if(!strcmp(track->general_parameters[i].name, "video_encoder"))
      track->general_parameters[i].val_default.val_str =
        bg_strdup((char*)0, encoder_info->video_info->name);
    
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
      if(track_info->seekable)
        track->general_parameters[i].flags &= ~BG_PARAMETER_HIDE_DIALOG;
      }
    else if(!strcmp(track->general_parameters[i].name, "start_time"))
      {
      if(track_info->seekable)
        {
        track->general_parameters[i].flags &= ~BG_PARAMETER_HIDE_DIALOG;
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
      track->general_parameters[i].flags &= ~BG_PARAMETER_HIDE_DIALOG;
      }
    else if(!strcmp(track->general_parameters[i].name, "set_end_time"))
      {
      track->general_parameters[i].flags &= ~BG_PARAMETER_HIDE_DIALOG;
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
    
    for(i = 0; i < track_info->num_audio_streams; i++)
      {
      track->audio_streams[i].label =
        create_stream_label(track_info->audio_streams[i].info,
                            track_info->audio_streams[i].language);
      
      }
    
    }
  
  /* Video streams */
  
  if(track_info->num_video_streams)
    {
    track->num_video_streams = track_info->num_video_streams; 
    track->video_streams = calloc(track_info->num_video_streams,
                                  sizeof(*(track->video_streams)));

    for(i = 0; i < track_info->num_video_streams; i++)
      {
      track->video_streams[i].label =
        create_stream_label(track_info->video_streams[i].info,
                            track_info->video_streams[i].language);
      }
    }

  /* Subtitle streams */
  
  if(track_info->num_subtitle_streams)
    {
    for(i = 0; i < track_info->num_subtitle_streams; i++)
      {
      if(track_info->subtitle_streams[i].is_text)
        track->num_subtitle_text_streams++;
      else
        track->num_subtitle_overlay_streams++;
      }
    track->subtitle_text_streams = calloc(track->num_subtitle_text_streams,
                                          sizeof(*(track->subtitle_text_streams)));
    track->subtitle_overlay_streams = calloc(track->num_subtitle_overlay_streams,
                                          sizeof(*(track->subtitle_overlay_streams)));
    
    subtitle_text_index = 0;
    subtitle_overlay_index = 0;
    
    for(i = 0; i < track_info->num_subtitle_streams; i++)
      {
      if(track_info->subtitle_streams[i].is_text)
        {
        track->subtitle_text_streams[subtitle_text_index].label =
          create_stream_label(track_info->subtitle_streams[i].info,
                       track_info->subtitle_streams[i].language);
        track->subtitle_text_streams[subtitle_text_index].in_index = i;
        subtitle_text_index++;
        }
      else
        {
        track->subtitle_overlay_streams[subtitle_overlay_index].label =
          create_stream_label(track_info->subtitle_streams[i].info,
                       track_info->subtitle_streams[i].language);
        track->subtitle_overlay_streams[subtitle_overlay_index].in_index = i;
        
        subtitle_overlay_index++;
        }
      }
    
    }
 
  create_subtitle_parameters(track);
  
  //  fprintf(stderr, "Track name: %s\n", track->name);
  }

static void enable_streams(bg_input_plugin_t * plugin, void * priv, 
                           int track,
                           int num_audio_streams, int num_video_streams,
                           int num_subtitle_streams)
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
  if(plugin->set_subtitle_stream)
    {
    for(i = 0; i < num_subtitle_streams; i++)
      {
      plugin->set_subtitle_stream(priv, i, BG_STREAM_ACTION_DECODE);
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
  
  bg_cfg_section_t * input_section;

  bg_transcoder_encoder_info_t encoder_info;
  
  if(!bg_transcoder_encoder_info_get_from_registry(plugin_reg,
                                                   &encoder_info))
    return (bg_transcoder_track_t*)0;
    
  /* Load the plugin */
  
  if(!bg_input_plugin_load(plugin_reg, url,
                           input_info, &plugin_handle, &error_msg, (bg_input_callbacks_t*)0))
    {
    fprintf(stderr, __FILE__": Loading %s failed: %s\n", url, error_msg);
    free(error_msg);
    return (bg_transcoder_track_t*)0;
    }

  input = (bg_input_plugin_t*)(plugin_handle->plugin);
  
  input_section = bg_plugin_registry_get_section(plugin_reg,
                                                 plugin_handle->info->name);
  
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
                     track_info->num_video_streams,
                     track_info->num_subtitle_streams);
      streams_enabled = 1;
      }
    
    set_track(new_track, track_info, plugin_handle, url, track, &encoder_info);
    create_sections(new_track, track_defaults_section, input_section,
                    &encoder_info);
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
                       track_info->num_video_streams,
                       track_info->num_subtitle_streams);
        streams_enabled = 1;
        }
      
      set_track(new_track, track_info, plugin_handle, url, i, &encoder_info);
      create_sections(new_track, track_defaults_section, input_section,
                      &encoder_info);
      if(streams_enabled)
        {
        disable_streams(input, plugin_handle->priv);
        streams_enabled = 0;
        }
      }
    }
  bg_plugin_unref(plugin_handle);
  
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

  //  fprintf(stderr, "Remove redirectors\n");
  
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

  if(track->subtitle_text_encoder_section)
    {
    bg_cfg_section_destroy(track->subtitle_text_encoder_section);
    track->subtitle_text_encoder_section = NULL;
    }

  if(track->subtitle_overlay_encoder_section)
    {
    bg_cfg_section_destroy(track->subtitle_overlay_encoder_section);
    track->subtitle_overlay_encoder_section = NULL;
    }
  
  for(i = 0; i < track->num_audio_streams; i++)
    {
    if(track->audio_streams[i].encoder_section)
      {
      bg_cfg_section_destroy(track->audio_streams[i].encoder_section);
      track->audio_streams[i].encoder_section = NULL;
      }
    }
  
  for(i = 0; i < track->num_video_streams; i++)
    {
    if(track->video_streams[i].encoder_section)
      {
      bg_cfg_section_destroy(track->video_streams[i].encoder_section);
      track->video_streams[i].encoder_section = NULL;
      }
    }

  for(i = 0; i < track->num_subtitle_text_streams; i++)
    {
    if(track->subtitle_text_streams[i].encoder_section_text)
      {
      bg_cfg_section_destroy(track->subtitle_text_streams[i].encoder_section_text);
      track->subtitle_text_streams[i].encoder_section_text = NULL;
      }
    if(track->subtitle_text_streams[i].encoder_section_overlay)
      {
      bg_cfg_section_destroy(track->subtitle_text_streams[i].encoder_section_overlay);
      track->subtitle_text_streams[i].encoder_section_overlay = NULL;
      }
    }

  for(i = 0; i < track->num_subtitle_overlay_streams; i++)
    {
    if(track->subtitle_overlay_streams[i].encoder_section)
      {
      bg_cfg_section_destroy(track->subtitle_overlay_streams[i].encoder_section);
      track->subtitle_overlay_streams[i].encoder_section = NULL;
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
    if(t->audio_streams[i].label) free(t->audio_streams[i].label);
    }
  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->video_streams[i].general_section)
      bg_cfg_section_destroy(t->video_streams[i].general_section);
    if(t->video_streams[i].label) free(t->video_streams[i].label);
    }
  for(i = 0; i < t->num_subtitle_text_streams; i++)
    {
    if(t->subtitle_text_streams[i].general_section)
      bg_cfg_section_destroy(t->subtitle_text_streams[i].general_section);
    if(t->subtitle_text_streams[i].general_parameters)
      bg_parameter_info_destroy_array(t->subtitle_text_streams[i].general_parameters);

    if(t->subtitle_text_streams[i].textrenderer_section)
      bg_cfg_section_destroy(t->subtitle_text_streams[i].textrenderer_section);
    
    if(t->subtitle_text_streams[i].label) free(t->subtitle_text_streams[i].label);
    }
  for(i = 0; i < t->num_subtitle_overlay_streams; i++)
    {
    if(t->subtitle_overlay_streams[i].general_section)
      bg_cfg_section_destroy(t->subtitle_overlay_streams[i].general_section);
    if(t->subtitle_overlay_streams[i].general_parameters)
      bg_parameter_info_destroy_array(t->subtitle_overlay_streams[i].general_parameters);
    if(t->subtitle_overlay_streams[i].label) free(t->subtitle_overlay_streams[i].label);
    }
  
  
  if(t->audio_streams)
    free(t->audio_streams);
  if(t->video_streams)
    free(t->video_streams);

  if(t->general_section)
    bg_cfg_section_destroy(t->general_section);
  if(t->input_section)
    bg_cfg_section_destroy(t->input_section);
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
      name:       "general",
      long_name:  "General",
      type:       BG_PARAMETER_SECTION
    },
    {
      name:        "action",
      long_name:   "Action",
      type:        BG_PARAMETER_STRINGLIST,
      multi_names: (char*[]){ "transcode", "forget", (char*)0 },
      multi_labels:  (char*[]){ "Transcode", "Forget", (char*)0 },
      val_default: { val_str: "transcode" },
    },
    {
      name:       "twopass",
      long_name:  "Enable 2-pass encoding",
      type:       BG_PARAMETER_CHECKBUTTON,
      help_string: "Encode this stream in 2 passes, i.e. analyze it first and do the final\
 transcoding in the second pass. This enables higher quality within the given bitrate constraints but roughly doubles the video encoding time.",
    },
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_FRAMERATE,
    BG_GAVL_PARAM_DEINTERLACE,
    BG_GAVL_PARAM_ALPHA,
#if 1
    {
      name:       "crop_scale",
      long_name:  "Crop & Scale",
      type:       BG_PARAMETER_SECTION
    },
    BG_GAVL_PARAM_CROP,
    BG_GAVL_PARAM_FRAME_SIZE,
#endif
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
      name:        "normalize",
      long_name:   "Normalize audio",
      type:        BG_PARAMETER_CHECKBUTTON,
      help_string: "This will enable 2 pass transcoding. In the first pass, the peak volume\
 is detected. In the second pass, the stream is transcoded with normalized volume."
    },
    BG_GAVL_PARAM_FORCE_FLOAT,
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_AUDIO_DITHER_MODE,
    BG_GAVL_PARAM_SAMPLERATE,
    BG_GAVL_PARAM_RESAMPLE_MODE,
    BG_GAVL_PARAM_CHANNEL_SETUP,
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

bg_parameter_info_t *
bg_transcoder_track_subtitle_text_get_general_parameters()
  {
  return general_parameters_subtitle_text;
  }

bg_parameter_info_t *
bg_transcoder_track_subtitle_overlay_get_general_parameters()
  {
  return general_parameters_subtitle_overlay;
  }

bg_parameter_info_t *
bg_transcoder_track_get_general_parameters(bg_transcoder_track_t * t)
  {
  return parameters_general;
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

char * bg_transcoder_track_get_subtitle_text_encoder(bg_transcoder_track_t * t)
  {
  bg_parameter_value_t val;
  bg_parameter_info_t info;

  memset(&val, 0, sizeof(val));
  memset(&info, 0, sizeof(info));
  info.name = "subtitle_text_encoder";
  
  bg_cfg_section_get_parameter(t->general_section, &info, &val);
  return val.val_str;
  }

char * bg_transcoder_track_get_subtitle_overlay_encoder(bg_transcoder_track_t * t)
  {
  bg_parameter_value_t val;
  bg_parameter_info_t info;

  memset(&val, 0, sizeof(val));
  memset(&info, 0, sizeof(info));
  info.name = "subtitle_overlay_encoder";
  
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
                                 bg_transcoder_encoder_info_t * info)
  {
  free_encoders(track);
 
  /* Update the plugin entries in the general section */

  bg_cfg_section_set_parameter_string(track->general_section,
                                      "audio_encoder",
                                      (info->audio_info) ?
                                      info->audio_info->name : info->video_info->name);

  bg_cfg_section_set_parameter_string(track->general_section,
                                      "video_encoder", info->video_info->name);

  bg_cfg_section_set_parameter_string(track->general_section,
                                      "subtitle_text_encoder",
                                      (info->subtitle_text_info) ?
                                      info->subtitle_text_info->name : info->video_info->name);

  bg_cfg_section_set_parameter_string(track->general_section,
                                      "subtitle_overlay_encoder",
                                      (info->subtitle_overlay_info) ?
                                      info->subtitle_overlay_info->name : info->video_info->name);
  
  bg_transcoder_track_create_encoder_sections(track, info);
  }

void
bg_transcoder_track_global_to_reg(bg_transcoder_track_global_t * g,
                                  bg_plugin_registry_t * plugin_reg)
  {
  bg_cfg_section_t * plugin_section;
  
  if(g->pp_plugin)
    {
    fprintf(stderr, "bg_transcoder_track_global_to_reg %s\n", g->pp_plugin);
    bg_plugin_registry_set_default(plugin_reg, BG_PLUGIN_ENCODER_PP,
                                   g->pp_plugin);
    bg_plugin_registry_set_encode_pp(plugin_reg, 1);

    plugin_section =
      bg_plugin_registry_get_section(plugin_reg, g->pp_plugin);

    fprintf(stderr, "Plugin section: %p %p\n", g->pp_section, plugin_section);
    
    bg_cfg_section_transfer(g->pp_section, plugin_section);
    }
  else
    {
    bg_plugin_registry_set_encode_pp(plugin_reg, 0);
    }
  }

void
bg_transcoder_track_global_from_reg(bg_transcoder_track_global_t * g,
                                    bg_plugin_registry_t * plugin_reg)
  {
  bg_cfg_section_t * plugin_section;
  const bg_plugin_info_t * plugin_info;
  
  bg_transcoder_track_global_free(g);
  if(bg_plugin_registry_get_encode_pp(plugin_reg))
    {
    plugin_info = bg_plugin_registry_get_default(plugin_reg, BG_PLUGIN_ENCODER_PP);
    g->pp_plugin = bg_strdup((char*)0, plugin_info->name);
    plugin_section = bg_plugin_registry_get_section(plugin_reg, plugin_info->name);
    g->pp_section = bg_cfg_section_copy(plugin_section);
    }
  }

void
bg_transcoder_track_global_free(bg_transcoder_track_global_t * g)
  {
  if(g->pp_plugin)
    {
    free(g->pp_plugin);
    g->pp_plugin = (char*)0;
    }
  if(g->pp_section)
    {
    bg_cfg_section_destroy(g->pp_section);
    g->pp_section = (bg_cfg_section_t*)0;
    }

  }
