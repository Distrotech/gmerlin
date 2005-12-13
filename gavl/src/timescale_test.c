
#include <stdio.h>
#include <inttypes.h>
#include <gavl/gavltime.h>

/* We choose really large numbers */

/* NTSC */
#define FRAMERATE_NUM 300000000
#define FRAMERATE_DEN  10010000

/* PAL */
// #define FRAMERATE_NUM 250000000
// #define FRAMERATE_DEN  10000000

#define SAMPLERATE 48000

#define SCALE_1 1000000000
#define SCALE_2 2000000000

/*
 * int64_t     gavl_time_scale(int scale, gavl_time_t time);
 * gavl_time_t gavl_time_unscale(int scale, int64_t time);
 * int64_t     gavl_time_rescale(int scale1, int scale2, int64_t time);
 * int64_t     gavl_time_to_samples(int samplerate, gavl_time_t time);
 * gavl_time_t gavl_samples_to_time(int samplerate, int64_t samples);
 * int64_t     gavl_time_to_frames(int rate_num, int rate_den, gavl_time_t time); 
 * gavl_time_t gavl_frames_to_time(int rate_num, int rate_den, int64_t frames);
 */


int main(int argc, char ** argv)
  {
  gavl_time_t time;
  int64_t samples;
  int64_t time_scaled;
  int64_t frames;
  int time_scale_1, time_scale_2;
    
  /* int64_t     gavl_time_scale(int scale, gavl_time_t time); */

  time =         (gavl_time_t)GAVL_TIME_SCALE * 10000;
  //  time_scale_1 = GAVL_TIME_SCALE * 2;
  time_scale_1 = 3333333;
  time_scaled = gavl_time_scale(time_scale_1, time);

  fprintf(stderr, "gavl_time_scale:\n");
  fprintf(stderr, "  time:          %lld\n", time);
  fprintf(stderr, "  time_scale:    %d\n", time_scale_1);
  fprintf(stderr, "  time_scaled:   %lld\n", time_scaled);
  
  /* gavl_time_t gavl_time_unscale(int scale, int64_t time); */
  fprintf(stderr, "gavl_time_unscale:\n");
  fprintf(stderr, "  time_scaled:   %lld\n", time_scaled);
  fprintf(stderr, "  time_scale:    %d\n", time_scale_1);
  fprintf(stderr, "  time_unscaled: %lld\n", gavl_time_unscale(time_scale_1, time_scaled));
  
  /* int64_t     gavl_time_rescale(int scale1, int scale2, int64_t time); */
  time_scale_2 = 6666666;

  fprintf(stderr, "gavl_time_rescale:\n");
  fprintf(stderr, "  time_1:        %lld\n", time_scaled);
  fprintf(stderr, "  time_scale_1:  %d\n", time_scale_1);
  fprintf(stderr, "  time_scale_2:  %d\n", time_scale_2);
  fprintf(stderr, "  time_rescaled: %lld\n", gavl_time_rescale(time_scale_1, time_scale_2, time_scaled));
  
  /* int64_t     gavl_time_to_samples(int samplerate, gavl_time_t time); */
  time = (gavl_time_t)GAVL_TIME_SCALE * 100000;
  samples = gavl_time_to_samples(SAMPLERATE, time);
  fprintf(stderr, "gavl_time_to_samples:\n");
  fprintf(stderr, "  time:          %lld\n", time);
  fprintf(stderr, "  samplerate:    %d\n", SAMPLERATE);
  fprintf(stderr, "  samples:       %lld\n",  samples);
  
  /* gavl_time_t gavl_samples_to_time(int samplerate, int64_t samples); */
  fprintf(stderr, "gavl_samples_to_time:\n");
  fprintf(stderr, "  samples:       %lld\n", samples);
  fprintf(stderr, "  samplerate:    %d\n", SAMPLERATE);
  fprintf(stderr, "  time:          %lld\n", gavl_samples_to_time(SAMPLERATE, samples));
  
  /* int64_t     gavl_time_to_frames(int rate_num, int rate_den, gavl_time_t time); */
  time = (gavl_time_t)GAVL_TIME_SCALE * 100000;
  frames = gavl_time_to_frames(FRAMERATE_NUM, FRAMERATE_DEN, time);

  fprintf(stderr, "gavl_time_to_frames:\n");
  fprintf(stderr, "  time:          %lld\n", time);
  fprintf(stderr, "  framerate:     %f (%d:%d)\n", (float)FRAMERATE_NUM / (float)FRAMERATE_DEN,
          FRAMERATE_NUM, FRAMERATE_DEN);
  fprintf(stderr, "  frames:        %lld\n",frames);
  
  /* gavl_time_t gavl_frames_to_time(int rate_num, int rate_den, int64_t frames); */
  fprintf(stderr, "gavl_frames_to_time:\n");

  fprintf(stderr, "  frames:        %lld\n", frames);
  fprintf(stderr, "  framerate:     %f (%d:%d)\n", (float)FRAMERATE_NUM / (float)FRAMERATE_DEN,
          FRAMERATE_NUM, FRAMERATE_DEN);
  fprintf(stderr, "  frames:        %lld\n",  
          gavl_frames_to_time(FRAMERATE_NUM, FRAMERATE_DEN, frames));
  
  return 0;
  }
