#include <avdec.h>

int main(int argc, char ** argv)
  {
  bgav_t * bgav;

  bgav = bgav_create();
    
  bgav_open_vcd(bgav, "/dev/cdrom");
  
  return 0;
  }
