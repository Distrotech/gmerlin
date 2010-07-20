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

#include <avdec.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int track = 0;
int audio_stream = 0;
int video_stream = 0;
int sample_accurate = 0;

int64_t audio_position = -1;
int64_t video_position = -1;

static bgav_t * open_common(const char * filename)
  {
  bgav_t * b;
  bgav_options_t * opt;
  int num_tracks;
  
  b = bgav_create();
  opt = bgav_get_options(b);
  bgav_options_set_sample_accurate(opt, sample_accurate);
  
  if(!bgav_open(b, filename))
    goto fail;
    
  if(sample_accurate && !bgav_can_seek_sample(b))
    {
    fprintf(stderr, "%s supports no sample accuracy\n", filename);
    goto fail;
    }

  num_tracks = bgav_num_tracks(b);

  if(track >= num_tracks)
    {
    fprintf(stderr, "requested track %d, but file has just %d tracks\n",
            track+1, num_tracks);
    goto fail;
    }
  if(track < 0)
    {
    fprintf(stderr, "Negative track\n");
    goto fail;
    }
  bgav_select_track(b, track);
  return b;
  
  fail:

  if(b)
    bgav_close(b);
  return NULL;
  }

static bgav_t * open_audio(const char * filename)
  {
  bgav_t * b = open_common(filename);
  
  if(!b)
    goto fail;

  bgav_set_audio_stream(b, audio_stream, BGAV_STREAM_DECODE);
  if(!bgav_start(b))
    goto fail;

  return b;
  
  fail:

  if(b)
    bgav_close(b);
  return NULL;
  }

static bgav_t * open_video(const char * filename)
  {
  bgav_t * b = open_common(filename);
  
  if(!b)
    goto fail;

  bgav_set_video_stream(b, video_stream, BGAV_STREAM_DECODE);
  if(!bgav_start(b))
    goto fail;

  return b;
  
  fail:

  if(b)
    bgav_close(b);
  return NULL;
  
  }

#define SAMPLES_TO_READ (10*1024)

static int test_audio(const char * filename)
  {
  const gavl_audio_format_t * format_orig;
  gavl_audio_format_t format;
  
  gavl_audio_frame_t * f1 = NULL, *f2 = NULL;
  int result;
  int64_t samples_to_skip;
  int64_t samples_to_read;
  bgav_t * b = open_audio(filename);

  int ret = 0;
  
  if(!b)
    goto fail;
  
  format_orig = bgav_get_audio_format(b, audio_stream);
  gavl_audio_format_copy(&format, format_orig);
  format.samples_per_frame = SAMPLES_TO_READ;
  
  f1 = gavl_audio_frame_create(&format);
  f2 = gavl_audio_frame_create(&format);
  
  /* Skip to the requested sample */

  samples_to_skip =  audio_position;

  while(samples_to_skip)
    {
    samples_to_read = SAMPLES_TO_READ;
    if(samples_to_read > samples_to_skip)
      samples_to_read = samples_to_skip;
    
    result = bgav_read_audio(b, f1, audio_stream, samples_to_read);

    if(!result)
      {
      fprintf(stderr, "EOF while skipping to sample position\n");
      goto fail;
      }
    samples_to_skip -= result;
    }
  /* Read first frame */
  result = bgav_read_audio(b, f1, audio_stream, SAMPLES_TO_READ);
  fprintf(stderr, "Decoded frame 1: %d samples, pts: %" PRId64 "\n",
          result, f1->timestamp);

  /* Close */
  bgav_close(b);

  /* Reopen and seek */
  b = open_audio(filename);

  if(!b)
    goto fail;

  samples_to_skip =  audio_position;

  bgav_seek_scaled(b, &samples_to_skip, format.samplerate);
  
  result = bgav_read_audio(b, f2, audio_stream, SAMPLES_TO_READ);
  fprintf(stderr, "Decoded frame 2: %d samples, pts: %" PRId64 "\n",
          result, f2->timestamp);

  fprintf(stderr, "Testing if frames are equal: ");

  if(gavl_audio_frames_equal(&format, f1, f2))
    {
    fprintf(stderr, ":)\n");
    ret = 1;
    }
  else
    fprintf(stderr, ":(\n");
  
  fail:
  if(b)
    bgav_close(b);
  if(f1)
    gavl_audio_frame_destroy(f1);
  if(f2)
    gavl_audio_frame_destroy(f2);

  return ret;
  
  }

