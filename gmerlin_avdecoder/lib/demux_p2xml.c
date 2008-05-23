/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <avdec_private.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <dirent.h>
#include <limits.h>

static char * find_file_nocase(const char * dir, const char * file)
  {
  DIR * d;
  struct dirent * res;
  char * ret;
  union
    {
    struct dirent d;
    char b[sizeof (struct dirent) + NAME_MAX];
    } u;
  d = opendir(dir);

  while(!readdir_r(d, &u.d, &res))
    {
    if(!res)
      break;
    
    /* Check, if the filenames match */
    if(!strcasecmp(u.d.d_name, file))
      {
      ret = bgav_sprintf("%s/%s", dir, u.d.d_name);
      closedir(d);
      return ret;
      }
    }
  closedir(d);
  return (char*)0;
  }

static char * find_audio_file(const char * dir, const char * name_root, int stream)
  {
  DIR * d;
  struct dirent * res;
  char * ret;
  char * rest;
  int root_len;
  int i;
  union
    {
    struct dirent d;
    char b[sizeof (struct dirent) + NAME_MAX];
    } u;
  d = opendir(dir);

  root_len = strlen(name_root);
  
  while(!readdir_r(d, &u.d, &res))
    {
    if(!res)
      break;
    
    /* Check, if the filenames match */
    if(!strncasecmp(u.d.d_name, name_root, root_len))
      {
      i = strtol(&u.d.d_name[root_len], &rest, 10);
      if((rest == &u.d.d_name[root_len]) ||
         (i != stream) ||
         strcasecmp(rest, ".mxf"))
        continue;
      ret = bgav_sprintf("%s/%s", dir, u.d.d_name);
      closedir(d);
      return ret;
      }
    }
  closedir(d);
  return (char*)0;
  
  }

static int probe_p2xml(bgav_yml_node_t * node)
  {
  if(node->name && !strcasecmp(node->name, "P2Main"))
    return 1;
  return 0;
  }

static void init_stream(bgav_yml_node_t * node, bgav_edl_stream_t * s, char * filename)
  {
  bgav_edl_segment_t * seg;
  seg = bgav_edl_add_segment(s);
  seg->url = filename;
  seg->speed_num = 1;
  seg->speed_den = 1;
  seg->src_time = 0;
  seg->dst_time = 0;
  seg->dst_duration = -1;
  }

static int open_p2xml(bgav_demuxer_context_t * ctx, bgav_yml_node_t * yml)
  {
  char * audio_directory;
  char * video_directory;
  char * directory_parent;
  char * ptr;
  bgav_yml_node_t * node;
  bgav_edl_track_t * t = (bgav_edl_track_t *)0;
  bgav_edl_stream_t * s;
  const char * root_name = (const char*)0;
  char * filename;
  char * tmp_string;
  
  if(!ctx->input || !ctx->input->filename)
    return 0;

  directory_parent = bgav_strdup(ctx->input->filename);

  /* Strip off xml filename */
  ptr = strrchr(directory_parent, '/');
  if(!ptr)
    return 0;
  *ptr = '\0';
  
  /* Strip off "CLIP" directory */
  ptr = strrchr(directory_parent, '/');
  if(!ptr)
    return 0;
  *ptr = '\0';

  video_directory = find_file_nocase(directory_parent, "video");
  audio_directory = find_file_nocase(directory_parent, "audio");

  if(!audio_directory && !video_directory)
    return 0;
  
  //  fprintf(stderr, "Got directories: %s %s\n", video_directory, audio_directory);

  ctx->edl = bgav_edl_create();
  t = bgav_edl_add_track(ctx->edl);
  
  node = yml->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    else if(!strcasecmp(node->name, "ClipContent"))
      {
      node = node->children;
      continue;
      }
    else if(!strcasecmp(node->name, "ClipName"))
      {
      if(node->children)
        root_name = node->children->str;
      }
    else if(!strcasecmp(node->name, "EssenceList"))
      {
      node = node->children;
      continue;
      }
    else if(!strcasecmp(node->name, "Audio"))
      {
      if(root_name && audio_directory)
        {
        filename = find_audio_file(audio_directory, root_name, t->num_audio_streams);
        if(filename)
          {
          s = bgav_edl_add_audio_stream(t);
          init_stream(node, s, filename);
          }
        else
          fprintf(stderr, "Got no file for audio stream %d\n", t->num_audio_streams);
        }
      }
    else if(!strcasecmp(node->name, "Video"))
      {
      if(root_name && video_directory)
        {
        tmp_string = bgav_sprintf("%s.mxf", root_name);
        filename = find_file_nocase(video_directory, tmp_string);
        if(filename)
          {
          s = bgav_edl_add_video_stream(t);
          init_stream(node, s, filename);
          }
        else
          fprintf(stderr, "Got no file for video stream %d\n", t->num_video_streams);
        }
      }
    node = node->next;
    }
  return 1;
  }

const bgav_demuxer_t bgav_demuxer_p2xml =
  {
    .probe_yml =       probe_p2xml,
    .open_yml  =        open_p2xml,
  };

