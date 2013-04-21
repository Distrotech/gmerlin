#include <gmerlin/bgsocket.h>
#include <stdio.h>
#include <gavl/gavl.h>
#include <gavl/utils.h>

#define BUF_LEN 2048
int main(int argc, char ** argv)
  {
  int len;
  bg_host_address_t * addr = bg_host_address_create();
  bg_host_address_t * mcast = bg_host_address_create();
  bg_host_address_t * sender = bg_host_address_create();

  uint8_t buf[BUF_LEN];
  int fd;
  
  if(!bg_host_address_set(mcast, "239.255.255.250", 1900, SOCK_DGRAM))
    return -1;
  
  fd = bg_udp_socket_create_multicast(mcast);

  if(fd < 0)
    return -1;
  
  while(1)
    {
    //    if(bg_socket_can_read(fd, -1))
    //      {
      len = bg_udp_socket_receive(fd, buf, BUF_LEN, sender);

      if(len)
        {
        fprintf(stderr, "Got SSDP message:\n");
        fwrite(buf, 1, len, stderr);

        //        gavl_hexdump(buf, len, 16);
        }
      //      }
    }
  
  }
