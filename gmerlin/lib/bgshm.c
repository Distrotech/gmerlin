#include <config.h>


#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <unistd.h>
#include <sys/types.h>


#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include <bgshm.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "shm"

bg_shm_t * bg_shm_alloc_write(int size)
  {
  int shm_fd = -1;
  void * addr;
  bg_shm_t * ret = NULL;

  char name[NAME_MAX];
  int counter = 0;
  
  while(1)
    {
    counter++;
    snprintf(name, NAME_MAX, "/gmerlin-shm-%08x", counter);
    
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
  
  if(ftruncate(shm_fd, size))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "ftruncate failed: %s", strerror(errno));
    goto fail;
    }

  if((addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "mmap failed: %s", strerror(errno));
    return NULL;
    }
  ret = calloc(1, sizeof(*ret));
  ret->addr = addr;
  ret->size = size;
  strcpy(ret->name, name);
  
  fail:
  
  close(shm_fd);
  
  return ret;
  }

bg_shm_t * bg_shm_alloc_read(const char * name, int size)
  {
  void * addr;
  bg_shm_t * ret = NULL;
  int shm_fd;
  
  shm_fd = shm_open(name, O_RDONLY, 0);
  if(shm_fd < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "shm_open failed: %s", strerror(errno));
    goto fail;
    }
  if((addr = mmap(0, size, PROT_READ, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "mmap failed: %s", strerror(errno));
    goto fail;
    }

  if(shm_unlink(name) == -1)
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "shm_unlink failed: %s", strerror(errno));
  
  ret = calloc(1, sizeof(*ret));
  ret->addr = addr;
  ret->size = size;

  fail:
  if(shm_fd >= 0)
    close(shm_fd);
  
  return ret;
  }

void bg_shm_free(bg_shm_t * shm)
  {
  munmap(shm->addr, shm->size);
  if(shm->name[0] != '\0')
    shm_unlink(shm->name);
  free(shm);
  }
