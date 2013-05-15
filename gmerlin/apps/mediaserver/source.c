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


#include <server.h>
#include <gmerlin/bgplug.h>
#include <gmerlin/http.h>

#define CLIENT_TIMEOUT 1000
#define BUF_ELEMENTS 100

#define HAVE_HEADER    (1<<0)

#define LOG_DOMAIN "server.source"

typedef struct
  {
  gavl_metadata_t m;
  int have_m; // 1 if we received a metadata update yet
  pthread_mutex_t metadata_mutex;
  
  bg_plug_t * in_plug;
  bg_mediaconnector_t conn;
  bg_plug_t * conn_plug;
  
  buffer_t * buf;
  
  int flags;
  gavf_program_header_t h;
  gavl_timer_t * timer;

  gavf_program_header_t ph;
  } source_t;

static void enable_streams(bg_plug_t * plug, gavf_stream_type_t type)
  {
  gavf_t * g;
  int num, i;
  const gavf_stream_header_t * sh;

  g = bg_plug_get_gavf(plug);

  num = gavf_get_num_streams(g, type);

  if(!num)
    return;

  for(i = 0; i < num; i++)
    {
    sh = gavf_get_stream(g, i, type);
    bg_plug_set_stream_action(plug, sh, BG_STREAM_ACTION_READRAW);
    }
  
  }

static int write_func(void * priv, const uint8_t * data, int len)
  {
  return len;
  }

static int conn_cb_func(void * priv, int type, const void * data)
  {
  buffer_element_t * el;
  source_t * p = priv;
  
  switch(type)
    {
    case GAVF_IO_CB_PROGRAM_HEADER_START:
      //      fprintf(stderr, "Got program header:\n");
      //      gavf_program_header_dump(data);

      if(!(p->flags & HAVE_HEADER))
        {
        gavf_program_header_copy(&p->ph, data);
        p->flags |= HAVE_HEADER;
        }
      break;
    case GAVF_IO_CB_PROGRAM_HEADER_END:
      break;
    case GAVF_IO_CB_PACKET_START:
      //      fprintf(stderr, "Got packet: %d\n", ((gavl_packet_t*)data)->id);
      //      gavl_packet_dump(data);

      el = buffer_get_write(p->buf);
      if(!el)
        return 0;
      
      buffer_element_set_packet(el, data);
      buffer_done_write(p->buf);
      
      break;
    case GAVF_IO_CB_PACKET_END:
      break;
    case GAVF_IO_CB_METADATA_START:
      //      fprintf(stderr, "Got metadata:\n");
      //      gavl_metadata_dump(data, 2);

      pthread_mutex_lock(&p->metadata_mutex);
      gavl_metadata_free(&p->m);
      gavl_metadata_init(&p->m);
      gavl_metadata_copy(&p->m, data);
      p->have_m = 1;
      pthread_mutex_unlock(&p->metadata_mutex);

      el = buffer_get_write(p->buf);
      if(!el)
        return 0;

      buffer_element_set_metadata(el, data);
      buffer_done_write(p->buf);

      break;
    case GAVF_IO_CB_METADATA_END:
      break;
    case GAVF_IO_CB_SYNC_HEADER_START:
      //      fprintf(stderr, "Got sync_header\n");

      if(!gavl_timer_is_running(p->timer))
        gavl_timer_start(p->timer);
      
      el = buffer_get_write(p->buf);
      if(!el)
        return 0;

      buffer_element_set_sync_header(el);
      buffer_done_write(p->buf);
      
      break;
    case GAVF_IO_CB_SYNC_HEADER_END:
      break;
    }
  
  return 1;
  }

static void source_destroy(void * data)
  {
  source_t * p = data;

  if(p->in_plug)
    bg_plug_destroy(p->in_plug);

  if(p->conn_plug)
    bg_plug_destroy(p->conn_plug);

  bg_mediaconnector_free(&p->conn);
    
  if(p->buf)
    buffer_stop(p->buf);

  if(p->timer)
    gavl_timer_destroy(p->timer);

  if(p->buf)
    buffer_destroy(p->buf);

  gavf_program_header_free(&p->ph);

  gavl_metadata_free(&p->m);

  pthread_mutex_destroy(&p->metadata_mutex);
  free(p);
  }

static void source_func(client_t * cl)
  {
  gavl_time_t conn_time;
  gavl_time_t program_time;
  
  source_t * p = cl->data;

  while(1)
    {
    /* Read packet from source */
    if(!bg_mediaconnector_iteration(&p->conn))
      break;
    
    conn_time = bg_mediaconnector_get_min_time(&p->conn);
    program_time = gavl_timer_get(p->timer);
    
    if((conn_time != GAVL_TIME_UNDEFINED) &&
       (conn_time - program_time > GAVL_TIME_SCALE/2))
      {
      gavl_time_t delay_time;
      delay_time = conn_time - program_time - GAVL_TIME_SCALE/2;
      //    fprintf(stderr, "delay %"PRId64"\n", delay_time);
      gavl_time_delay(&delay_time);
      }
    }
  }

