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
#include <stdlib.h>
#include <string.h>

#define LOG_DOMAIN "server.buffer"


static buffer_element_t * buffer_element_create();
static void buffer_element_destroy(buffer_element_t *);

struct buffer_s
  {
  int elements_alloc;
  int num_elements;
  buffer_element_t ** elements;

  int64_t seq;
  
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  };

buffer_element_t * buffer_get_first(buffer_t * b)
  {
  return b->num_elements ? b->elements[0] : NULL;
  }

void buffer_element_set_sync_header(buffer_element_t * el)
  {
  el->type = BUFFER_TYPE_SYNC_HEADER;
  }

void buffer_element_set_packet(buffer_element_t * el,
                               const gavl_packet_t * p)
  {
  gavl_packet_reset(&el->p);
  gavl_packet_copy(&el->p, p);
  el->type = BUFFER_TYPE_PACKET;
  }

void buffer_element_set_metadata(buffer_element_t * el,
                                 const gavl_metadata_t * m)
  {
  gavl_metadata_free(&el->m);
  gavl_metadata_init(&el->m);
  gavl_metadata_copy(&el->m, m);
  el->type = BUFFER_TYPE_METADATA;
  }

static buffer_element_t * buffer_element_create()
  {
  buffer_element_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void buffer_element_destroy(buffer_element_t * el)
  {
  gavl_metadata_free(&el->m);
  gavl_packet_free(&el->p);
  free(el);
  }

buffer_t * buffer_create(int num_elements)
  {
  int i;
  buffer_t * ret = calloc(1, sizeof(*ret));

  ret->elements_alloc = num_elements;
  ret->elements = calloc(ret->elements_alloc, sizeof(*ret->elements));

  pthread_mutex_init(&ret->mutex, NULL);
  pthread_cond_init(&ret->cond, NULL);
  
  for(i = 0; i < ret->elements_alloc; i++)
    ret->elements[i] = buffer_element_create();
  
  return ret;
  }

void buffer_destroy(buffer_t * b)
  {
  pthread_mutex_destroy(&b->mutex);
  pthread_cond_destroy(&b->cond);
  
  free(b);
  }

void buffer_stop(buffer_t * b)
  {
  int i;

  pthread_mutex_lock(&b->mutex);

  for(i = 0; i < b->elements_alloc; i++)
    buffer_element_destroy(b->elements[i]);
  free(b->elements);
  b->elements = NULL;
  
  pthread_cond_broadcast(&b->cond);
  pthread_mutex_unlock(&b->mutex);
  }

buffer_element_t * buffer_get_write(buffer_t * b)
  {
  buffer_element_t * ret;

  pthread_mutex_lock(&b->mutex);
  if(b->num_elements >= b->elements_alloc)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Buffer full");
    pthread_mutex_unlock(&b->mutex);
    return NULL;
    }
  ret = b->elements[b->num_elements];
  ret->seq = b->seq++;

  //  fprintf(stderr, "get_write %d %"PRId64"\n", b->num_elements,
  //          ret->seq);
  
  pthread_mutex_unlock(&b->mutex);
  
  return ret;
  }

void buffer_done_write(buffer_t * b)
  {
  pthread_mutex_lock(&b->mutex);
  
  b->num_elements++;
  pthread_cond_broadcast(&b->cond);
  
  pthread_mutex_unlock(&b->mutex);
  }

void buffer_advance(buffer_t * b)
  {
  buffer_element_t * el;
  //  fprintf(stderr, "buffer_advance\n");
  pthread_mutex_lock(&b->mutex);
  el = b->elements[0];

  memmove(b->elements, b->elements + 1,
          (b->elements_alloc - 1) * sizeof(*b->elements));

  b->elements[b->elements_alloc-1] = el;
  
  b->num_elements--;
  pthread_mutex_unlock(&b->mutex);
  }

int buffer_get_free(buffer_t * b)
  {
  int ret;
  pthread_mutex_lock(&b->mutex);
  ret = b->elements_alloc - b->num_elements;
  pthread_mutex_unlock(&b->mutex);
  return ret;
  }

int64_t buffer_get_start_seq(buffer_t * b)
  {
  int64_t ret;

  pthread_mutex_lock(&b->mutex);
  
  ret = b->num_elements ?
    b->elements[0]->seq : b->seq;

  pthread_mutex_unlock(&b->mutex);
  return ret;
  }


int buffer_get_read(buffer_t * b, int64_t * seq, buffer_element_t ** el)
  {
  int idx;
  
  pthread_mutex_lock(&b->mutex);
  
  if(!b->elements)
    return 0;
    
  if(*seq < b->elements[0]->seq)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Tried to read expired buffer element");
    *seq = b->elements[0]->seq;
    }

  idx = *seq - b->elements[0]->seq;
  
  if(idx < b->num_elements)
    {
    if(b->elements[idx]->seq != *seq)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Got wrong buffer element: Wanted %"PRId64", got %"PRId64,
             *seq, b->elements[idx]->seq);
    
    //    fprintf(stderr, "get_read %d %"PRId64"\n", idx,
    //            b->elements[idx]->seq);
    *el = b->elements[idx];
    (*seq)++;
    pthread_mutex_unlock(&b->mutex);

    return 1;
    }
  
  pthread_mutex_unlock(&b->mutex);
  
  return 1;
  }

void buffer_wait(buffer_t * b)
  {
  pthread_mutex_lock(&b->mutex);
  pthread_cond_wait(&b->cond, &b->mutex);
  pthread_mutex_unlock(&b->mutex);
  }
