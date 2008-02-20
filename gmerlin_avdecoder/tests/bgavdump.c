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


// #include <avdec.h>
#include <avdec_private.h>
#include <locale.h>

#include <utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h> /* Ask passwords */

// #define TRACK 6

/* Taken from the Unix programmer FAQ: http://www.faqs.org/faqs/unix-faq/programmer/faq/ */

static struct termios stored_settings;
static void echo_off(void)
  {
  struct termios new_settings;
  tcgetattr(0,&stored_settings);
  new_settings = stored_settings;
  new_settings.c_lflag &= (~ECHO);
  tcsetattr(0,TCSANOW,&new_settings);
  return;
  }

static void echo_on(void)
  {
  tcsetattr(0,TCSANOW,&stored_settings);
  return;
  }

/* Ok this is not so clean. But the library allows arbitrary string lengths */

char buf[1024];

static int user_pass_func(void * data, const char * resource, char ** user, char ** pass)
  {
  char * pos;
  fprintf(stderr, "Enter authentication for \"%s\"\n", resource);
  fprintf(stderr, "Username: ");

  fgets(buf, 1024, stdin);

  pos = strchr(buf, '\n');
  if(pos)
    *pos = '\0';
  if(buf[0] == '\0')
    return 0;
  *user = bgav_strndup(buf, (char*)0);
    
  fprintf(stderr, "Password: ");
  echo_off();
  fgets(buf, 1024, stdin);
  echo_on();
  fprintf(stderr, "\n");

  pos = strchr(buf, '\n');
  if(pos)
    *pos = '\0';
  if(buf[0] == '\0')
    return 0;
  *pass = bgav_strndup(buf, (char*)0);
  return 1;
  }

/* Configuration data */

static int connect_timeout   = 5000;
static int read_timeout      = 5000;
static int network_bandwidth = 524300; /* 524.3 Kbps (Cable/DSL) */

static int frames_to_read  = 10;

