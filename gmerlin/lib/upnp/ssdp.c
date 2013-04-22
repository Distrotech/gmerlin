/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <config.h>
#include <stdlib.h>
#include <gavl/gavl.h>
#include <gavl/metadata.h>

#include <gmerlin/upnp/ssdp.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "ssdp"

#define DUMP_UDP

#define UDP_BUFFER_SIZE 2048

#if 0
typedef struct bg_ssdp_service_s
  {
  gavl_metadata_t m;
  } bg_ssdp_service_t;

typedef struct bg_ssdp_device_s
  {
  gavl_metadata_t m;

  int num_devices;
  bg_ssdp_device_t * devices;
  
  int num_services;
  bg_ssdp_service_t * services;
  } bg_ssdp_device_t;
#endif

struct bg_ssdp_s
  {
  int mcast_fd;
  int ucast_fd;
  gavl_timer_t * timer;
  bg_socket_address_t * sender_addr;
  bg_socket_address_t * local_addr;
  bg_socket_address_t * mcast_addr;
  uint8_t buf[UDP_BUFFER_SIZE];
  };

static void parse_request(char * buffer, gavl_metadata_t * m)
  {

  }

static char * search_string =
"M-SEARCH * HTTP/1.1\r\n"
"Host:239.255.255.250:1900\r\n"
"ST: ssdp:all\r\n"
"Man:\"ssdp:discover\"\r\n"
"MX:3\r\n\r\n";

bg_ssdp_t * bg_ssdp_create()
  {
  char addr_str[BG_SOCKET_ADDR_STR_LEN];

  bg_ssdp_t * ret = calloc(1, sizeof(*ret));


  ret->sender_addr = bg_socket_address_create();
  ret->local_addr  = bg_socket_address_create();
  ret->mcast_addr  = bg_socket_address_create();

  if(!bg_socket_address_set_local(ret->local_addr, 0))
    goto fail;

  if(!bg_socket_address_set(ret->mcast_addr, "239.255.255.250", 1900, SOCK_DGRAM))
    goto fail;

  ret->mcast_fd = bg_udp_socket_create_multicast(ret->mcast_addr);
  ret->ucast_fd = bg_udp_socket_create(ret->local_addr);
  
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Unicast socket bound at %s", 
         bg_socket_address_to_string(ret->local_addr, addr_str));

  /* Send seach packet */
  bg_udp_socket_send(ret->ucast_fd, (uint8_t*)search_string, strlen(search_string), ret->mcast_addr);

  return ret;

  fail:
  bg_ssdp_destroy(ret);
  return NULL;
  }

void bg_ssdp_update(bg_ssdp_t * s)
  {
  int len;
#ifdef DUMP_UDP
  char addr_str[BG_SOCKET_ADDR_STR_LEN];
#endif
  /* Read multicast messages */
  while(bg_socket_can_read(s->mcast_fd, 0))
    {
    len = bg_udp_socket_receive(s->mcast_fd, s->buf, UDP_BUFFER_SIZE, s->sender_addr);
    if(len <= 0)
      continue;
    s->buf[len] = '\0';
#ifdef DUMP_UDP
    fprintf(stderr, "Got SSDP multicast message from %s\n%s", bg_socket_address_to_string(s->sender_addr, addr_str), (char*)s->buf);
#endif
    }

  /* Read unicast messages */
  while(bg_socket_can_read(s->ucast_fd, 0))
    {
    len = bg_udp_socket_receive(s->ucast_fd, s->buf, UDP_BUFFER_SIZE, s->sender_addr);
    if(len <= 0)
      continue;
    s->buf[len] = '\0';
#ifdef DUMP_UDP
    fprintf(stderr, "Got SSDP unicast message from %s\n%s", bg_socket_address_to_string(s->sender_addr, addr_str), (char*)s->buf);
#endif
    }


  }

void bg_ssdp_destroy(bg_ssdp_t * s)
  {
  if(s->local_addr) 
    bg_socket_address_destroy(s->local_addr);
  if(s->mcast_addr) 
    bg_socket_address_destroy(s->mcast_addr);
  if(s->sender_addr) 
    bg_socket_address_destroy(s->sender_addr);
  }