static int test_video(const char * filename)
  {
  const gavl_video_format_t * format_orig;
  gavl_video_format_t format;
  
  gavl_video_frame_t * f1, *f2;
  int result;
  int64_t frames_to_skip;
  int64_t video_time;
  
  bgav_t * b = open_video(filename);
  int ret = 0;

  if(!b)
    goto fail;
  
  format_orig = bgav_get_video_format(b, video_stream);
  gavl_video_format_copy(&format, format_orig);
  
  f1 = gavl_video_frame_create(&format);
  f2 = gavl_video_frame_create(&format);
  
  /* Skip to the requested sample */
  
  frames_to_skip =  video_position;
  
  while(frames_to_skip)
    {
    result = bgav_read_video(b, f1, video_stream);
    if(!result)
      {
      fprintf(stderr, "EOF while skipping to frame position\n");
      goto fail;
      }
    frames_to_skip--;
    }
  
  /* Read first frame */
  result = bgav_read_video(b, f1, video_stream);
  fprintf(stderr, "Decoded frame 1: pts: %" PRId64 ", duration: %" PRId64 "\n",
          f1->timestamp, f1->duration);
  
  /* Close */
  bgav_close(b);

  /* Reopen and seek */
  b = open_video(filename);

  if(!b)
    goto fail;

  video_time = f1->timestamp;
  
  bgav_seek_scaled(b, &video_time, format.timescale);
  
  result = bgav_read_video(b, f2, video_stream);

  if(!result)
    {
    fprintf(stderr, "EOF after seeking to frame position\n");
    goto fail;
    }
  
  fprintf(stderr, "Decoded frame 2: pts: %" PRId64 ", duration: %" PRId64 "\n",
          f2->timestamp, f2->duration);
  
  fprintf(stderr, "Testing if frames are equal: ");

  if(gavl_video_frames_equal(&format, f1, f2))
    {
    fprintf(stderr, ":)\n");
    ret = 1;
    }
  else
    fprintf(stderr, ":(\n");
  
  fail:
  if(b)
    bgav_close(b);
  if(f1)
    gavl_video_frame_destroy(f1);
  if(f2)
    gavl_video_frame_destroy(f2);
  
  return ret;
  }

#if 0
static void test_subtitle(bgav_t * b, int stream)
  {
  
  }
#endif

int main(int argc, char ** argv)
  {
  int i;
  char * filename = NULL;
  
  i = 1;
  while(i < argc)
    {
    if(!strcmp(argv[i], "-apos"))
      {
      audio_position = strtoll(argv[i+1], NULL, 10);
      i++;
      }
    else if(!strcmp(argv[i], "-vpos"))
      {
      video_position = strtoll(argv[i+1], NULL, 10);
      i++;
      }
    else if(!strcmp(argv[i], "-sa"))
      {
      sample_accurate = 1;
      }
    else if(!strcmp(argv[i], "-as"))
      {
      audio_stream = atoi(argv[i+1]);
      i++;
      }
    else if(!strcmp(argv[i], "-vs"))
      {
      video_stream = atoi(argv[i+1]);
      i++;
      }
    else if(!strcmp(argv[i], "-t"))
      {
      track = atoi(argv[i+1])-1;
      i++;
      }
    else
      {
      filename = argv[i];
      }
    i++;
    }

  if(!filename)
    {
    fprintf(stderr, "Filename missing");
    return 1;
    }

  if(audio_position >= 0)
    test_audio(filename);

  if(video_position >= 0)
    test_video(filename);
  return 0;
  }