int main(int argc, char ** argv)
  {
  int i, j;
  int num_audio_streams;
  int num_video_streams;
  int num_subtitle_streams;
  int num_urls;
  int num_tracks;
  int track;
  int arg_index;
  char * sub_text = (char *)0;
  int sub_text_alloc = 0;
  gavl_time_t sub_time;
  gavl_time_t sub_duration;
  int sample_accurate = 0;
  
  bgav_t * file;
  bgav_options_t * opt;
  gavl_overlay_t ovl;
  
  gavl_audio_frame_t * af;
  gavl_video_frame_t * vf;

  const gavl_audio_format_t * audio_format;
  const gavl_video_format_t * video_format;

  setlocale(LC_MESSAGES, "");
  
  if(argc == 1)
    {
    fprintf(stderr, "Usage: bgavdump <location>\n");

    bgav_inputs_dump();
    bgav_redirectors_dump();
    
    bgav_formats_dump();
    
    bgav_codecs_dump();

    bgav_subreaders_dump();
    
    return 0;
    }
  
  file = bgav_create();
  opt = bgav_get_options(file);

  arg_index = 1;
  while(arg_index < argc - 1)
    {
    if(!strcmp(argv[arg_index], "-s"))
      {
      sample_accurate = 1;
      arg_index++;
      }
    }
  
  /* Configure */

  bgav_options_set_connect_timeout(opt,   connect_timeout);
  bgav_options_set_read_timeout(opt,      read_timeout);
  bgav_options_set_network_bandwidth(opt, network_bandwidth);

  bgav_options_set_seek_subtitles(opt, 1);

  if(sample_accurate)
    bgav_options_set_sample_accurate(opt, 1);
  
  bgav_options_set_user_pass_callback(opt, user_pass_func, (void*)0);
  
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
              argv[argc-1] + 5);
      return -1;
      }
    }
  else if(!bgav_open(file, argv[argc-1]))
    {
    fprintf(stderr, "Could not open file %s\n",
            argv[argc-1]);
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

#ifdef TRACK  
  track = TRACK;
#else
  for(track = 0; track < num_tracks; track++)
    {
#endif
    bgav_select_track(file, track);
    if(sample_accurate && !bgav_can_seek_sample(file))
      {
      fprintf(stderr, "Sample accurate access not possible for track %d\n", track+1);
      return -1;
      }
    fprintf(stderr, "===================================\n");
    fprintf(stderr, "============ Track %3d ============\n", track+1);
    fprintf(stderr, "===================================\n");
    

    
    num_audio_streams = bgav_num_audio_streams(file, track);
    num_video_streams = bgav_num_video_streams(file, track);
    num_subtitle_streams = bgav_num_subtitle_streams(file, track);
    for(i = 0; i < num_audio_streams; i++)
      bgav_set_audio_stream(file, i, BGAV_STREAM_DECODE);
    for(i = 0; i < num_video_streams; i++)
      bgav_set_video_stream(file, i, BGAV_STREAM_DECODE);
    for(i = 0; i < num_subtitle_streams; i++)
      bgav_set_subtitle_stream(file, i, BGAV_STREAM_DECODE);
    
    fprintf(stderr, "Starting decoders...");
    if(!bgav_start(file))
      {
      fprintf(stderr, "failed\n");
#ifndef TRACK  
      continue;
#else
      return -1;
#endif
      }
    else
      fprintf(stderr, "done\n");
    
    fprintf(stderr, "Dumping file contents...\n");
    bgav_dump(file);
    fprintf(stderr, "End of file contents\n");

    /* Try to get some frames from each stream */
  
    for(i = 0; i < num_audio_streams; i++)
      {
      audio_format = bgav_get_audio_format(file, i);
      af = gavl_audio_frame_create(audio_format);

      for(j = 0; j < frames_to_read; j++)
        {
        fprintf(stderr, "Reading %d samples from audio stream %d...",
                audio_format->samples_per_frame, i+1);
        if(bgav_read_audio(file, af, i, audio_format->samples_per_frame))
          fprintf(stderr, "Done, PTS: %"PRId64", Samples: %d\n", af->timestamp, af->valid_samples);
        else
          fprintf(stderr, "Failed\n");
        }
      
      gavl_audio_frame_destroy(af);
      }
    
    for(i = 0; i < num_video_streams; i++)
      {
      video_format = bgav_get_video_format(file, i);
      vf = gavl_video_frame_create(video_format);
#if 1
      for(j = 0; j < frames_to_read; j++)
        {
        fprintf(stderr, "Reading frame from video stream %d...", i+1);
        if(bgav_read_video(file, vf, i))
          {
          fprintf(stderr, "Done, timestamp: %"PRId64", Duration: %"PRId64"\n",
                  vf->timestamp, vf->duration);
          // fprintf(stderr, "First 16 bytes of first plane follow\n");
          // bgav_hexdump(vf->planes[0] + vf->strides[0] * 20, 16, 16);
          }
        else
          {
          fprintf(stderr, "Failed\n");
          break;
          }
        }
#endif
      //      gavl_video_frame_dump(vf, video_format, "frame");
      gavl_video_frame_destroy(vf);
      }

    for(i = 0; i < num_subtitle_streams; i++)
      {
      video_format = bgav_get_subtitle_format(file, i);
      
      if(bgav_subtitle_is_text(file, i))
        {
        fprintf(stderr, "Reading text subtitle from stream %d...", i+1);

        if(bgav_read_subtitle_text(file, &sub_text, &sub_text_alloc,
                                   &sub_time, &sub_duration, i))
          {
          fprintf(stderr, "Done\nstart: %f, duration: %f\n%s\n",
                  gavl_time_to_seconds(sub_time),
                  gavl_time_to_seconds(sub_duration),
                                       sub_text);
          }
        else
          fprintf(stderr, "Failed\n");
        }
      else
        {
        fprintf(stderr, "Reading overlay subtitle from stream %d...", i+1);
        ovl.frame = gavl_video_frame_create(video_format);
        if(bgav_read_subtitle_overlay(file, &ovl, i))
          {
          fprintf(stderr, "Done\nsrc_rect: ");
          gavl_rectangle_i_dump(&ovl.ovl_rect);
          fprintf(stderr, "\ndst_coords: %d,%d\n", ovl.dst_x, ovl.dst_y);
          fprintf(stderr, "Time: %" PRId64 " -> %" PRId64 "\n",
                  ovl.frame->timestamp,
                  ovl.frame->timestamp+ovl.frame->duration);
          }
        else
          fprintf(stderr, "Failed\n");
        gavl_video_frame_destroy(ovl.frame);
        }
      }
#ifndef TRACK
    }
#endif
  if(sub_text)
    free(sub_text);
    
  bgav_close(file);
    
  return -1;
  }
