#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <accelerator.h>
#include <keycodes.h>

struct bg_accelerator_map_s
  {
  int num;
  int num_alloc;
  bg_accelerator_t * accels;
  };

void
bg_accelerator_map_append(bg_accelerator_map_t * m,
                          int key, int mask, int id)
  {
  if(m->num_alloc <= m->num + 2)
    {
    m->num_alloc = m->num + 100;
    m->accels = realloc(m->accels, m->num_alloc * sizeof(*m->accels));
    }
  m->accels[m->num].key = key;
  m->accels[m->num].mask = mask;
  m->accels[m->num].id = id;
  
  m->num++;
  m->accels[m->num].key = BG_KEY_NONE;
  }

/* Array is terminated with BG_KEY_NONE */

void
bg_accelerator_map_append_array(bg_accelerator_map_t * m,
                                const bg_accelerator_t * tail)
  {
  int num_append = 0;
  while(tail[num_append].key != BG_KEY_NONE)
    num_append++;
  
  if(m->num_alloc <= m->num + num_append + 1)
    {
    m->num_alloc = m->num + num_append + 100;
    m->accels = realloc(m->accels, m->num_alloc * sizeof(*m->accels));
    }
  memcpy(m->accels + m->num, tail, num_append * sizeof(*m->accels));
  m->num += num_append;
  m->accels[m->num].key = BG_KEY_NONE;
  }

const bg_accelerator_t * 
bg_accelerator_map_get_accels(const bg_accelerator_map_t * m)
  {
  return m->accels;
  }
     
void
bg_accelerator_map_destroy(bg_accelerator_map_t * m)
  {
  if(m->accels)
    free(m->accels);
  free(m);
  }

bg_accelerator_map_t *
bg_accelerator_map_create()
  {
  bg_accelerator_map_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  ret->num_alloc = 1;
  ret->accels = calloc(ret->num_alloc, sizeof(*ret->accels));
  ret->accels[0].key = BG_KEY_NONE;
  
  return ret;
  }

static int find_by_id(const bg_accelerator_map_t * m, int id)
  {
  int i;
  for(i = 0; i < m->num; i++)
    {
    if(m->accels[i].id == id)
      return i;
    }
  return -1;
  }

static int find_by_key(const bg_accelerator_map_t * m, int key, int mask)
  {
  int i;
  for(i = 0; i < m->num; i++)
    {
    if((m->accels[i].key == key) &&
       (m->accels[i].mask == mask))
      return i;
    }
  return -1;
  }

void
bg_accelerator_map_remove(bg_accelerator_map_t * m, int id)
  {
  int index;
  index = find_by_id(m, id);
  if(index < 0)
    return;
  if(index < m->num - 1)
    {
    memmove(m->accels + index,
            m->accels + index + 1,
            sizeof(*m->accels) * (m->num - 1 - index));
    }
  m->num--;
  }

int
bg_accelerator_map_has_accel(const bg_accelerator_map_t * m,
                             int key, int mask, int * id)
  {
  int result;
  result = find_by_key(m, key, mask);
  if(result >= 0)
    {
    if( id) *id = m->accels[result].id;
    return 1;
    }
  return 0;
  }

int
bg_accelerator_map_has_accel_with_id(const bg_accelerator_map_t * m,
                                     int id)
  {
  if(find_by_id(m, id) < 0)
    return 0;
  return 1;
  }


void
bg_accelerator_map_clear(bg_accelerator_map_t * m)
  {
  m->num = 0;
  }