client_t * source_client_create(int * fd, const char * path, bg_plugin_registry_t * plugin_reg,
                                int type)
  {
  gavf_io_t * io = NULL; 
  int flags;
  source_t * priv;
  client_t * ret;
  
  priv = calloc(1, sizeof(*priv));

  pthread_mutex_init(&priv->metadata_mutex, NULL);
  
  io = bg_plug_io_open_socket(*fd, BG_PLUG_IO_METHOD_READ, &flags, CLIENT_TIMEOUT);
  if(!io)
    goto fail;
  
  priv->in_plug = bg_plug_create_reader(plugin_reg);

  if(!priv->in_plug)
    goto fail;
  
  if(!bg_plug_open(priv->in_plug, io,
                   NULL, NULL, flags))
    goto fail;

  enable_streams(priv->in_plug, GAVF_STREAM_AUDIO);
  enable_streams(priv->in_plug, GAVF_STREAM_VIDEO);
  enable_streams(priv->in_plug, GAVF_STREAM_TEXT);
  enable_streams(priv->in_plug, GAVF_STREAM_OVERLAY);

  if(!bg_plug_start(priv->in_plug))
    goto fail;

  priv->buf = buffer_create(BUF_ELEMENTS);
  priv->timer = gavl_timer_create();

  /* Set up media connector */
  bg_plug_setup_reader(priv->in_plug, &priv->conn);
  bg_mediaconnector_create_conn(&priv->conn);

  /* Setup connection plug */
  io = gavf_io_create(NULL,
                      write_func,
                      NULL,
                      NULL,
                      NULL,
                      priv);

  gavf_io_set_cb(io, conn_cb_func, priv);

  priv->conn_plug = bg_plug_create_writer(plugin_reg);
  
  bg_plug_open(priv->conn_plug, io,
               bg_plug_get_metadata(priv->in_plug), NULL, 0);

  bg_plug_transfer_metadata(priv->in_plug, priv->conn_plug);
  
  if(!bg_plug_setup_writer(priv->conn_plug, &priv->conn))
    goto fail;

  bg_mediaconnector_start(&priv->conn);
  
  ret = client_create(type, *fd, priv, source_destroy, source_func);

  *fd = -1;

  return ret;
  
  fail:

  source_destroy(priv);  
  return NULL;
  }

int server_handle_source(server_t * s, int * fd,
                         const char * method,
                         const char * path_orig,
                         const gavl_metadata_t * req)
  {
  const char * var;
  gavl_metadata_t res;
  client_t * cl;
  
  if(strcmp(method, "PUT") ||
     !(var = gavl_metadata_get(req, "Content-Type")) ||
     strcmp(var, bg_plug_mimetype))
    return 0;

  gavl_metadata_init(&res);
  
  if(!strcmp(path_orig, "/") || (path_orig[0] != '/'))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid path \"%s\"", path_orig);
    bg_http_response_init(&res, "HTTP/1.1", 404, "Not Found");
    goto fail;
    }
  
  /* Send ok if everything is fine */

  if((var = gavl_metadata_get(req, "Expect")) &&
     !strcmp(var, "100-continue"))
    {
    bg_http_response_init(&res, "HTTP/1.1", 100, "Continue");
    if(!bg_http_response_write(*fd, &res))
      goto fail;
    }
  cl = source_client_create(fd, path_orig, s->plugin_reg, CLIENT_TYPE_SOURCE_STREAM);
  cl->name = gavl_strdup(path_orig + 1);
  
  server_attach_client(s, cl);
  
  fail:
  
  return 1;
  }

int server_handle_stream(server_t * s, int * fd,
                         const char * method,
                         const char * path_orig,
                         const gavl_metadata_t * req)
  {
  int i;

  source_t * sp;
  
  client_t * source = NULL;
  client_t * sink;
  
  if(strcmp(method, "GET") ||
     strncmp(path_orig, "/stream/", 8))
    return 0;
  
  path_orig += 8;

  for(i = 0; i < s->num_clients; i++)
    {
    if((s->clients[i]->type == CLIENT_TYPE_SOURCE_STREAM) &&
       s->clients[i]->name &&
       !strcmp(s->clients[i]->name, path_orig))
      {
      source = s->clients[i];
      break;
      }
    }
  if(!source)
    return 0; // 404

  sp = source->data;

  pthread_mutex_lock(&sp->metadata_mutex);
  sink = sink_client_create(fd, req, s->plugin_reg,
                            &sp->ph,
                            &sp->m,
                            sp->buf);
  pthread_mutex_unlock(&sp->metadata_mutex);

  if(sink)
    server_attach_client(s, sink);
  return 1;
  }
