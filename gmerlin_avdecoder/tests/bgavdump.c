/*****************************************************************
 
  bgavdump.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <avdec.h>
#include <stdio.h>
#include <string.h>

/* Configuration data */

static int connect_timeout   = 5000;
static int read_timeout      = 5000;
static int network_bandwidth = 524300; /* 524.3 Kbps (Cable/DSL) */

int main(int argc, char ** argv)
  {
  int i;
  int num_audio_streams;
  int num_video_streams;
  int num_urls;
  int num_tracks;
  int track;
    
  bgav_t * file;
  
  gavl_audio_frame_t * af;
  gavl_video_frame_t * vf;

  const gavl_audio_format_t * audio_format_c;
  gavl_audio_format_t audio_format;
  const gavl_video_format_t * video_format;

  if(argc == 1)
    {
    fprintf(stderr, "Usage: bgavdump <location>");

    fprintf(stderr, "Available formats:\n");
    bgav_formats_dump();

    fprintf(stderr, "Available decoders:\n");
    bgav_codecs_dump();
    return 0;
    }
  
  file = bgav_create();

  /* Configure */

  bgav_set_connect_timeout(file,   connect_timeout);
  bgav_set_read_timeout(file,      read_timeout);
  bgav_set_network_bandwidth(file, network_bandwidth);

  if(!strncmp(argv[1], "vcd://", 6))
    {
    if(!bgav_open_vcd(file, argv[1] + 5))
      {
      fprintf(stderr, "Could not open VCD Device %s\n",
              argv[1] + 5);
      return -1;
      }
    }
  else if(!bgav_open(file, argv[1]))
    {
    fprintf(stderr, "Could not open file %s\n",
            argv[1]);
    free(file);
    return -1;
    }
  
  if(bgav_is_redirector(file))
    {
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
  num_tracks = bgav_num_tracks(file);
  for(track = 0; track < num_tracks; track++)
    {
    bgav_select_track(file, track);
    
    num_audio_streams = bgav_num_audio_streams(file, track);
    num_video_streams = bgav_num_video_streams(file, track);
    for(i = 0; i < num_audio_streams; i++)
      {
      bgav_set_audio_stream(file, i, BGAV_STREAM_DECODE);
      }
    for(i = 0; i < num_video_streams; i++)
      {
      bgav_set_video_stream(file, i, BGAV_STREAM_DECODE);
      }
    fprintf(stderr, "Starting decoders...");
    bgav_start(file);
    fprintf(stderr, "done\n");
    
    fprintf(stderr, "Dumping file contents...\n");
    bgav_dump(file);
    fprintf(stderr, "End of file contents\n");

    /* Try to get one frame from each stream */
  
    for(i = 0; i < num_audio_streams; i++)
      {
      audio_format_c = bgav_get_audio_format(file, i);
      gavl_audio_format_copy(&audio_format, audio_format_c);
      audio_format.samples_per_frame = 1024;
      af = gavl_audio_frame_create(&audio_format);
      fprintf(stderr, "Reading 1024 samples from audio stream %d...", i+1);
      if(bgav_read_audio(file, af, i, 1024))
        fprintf(stderr, "Done\n");
      else
        fprintf(stderr, "Failed\n");
      gavl_audio_frame_destroy(af);
      }
    
    for(i = 0; i < num_video_streams; i++)
      {
      video_format = bgav_get_video_format(file, i);
      vf = gavl_video_frame_create(video_format);
#if 1
      fprintf(stderr, "Reading frame from video stream %d...", i+1);
      if(bgav_read_video(file, vf, i))
        fprintf(stderr, "Done\n");
      else
        fprintf(stderr, "Failed\n");
#endif
      //      gavl_video_frame_dump(vf, video_format, "frame");
      gavl_video_frame_destroy(vf);
      }
    }
    

  
  bgav_close(file);
    
  return -1;
  }
