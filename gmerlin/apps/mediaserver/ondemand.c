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

#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "server.h"
#include <gmerlin/http.h>

#define LOG_DOMAIN "ondemand"

#define CLIENT_TIMEOUT 1000

static void query_callback(void * priv, void * obj)
  {
  FILE * m3u = priv;

  switch(bg_db_object_get_type(obj))
    {
    case BG_DB_OBJECT_AUDIO_FILE:
      {
      bg_db_audio_file_t * af = obj;
      if(af->title && af->artist)
        fprintf(m3u, "#EXTINF:%d, %s - %s\r\n",
                (int)(af->file.obj.duration/GAVL_TIME_SCALE), af->artist, af->title);
      else
        fprintf(m3u, "#EXTINF:%d, %s\r\n",
                (int)(af->file.obj.duration/GAVL_TIME_SCALE), bg_db_object_get_label(obj));
      fprintf(m3u, "%s\r\n", af->file.path);
      }
      break;
    default:
      break;
    }
  }

static int open_ondemand_fd(const char * command, bg_subprocess_t ** proc)
  {
  int ret;
  int listen_fd = -1;
  bg_socket_address_t * addr;
  char addr_str[BG_SOCKET_ADDR_STR_LEN];
  char * full_command = NULL;
  
  addr = bg_socket_address_create();
  bg_socket_address_set(addr, "127.0.0.1", 0, SOCK_STREAM);

  listen_fd = bg_listen_socket_create_inet(addr, 0, 1, 0);
  bg_socket_get_address(listen_fd, addr, NULL);

  bg_socket_address_to_string(addr, addr_str);

  full_command = bg_sprintf("%s -o tcp://%s", command, addr_str);
  
  *proc = bg_subprocess_create(full_command, 0, 0, 0);

  /* Wait until the process connects */
  ret = bg_listen_socket_accept(listen_fd, -1);
  if(ret < 0)
    goto fail;
  
  if(!bg_plug_io_server_handshake(ret, BG_PLUG_IO_METHOD_READ, CLIENT_TIMEOUT))
    {
    close(ret);
    ret = -1;
    }
  
  fail:

  close(listen_fd);
  bg_socket_address_destroy(addr);
  if(full_command)
    free(full_command);

  if(ret < 0)
    {
    if(*proc)
      {
      bg_subprocess_kill(*proc, SIGTERM);
      bg_subprocess_close(*proc);
      *proc = NULL;
      }
    }
  
  return ret;
  }

static client_t * create_source(server_t * s, int64_t id, gavl_metadata_t * res)
  {
  FILE * m3u;
  bg_db_object_t * obj;
  char * m3u_name;
  char * command = NULL;
  bg_subprocess_t * proc = NULL;
  int fd;
  client_t * ret;
  gavf_io_t * io = NULL; 
  bg_plug_t * plug = NULL;
  int flags = 0;
  
  obj = bg_db_object_query(s->db, id);

  if(!obj)
    {
    bg_http_response_init(res, "HTTP/1.1", 404, "Not Found");
    goto fail;
    }
  
  switch(bg_db_object_get_type(obj))
    {
    case BG_DB_OBJECT_PLAYLIST:
      m3u_name = bg_create_unique_filename("/tmp/gmerlin-%d.m3u");
      m3u = fopen(m3u_name, "w");
      fprintf(m3u, "#EXTM3U\r\n");
      bg_db_query_children(s->db, id, query_callback, m3u, 0, 0, NULL);
      fclose(m3u);

      command = bg_sprintf("gavf-decode -loop -shuffle -m3u %s | "
                           "gavf-recompress -ac 'codec=c_lame{cbr_bitrate=320}'",
                           m3u_name);
      
      /* Open plug */
      fd = open_ondemand_fd(command, &proc);

      io = bg_plug_io_open_socket(fd, BG_PLUG_IO_METHOD_READ, &flags, CLIENT_TIMEOUT);
      if(!io)
        goto fail;
  
      plug = bg_plug_create_reader(s->plugin_reg);

      if(!plug)
        goto fail;
  
      if(!bg_plug_open(plug, io, NULL, NULL, flags))
        goto fail;
      io = NULL;
      ret = source_client_create(&fd, s->plugin_reg, plug, CLIENT_TYPE_SOURCE_ONDEMAND, proc);
      break;
    default:
      bg_http_response_init(res, "HTTP/1.1", 404, "Not Found");
      goto fail;
      break;
    }

  fail:

  if(m3u_name)
    {
    remove(m3u_name);
    free(m3u_name);
    }
  if(command)
    free(command);
  
  if(obj)
    bg_db_object_unref(obj);
  
  return ret;
  
  }

int server_handle_ondemand(server_t * s, int * fd,
                           const char * method,
                           const char * path_orig,
                           const gavl_metadata_t * req)
  {
  int i;
  client_t * source = NULL;
  client_t * sink;
  
  int64_t id;
  gavl_metadata_t res;
  
  
  if(strcmp(method, "GET") ||
     strncmp(path_orig, "/ondemand/", 10))
    return 0;

  gavl_metadata_init(&res);
  
  path_orig += 10;

  id = strtoll(path_orig, NULL, 10);

  for(i = 0; i < s->num_clients; i++)
    {
    if((s->clients[i]->type == CLIENT_TYPE_SOURCE_ONDEMAND) &&
       s->clients[i]->id == id)
      {
      source = s->clients[i];
      break;
      }
    }

  if(!source)
    {
    /* Create ondemand source */
    source = create_source(s, id, &res);
    if(!source)
      goto fail;
    source->id = id;
    server_attach_client(s, source);
    }

  sink = sink_create_from_source(s, fd, source, req);
  server_attach_client(s, sink);
  
  fail:
  return 1;
  }
