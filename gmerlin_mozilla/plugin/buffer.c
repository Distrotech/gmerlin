#include <gmerlin_mozilla.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <gmerlin/utils.h>


struct bg_mozilla_buffer_s
  {
  char * filename;
  FILE * read_file;
  FILE * write_file;
  
  int64_t read_pos, write_pos;
  pthread_mutex_t pos_mutex;
  
  int inotify_fd;
  int inotify_wd;
  };

bg_mozilla_buffer_t * bg_mozilla_buffer_create()
  {
  bg_mozilla_buffer_t * ret = calloc(1, sizeof(*ret));
  ret->filename = bg_create_unique_filename("/tmp/gmerlin_mozilla_%05x");
  ret->write_file = fopen(ret->filename, "w");
  ret->read_file = fopen(ret->filename, "r");
  
  ret->inotify_fd = inotify_init();
  ret->inotify_wd = inotify_add_watch(ret->inotify_fd,
                                      ret->filename, IN_MODIFY | IN_CLOSE_WRITE);
  return ret;
  }

void bg_mozilla_buffer_destroy(bg_mozilla_buffer_t * b)
  {
  if(b->filename)
    {
    remove(b->filename);
    free(b->filename);
    }
  if(b->write_file)
    fclose(b->write_file);
  if(b->read_file)
    fclose(b->read_file);

  if(b->inotify_fd >= 0)
    {
    inotify_rm_watch(b->inotify_fd, b->inotify_wd);
    close(b->inotify_fd);
    }
  
  free(b);
  }

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

static int handle_inotify(bg_mozilla_buffer_t * b, int to)
  {
  char buffer[BUF_LEN];
  struct timeval timeout;
  int i;
  int result;
  fd_set read_fds;
  
  /* Wait for event */
  timeout.tv_sec = to / 1000;
  timeout.tv_usec = to % 1000;
  FD_ZERO(&read_fds);
  FD_SET(b->inotify_fd, &read_fds);
  if(!select(b->inotify_fd+1, &read_fds, (fd_set*)0, (fd_set*)0,&timeout))
    {
    //        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connection timed out");
    return 0;
    }
  result = read(b->inotify_fd, buffer, BUF_LEN);
  
  if(result < 0)
    {
    fprintf(stderr, "inotify Read failed\n");
    return 0;
    }

  i = 0;
  while ( i < result )
    {
    struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
    if(event->mask & IN_MODIFY)
      {
      //      fprintf(stderr, "inotify modified\n");
      }
    if(event->mask & IN_CLOSE_WRITE)
      {
      inotify_rm_watch(b->inotify_fd, b->inotify_wd);
      close(b->inotify_fd);
      b->inotify_fd = -1;
      //      fprintf(stderr, "inotify closed\n");
      }
    i += EVENT_SIZE + event->len;
    }
  return 1;
  }


/* b is a bg_mozilla_buffer_t */
int bg_mozilla_buffer_read(void * b1,
                           uint8_t * data, int len)
  {
  int bytes_read = 0;
  int result;
  bg_mozilla_buffer_t * b = b1;

  while(bytes_read < len)
    {
    //    fprintf(stderr, "Read %ld %d...", ftell(b->read_file), len - bytes_read);
    result = fread(data + bytes_read, 1,
                   len - bytes_read, b->read_file);
    //    fprintf(stderr, "done %d\n", result);
    
    if((result < len - bytes_read) && feof(b->read_file)) /* Hit end of file? */
      {
      if(b->inotify_fd >= 0)
        {
        if(!handle_inotify(b, 10000))
          break;
        clearerr(b->read_file);
        }
      else
        break;
      }
    else
      bytes_read += result;
    }
  
  return bytes_read;
  }

/* b is a bg_mozilla_buffer_t */
int bg_mozilla_buffer_write(bg_mozilla_buffer_t * b,
                            void * data1, int len)
     //int bg_mozilla_buffer_read(void * b1,
     //                           uint8_t * data, int len)
  {
  if(!len)
    {
    fclose(b->write_file);
    return 0;
    }
  else
    return fwrite(data1, 1, len, b->write_file);
  }

