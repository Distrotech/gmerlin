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
  } stream_t;

int main(int argc, char ** argv)
  {
  bgav_t * file;
  bgav_options_t * opt;

  int audio_index = -1;
  int video_index = -1;
  int track = -1;
  
  file = bgav_create();
  opt = bgav_get_options(file);

  if(argc == 1)
    {
    fprintf(stderr, "Usage: bgavdemux [-t track] [-as <num>] [-vs <num>] <location>\n");
    
    bgav_inputs_dump();
    bgav_redirectors_dump();
    
    bgav_formats_dump();
    
    bgav_codecs_dump();

    bgav_subreaders_dump();
    
    return 0;
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

  
  
  return 0;
  }
