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
#include <upnp/devicepriv.h>
#include <string.h>

#include <gmerlin/upnp/soap.h>

#include <gmerlin/utils.h>
#include <gmerlin/http.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "upnp.action"

#define DUMP_SOAP

static const bg_upnp_soap_arg_t *
get_in_arg_by_id(bg_upnp_soap_request_t * req,
                 int id)
  {
  int i;
  for(i = 0; i < req->num_args_in; i++)
    {
    if(req->args_in[i].desc->id == id)
      return &req->args_in[i];
    }
  bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No input argument for ID %d", id);
  return NULL;
  }

int
bg_upnp_service_get_arg_in_int(bg_upnp_soap_request_t * req,
                               int id)
  {
  const bg_upnp_soap_arg_t * arg = get_in_arg_by_id(req, id);
  if(arg)
    return arg->val.i;
  return 0;
  }

const char *
bg_upnp_service_get_arg_in_string(bg_upnp_soap_request_t * req,
                                  int id)
  {
  const bg_upnp_soap_arg_t * arg = get_in_arg_by_id(req, id);
  if(arg)
    return arg->val.s;
  return NULL;
  }

static bg_upnp_soap_arg_t *
get_out_arg_by_id(bg_upnp_soap_request_t * req,
                 int id)
  {
  int i;
  for(i = 0; i < req->num_args_out; i++)
    {
    if(req->args_out[i].desc->id == id)
      return &req->args_out[i];
    }
  bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No output argument for ID %d", id);
  return NULL;
  }


void
bg_upnp_service_set_arg_out_int(bg_upnp_soap_request_t * req,
                                int id, int val)
  {
  bg_upnp_soap_arg_t * arg = get_out_arg_by_id(req, id);
  if(arg)
    arg->val.i = val;
  }

void
bg_upnp_service_set_arg_out_string(bg_upnp_soap_request_t * req,
                                   int id, char * val)
  {
  bg_upnp_soap_arg_t * arg = get_out_arg_by_id(req, id);
  if(arg)
    arg->val.s = val;
  }

#define TIMEOUT 500

int
bg_upnp_service_handle_action_request(bg_upnp_service_t * s, int fd,
                                      const char * method,
                                      const gavl_metadata_t * header)
  {
  gavl_metadata_t res;
  int content_length = -1;
  char * buf;
  
  gavl_metadata_init(&res);
  
  if(!strcmp(method, "POST"))
    {
    if(!gavl_metadata_get_int_i(header, "CONTENT-LENGTH", &content_length))
      {
      /* Error */
      }

    buf = malloc(content_length + 1);
    if(bg_socket_read_data(fd, (uint8_t*)buf,
                           content_length, TIMEOUT) < content_length)
      {
      /* Error */
      }
    buf[content_length] = '\0';

#ifdef DUMP_SOAP
    fprintf(stderr, "Got SOAP request\n");
    gavl_metadata_dump(header, 0);
    fprintf(stderr, "%s\n", buf);
#endif
    if(!bg_upnp_soap_request_from_xml(s, buf, content_length, header))
      {
      /* Error */
      }

    free(buf);

    if(!s->req.action->func(s))
      {
      /* Error */
      }
    
    buf = bg_upnp_soap_response_to_xml(s, &content_length);
    bg_http_response_init(&res, "HTTP/1.1", 200, "OK");
    gavl_metadata_set_int(&res, "CONTENT-LENGTH", content_length);
    gavl_metadata_set(&res, "CONTENT-TYPE", "text/xml; charset=\"utf-8\"");
    gavl_metadata_set(&res, "CONNECTION", "CLOSE");
    bg_http_header_set_date(&res, "DATE");
    bg_http_header_set_empty_var(&res, "EXT");
    gavl_metadata_set(&res, "SERVER", s->dev->server_string);
#ifdef DUMP_SOAP
    fprintf(stderr, "Sending SOAP response\n");
    gavl_metadata_dump(&res, 0);
    fprintf(stderr, "%s\n", buf);
#endif
    
    if(bg_http_response_write(fd, &res))
      bg_socket_write_data(fd, (uint8_t*)buf, content_length);
    bg_upnp_soap_request_cleanup(&s->req);
    free(buf);
    }
  else
    {
    
    }
  gavl_metadata_free(&res);
  return 1;
  }
