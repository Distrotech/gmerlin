#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "../include/gavl.h"

//#include "colorspace.h"

static struct timeval time_before;
static struct timeval time_after;

#define NUM_CONVERSIONS 20

void timer_init()
  {
  gettimeofday(&time_before, (struct timezone*)0);
  }

void timer_stop()
  {
  double before, after, diff;
  
  gettimeofday(&time_after, (struct timezone*)0);

  before = time_before.tv_sec + time_before.tv_usec / 1.0e6;
  after  = time_after.tv_sec  + time_after.tv_usec  / 1.0e6;

/*   fprintf(stderr, "Before: %f After: %f\n", before, after); */
    
  diff = after - before;
    
  fprintf(stderr, "Made %d conversions, Time: %f (%f per conversion)\n",
          NUM_CONVERSIONS, diff, diff/NUM_CONVERSIONS);
  }

int main()
  {
  int width = 720;
  int height = 576;

  const char * tmp;
  
  int i, j, k;

  int num_colorspaces = gavl_num_colorspaces();
  
  gavl_video_format_t input_format;
  gavl_video_format_t output_format;

  gavl_video_frame_t * input_frame;
  gavl_video_frame_t * output_frame;

  gavl_video_options_t opt;
  
  gavl_video_converter_t * cnv = gavl_create_video_converter();

  input_format.colorspace = GAVL_RGB_15;
  output_format.colorspace = GAVL_RGB_15;

  input_format.width = width;
  input_format.height = height;

  output_format.width = width;
  output_format.height = height;

  //  char colorspace_buffer[20];
  
  for(i = 0; i < num_colorspaces; i++) /* Input format loop */
    {
    input_format.colorspace = gavl_get_colorspace(i);
    input_frame = gavl_create_video_frame(&input_format);
    
    for(j = 0; j < num_colorspaces; j++) /* Output format loop */
      {
      output_format.colorspace = gavl_get_colorspace(j);
      if(input_format.colorspace != output_format.colorspace)
        {
        output_frame = gavl_create_video_frame(&output_format);
        fprintf(stderr, "************* Colorspace conversion ");

        tmp = gavl_colorspace_to_string(input_format.colorspace);
        fprintf(stderr, "%s -> ", tmp);
        tmp = gavl_colorspace_to_string(output_format.colorspace);
        fprintf(stderr, "%s *************\n", tmp);

        gavl_video_default_options(&opt);
        opt.accel_flags |= GAVL_ACCEL_C;
        
        if(!gavl_video_init(cnv, &opt, &input_format, &output_format))
          fprintf(stderr, "No Conversion defined yet\n");
        else
          {
          /* Now, do some conversions */
          fprintf(stderr, "ANSI C Version: ");
          timer_init();
          for(k = 0; k < NUM_CONVERSIONS; k++)
            gavl_video_convert(cnv, input_frame, output_frame);
          timer_stop();
          }
        
        /* Now, initialize with MMX */

        fprintf(stderr, "MMX Version:    ");

        opt.accel_flags = GAVL_ACCEL_MMX;

        if(!gavl_video_init(cnv, &opt, &input_format, &output_format))
          fprintf(stderr, "No Conversion defined yet\n");
        else
          {
          timer_init();
          for(k = 0; k < NUM_CONVERSIONS; k++)
            gavl_video_convert(cnv, input_frame, output_frame);
          timer_stop();
          }
        
        /* Now, initialize with MMXEXT */

        opt.accel_flags = GAVL_ACCEL_MMX;
        fprintf(stderr, "MMXEXT Version: ");
        
        if(!gavl_video_init(cnv, &opt, &input_format, &output_format))
          fprintf(stderr, "No Conversion defined yet\n");
        else
          {
          timer_init();
          for(k = 0; k < NUM_CONVERSIONS; k++)
            gavl_video_convert(cnv, input_frame, output_frame);
          timer_stop();
          }
        gavl_destroy_video_frame(output_frame);
        }
      }
    gavl_destroy_video_frame(input_frame);
    }
  return 0;
  }
