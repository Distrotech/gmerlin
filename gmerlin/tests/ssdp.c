#include <gmerlin/bgsocket.h>
#include <stdio.h>
#include <stdlib.h>

#include <gavl/gavl.h>
#include <gavl/utils.h>

#define BUF_LEN 2048
int main(int argc, char ** argv)
  {
  char * addr_str;

  int len;
  bg_socket_address_t * addr = bg_socket_address_create();
  bg_socket_address_t * mcast = bg_socket_address_create();
  bg_socket_address_t * sender = bg_socket_address_create();

  uint8_t buf[BUF_LEN];
  int fd;
  
  if(!bg_socket_address_set(mcast, "239.255.255.250", 1900, SOCK_DGRAM))
    return -1;

  if(!bg_socket_address_set_local(addr, 0, SOCK_DGRAM))
    return -1;

  addr_str = bg_socket_address_to_string(addr);
  fprintf(stderr, "Local address %s\n", addr_str);
  free(addr_str);
  
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
        addr_str = bg_socket_address_to_string(sender);
        fprintf(stderr, "Got SSDP message from %s\n", addr_str);
        fwrite(buf, 1, len, stderr);
        free(addr_str);
        
        //        gavl_hexdump(buf, len, 16);
        }
      //      }
    }
  
  }
