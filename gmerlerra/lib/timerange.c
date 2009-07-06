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

int64_t bg_nle_pos_2_time(bg_nle_time_range_t * visible, int width, double pos)
  {
  double ret_d = (double)pos /
    (double)width * (double)(visible->end - visible->start);
  return (int64_t)(ret_d + 0.5) + visible->start;
  }

double bg_nle_time_2_pos(bg_nle_time_range_t * visible, int width, int64_t time)
  {
  double ret_d = (double)(time - visible->start) /
    (double)(visible->end - visible->start) * (double)width;
  return ret_d;
  }
