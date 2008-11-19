/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>

#include <gmerlin/cfg_dialog.h>
#include <gmerlin/transcoder_track.h>
#include "trackdialog.h"

#include <gmerlin/textrenderer.h>


struct track_dialog_s
  {
  /* Config dialog */

  bg_dialog_t * cfg_dialog;

  void (*update_callback)(void * priv);
  void * update_priv;
  };

static void set_parameter_general(void * priv, const char * name, const bg_parameter_value_t * val)
  {
  track_dialog_t * d;
  d = (track_dialog_t *)priv;

  if(!name)
    {
    if(d->update_callback)
      d->update_callback(d->update_priv);
    }
  }

track_dialog_t * track_dialog_create(bg_transcoder_track_t * t,
                                     void (*update_callback)(void * priv),
                                     void * update_priv, int show_tooltips,
                                     bg_plugin_registry_t * plugin_reg)
  {
  int i;
  char * label;
  track_dialog_t * ret;
  void * parent, * child;
  bg_encoder_info_t encoder_info;
  
  bg_encoder_info_get_from_track(plugin_reg, t, &encoder_info);
                                            
  ret = calloc(1, sizeof(*ret));

  ret->update_callback = update_callback;
  ret->update_priv     = update_priv;
  
  ret->cfg_dialog = bg_dialog_create_multi(TR("Track options"));
  
  /* General */
  
  bg_dialog_add(ret->cfg_dialog,
                TR("General"),
                t->general_section,
                set_parameter_general, NULL, ret,
                t->general_parameters);

  
  /* Metadata */
  
  bg_dialog_add(ret->cfg_dialog,
                TR("Metadata"),
                t->metadata_section,
                NULL,
                NULL,
                NULL,
                t->metadata_parameters);

  /* Audio encoder */

  if(encoder_info.audio_encoder_parameters && t->num_audio_streams)
    {
    label = bg_cfg_section_get_name_translated(t->audio_encoder_section);
    bg_dialog_add(ret->cfg_dialog,
                  label,
                  t->audio_encoder_section,
                  NULL,
                  NULL,
                  NULL,
                  encoder_info.audio_encoder_parameters);
    free(label);
    }
  
  /* Video encoder */

  if(encoder_info.video_encoder_parameters &&
     (!(encoder_info.audio_info) || t->num_video_streams))
    {
    label = bg_cfg_section_get_name_translated(t->video_encoder_section);
    bg_dialog_add(ret->cfg_dialog,
                  label,
                  t->video_encoder_section,
                  NULL,
                  NULL,
                  NULL,
                  encoder_info.video_encoder_parameters);
    free(label);
    }

  /* Subtitle text encoder */

  if(encoder_info.subtitle_text_encoder_parameters && t->num_subtitle_text_streams)
    {
    label = bg_cfg_section_get_name_translated(t->subtitle_text_encoder_section);
    bg_dialog_add(ret->cfg_dialog,
                  label,
                  t->subtitle_text_encoder_section,
                  NULL,
                  NULL,
                  NULL,
                  encoder_info.subtitle_text_encoder_parameters);
    free(label);
    }

  /* Subtitle overlay encoder */

  if(encoder_info.subtitle_overlay_encoder_parameters &&
     (t->num_subtitle_text_streams || t->num_subtitle_overlay_streams))
    {
    label = bg_cfg_section_get_name_translated(t->subtitle_overlay_encoder_section);
    bg_dialog_add(ret->cfg_dialog,
                  label,
                  t->subtitle_overlay_encoder_section,
                  NULL,
                  NULL,
                  NULL,
                  encoder_info.subtitle_overlay_encoder_parameters);
    free(label);
    }

  
  /* Audio streams */

  for(i = 0; i < t->num_audio_streams; i++)
    {
    if(t->num_audio_streams > 1)
      {
      if(t->audio_streams[i].label)
        label = bg_sprintf(TR("Audio #%d: %s"), i+1, t->audio_streams[i].label);
      else
        label = bg_sprintf(TR("Audio #%d"), i+1);
      }
    else
      {
      if(t->audio_streams[i].label)
        label = bg_sprintf(TR("Audio: %s"), t->audio_streams[i].label);
      else
        label = bg_sprintf(TR("Audio"));
      }
    
    
    parent = bg_dialog_add_parent(ret->cfg_dialog, NULL,
                                  label);
    free(label);
    
    bg_dialog_add_child(ret->cfg_dialog, parent,
                        TR("General"),
                        t->audio_streams[i].general_section,
                        NULL,
                        NULL,
                        NULL,
                        bg_transcoder_track_audio_get_general_parameters());

    bg_dialog_add_child(ret->cfg_dialog, parent,
                        TR("Filters"),
                        t->audio_streams[i].filter_section,
                        NULL,
                        NULL,
                        NULL,
                        t->audio_streams[i].filter_parameters);
    
    if(encoder_info.audio_stream_parameters)
      {
      label = bg_cfg_section_get_name_translated(t->audio_streams[i].encoder_section);
      if(encoder_info.audio_stream_parameters[0].type != BG_PARAMETER_SECTION)
        {
        bg_dialog_add_child(ret->cfg_dialog, parent,
                            label,
                            t->audio_streams[i].encoder_section,
                            NULL,
                            NULL,
                            NULL,
                            encoder_info.audio_stream_parameters);
        }
      else
        {
        child = bg_dialog_add_parent(ret->cfg_dialog, parent, label);
        bg_dialog_add_child(ret->cfg_dialog, child,
                            NULL,
                            t->audio_streams[i].encoder_section,
                            NULL,
                            NULL,
                            NULL,
                            encoder_info.audio_stream_parameters);
        
        }
      free(label);
      }
    }

  /* Video streams */

  for(i = 0; i < t->num_video_streams; i++)
    {
    if(t->num_video_streams > 1)
      {
      if(t->video_streams[i].label)
        label = bg_sprintf(TR("Video #%d: %s"), i+1, t->video_streams[i].label);
      else
        label = bg_sprintf(TR("Video #%d"), i+1);
      }
    else
      {
      if(t->video_streams[i].label)
        label = bg_sprintf(TR("Video: %s"), t->video_streams[i].label);
      else
        label = bg_sprintf(TR("Video"));
      }

    parent = bg_dialog_add_parent(ret->cfg_dialog, NULL,
                                  label);
    free(label);
    
    bg_dialog_add_child(ret->cfg_dialog, parent,
                        TR("General"),
                        t->video_streams[i].general_section,
                        NULL,
                        NULL,
                        NULL,
                        bg_transcoder_track_video_get_general_parameters());

    bg_dialog_add_child(ret->cfg_dialog, parent,
                        TR("Filters"),
                        t->video_streams[i].filter_section,
                        NULL,
                        NULL,
                        NULL,
                        t->video_streams[i].filter_parameters);

    
    if(encoder_info.video_stream_parameters)
      {
      label = bg_cfg_section_get_name_translated(t->video_streams[i].encoder_section);
      if(encoder_info.video_stream_parameters[0].type != BG_PARAMETER_SECTION)
        {
        bg_dialog_add_child(ret->cfg_dialog, parent,
                            label,
                            t->video_streams[i].encoder_section,
                            NULL,
                            NULL,
                            NULL,
                            encoder_info.video_stream_parameters);
        }
      else
        {
        child = bg_dialog_add_parent(ret->cfg_dialog, parent,
                                     label);
        bg_dialog_add_child(ret->cfg_dialog, child,
                            NULL,
                            t->video_streams[i].encoder_section,
                            NULL,
                            NULL,
                            NULL,
                            encoder_info.video_stream_parameters);
        
        }
      free(label);
      }
    }
  
  /* Subtitle streams */
  
  for(i = 0; i < t->num_subtitle_text_streams; i++)
    {
    if(t->num_subtitle_text_streams > 1)
      {
      if(t->subtitle_text_streams[i].label)
        label = bg_sprintf(TR("Subtitles #%d: %s"), i+1, t->subtitle_text_streams[i].label);
      else
        label = bg_sprintf(TR("Subtitles #%d"), i+1);
      }
    else
      {
      if(t->subtitle_text_streams[i].label)
        label = bg_sprintf(TR("Subtitles: %s"), t->subtitle_text_streams[i].label);
      else
        label = bg_sprintf(TR("Subtitles"));
      }
    
    parent = bg_dialog_add_parent(ret->cfg_dialog, NULL,
                                  label);
    free(label);
    
    bg_dialog_add_child(ret->cfg_dialog, parent,
                        TR("General"),
                        t->subtitle_text_streams[i].general_section,
                        NULL,
                        NULL,
                        NULL,
                        t->subtitle_text_streams[i].general_parameters);

    bg_dialog_add_child(ret->cfg_dialog, parent,
                        TR("Textrenderer"),
                        t->subtitle_text_streams[i].textrenderer_section,
                        NULL,
                        NULL,
                        NULL,
                        bg_text_renderer_get_parameters());

    if(encoder_info.subtitle_text_stream_parameters)
      {
      label = bg_cfg_section_get_name_translated(t->subtitle_text_streams[i].encoder_section_text);
      bg_dialog_add_child(ret->cfg_dialog, parent,
                          label,
                          t->subtitle_text_streams[i].encoder_section_text,
                          NULL,
                          NULL,
                          NULL,
                          encoder_info.subtitle_text_stream_parameters);
      free(label);
      }
    }

  for(i = 0; i < t->num_subtitle_overlay_streams; i++)
    {
    if(t->num_subtitle_overlay_streams > 1)
      {
      if(t->subtitle_overlay_streams[i].label)
        label = bg_sprintf(TR("Subtitles #%d: %s"), i+1+t->num_subtitle_text_streams,
                           t->subtitle_overlay_streams[i].label);
      else
        label = bg_sprintf(TR("Subtitles #%d"), i+1+t->num_subtitle_text_streams);
      }
    else
      {
      if(t->subtitle_overlay_streams[i].label)
        label = bg_sprintf(TR("Subtitles: %s"), t->subtitle_overlay_streams[i].label);
      else
        label = bg_sprintf(TR("Subtitles"));
      }
    
    parent = bg_dialog_add_parent(ret->cfg_dialog, NULL,
                                  label);
    free(label);
    
    bg_dialog_add_child(ret->cfg_dialog, parent,
                        TR("General"),
                        t->subtitle_overlay_streams[i].general_section,
                        NULL,
                        NULL,
                        NULL,
                        t->subtitle_overlay_streams[i].general_parameters);

    if(encoder_info.subtitle_overlay_stream_parameters)
      {
      label = bg_cfg_section_get_name_translated(t->subtitle_overlay_streams[i].encoder_section);
      bg_dialog_add_child(ret->cfg_dialog, parent,
                          label,
                          t->subtitle_overlay_streams[i].encoder_section,
                          NULL,
                          NULL,
                          NULL,
                          encoder_info.subtitle_overlay_stream_parameters);
      }

    }
  
  return ret;
  
  }

void track_dialog_run(track_dialog_t * d, GtkWidget * parent)
  {
  bg_dialog_show(d->cfg_dialog, parent);
  }

void track_dialog_destroy(track_dialog_t * d)
  {
  bg_dialog_destroy(d->cfg_dialog);
  free(d);
  }

