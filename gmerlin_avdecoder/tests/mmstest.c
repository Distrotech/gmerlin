#include <avdec_private.h>
#include <mms.h>

int main(int argc, char ** argv)
  {
  bgav_mms_t * mms;

  mms = bgav_mms_open(argv[1], 5000, 5000);

  if(!mms)
    fprintf(stderr, "bgav_mms_open failed\n");
  return 0;
  }
