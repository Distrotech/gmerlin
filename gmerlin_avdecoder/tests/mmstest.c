#include <avdec_private.h>
#include <mms.h>
#include <stdio.h>

int main(int argc, char ** argv)
  {
  bgav_options_t * opt;
  
  bgav_mms_t * mms;
  char * error_msg = (char*)0;

  opt = bgav_options_create();
    
  mms = bgav_mms_open(opt, argv[1], &error_msg);

  if(!mms)
    fprintf(stderr, "bgav_mms_open failed: %s\n", error_msg);
  return 0;
  }
