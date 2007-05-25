#include <stdlib.h>
#include <gavl.h>
//#include "colorspace.h" // Common routines
#include <stdio.h>
#include <pngutil.h>
#include <accel.h>

#define IN_X 0
#define IN_Y 0

#define OUT_X 10
#define OUT_Y 10




static float coeffs_h[] = 
{ 0.2, 0.4, 0.6, 0.8, 1.0, 0.8, 0.6, 0.4, 0.2 };
static int num_coeffs_h = sizeof(coeffs_h)/sizeof(coeffs_h[0]);

static float * coeffs_v = coeffs_h;
static int num_coeffs_v = sizeof(coeffs_h)/sizeof(coeffs_h[0]);

int main(int argc, char ** argv)
  {
  char filename_buffer[1024];
  int i, imax;
  gavl_video_scaler_t *scaler;
    
  gavl_video_format_t format, format_1;
  gavl_video_frame_t * frame, * frame_1;

  gavl_video_options_t * opt;
    
  gavl_pixelformat_t csp;

  memset(&format, 0, sizeof(format));
  memset(&format_1, 0, sizeof(format_1));
  
  imax = gavl_num_pixelformats();
  scaler = gavl_video_scaler_create();

  opt = gavl_video_scaler_get_options(scaler);

  //  imax = 1;
  
  for(i = 0; i < imax; i++)
    {
    csp = gavl_get_pixelformat(i);
    
    //    csp = GAVL_RGB_24;
    
    fprintf(stderr, "Pixelformat: %s\n", gavl_pixelformat_to_string(csp));
        
    frame = read_png(argv[1], &format, csp);

    if(csp == GAVL_RGBA_64)
      {
      write_png("test.png", &format, frame);
      }

#if 0
    /* Write test frame */
    sprintf(filename_buffer, "%s-test.png", gavl_pixelformat_to_string(csp));
    write_png(filename_buffer, &format, frame);
#endif   
    gavl_video_format_copy(&format_1, &format);
    
    gavl_video_options_set_defaults(opt);

    gavl_video_options_set_conversion_flags(opt,
                                            GAVL_CONVOLVE_NORMALIZE | GAVL_CONVOLVE_CHROMA);
    gavl_video_options_set_accel_flags(opt, GAVL_ACCEL_C);

#if 0    
    if(gavl_video_scaler_init_convolve(scaler,
                              &format, num_coeffs_h / 2,
                              coeffs_h, 
                              num_coeffs_v / 2,
                              coeffs_v) < 0)  
      {
      fprintf(stderr, "No scaling routine defined\n");
      continue;
      }
#endif

#if 1
    if(gavl_video_scaler_init_convolve(scaler,
                              &format, 0, /* num_coeffs_h / 2, */
                              coeffs_h, 
                              num_coeffs_v / 2,
                              coeffs_v) < 0)  
      {
      fprintf(stderr, "No scaling routine defined\n");
      continue;
      }
#endif
    

    frame_1 = gavl_video_frame_create(&format_1);

    gavl_video_frame_clear(frame_1, &format_1);
    
    gavl_video_scaler_scale(scaler, frame, frame_1);

    sprintf(filename_buffer, "%s-convolved.png", gavl_pixelformat_to_string(csp));
    write_png(filename_buffer, &format_1, frame_1);
    fprintf(stderr, "Wrote %s\n", filename_buffer);
    gavl_video_frame_destroy(frame);
    gavl_video_frame_destroy(frame_1);
    }
  gavl_video_scaler_destroy(scaler);
  return 0;
  }
