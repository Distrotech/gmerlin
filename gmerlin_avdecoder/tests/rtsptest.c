#include <avdec_private.h>
#include <rtsp.h>

char * url = "rtsp://rd01.t-bn.de/live/viva/viva1tv_live_adsl.rm";
// char * url = "rtsp://193.159.249.23:554/farm/*/encoder/ntv/ntv1_300.rm";


int main(int argc, char ** argv)
  {
  char * error_msg = (char*)0;
  int got_redirected = 0;
  bgav_rtsp_t * rtsp;
  rtsp = bgav_rtsp_create();

  bgav_rtsp_set_connect_timeout(rtsp, 5000);
  bgav_rtsp_set_read_timeout(rtsp, 5000);
  bgav_rtsp_set_user_agent(rtsp, "RealMedia Player Version 6.0.9.1235 (linux-2.0-libc6-i386-gcc2.95)");
  bgav_rtsp_set_network_bandwidth(rtsp, 768000);
  bgav_rtsp_open(rtsp, argv[1], &got_redirected, &error_msg);

  if(got_redirected)
    fprintf(stderr, "Got redirected\n");
  
  return 0;
  }
