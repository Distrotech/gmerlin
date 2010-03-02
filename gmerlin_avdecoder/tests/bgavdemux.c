/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <avdec.h>


typedef struct
  {
  int active;
  gavl_compression_info_t info;

  int64_t time;
  FILE * out;
  } stream_t;

static void init_audio_stream(bgav_t * b, int index, stream_t * ret)
  {
  if(!bgav_get_audio_compression_info(b, index,
                                      &ret->info))
    fprintf(stderr, "Audio stream %d doesn't support compressed output\n",
            index+1);
  ret->active = 1;
  }

static void init_video_stream(bgav_t * b, int index, stream_t * ret)
  {
  if(!bgav_get_video_compression_info(b, index,
                                      &ret->info))
    fprintf(stderr, "Video stream %d doesn't support compressed output\n",
            index+1);
  ret->active = 1;
  }

int main(int argc, char ** argv)
  {
  bgav_t * file;
  bgav_options_t * opt;

  int num_audio_streams;
  int num_video_streams;

  stream_t * audio_streams;
  stream_t * video_streams;
  
  int audio_stream = 0;
  int video_stream = 0;
  int track = -1;
  int arg_index;
  int i;
  file = bgav_create();
  opt = bgav_get_options(file);

  if(argc == 1)
    {
    fprintf(stderr, "Usage: bgavdemux [-t track] [-as <num>] [-vs <num>] <location>\n");
    
    return 0;
    }

  arg_index = 1;
  while(arg_index < argc - 1)
    {
    if(!strcmp(argv[arg_index], "-t"))
      {
      arg_index++;
      track = atoi(argv[arg_index]);
      arg_index++;
      }
    else if(!strcmp(argv[arg_index], "-as"))
      {
      arg_index++;
      audio_stream = atoi(argv[arg_index]);
      arg_index++;
      }
    else if(!strcmp(argv[arg_index], "-vs"))
      {
      arg_index++;
      video_stream = atoi(argv[arg_index]);
      arg_index++;
      }
    }
  
  if(!strncmp(argv[argc-1], "vcd://", 6))
    {
    if(!bgav_open_vcd(file, argv[argc-1] + 5))
      {
      fprintf(stderr, "Could not open VCD Device %s\n",
              argv[argc-1] + 5);
      return -1;
      }
    }
  else if(!strncmp(argv[argc-1], "dvd://", 6))
    {
    if(!bgav_open_dvd(file, argv[argc-1] + 5))
      {
      fprintf(stderr, "Could not open DVD Device %s\n",
              argv[argc-1] + 5);
      return -1;
      }
    }
  else if(!strncmp(argv[argc-1], "dvb://", 6))
    {
    if(!bgav_open_dvb(file, argv[argc-1] + 6))
      {
      fprintf(stderr, "Could not open DVB Device %s\n",
              argv[argc-1] + 6);
      return -1;
      }
    }
  else if(!bgav_open(file, argv[argc-1]))
    {
    fprintf(stderr, "Could not open file %s\n",
            argv[argc-1]);
    bgav_close(file);
    return -1;
    }

  if(track < 0)
    track = 0;

  /* Select track */
  bgav_select_track(file, track);

  num_audio_streams = bgav_num_audio_streams(file, track);
  num_video_streams = bgav_num_video_streams(file, track);
  
  audio_streams = calloc(num_audio_streams, sizeof(*audio_streams));
  video_streams = calloc(num_audio_streams, sizeof(*audio_streams));

  if(!audio_stream && !video_stream)
    {
    for(i = 0; i < num_audio_streams; i++)
      {
      init_audio_stream(file, i, &audio_streams[i]);
      }
    for(i = 0; i < num_audio_streams; i++)
      {
      init_video_stream(file, i, &video_streams[i]);
      }
    }
  else if(audio_stream >= 0)
    {
    init_audio_stream(file, audio_stream-1, &audio_streams[audio_stream-1]);
    }
  else if(video_stream >= 0)
    {
    init_video_stream(file, video_stream-1, &video_streams[video_stream-1]);
    }
  
  
  return 0;
  }
