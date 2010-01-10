#ifndef TRACK_H
#define TRACK_H

#include <gmerlin/parameter.h>
#include <gmerlin/xmlutils.h>

#include <types.h>

#include <medialist.h>

typedef struct
  {
  int scale; /* Source scale */
  
  int64_t src_pos; /* In source scale */
  int64_t dst_pos; /* In track scale */
  int64_t len;     /* In track scale */
  
  bg_nle_id_t file_id;
  int stream;
  } bg_nle_track_segment_t;

typedef struct
  {
  //  int scale; dst scale is always GAVL_TIME_SCALE
  
  int num_segments;
  int segments_alloc;
  bg_nle_track_segment_t * segments;
  
  int flags;
  int index;
  bg_nle_track_type_t type;
  bg_nle_id_t id;
  
  bg_cfg_section_t * section;

  /* Pointer to the project */
  bg_nle_project_t * p;
  
  } bg_nle_track_t;

bg_nle_track_t * bg_nle_track_create(bg_nle_track_type_t type);

void bg_nle_track_destroy(bg_nle_track_t *);

void bg_nle_track_alloc_segments(bg_nle_track_t *, int num);

void bg_nle_track_set_name(bg_nle_track_t *, const char * name);
const char * bg_nle_track_get_name(bg_nle_track_t *);

const bg_parameter_info_t * bg_nle_track_get_parameters(bg_nle_track_t * t);

gavl_time_t bg_nle_track_duration(bg_nle_track_t * t);


/* 
void bg_nle_track_set_parameter(void * data, const char * name,
                                const bg_parameter_info_t * value);
*/

/* track_xml.c */

bg_nle_track_t * bg_nle_track_load(xmlDocPtr xml_doc, xmlNodePtr node);
void bg_nle_track_save(bg_nle_track_t * t, xmlNodePtr parent);

extern const bg_parameter_info_t bg_nle_track_audio_parameters[];
extern const bg_parameter_info_t bg_nle_track_video_parameters[];

#endif
