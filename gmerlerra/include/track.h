#ifndef TRACK_H
#define TRACK_H

#include <gmerlin/parameter.h>
#include <gmerlin/xmlutils.h>

#include <medialist.h>

typedef enum
  {
    BG_NLE_TRACK_NONE = 0,
    BG_NLE_TRACK_AUDIO,
    BG_NLE_TRACK_VIDEO,
  } bg_nle_track_type_t;

/* WARNING: Changing these alters the file format */
#define BG_NLE_TRACK_SELECTED (1<<0)
#define BG_NLE_TRACK_EXPANDED (1<<1)

typedef struct
  {
  int scale;

  int64_t src_pos;
  int64_t dst_pos;
  int64_t len;
  
  int file_id;
  int stream;
  } bg_nle_track_segment_t;

typedef struct
  {
  int scale;

  int num_segments;
  bg_nle_track_segment_t * segments;
  
  int flags;
  int index;
  bg_nle_track_type_t type;
  int id;
  
  bg_cfg_section_t * section;
  } bg_nle_track_t;

bg_nle_track_t * bg_nle_track_create(bg_nle_track_type_t type);

void bg_nle_track_destroy(bg_nle_track_t *);
void bg_nle_track_set_name(bg_nle_track_t *, const char * name);
const char * bg_nle_track_get_name(bg_nle_track_t *);

const bg_parameter_info_t * bg_nle_track_get_parameters(bg_nle_track_t * t);

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
