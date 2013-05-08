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

#include "server.h"
#include <unistd.h> // close

#define LOG_DOMAIN "server.client"

static void client_set_status(client_t * c, int status)
  {
  pthread_mutex_lock(&c->status_mutex);
  c->status = status;
  pthread_mutex_unlock(&c->status_mutex);
  }

static void * client_func(void * priv)
  {
  client_t * c = priv;
  c->func(c);
  client_set_status(c, CLIENT_STATUS_DONE);
  return NULL;
  }
  
client_t * client_create(int type, int fd, void * data,
                         void (*free_data)(void*), void (*func)(client_t*))
  {
  client_t * ret = calloc(1, sizeof(*ret));
  pthread_mutex_init(&ret->status_mutex, NULL);
  ret->fd = fd;
  ret->free_data = free_data;
  ret->func = func;
  ret->data = data;
  ret->type = type;
  pthread_create(&ret->thread, NULL, client_func, ret);
  return ret;
  }

int client_get_status(client_t * c)
  {
  int ret;
  pthread_mutex_lock(&c->status_mutex);
  ret = c->status;
  pthread_mutex_unlock(&c->status_mutex);
  return ret;
  }


void client_destroy(client_t * c)
  {
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Closing client thread");
  shutdown(c->fd, SHUT_RDWR);
  client_set_status(c, CLIENT_STATUS_STOP);
  pthread_join(c->thread, NULL);

  if(c->free_data)
    c->free_data(c->data);
  pthread_mutex_destroy(&c->status_mutex);
  close(c->fd);
  free(c);
  }
