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

int main(int argc, char ** argv)
  {
  int i;
  int num_audio_streams;
  int num_video_streams;
  
  bgav_t * file;
  
  gavl_audio_frame_t * af;
  gavl_video_frame_t * vf;

  gavl_audio_format_t * audio_format;
  gavl_video_format_t * video_format;

  bgav_codecs_dump();
  
  file = bgav_open(argv[1], 10000);
  
  if(!file)
    {
    fprintf(stderr, "Could not open file %s\n",
            argv[1]);
    return -1;
    }
  
  /* Try to get one frame from each stream */

  num_audio_streams = bgav_num_audio_streams(file);
  num_video_streams = bgav_num_video_streams(file);
  for(i = 0; i < num_audio_streams; i++)
    {
    bgav_set_audio_stream(file, i, BGAV_STREAM_DECODE);
    }
  for(i = 0; i < num_video_streams; i++)
    {
    bgav_set_video_stream(file, i, BGAV_STREAM_DECODE);
    }
  
  bgav_start_decoders(file);
    
  fprintf(stderr, "Dumping file contents...\n");
  bgav_dump(file);
  fprintf(stderr, "End of file contents\n");

  for(i = 0; i < num_audio_streams; i++)
    {
    audio_format = bgav_get_audio_format(file, i);
    af = gavl_audio_frame_create(audio_format);
    fprintf(stderr, "Reading frame from audio stream %d...", i+1);
    if(bgav_read_audio(file, af, i))
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
    gavl_video_frame_destroy(vf);
    }

  
  bgav_close(file);
    
  return -1;
  }
