#include <stdlib.h>
#include <string.h>

#include <gavfprivate.h>

void gavf_sync_index_init(gavf_sync_index_t * idx, int num_streams)
  {
  idx->num_streams = num_streams;
  idx->pts_len =
    idx->num_streams * sizeof(int64_t);
  }

void gavf_sync_index_add(gavf_sync_index_t * idx,
                         uint64_t pos, int64_t * pts)
  {
  if(idx->num_entries >= idx->entries_alloc)
    {
    idx->entries_alloc += 1024;
    idx->entries = realloc(idx->entries,
                           idx->entries_alloc * sizeof(*idx->entries));
    }
  idx->entries[idx->num_entries].pts = malloc(idx->pts_len);
  memcpy(idx->entries[idx->num_entries].pts, pts, idx->pts_len);
  }

int gavf_sync_index_read(gavf_io_t * io, gavf_sync_index_t * idx)
  {
  uint64_t i;
  int j;
  
  if(!gavf_io_read_uint64v(io, &idx->num_entries))
    return 0;
  idx->entries = malloc(idx->num_entries * sizeof(*idx->entries));

  for(i = 0; i < idx->num_entries; i++)
    {
    if(!gavf_io_read_uint64v(io, &idx->entries[i].pos))
      return 0;
    
    idx->entries[i].pts = malloc(idx->pts_len);
    for(j = 0; j < idx->num_streams; j++)
      {
      if(!gavf_io_read_int64v(io, &idx->entries[i].pts[j]))
        return 0;
      }
    }
  return 1;
  }

int gavf_sync_index_write(gavf_io_t * io, const gavf_sync_index_t * idx)
  {
  uint64_t i;
  int j;

  if(gavf_io_write_data(io, (uint8_t*)GAVF_TAG_SYNC_INDEX, 8) < 8)
    return 0;
  
  if(!gavf_io_write_uint64v(io, idx->num_entries))
    return 0;

  for(i = 0; i < idx->num_entries; i++)
    {
    if(!gavf_io_write_uint64v(io, idx->entries[i].pos))
      return 0;
    for(j = 0; j < idx->num_streams; j++)
      {
      if(!gavf_io_write_int64v(io, idx->entries[i].pts[j]))
        return 0;
      }
    }
  return 1;
  }

void gavf_sync_index_free(gavf_sync_index_t * idx)
  {
  if(idx->entries)
    free(idx->entries);
  }
