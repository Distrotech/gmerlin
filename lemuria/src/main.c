#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <esd.h>
#include <unistd.h> // read

#include <math.h>

#include <lemuria.h>
#include <gavl/gavl.h>

/* Return FALSE of failure */

static int open_esd_connection(int monitor, gavl_audio_format_t * format)
  {
  int ret;
  int samplerate;
  int esd_format;

  int esd_bits, esd_channels, esd_mode, esd_func;

  esd_bits = ESD_BITS16;
  esd_channels = ESD_STEREO;
  esd_mode = ESD_STREAM;
  esd_func = ESD_MONITOR;
  
  esd_format = esd_bits | esd_channels | esd_mode | esd_func;
  
  samplerate = 44100;

  if(monitor)
    ret = esd_monitor_stream(esd_format, samplerate, 
                             (const char *)0, "lemuria");
  else
    ret = esd_record_stream(esd_format, samplerate, 
                            (const char *)0, "lemuria");

  if(ret == -1)
    {
    fprintf(stderr, "Cannot connect to esd in %s mode, too bad.\n\
I'm going away now, call be again after you fixed this\n",
            (monitor ? "monitoring" : "recoding"));
    return -1;
    }

  memset(format, 0, sizeof(*format));
  
  format->num_channels = 2;
  format->samplerate = 44100;
  format->interleave_mode = GAVL_INTERLEAVE_ALL;
  format->sample_format = GAVL_SAMPLE_S16;
  gavl_set_channel_setup(format);
  format->samples_per_frame = LEMURIA_TIME_SAMPLES;
  
  return ret;
  }

static void esd_iteration(int fd, gavl_audio_frame_t * frame)
  {
#if 0
  struct timeval timeout;
  fd_set esd_fdset;
  timeout.tv_sec = 0;
  timeout.tv_usec = 200000;

  FD_ZERO(&(esd_fdset));
  FD_SET(fd, &(esd_fdset));
 
  if(select(fd + 1, &(esd_fdset), (fd_set*)0, (fd_set*)0,
            &timeout) == 1)
    {
    fprintf(stderr, "Got samples\n");
    read(fd, (void*)frame->samples.s_16, 2048);
    frame->valid_samples = 512;
    }
  else
    {
    fprintf(stderr, "Got no samples\n");
    memset((void*)frame->samples.s_16, 0x00, 2048);
    }
#else
  int result;
  result = read(fd, (void*)frame->samples.s_16, 2048);
  frame->valid_samples = 512;
#endif
  }

int opt_monitor = 1;

static void parse_args(int argc, char ** argv)
  {
  int i;
  for(i = 1; i < argc; i++)
    {
    if(!strcmp(argv[i], "-r"))
      opt_monitor = 0;
    }
  }

int main(int argc, char ** argv)
  {
  int esd_fd;
  lemuria_engine_t * e;
  int do_convert;
  
  gavl_audio_frame_t * in_frame;
  gavl_audio_frame_t * out_frame = (gavl_audio_frame_t*)0;
  gavl_audio_converter_t * cnv;

  gavl_audio_format_t in_format;
  gavl_audio_format_t out_format;
  
  cnv = gavl_audio_converter_create();
  
  parse_args(argc, argv);
  
  if((esd_fd = open_esd_connection(opt_monitor, &in_format)) == -1)
    return -1;
  
  e = lemuria_create(NULL, 640, 480);

  gavl_audio_format_copy(&out_format, &in_format);

  
  
  lemuria_adjust_format(e, &out_format);
  
  do_convert = gavl_audio_converter_init(cnv, &in_format, &out_format);

  in_frame = gavl_audio_frame_create(&in_format);
  if(do_convert)
    out_frame = gavl_audio_frame_create(&out_format);

  gavl_audio_format_dump(&in_format);
  gavl_audio_format_dump(&out_format);
  
  lemuria_start(e);

  //  lemuria_update(e);
  
  while(1)
    {
    esd_iteration(esd_fd, in_frame);

    if(do_convert)
      {
      gavl_audio_convert(cnv, in_frame, out_frame);
      lemuria_update_audio(e, out_frame);
      }
    else
      lemuria_update_audio(e, in_frame);
    
    //    lemuria_update(e);
    }
  return 0;
  }
