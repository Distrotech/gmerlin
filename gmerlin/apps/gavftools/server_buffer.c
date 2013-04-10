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

#include "gavf-server.h"
#include <stdlib.h>
#include <string.h>

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

buffer_element_t * buffer_element_create()
  {
  buffer_element_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void buffer_element_destroy(buffer_element_t * el)
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
  
  for(i = 0; i < ret->elements_alloc; i++)
    ret->elements[i] = buffer_element_create();
  
  return ret;
  }

void buffer_destroy(buffer_t * b)
  {
  int i;
  for(i = 0; i < b->elements_alloc; i++)
    buffer_element_destroy(b->elements[i]);
  free(b->elements);
  free(b);
  }

buffer_element_t * buffer_get_write(buffer_t * b)
  {
  buffer_element_t * ret;

  if(b->num_elements == b->elements_alloc)
    return NULL;

  ret = b->elements[b->num_elements];
  b->num_elements++;

  ret->seq = b->seq++;
  
  return ret;
  }

void buffer_advance(buffer_t * b)
  {
  buffer_element_t * el;
  
  el = b->elements[0];

  memmove(b->elements, b->elements + 1,
          (b->elements_alloc - 1) * sizeof(*b->elements));

  b->elements[b->elements_alloc-1] = el;
  
  b->num_elements--;
  }

buffer_element_t *
buffer_get_read(buffer_t * b, uint64_t seq)
  {
  int idx = seq - b->elements[0]->seq;

  if(idx >= b->num_elements)
    return NULL;
  
  return b->elements[idx];
  }
