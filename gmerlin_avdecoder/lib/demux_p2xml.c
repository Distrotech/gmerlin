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

#include <sys/types.h>
#include <dirent.h>

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
      ret = bgav_strdup(dir);
      ret = bgav_strncat(ret, "/", NULL);
      ret = bgav_strncat(ret, u.d.d_name, NULL);
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

static void init_video_stream(bgav_yml_node_t * node, bgav_edl_track_t * t,
                              const char * directory)
  {
  
  }

static void init_audio_stream(bgav_yml_node_t * node, bgav_edl_track_t * t,
                              const char * directory)
  {
  
  }

static int open_p2xml(bgav_demuxer_context_t * ctx, bgav_yml_node_t * yml)
  {
  char * audio_directory;
  char * video_directory;
  char * directory_parent;
  char * ptr;
  
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

  //  fprintf(stderr, "Got directories: %s %s\n", video_directory, audio_directory);
  
  return 1;
  }

const bgav_demuxer_t bgav_demuxer_p2xml =
  {
    .probe_yml =       probe_p2xml,
    .open_yml  =        open_p2xml,
  };

