#include <inttypes.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#include <gavltime.h>


static void get_time(gavl_time_t * ret)
  {
  struct timeval time;
  gettimeofday(&time, NULL);
  *ret = (int64_t)(time.tv_sec)*1000000LL + time.tv_usec;
  }

struct gavl_timer_s
  {
  int64_t start_time_real;
  int64_t start_time;
  int is_running;
  };

gavl_timer_t * gavl_timer_create()
  {
  gavl_timer_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void gavl_timer_destroy(gavl_timer_t * t)
  {
  free(t);
  }

void gavl_timer_start(gavl_timer_t * t)
  {
  get_time(&(t->start_time_real));
  t->is_running = 1;
  }

void gavl_timer_stop(gavl_timer_t * t)
  {
  gavl_time_t tmp;
  tmp = gavl_timer_get(t);
  t->start_time = tmp;
  t->is_running = 0;
  }

gavl_time_t gavl_timer_get(gavl_timer_t * t)
  {
  gavl_time_t ret;
  if(t->is_running)
    {
    get_time(&ret);
    ret -= t->start_time_real;
    ret += t->start_time;
    return ret;
    }
  else
    return t->start_time;
  }

void gavl_timer_set(gavl_timer_t * t, gavl_time_t v)
  {
  t->start_time = v;
  }
