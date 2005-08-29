#include <avdec_private.h>
#include <mms.h>
#include <stdio.h>

int main(int argc, char ** argv)
  {
  bgav_mms_t * mms;
  char * error_msg = (char*)0;

  mms = bgav_mms_open(argv[1], 5000, 5000, &error_msg);

  if(!mms)
    fprintf(stderr, "bgav_mms_open failed: %s\n", error_msg);
  return 0;
  }
