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

#include <avdec.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int track = 0;
int stream = 0;
int sample_accurate = 0;

int main(int argc, char ** argv)
  {
  bgav_t * file;
  int arg_index;
  bgav_options_t * opt;
  
  int64_t count;
  int i;
  
  const gavl_audio_format_t * format;
  gavl_audio_frame_t * frame;
  
  if(argc == 1)
    {
    fprintf(stderr,
            "Usage: count_samples [-s] [-as stream] [-t track] <location>\n");
    return 0;
    }
  file = bgav_create();

  arg_index = 1;
  
  while(arg_index < argc - 1)
    {
    if(!strcmp(argv[arg_index], "-s"))
      {
      sample_accurate = 1;
      arg_index++;
      }
    else if(!strcmp(argv[arg_index], "-as"))
      {
      stream = strtol(argv[arg_index+1], (char**)0, 10);
      arg_index+=2;
      }
    else if(!strcmp(argv[arg_index], "-t"))
      {
      track = strtol(argv[arg_index+1], (char**)0, 10);
      arg_index+=2;
      }
    else
      arg_index++;
    }

  opt = bgav_get_options(file);
  
  if(sample_accurate)
    bgav_options_set_sample_accurate(opt, 1);

  if(!bgav_open(file, argv[argc-1]))
    {
    fprintf(stderr, "Could not open file %s\n",
            argv[argc-1]);
    bgav_close(file);
    return -1;
    }
  
  if(bgav_is_redirector(file))
    {
    int num_urls;
    fprintf(stderr, "Found redirector:\n");
    num_urls = bgav_redirector_get_num_urls(file);
    for(i = 0; i < num_urls; i++)
      {
      fprintf(stderr, "Name %d: %s\n", i+1, bgav_redirector_get_name(file, i));
      fprintf(stderr, "URL %d: %s\n",  i+1, bgav_redirector_get_url(file, i));
      }
    bgav_close(file);
    return 0;
    }
  
  if(track < 0 || track >= bgav_num_tracks(file))
    {
    fprintf(stderr,
            "No such track %d\n", track);
    return -1;
    }

  bgav_select_track(file, track);
  
  if(sample_accurate && !bgav_can_seek_sample(file))
    {
    fprintf(stderr,
            "Sample accurate access not possible for track %d\n", track+1);
    return -1;
    }

  if(stream < 0 || stream >= bgav_num_audio_streams(file, track))
    {
    fprintf(stderr,
            "No such stream %d\n", stream);
    return -1;
    }

  bgav_set_audio_stream(file, stream, BGAV_STREAM_DECODE);

  if(!bgav_start(file))
    {
    fprintf(stderr, "Starting decoders failed\n");
    return -1;
    }
  else
    fprintf(stderr, "Starting decoders done\n");

  format = bgav_get_audio_format(file, stream);

  frame = gavl_audio_frame_create(format);

  count = 0;
  
  while(bgav_read_audio(file, frame, stream, format->samples_per_frame))
    {
    count += frame->valid_samples;
    }

  fprintf(stderr, "Track %d stream %d contains %"PRId64" samples\n",
          track+1, stream+1, count);

  bgav_close(file);
  gavl_audio_frame_destroy(frame);
  return 0;
  }
