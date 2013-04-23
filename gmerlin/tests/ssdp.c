#include <stdlib.h>

#include <gavl/gavl.h>
#include <gavl/metadata.h>
#include <gmerlin/upnp/ssdp.h>

int main(int argc, char ** argv)
  {
  gavl_time_t delay_time = GAVL_TIME_SCALE / 100;
  bg_ssdp_t * s = bg_ssdp_create(NULL, 1);
  
  while(1)
    {
    bg_ssdp_update(s);
    gavl_time_delay(&delay_time);
    }
  }
