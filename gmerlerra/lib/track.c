#include <gavl/gavl.h>

#include <track.h>
#include <stdlib.h>

bg_nle_track_t * bg_nle_track_create(bg_nle_track_type_t type)
  {
  bg_nle_track_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->type = type;
  ret->flags = BG_NLE_TRACK_SELECTED | BG_NLE_TRACK_EXPANDED;
  return ret;
  }

void bg_nle_track_destroy(bg_nle_track_t * t)
  {
  free(t);
  }

const bg_parameter_info_t * bg_nle_track_get_parameters(bg_nle_track_t * t)
  {
  
  }

void bg_nle_track_set_parameter(void * data, const char * name,
                                const bg_parameter_info_t * value)
  {
  
  }

