#include <avdec_private.h>
#include <mms.h>
#include <stdio.h>

int main(int argc, char ** argv)
  {
  bgav_options_t * opt;
  
  bgav_mms_t * mms;

  opt = bgav_options_create();
    
  mms = bgav_mms_open(opt, argv[1]);

  if(!mms)
    fprintf(stderr, "bgav_mms_open failed\n");
  return 0;
  }
