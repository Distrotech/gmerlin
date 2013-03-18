/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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
  d = opendir(dir);

  while( (res=readdir(d)) )
    {
    if(!res)
      break;
    
    /* Check, if the filenames match */
    if(!strcasecmp(res->d_name, file))
      {
      ret = bgav_sprintf("%s/%s", dir, res->d_name);
      closedir(d);
      return ret;
      }
    }
  closedir(d);
  return NULL;
  }

static char * find_audio_file(const char * dir, const char * name_root, int stream)
  {
  DIR * d;
  struct dirent * res;
  char * ret;
  char * rest;
  int root_len;
  int i;
  d = opendir(dir);

  root_len = strlen(name_root);
  
  while( (res=readdir(d)) )
    {
    if(!res)
      break;
    
    /* Check, if the filenames match */
    if(!strncasecmp(res->d_name, name_root, root_len))
      {
      i = strtol(&res->d_name[root_len], &rest, 10);
      if((rest == &res->d_name[root_len]) ||
         (i != stream) ||
         strcasecmp(rest, ".mxf"))
        continue;
      ret = bgav_sprintf("%s/%s", dir, res->d_name);
      closedir(d);
      return ret;
      }
    }
  closedir(d);
  return NULL;
  
  }

static int probe_p2xml(bgav_yml_node_t * node)
  {
  if(bgav_yml_find_by_name(node, "P2Main"))
    return 1;
  return 0;
  }

static void init_stream(bgav_yml_node_t * node,
                        gavl_edl_stream_t * s, char * filename,
                        int duration, int edit_unit_num,
                        int edit_unit_den)
  {
  gavl_edl_segment_t * seg;
  seg = gavl_edl_add_segment(s);
  seg->url = filename;
  seg->speed_num = 1;
  seg->speed_den = 1;
  seg->src_time = 0;
  seg->dst_time = 0;
  seg->dst_duration = -1;
  s->timescale = edit_unit_den;
  seg->timescale = edit_unit_den;
  seg->dst_duration = duration * edit_unit_num;
  }

static int open_p2xml(bgav_demuxer_context_t * ctx, bgav_yml_node_t * yml)
  {
  int ret = 0;
  char * audio_directory = NULL;
  char * video_directory = NULL;
  char * directory_parent = NULL;
  char * ptr;
  bgav_yml_node_t * node;
  gavl_edl_track_t * t = NULL;
  gavl_edl_stream_t * s;
  const char * root_name = NULL;
  char * filename;
  char * tmp_string;
  const char * attr;
  int duration = 0;
  int edit_unit_num = 0;
  int edit_unit_den = 0;

  int have_audio = 1;
  
  if(!ctx->input || !ctx->input->filename)
    goto fail;

  directory_parent = bgav_strdup(ctx->input->filename);
  
  yml = bgav_yml_find_by_name(yml, "P2Main");
  
  /* Strip off xml filename */
  ptr = strrchr(directory_parent, '/');
  if(!ptr)
    goto fail;
  *ptr = '\0';
  
  /* Strip off "CLIP" directory */
  ptr = strrchr(directory_parent, '/');
  if(!ptr)
    goto fail;
  *ptr = '\0';

  video_directory = find_file_nocase(directory_parent, "video");
  audio_directory = find_file_nocase(directory_parent, "audio");

  if(!audio_directory && !video_directory)
    goto fail;
  

  ctx->edl = gavl_edl_create();
  t = gavl_edl_add_track(ctx->edl);
  
  node = yml->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    else if(!strcasecmp(node->name, "Duration"))
      {
      duration = atoi(node->children->str);
      }
    else if(!strcasecmp(node->name, "EditUnit"))
      {
      sscanf(node->children->str, "%d/%d", &edit_unit_num,
             &edit_unit_den);
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
      if(!duration || !edit_unit_num || !edit_unit_den)
        goto fail;
      
      node = node->children;
      continue;
      }
    else if(!strcasecmp(node->name, "Audio") && have_audio)
      {
      if(root_name && audio_directory)
        {
        filename = find_audio_file(audio_directory, root_name, t->num_audio_streams);
        if(filename)
          {
          s = gavl_edl_add_audio_stream(t);
          init_stream(node, s, filename,
                      duration, edit_unit_num, edit_unit_den);
          }
        }
      }
    else if(!strcasecmp(node->name, "Video"))
      {
      attr = bgav_yml_get_attribute(node, "ValidAudioFlag");
      if(attr && !strcasecmp(attr, "false"))
        have_audio = 0;
      
      if(root_name && video_directory)
        {
        tmp_string = bgav_sprintf("%s.mxf", root_name);
        filename = find_file_nocase(video_directory, tmp_string);
        if(filename)
          {
          s = gavl_edl_add_video_stream(t);
          init_stream(node, s, filename,
                      duration, edit_unit_num, edit_unit_den);
          }
        free(tmp_string);
        }
      }
    node = node->next;
    }

  ret = 1;
  fail:
  if(directory_parent) free(directory_parent);
  if(audio_directory) free(audio_directory);
  if(video_directory) free(video_directory);
  
  return ret;
  }

const bgav_demuxer_t bgav_demuxer_p2xml =
  {
    .probe_yml =       probe_p2xml,
    .open_yml  =        open_p2xml,
  };

