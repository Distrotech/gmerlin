#include <inttypes.h>
#include <string.h>

#include <types.h>

void bg_nle_time_range_copy(bg_nle_time_range_t * dst, const bg_nle_time_range_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

void bg_nle_time_range_swap(bg_nle_time_range_t * r1, bg_nle_time_range_t * r2)
  {
  bg_nle_time_range_t tmp;
  tmp.start = r1->start;
  tmp.end   = r1->end;

  r1->start = r2->start;
  r1->end   = r2->end;

  r2->start = tmp.start;
  r2->end   = tmp.end;
  
  }

