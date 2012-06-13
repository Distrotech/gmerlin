#include <gavfprivate.h>

void
gavf_options_set_sync_distance(gavf_options_t * opt, gavl_time_t sync_distance)
  {
  opt->sync_distance = sync_distance;
  }

void
gavf_options_set_flags(gavf_options_t * opt, int flags)
  {
  opt->flags = flags;
  }
