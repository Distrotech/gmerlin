/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

static void bg_subtitle_info_free(bg_subtitle_info_t * info)
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
  if(info->subtitle_streams)
    {
    for(i = 0; i < info->num_subtitle_streams; i++)
      bg_subtitle_info_free(&info->subtitle_streams[i]);
    MY_FREE(info->subtitle_streams);
    }

  gavl_metadata_free(&info->metadata);

  if(info->chapter_list)
    gavl_chapter_list_destroy(info->chapter_list);
  
  MY_FREE(info->name);
  MY_FREE(info->url);
  memset(info, 0, sizeof(*info));
  }

void bg_set_track_name_default(bg_track_info_t * info,
                               const char * location)
  {
  const char * start_pos;
  const char * end_pos;
  
  if(bg_string_is_url(location))
    {
    info->name = bg_strdup(info->name, location);
    return;
    }
  
  start_pos = strrchr(location, '/');
  if(start_pos)
    start_pos++;
  else
    start_pos = location;
  end_pos = strrchr(start_pos, '.');
  if(!end_pos)
    end_pos = &start_pos[strlen(start_pos)];
  info->name = bg_strndup(info->name, start_pos, end_pos);
  
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

