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

int bg_nle_time_range_intersect(const bg_nle_time_range_t * r1,
                                 const bg_nle_time_range_t * r2)
  {
  // r1  |
  // r2 |  |
  if(r1->end < 0)
    {
    if(r2->end < 0)
      return (r1->start == r2->start) ? 1 : 0;
    if((r1->start < r2->start) || (r1->start >= r2->end))
      return 0;
    }
  else
    {
    if(r2->end < 0)
      {
      if((r2->start < r1->start) || (r2->start >= r1->end))
        return 0;
      else
        return 1;
      }
    else
      if((r1->end < r2->start) || (r1->start >= r2->end))
        return 0;
      else
        return 1;
    }

  }
