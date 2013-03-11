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

#include "gavf-decode.h"
#include <string.h>

#define LOG_DOMAIN "gavf-decode.load"

int load_input_file(const char * file, const char * plugin_name,
                    int track, bg_mediaconnector_t * conn,
                    bg_input_callbacks_t * cb,
                    bg_plugin_handle_t ** hp, 
                    gavl_metadata_t * m, gavl_chapter_list_t ** cl,
                    int edl, int force_decompress)
  {
  gavl_audio_source_t * asrc;
  gavl_video_source_t * vsrc;
  gavl_packet_source_t * psrc;
  bg_track_info_t * track_info;
  const bg_plugin_info_t * plugin_info;
  bg_input_plugin_t * plugin;
  void * priv;
  int i;
  const gavl_metadata_t * stream_metadata;
  
  if(plugin_name)
    {
    plugin_info = bg_plugin_find_by_name(plugin_reg, plugin_name);
    if(!plugin_info)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Input plugin %s not found", plugin_name);
      goto fail;
      }
    }
  else
    plugin_info = NULL;

  if(!bg_input_plugin_load(plugin_reg,
                           file,
                           plugin_info,
                           hp, cb, edl))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't load %s", file);
    goto fail;
    }
  
  plugin = (bg_input_plugin_t*)((*hp)->plugin);
  priv = (*hp)->priv;
  
  /* Select track and get track info */
  if((track < 0) ||
     (track >= plugin->get_num_tracks(priv)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No such track %d", track);
    goto fail;
    }

  if(plugin->set_track)
    plugin->set_track(priv, track);
  
  track_info = plugin->get_track_info(priv, track);

  /* Extract metadata */
  gavl_metadata_free(m);
  gavl_metadata_init(m);
  gavl_metadata_copy(m, &track_info->metadata);

  if(!gavl_metadata_get(m, GAVL_META_LABEL))
    {
    if(strcmp(file, "-"))
      {
      gavl_metadata_set_nocpy(m, GAVL_META_LABEL,
                              bg_get_track_name_default(file,
                                                        track,
                                                        plugin->get_num_tracks(priv)));
      }
    }
  if(cl)
    *cl = track_info->chapter_list;

  /* Select A/V streams */
  if(!audio_actions)  
    audio_actions = gavftools_get_stream_actions(track_info->num_audio_streams,
                                                 GAVF_STREAM_AUDIO);
  if(!video_actions)
    video_actions = gavftools_get_stream_actions(track_info->num_video_streams,
                                                 GAVF_STREAM_VIDEO);
  if(!text_actions)
    text_actions = gavftools_get_stream_actions(track_info->num_text_streams,
                                                GAVF_STREAM_TEXT);
  if(!overlay_actions)
    overlay_actions = gavftools_get_stream_actions(track_info->num_overlay_streams,
                                                   GAVF_STREAM_OVERLAY);

  /* Check for compressed reading */

  for(i = 0; i < track_info->num_audio_streams; i++)
    {
    if((audio_actions[i] == BG_STREAM_ACTION_READRAW) &&
       (force_decompress || !plugin->get_audio_compression_info ||
        !plugin->get_audio_compression_info(priv, i, NULL)))
      {
      audio_actions[i] = BG_STREAM_ACTION_DECODE;
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Audio stream %d cannot be read compressed",
             i+1);
      }
    }

  for(i = 0; i < track_info->num_video_streams; i++)
    {
    if((video_actions[i] == BG_STREAM_ACTION_READRAW) &&
       (force_decompress || !plugin->get_video_compression_info ||
        !plugin->get_video_compression_info(priv, i, NULL)))
      {
      video_actions[i] = BG_STREAM_ACTION_DECODE;
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Video stream %d cannot be read compressed",
             i+1);
      }
    }

  for(i = 0; i < track_info->num_overlay_streams; i++)
    {
    if((overlay_actions[i] == BG_STREAM_ACTION_READRAW) &&
       (force_decompress || !plugin->get_overlay_compression_info ||
        !plugin->get_overlay_compression_info(priv, i, NULL)))
      {
      overlay_actions[i] = BG_STREAM_ACTION_DECODE;
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Overlay stream %d cannot be read compressed",
             i+1);
      }
    }

  /* Enable streams */
  
  if(plugin->set_audio_stream)
    {
    for(i = 0; i < track_info->num_audio_streams; i++)
      plugin->set_audio_stream(priv, i, audio_actions[i]);
    }
  if(plugin->set_video_stream)
    {
    for(i = 0; i < track_info->num_video_streams; i++)
      plugin->set_video_stream(priv, i, video_actions[i]);
    }
  if(plugin->set_text_stream)
    {
    for(i = 0; i < track_info->num_text_streams; i++)
      plugin->set_text_stream(priv, i, text_actions[i]);
    }
  if(plugin->set_overlay_stream)
    {
    for(i = 0; i < track_info->num_overlay_streams; i++)
      plugin->set_overlay_stream(priv, i, overlay_actions[i]);
    }
  
  /* Start plugin */
  if(plugin->start && !plugin->start(priv))
    goto fail;
  
  for(i = 0; i < track_info->num_audio_streams; i++)
    {
    stream_metadata = &track_info->audio_streams[i].m;
    
    switch(audio_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        asrc = NULL;
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
        asrc = plugin->get_audio_source(priv, i);
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_READRAW:
        psrc = plugin->get_audio_packet_source(priv, i);
        asrc = NULL;
        break;
      }

    if(asrc || psrc)
      bg_mediaconnector_add_audio_stream(conn, stream_metadata, asrc, psrc,
                                         gavftools_ac_section());
    }
  
  for(i = 0; i < track_info->num_video_streams; i++)
    {
    stream_metadata = &track_info->video_streams[i].m;
    switch(video_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        vsrc = NULL;
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
        vsrc = plugin->get_video_source(priv, i);
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_READRAW:
        psrc = plugin->get_video_packet_source(priv, i);
        vsrc = NULL;
        break;
      }
    if(vsrc || psrc)
      bg_mediaconnector_add_video_stream(conn, stream_metadata, vsrc, psrc,
                                         gavftools_vc_section());
    }
  
  for(i = 0; i < track_info->num_text_streams; i++)
    {
    stream_metadata = &track_info->text_streams[i].m;
    switch(text_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
      case BG_STREAM_ACTION_READRAW:
        psrc = plugin->get_text_source(priv, i);
        break;
      }
    if(psrc)
      bg_mediaconnector_add_text_stream(conn, stream_metadata, psrc,
                                        track_info->text_streams[i].timescale);
    }
  
  for(i = 0; i < track_info->num_overlay_streams; i++)
    {
    stream_metadata = &track_info->overlay_streams[i].m;
    switch(text_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        vsrc = NULL;
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
        vsrc = plugin->get_overlay_source(priv, i);
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_READRAW:
        vsrc = NULL;
        psrc = plugin->get_overlay_packet_source(priv, i);
        break;
      }
    if(vsrc || psrc)
      bg_mediaconnector_add_overlay_stream(conn, stream_metadata, vsrc, psrc,
                                           gavftools_oc_section());
    }

  return 1;
  fail:
  
  return 0;
  }

/* Stuff for loading albums */

int load_album_entry(bg_album_entry_t * entry,
                     bg_mediaconnector_t * conn,
                     bg_plugin_handle_t ** hp, 
                     gavl_metadata_t * m)
  {
  int ret = load_input_file(entry->location, entry->plugin,
                            entry->index, conn,
                            NULL, hp, m, NULL,
                            !!(entry->flags & BG_ALBUM_ENTRY_EDL), 1);
  if(!ret)
    return ret;
  
  gavl_metadata_set(m, GAVL_META_LABEL, entry->name);
  return ret;
  }
