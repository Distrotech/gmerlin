#include <gmerlin/parameter.h>

typedef enum
  {
    BG_NLE_TRACK_AUDIO,
    BG_NLE_TRACK_VIDEO,
  } bg_nle_track_type_t;

#define BG_NLE_TRACK_SELECTED (1<<0)
#define BG_NLE_TRACK_EXPANDED (1<<1)

typedef struct
  {
  int scale;

  int64_t src_pos;
  int64_t dst_pos;
  int64_t len;
  
  char * location;
  int track;
  int stream;
  } bg_nle_track_segment_t;

typedef struct
  {
  int scale;

  int num_segments;
  bg_nle_track_segment_t * segments;
  
  char * name;

  int flags;
  int index;
  bg_nle_track_type_t type;
  } bg_nle_track_t;

bg_nle_track_t * bg_nle_track_create(bg_nle_track_type_t type);

void bg_nle_track_destroy(bg_nle_track_t *);

const bg_parameter_info_t * bg_nle_track_get_parameters(bg_nle_track_t * t);

void bg_nle_track_set_parameter(void * data, const char * name,
                                const bg_parameter_info_t * value);

/* track_xml.c */

