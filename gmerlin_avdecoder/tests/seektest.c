/*****************************************************************
 
  seektest.c
 
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

#include <avdec_private.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void test_audio(bgav_t * b, int stream)
  {
  const gavl_audio_format_t * format;
  gavl_audio_frame_t * f;
  int result;
  format = bgav_get_audio_format(b, stream);
  f = gavl_audio_frame_create(format);
  result = bgav_read_audio(b, f, stream, 1000);
  fprintf(stderr, "Decoded %d samples, pts: %" PRId64 "\n",
          result, f->timestamp);

  result = bgav_read_audio(b, f, stream, 1000);
  fprintf(stderr, "Decoded %d samples, pts: %" PRId64 "\n",
          result, f->timestamp);
  gavl_audio_frame_destroy(f);
  }

static void test_video(bgav_t * b, int stream)
  {
  const gavl_video_format_t * format;
  gavl_video_frame_t * f;
  int result;
  format = bgav_get_video_format(b, stream);
  f = gavl_video_frame_create(format);
  result = bgav_read_video(b, f, stream);
  fprintf(stderr, "Decoded %d frames, pts: %" PRId64 "\n",
          result, f->timestamp);

  result = bgav_read_video(b, f, stream);
  fprintf(stderr, "Decoded %d frames, pts: %" PRId64 "\n",
          result, f->timestamp);
  gavl_video_frame_destroy(f);
  
  }

static void test_subtitle(bgav_t * b, int stream)
  {
  
  }

int main(int argc, char ** argv)
  {
  int i, j;
  bgav_t * b;
  bgav_options_t * opt;
  int tracks, streams;

  /*
   * Create the decoder and request sample accurate access
   */
  
  b = bgav_create();
  opt = bgav_get_options(b);
  bgav_options_set_sample_accurate(opt, 1);
  
  if(!bgav_open(b, argv[1]))
    return -1;
  
  if(!bgav_can_seek_sample(b))
    {
    fprintf(stderr, "%s supports no sample accuracy\n", argv[1]);
    return 0;
    }

  tracks = bgav_num_tracks(b);

  for(i = 0; i < tracks; i++)
    {
    bgav_select_track(b, i);

    /* Enable all streams */
    streams = bgav_num_audio_streams(b, i);
    for(j = 0; j < streams; j++)
      bgav_set_audio_stream(b, j, BGAV_STREAM_DECODE);
    
    streams = bgav_num_video_streams(b, i);
    for(j = 0; j < streams; j++)
      bgav_set_video_stream(b, j, BGAV_STREAM_DECODE);
    
    streams = bgav_num_subtitle_streams(b, i);
    for(j = 0; j < streams; j++)
      bgav_set_subtitle_stream(b, j, BGAV_STREAM_DECODE);

    /* Start decoder */
    
    if(!bgav_start(b))
      {
      fprintf(stderr, "Starting decoder failed\n");
      bgav_close(b);
      return -1;
      }

    /* Get durations */
    streams = bgav_num_audio_streams(b, i);
    for(j = 0; j < streams; j++)
      {
      fprintf(stderr, "AS 1: %"PRId64" samples\n",
              bgav_audio_duration(b, j));
      
      test_audio(b, j);
      }
    streams = bgav_num_video_streams(b, i);
    for(j = 0; j < streams; j++)
      {
      fprintf(stderr, "VS 1: Duration: %"PRId64"\n",
              bgav_video_duration(b, j));
      test_video(b, j);
      }
    streams = bgav_num_subtitle_streams(b, i);
    for(j = 0; j < streams; j++)
      {
      fprintf(stderr, "SS 1: Duration: %"PRId64"\n",
              bgav_video_duration(b, j));
      }
    
    }
  
  bgav_close(b);
  
  return 0;
  }
