#include <gmerlin_mozilla.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>

#define NUM_FRAMES 2

typedef struct
  {
  uint8_t data[BUFFER_SIZE];
  uint8_t * read_pos;
  int len;
  sem_t produced;
  sem_t consumed;
  } buf_t;

struct bg_mozilla_buffer_s
  {
  buf_t b[NUM_FRAMES];
  buf_t * r_current;
  buf_t * r;
  buf_t * w;
  int eof; /* Read side only */
  };

bg_mozilla_buffer_t * bg_mozilla_buffer_create()
  {
  int i;
  bg_mozilla_buffer_t * ret = calloc(1, sizeof(*ret));

  for(i = 0; i < NUM_FRAMES; i++)
    {
    sem_init(&(ret->b[i].produced), 0, 0);
    sem_init(&(ret->b[i].consumed), 0, 1);
    }
  ret->r = ret->b;
  ret->w = ret->b;
  return ret;
  }

void bg_mozilla_buffer_destroy(bg_mozilla_buffer_t * b)
  {
  int i;
  for(i = 0; i < NUM_FRAMES; i++)
    {
    sem_destroy(&(b->b[i].produced));
    sem_destroy(&(b->b[i].consumed));
    }
  free(b);
  }

/* A final call with 0 len signals EOF */
int bg_mozilla_buffer_write(bg_mozilla_buffer_t * b,
                            void * data, int len)
  {
  fprintf(stderr, "Wait write...");
  sem_wait(&(b->w->consumed));
  fprintf(stderr, "Wait write...done\n");

  if(len > BUFFER_SIZE)
    len = BUFFER_SIZE;

  if(len)
    memcpy(b->w->data, data, len);
  
  b->w->len = len;
  sem_post(&(b->w->produced));
  b->w++;
  if(b->w - b->b >= NUM_FRAMES)
    b->w = b->b;
  return len;
  }

/* b is a bg_mozilla_buffer_t */
int bg_mozilla_buffer_read(void * b1,
                           uint8_t * data, int len)
  {
  int bytes_read = 0;
  int bytes_to_copy;
  int buf_size;
  bg_mozilla_buffer_t * b = b1;
  // fprintf(stderr, "bg_mozilla_buffer_read %d %d\n", bytes_read, len);

  if(b->eof)
    return 0;
  
  while(bytes_read < len)
    {
    //    fprintf(stderr, "bg_mozilla_buffer_read_1 %d %d\n", bytes_read, len);
    if(!b->r_current)
      {
      // fprintf(stderr, "Wait read...");
      sem_wait(&(b->r->produced));
      // fprintf(stderr, "Wait read...done\n");
      b->r_current = b->r;
      b->r_current->read_pos = b->r_current->data;

      if(!b->r_current->len)
        {
        b->eof = 1;
        break;
        }
      }
    bytes_to_copy = len - bytes_read;
    buf_size = b->r_current->len - (b->r_current->read_pos - b->r_current->data);
    if(bytes_to_copy > buf_size)
      bytes_to_copy = buf_size;
    
    memcpy(data + bytes_read, b->r_current->read_pos, bytes_to_copy);

    buf_size -= bytes_to_copy;
    bytes_read += bytes_to_copy;
    b->r_current->read_pos += bytes_to_copy;

    /* Next frame */
    if(!buf_size)
      {
      sem_post(&(b->r->consumed));
      b->r++;
      if(b->r - b->b >= NUM_FRAMES)
        b->r = b->b;
      b->r_current = (buf_t*)0;
      }
    
    }
  return bytes_read;
  }

