#include <avdec_private.h>

char * url = "rtsp://rd01.t-bn.de/live/viva/viva1tv_live_adsl.rm";
// char * url = "rtsp://193.159.249.23:554/farm/*/encoder/ntv/ntv1_300.rm";


int main(int argc, char ** argv)
  {
  bgav_rtsp_t * rtsp;

  rtsp = bgav_rtsp_open(argv[1],
                        5000,
                        "RealMedia Player Version 6.0.9.1235 (linux-2.0-libc6-i386-gcc2.95)");
  return 0;
  }
