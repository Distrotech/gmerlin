#include <string.h>

#include <avdec_private.h>
#include <rtsp.h>
#include <stdio.h>


char * url = "rtsp://rd01.t-bn.de/live/viva/viva1tv_live_adsl.rm";
// char * url = "rtsp://193.159.249.23:554/farm/*/encoder/ntv/ntv1_300.rm";

int main(int argc, char ** argv)
  {
  int got_redirected = 0;
  bgav_rtsp_t * rtsp;

  bgav_options_t opt;

  memset(&opt, 0, sizeof(opt));

  bgav_options_set_network_bandwidth(&opt, 768000);
  
  bgav_options_set_connect_timeout(&opt, 5000);
  bgav_options_set_read_timeout(&opt, 5000);
  
  rtsp = bgav_rtsp_create(&opt);

  bgav_rtsp_open(rtsp, argv[1], &got_redirected);

  if(got_redirected)
    fprintf(stderr, "Got redirected\n");
  
  return 0;
  }
