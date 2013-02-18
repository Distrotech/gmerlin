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

int gavf_options_get_flags(gavf_options_t * opt)
  {
  return opt->flags;
  }


void gavf_options_set_metadata_callback(gavf_options_t * opt, 
                                        void (*cb)(void*,const gavl_metadata_t*),
                                        void *cb_priv)
  {
  opt->metadata_cb = cb;
  opt->metadata_cb_priv = cb_priv;
  }

