#include <sys/time.h>
#include <config.h>

#include <inttypes.h>
#include <timeutils.h>

static struct timeval time_before;
static struct timeval time_after;

void timer_init()
  {
  gettimeofday(&time_before, (struct timezone*)0);
  }

uint64_t timer_stop()
  {
  uint64_t before, after, diff;
  
  gettimeofday(&time_after, (struct timezone*)0);

  before = ((uint64_t)time_before.tv_sec)*1000000 + time_before.tv_usec;
  after  = ((uint64_t)time_after.tv_sec)*1000000  + time_after.tv_usec;
  
  /*   fprintf(stderr, "Before: %f After: %f\n", before, after); */
  
  diff = after - before;

  return diff;
  
  }

