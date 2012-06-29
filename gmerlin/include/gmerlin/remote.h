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

#ifndef __BG_REMOTE_H_
#define __BG_REMOTE_H_

#include <gmerlin/parameter.h>
#include <gmerlin/msgqueue.h>

#define BG_REMOTE_PORT_BASE 10100

/* Remote server */

typedef struct bg_remote_server_s bg_remote_server_t;

bg_remote_server_t * bg_remote_server_create(int default_listen_port,
                                             char * protocol_id);

const bg_parameter_info_t * bg_remote_server_get_parameters(bg_remote_server_t *);

void bg_remote_server_set_parameter(void * data,
                                    const char * name,
                                    const bg_parameter_value_t * v);

int bg_remote_server_init(bg_remote_server_t *);

bg_msg_t * bg_remote_server_get_msg(bg_remote_server_t *);

void bg_remote_server_put_msg(bg_remote_server_t *, bg_msg_t *);

void bg_remote_server_destroy(bg_remote_server_t *);

/* Wait until all client connections are closed (use with caution) */
void bg_remote_server_wait_close(bg_remote_server_t * s);

bg_msg_t * bg_remote_server_get_msg_write(bg_remote_server_t * s);
int bg_remote_server_done_msg_write(bg_remote_server_t * s);


/* Remote client */

typedef struct bg_remote_client_s bg_remote_client_t;

/* Create a remote client */

bg_remote_client_t * bg_remote_client_create(const char * protocol_id,
                                             int read_messages);

int bg_remote_client_init(bg_remote_client_t *,
                          const char * host, int port, int milliseconds);

void bg_remote_client_close(bg_remote_client_t *);

void bg_remote_client_destroy(bg_remote_client_t * c);

bg_msg_t * bg_remote_client_get_msg_write(bg_remote_client_t * c);
int bg_remote_client_done_msg_write(bg_remote_client_t * c);

bg_msg_t * bg_remote_client_get_msg_read(bg_remote_client_t * c);

#endif // __BG_REMOTE_H_
