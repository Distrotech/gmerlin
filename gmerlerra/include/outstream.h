#ifndef OUTSTREAM_H
#define OUTSTREAM_H

#include <track.h>

typedef struct
  {
  bg_nle_track_type_t type;
  int flags;
  bg_cfg_section_t * section;

  /* Source tracks */
  bg_nle_id_t * source_track_ids;
  bg_nle_id_t id;
  
  int num_source_tracks;
  int source_tracks_alloc;
  bg_nle_track_t ** source_tracks;
  
  /* Pointer to the project */
  bg_nle_project_t * p;
  
  } bg_nle_outstream_t;

bg_nle_outstream_t * bg_nle_outstream_create(bg_nle_track_type_t type);

void bg_nle_outstream_destroy(bg_nle_outstream_t *);

void bg_nle_outstream_set_name(bg_nle_outstream_t *, const char * name);
const char * bg_nle_outstream_get_name(bg_nle_outstream_t *);

const bg_parameter_info_t *
bg_nle_outstream_get_parameters(bg_nle_outstream_t * t);

void bg_nle_outstream_attach_track(bg_nle_outstream_t *,
                                   bg_nle_track_t * t);

void bg_nle_outstream_detach_track(bg_nle_outstream_t *,
                                   bg_nle_track_t * t);

int bg_nle_outstream_has_track(bg_nle_outstream_t *,
                               bg_nle_track_t * t);

void bg_nle_outstream_set_audio_stream(bg_nle_outstream_t * os,
                                       bg_nle_audio_stream_t * s);

void bg_nle_outstream_set_video_stream(bg_nle_outstream_t * os,
                                       bg_nle_video_stream_t * s);

extern const bg_parameter_info_t bg_nle_outstream_audio_parameters[];
extern const bg_parameter_info_t bg_nle_outstream_video_parameters[];

/* outstream_xml.c */

bg_nle_outstream_t *
bg_nle_outstream_load(xmlDocPtr xml_doc, xmlNodePtr node);
void bg_nle_outstream_save(bg_nle_outstream_t * t, xmlNodePtr parent);


#endif // OUTSTREAM_H
