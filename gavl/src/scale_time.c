#include <sys/time.h>
#include <stdlib.h>
#include <gavl.h>
//#include "colorspace.h" // Common routines
#include <stdio.h>
#include <accel.h>


#define NUM_CONVERSIONS 20

static struct timeval time_before;
static struct timeval time_after;

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


int main(int argc, char ** argv)
  {
  gavl_rectangle_t src_rect;
  gavl_rectangle_t dst_rect;
  
  int i, j, imax;
  gavl_video_scaler_t *scaler;
    
  gavl_video_format_t format, format_1;
  gavl_video_frame_t * frame, * frame_1;

  gavl_video_options_t * opt;
    
  gavl_colorspace_t csp;

  imax = gavl_num_colorspaces();
  scaler = gavl_video_scaler_create();

  opt = gavl_video_scaler_get_options(scaler);
    
  for(i = 0; i < imax; i++)
    {
    csp = gavl_get_colorspace(i);

    fprintf(stderr, "Colorspace: %s imax: %d\n", gavl_colorspace_to_string(csp), imax);
    
    src_rect.w = atoi(argv[1]);
    src_rect.h = atoi(argv[2]);
    src_rect.x = 0;
    src_rect.y = 0;
    
    dst_rect.w = atoi(argv[3]);
    dst_rect.h = atoi(argv[4]);
    dst_rect.x = 0;
    dst_rect.y = 0;
    
    format.image_width  = src_rect.w + src_rect.x;
    format.image_height = src_rect.h + src_rect.y;

    format.frame_width  = src_rect.w + src_rect.x;
    format.frame_height = src_rect.h + src_rect.y;

    format.pixel_width  = 1;
    format.pixel_height = 1;
        
    format_1.image_width  = dst_rect.w + dst_rect.x;
    format_1.image_height = dst_rect.h + dst_rect.y;

    format_1.frame_width  = dst_rect.w + dst_rect.x;
    format_1.frame_height = dst_rect.h + dst_rect.y;

    format_1.pixel_width  = 1;
    format_1.pixel_height = 1;

    format.colorspace = csp;
    format_1.colorspace = csp;
        
    frame   = gavl_video_frame_create(&format);
    frame_1 = gavl_video_frame_create(&format_1);

    gavl_video_frame_clear(frame_1, &format_1);

    /* Now, do the conversions */

    fprintf(stderr, "C-Version:\n");
    
    gavl_video_options_set_defaults(opt);
    gavl_video_options_set_scale_mode(opt, GAVL_SCALE_BILINEAR);
    gavl_video_options_set_accel_flags(opt, GAVL_ACCEL_C);
        
    if(gavl_video_scaler_init(scaler,
                              format.colorspace,
                              &src_rect, &dst_rect,
                              &format, &format_1) < 0)  // int output_height
      {
      fprintf(stderr, "No scaling routine defined\n");
      }
    else
      {
      timer_init();
      for(j = 0; j < NUM_CONVERSIONS; j++)
        {
        gavl_video_scaler_scale(scaler, frame, frame_1);
        }
      timer_stop();
      }

    fprintf(stderr, "MMX-Version:\n");
    gavl_video_options_set_defaults(opt);
    gavl_video_options_set_scale_mode(opt, GAVL_SCALE_BILINEAR);
    gavl_video_options_set_accel_flags(opt, GAVL_ACCEL_MMX);
        
    if(gavl_video_scaler_init(scaler,
                              format.colorspace,
                              &src_rect, &dst_rect,
                              &format, &format_1) < 0)  // int output_height
      {
      fprintf(stderr, "No scaling routine defined\n");
      }
    else
      {
      timer_init();
      for(j = 0; j < NUM_CONVERSIONS; j++)
        {
        gavl_video_scaler_scale(scaler, frame, frame_1);
        }
      timer_stop();
      }

    fprintf(stderr, "MMXEXT-Version:\n");
    gavl_video_options_set_defaults(opt);
    gavl_video_options_set_scale_mode(opt, GAVL_SCALE_BILINEAR);
    gavl_video_options_set_accel_flags(opt, GAVL_ACCEL_MMXEXT);
        
    if(gavl_video_scaler_init(scaler,
                              format.colorspace,
                              &src_rect, &dst_rect,
                              &format, &format_1) < 0)  // int output_height
      {
      fprintf(stderr, "No scaling routine defined\n");
      }
    else
      {
      timer_init();
      for(j = 0; j < NUM_CONVERSIONS; j++)
        {
        gavl_video_scaler_scale(scaler, frame, frame_1);
        }
      timer_stop();
      }
    gavl_video_frame_destroy(frame);
    gavl_video_frame_destroy(frame_1);
    }
  gavl_video_scaler_destroy(scaler);
  return 0;
  }
