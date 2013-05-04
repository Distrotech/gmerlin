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

/* Prepare and write a http request (client) */

#include <gavl/metadata.h>

void bg_http_request_init(gavl_metadata_t * req,
                          const char * method,
                          const char * path,
                          const char * protocol);

int bg_http_request_write(int socket, gavl_metadata_t * req);
char * bg_http_request_to_string(const gavl_metadata_t * req);

/* Read a http request (server) */

int bg_http_request_read(int socket, gavl_metadata_t * req, int timeout);
int bg_http_request_from_string(gavl_metadata_t * req, const char * str);

const char * bg_http_request_get_protocol(gavl_metadata_t * req);
const char * bg_http_request_get_method(gavl_metadata_t * req);
const char * bg_http_request_get_path(gavl_metadata_t * req);

/* Prepare and write a response (server) */

void bg_http_response_init(gavl_metadata_t * res,
                           const char * protocol,
                           int status_int, const char * status_str);

int bg_http_response_write(int socket, gavl_metadata_t * res);
char * bg_http_response_to_string(const gavl_metadata_t * res);

/* Read a response (client) */

int bg_http_response_read(int socket, gavl_metadata_t * res, int timeout);
int bg_http_response_from_string(gavl_metadata_t * res, const char * str);

const char * bg_http_response_get_protocol(gavl_metadata_t * res);
const int bg_http_response_get_status_int(gavl_metadata_t * res);
const char * bg_http_response_get_status_str(gavl_metadata_t * res);

/* Generic utilities */

void bg_http_header_set_empty_var(gavl_metadata_t * h, const char * name);


