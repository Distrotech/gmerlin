#include <config.h>


#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <pthread.h>

#include <unistd.h>
#include <sys/types.h>

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <bgshm.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "shm"
#define SHM_NAME_MAX 32

#define ALIGN_BYTES 16

typedef struct
  {
  pthread_mutex_t mutex;
  int refcount;
  } refcounter_t;

struct bg_shm_s
  {
  uint8_t * addr;
  int size;
  int id;
  int wr;
  refcounter_t * rc;
  };

static int align_size(int size)
  {
  size = ((size + ALIGN_BYTES - 1) / ALIGN_BYTES) * ALIGN_BYTES;
  return size;
  }

static int get_real_size(int size)
  {
  return align_size(size) + sizeof(refcounter_t);
  }

uint8_t * bg_shm_get_buffer(bg_shm_t * s, int * size)
  {
  if(size)
    *size = s->size;
  return s->addr;
  }

int bg_shm_get_id(bg_shm_t * s)
  {
  return s->id;
  }

static void gen_name(int id, char * ret)
  {
  snprintf(ret, SHM_NAME_MAX, "/gmerlin-shm-%08x", id);
  }

bg_shm_t * bg_shm_alloc_write(int size)
  {
  int shm_fd = -1;
  void * addr;
  bg_shm_t * ret = NULL;
  char name[SHM_NAME_MAX];
  int id = 0;
  pthread_mutexattr_t attr;
  int real_size = get_real_size(size);
  
  while(1)
    {
    id++;
    
    gen_name(id, name);
    
    if((shm_fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
      {
      if(errno != EEXIST)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,
               "shm_open failed: %s", strerror(errno));
        return NULL;
        }
      }
    else
      break;
    }
  
  if(ftruncate(shm_fd, real_size))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "ftruncate failed: %s", strerror(errno));
    goto fail;
    }

  if((addr = mmap(0, real_size, PROT_READ | PROT_WRITE,
                  MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "mmap failed: %s", strerror(errno));
    return NULL;
    }

  ret = calloc(1, sizeof(*ret));
  ret->addr = addr;
  ret->size = size;
  ret->id = id;
  ret->wr = 1;
  ret->rc = (refcounter_t*)(ret->addr + align_size(size));

  /* Initialize process shared mutex */

  pthread_mutexattr_init(&attr);
  if(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "cannot create process shared mutex: %s", strerror(errno));
    goto fail;
    }
  pthread_mutex_init(&ret->rc->mutex, &attr);
  ret->rc->refcount = 0;
  fail:
  
  close(shm_fd);
  
  return ret;
  }

bg_shm_t * bg_shm_alloc_read(int id, int size)
  {
  void * addr;
  bg_shm_t * ret = NULL;
  int shm_fd;
  char name[SHM_NAME_MAX];
  int real_size = get_real_size(size);
  
  gen_name(id, name);
  
  shm_fd = shm_open(name, O_RDWR, 0);
  if(shm_fd < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "shm_open failed: %s", strerror(errno));
    goto fail;
    }
  if((addr = mmap(0, real_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                  shm_fd, 0)) == MAP_FAILED)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "mmap failed: %s", strerror(errno));
    goto fail;
    }
  
  ret = calloc(1, sizeof(*ret));
  ret->addr = addr;
  ret->size = size;
  ret->rc = (refcounter_t*)(ret->addr + align_size(size));
  ret->id = id;
  fail:
  if(shm_fd >= 0)
    close(shm_fd);
  
  return ret;
  }

void bg_shm_free(bg_shm_t * shm)
  {
  munmap(shm->addr, shm->size);

  if(shm->wr)
    {
    char name[SHM_NAME_MAX];
    gen_name(shm->id, name);
    shm_unlink(name);
    }
  free(shm);
  }

void bg_shm_ref(bg_shm_t * s)
  {
  pthread_mutex_lock(&s->rc->mutex);
  s->rc->refcount++;
  // fprintf(stderr, "bg_shm_ref %d %d\n", s->id, s->rc->refcount);
  pthread_mutex_unlock(&s->rc->mutex);

  }

void bg_shm_unref(bg_shm_t * s)
  {
  pthread_mutex_lock(&s->rc->mutex);
  s->rc->refcount--;
  // fprintf(stderr, "bg_shm_unref %d %d\n", s->id, s->rc->refcount);
  pthread_mutex_unlock(&s->rc->mutex);
  }

int bg_shm_refcount(bg_shm_t * s)
  {
  int ret;
  pthread_mutex_lock(&s->rc->mutex);
  ret = s->rc->refcount;
  pthread_mutex_unlock(&s->rc->mutex);
  return ret;
  }


/* Shared memory pool */

struct bg_shm_pool_s
  {
  int wr;
  int segment_size;
  
  int num_segments;
  int segments_alloc;
  bg_shm_t ** segments;
  };

bg_shm_pool_t * bg_shm_pool_create(int seg_size, int wr)
  {
  bg_shm_pool_t * ret = calloc(1, sizeof(*ret));
  ret->wr = wr;
  ret->segment_size = seg_size;
  return ret;
  }

bg_shm_t * bg_shm_pool_get_read(bg_shm_pool_t * p, int id)
  {
  int i;
  bg_shm_t * ret;
  for(i = 0; i < p->num_segments; i++)
    {
    if(p->segments[i]->id == id)
      return p->segments[i];
    }

  /* Segment isn't in the pool yet, map a new one */
  if(p->num_segments + 1 >= p->segments_alloc)
    {
    p->segments_alloc += 16;
    p->segments = realloc(p->segments,
                          p->segments_alloc * sizeof(*p->segments));
    }
  p->segments[p->num_segments] =
    bg_shm_alloc_read(id, p->segment_size);
  ret = p->segments[p->num_segments];
  p->num_segments++;
  return ret;
  }

bg_shm_t * bg_shm_pool_get_write(bg_shm_pool_t * p)
  {
  int i;
  bg_shm_t * ret = NULL;
  for(i = 0; i < p->num_segments; i++)
    {
    if(!bg_shm_refcount(p->segments[i]))
      {
      ret = p->segments[i];
      break;
      }
    }

  if(!ret)
    {
    /* No unused segment available, create a new one */
    if(p->num_segments + 1 >= p->segments_alloc)
      {
      p->segments_alloc += 16;
      p->segments = realloc(p->segments,
                            p->segments_alloc * sizeof(*p->segments));
      }
    p->segments[p->num_segments] =
      bg_shm_alloc_write(p->segment_size);
    ret = p->segments[p->num_segments];
    p->num_segments++;
    }

  /* Set refcount to 1 */
  bg_shm_ref(ret);
  return ret;
  }

void bg_shm_pool_destroy(bg_shm_pool_t * p)
  {
  int i;
  for(i = 0; i < p->num_segments; i++)
    bg_shm_free(p->segments[i]);
  if(p->segments)
    free(p->segments);
  free(p);
  }
