#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <gavl/gavl.h>

#include <framecache.h>

struct bg_nle_frame_cache_s
  {
  bg_nle_frame_cache_entry_t * entries;
  int num_entries;
  int entries_alloc;
  int max_size;
  int num_consumers;
  
  int64_t start_seq;
  
  void (*create_callback)(void*, bg_nle_frame_cache_entry_t*e);
  void (*destroy_callback)(void*, bg_nle_frame_cache_entry_t*e);
  void * callback_data;
  };

bg_nle_frame_cache_t *
bg_nle_frame_cache_create(int max_cache_size, int num_consumers,
                          void (*create_callback)(void*, bg_nle_frame_cache_entry_t*e),
                          void (*destroy_callback)(void*, bg_nle_frame_cache_entry_t*e),
                          void * callback_data)
  {
  bg_nle_frame_cache_t * ret = calloc(1, sizeof(*ret));

  ret->create_callback = create_callback;
  ret->destroy_callback = destroy_callback;
  ret->callback_data = callback_data;
  ret->max_size = max_cache_size;
  ret->num_consumers = num_consumers;
  
  return ret;
  }

void bg_nle_frame_cache_destroy(bg_nle_frame_cache_t * c)
  {
  free(c);
  }

bg_nle_frame_cache_entry_t * bg_nle_frame_cache_get(bg_nle_frame_cache_t * c, int64_t seq)
  {
  bg_nle_frame_cache_entry_t * ret;
  if(seq < c->start_seq)
    {
    /* Shouldn't happen */
    return NULL;
    }
  while(seq >= c->start_seq + c->num_entries)
    {
    /* Remove frames */
    while(c->num_entries > c->max_size)
      {
      void * frame;
      if(c->entries[0].refcount)
        break;

      frame = c->entries[0].frame;

      memmove(&c->entries[0], &c->entries[1], sizeof(c->entries[0]) * (c->num_entries-1));
      c->num_entries--;
      c->entries[c->num_entries].frame = frame;
      c->start_seq++;
      }
    
    if(c->num_entries + 1 > c->entries_alloc)
      {
      c->entries_alloc += 10;
      c->entries = realloc(c->entries, c->entries_alloc * sizeof(*c->entries));
      memset(c->entries + c->num_entries, 0,
             (c->entries_alloc - c->num_entries) * sizeof(*c->entries));
      }
    /* Read new frame */
    c->create_callback(c->callback_data, &c->entries[c->num_entries]);
    
    c->entries[c->num_entries].refcount = c->num_consumers;
    c->num_entries++;
    }
  ret = &c->entries[seq - c->start_seq];
  ret->refcount--;
  return ret;
  }

void bg_nle_frame_cache_flush(bg_nle_frame_cache_t * c, int64_t time)
  {
  
  }
