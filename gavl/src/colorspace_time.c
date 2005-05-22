#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include <gavl.h>

//#include "colorspace.h"

static struct timeval time_before;
static struct timeval time_after;

#define NUM_CONVERSIONS 20

#define FIXED_INPUT_COLORSPACE
#define INPUT_COLORSPACE GAVL_RGBA_32


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
  
  int j, k;

  int num_colorspaces = gavl_num_colorspaces();
  
  gavl_video_format_t input_format;
  gavl_video_format_t output_format;

  gavl_video_frame_t * input_frame;
  gavl_video_frame_t * output_frame;

  gavl_video_options_t * opt;
  
  gavl_video_converter_t * cnv = gavl_video_converter_create();
  opt = gavl_video_converter_get_options(cnv);
  input_format.colorspace = GAVL_RGB_15;
  output_format.colorspace = GAVL_RGB_15;

  input_format.image_width = width;
  input_format.image_height = height;

  input_format.frame_width = width;
  input_format.frame_height = height;

  input_format.pixel_width = 1;
  input_format.pixel_height = 1;

  output_format.image_width = width;
  output_format.image_height = height;

  output_format.frame_width = width;
  output_format.frame_height = height;

  output_format.pixel_width = 1;
  output_format.pixel_height = 1;

  //  char colorspace_buffer[20];
  
#ifndef FIXED_INPUT_COLORSPACE
  for(i = 0; i < num_colorspaces; i++) /* Input format loop */
    {
    input_format.colorspace = gavl_get_colorspace(i);
#else
    input_format.colorspace = INPUT_COLORSPACE;
#endif    
    input_frame = gavl_video_frame_create(&input_format);
    gavl_video_frame_clear(input_frame, &input_format);
    for(j = 0; j < num_colorspaces; j++) /* Output format loop */
      {
      output_format.colorspace = gavl_get_colorspace(j);
      if(input_format.colorspace != output_format.colorspace)
        {
        output_frame = gavl_video_frame_create(&output_format);
        fprintf(stderr, "************* Colorspace conversion ");

        tmp = gavl_colorspace_to_string(input_format.colorspace);
        fprintf(stderr, "%s -> ", tmp);
        tmp = gavl_colorspace_to_string(output_format.colorspace);
        fprintf(stderr, "%s *************\n", tmp);

        gavl_video_options_set_defaults(opt);
        gavl_video_options_set_alpha_mode(opt, GAVL_ALPHA_BLEND_COLOR);
        
        gavl_video_options_set_accel_flags(opt, GAVL_ACCEL_C);
        
        if(gavl_video_converter_init(cnv, &input_format, &output_format) < 1)
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

        gavl_video_options_set_accel_flags(opt, GAVL_ACCEL_MMX);

        if(gavl_video_converter_init(cnv, &input_format, &output_format) < 1)
          fprintf(stderr, "No Conversion defined yet\n");
        else
          {
          timer_init();
          for(k = 0; k < NUM_CONVERSIONS; k++)
            gavl_video_convert(cnv, input_frame, output_frame);
          timer_stop();
          }
        
        /* Now, initialize with MMXEXT */

        gavl_video_options_set_accel_flags(opt, GAVL_ACCEL_MMXEXT);
        fprintf(stderr, "MMXEXT Version: ");
        
        if(gavl_video_converter_init(cnv, &input_format, &output_format) < 1)
          fprintf(stderr, "No Conversion defined yet\n");
        else
          {
          timer_init();
          for(k = 0; k < NUM_CONVERSIONS; k++)
            gavl_video_convert(cnv, input_frame, output_frame);
          timer_stop();
          }
        gavl_video_frame_destroy(output_frame);
        }
      }
    gavl_video_frame_destroy(input_frame);
#ifndef FIXED_INPUT_COLORSPACE
    }
#endif
  return 0;
  }
