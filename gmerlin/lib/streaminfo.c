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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <gavl/gavl.h>
#include <gavl/metatags.h>
#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
#include <gmerlin/utils.h>

#define MY_FREE(ptr) \
  if(ptr) \
    { \
    free(ptr); \
    ptr = NULL; \
    }

static void bg_audio_info_free(bg_audio_info_t * info)
  {
  gavl_metadata_free(&info->m);
  }

static void bg_video_info_free(bg_video_info_t * info)
  {
  gavl_metadata_free(&info->m);
  }

static void bg_text_info_free(bg_text_info_t * info)
  {
  gavl_metadata_free(&info->m);
  }

static void bg_overlay_info_free(bg_overlay_info_t * info)
  {
  gavl_metadata_free(&info->m);
  }

void bg_track_info_free(bg_track_info_t * info)
  {
  int i;

  if(info->audio_streams)
    {
    for(i = 0; i < info->num_audio_streams; i++)
      bg_audio_info_free(&info->audio_streams[i]);
    MY_FREE(info->audio_streams);
    }

  if(info->video_streams)
    {
    for(i = 0; i < info->num_video_streams; i++)
      bg_video_info_free(&info->video_streams[i]);
    MY_FREE(info->video_streams);
    }
  if(info->text_streams)
    {
    for(i = 0; i < info->num_text_streams; i++)
      bg_text_info_free(&info->text_streams[i]);
    MY_FREE(info->text_streams);
    }
  if(info->overlay_streams)
    {
    for(i = 0; i < info->num_overlay_streams; i++)
      bg_overlay_info_free(&info->overlay_streams[i]);
    MY_FREE(info->overlay_streams);
    }
  
  gavl_metadata_free(&info->metadata);

  if(info->chapter_list)
    gavl_chapter_list_destroy(info->chapter_list);
  
  MY_FREE(info->url);
  memset(info, 0, sizeof(*info));
  }

void bg_set_track_name_default(bg_track_info_t * info,
                               const char * location)
  {
  char * name;
  const char * start_pos;
  const char * end_pos;

  if(gavl_metadata_get(&info->metadata, GAVL_META_LABEL))
    return;
  
  if(bg_string_is_url(location))
    name = bg_strdup(NULL, location);
  else
    {
    start_pos = strrchr(location, '/');
    if(start_pos)
      start_pos++;
    else
      start_pos = location;
    end_pos = strrchr(start_pos, '.');
    if(!end_pos)
      end_pos = &start_pos[strlen(start_pos)];
    name = bg_strndup(NULL, start_pos, end_pos);
    }
  gavl_metadata_set_nocpy(&info->metadata, GAVL_META_LABEL, name);
  }

char * bg_get_track_name_default(const char * location, int track, int num_tracks)
  {
  const char * start_pos;
  const char * end_pos;
  char * tmp_string, *ret;
  
  if(bg_string_is_url(location))
    {
    tmp_string = bg_strdup(NULL, location);
    }
  else
    {
    start_pos = strrchr(location, '/');
    if(start_pos)
      start_pos++;
    else
      start_pos = location;
    end_pos = strrchr(start_pos, '.');
    if(!end_pos)
      end_pos = &start_pos[strlen(start_pos)];
    tmp_string = bg_system_to_utf8(start_pos, end_pos - start_pos);
    }
  if(num_tracks < 2)
    return tmp_string;
  else
    {
    ret = bg_sprintf("%s [%d]", tmp_string, track + 1);
    free(tmp_string);
    return ret;
    }
  }

gavl_time_t bg_track_info_get_duration(const bg_track_info_t * info)
  {
  gavl_time_t ret;
  if(gavl_metadata_get_long(&info->metadata, GAVL_META_APPROX_DURATION, &ret))
    return ret;
  else
    return GAVL_TIME_UNDEFINED;
  }
