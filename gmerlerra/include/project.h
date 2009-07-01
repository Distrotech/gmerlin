#ifndef PROJECT_H
#define PROJECT_H

#include <medialist.h>
#include <track.h>
#include <outstream.h>

#define BG_NLE_PROJECT_SELECTION_CHANGED  (1<<0)
#define BG_NLE_PROJECT_VISIBLE_CHANGED    (1<<1)
#define BG_NLE_PROJECT_TRACKS_CHANGED     (1<<2)
#define BG_NLE_PROJECT_OUTSTREAMS_CHANGED (1<<3)

typedef struct
  {
  int changed_flags;

  
  int num_audio_tracks;
  int num_video_tracks;
  int num_tracks;

  int audio_tracks_alloc;
  int video_tracks_alloc;
  int tracks_alloc;
  
  bg_nle_track_t ** audio_tracks;
  bg_nle_track_t ** video_tracks;
  bg_nle_track_t ** tracks;

  bg_nle_outstream_t ** audio_outstreams;
  bg_nle_outstream_t ** video_outstreams;
  bg_nle_outstream_t ** outstreams;

  int num_audio_outstreams;
  int num_video_outstreams;
  int num_outstreams;
  int audio_outstreams_alloc;
  int video_outstreams_alloc;
  int outstreams_alloc;
  
  bg_nle_media_list_t * media_list;
  
  /* Timeline status */
  gavl_time_t start_visible;
  gavl_time_t end_visible;
  gavl_time_t start_selection;
  gavl_time_t end_selection;
  
  bg_cfg_section_t * audio_track_section;
  bg_cfg_section_t * video_track_section;
  bg_cfg_section_t * audio_outstream_section;
  bg_cfg_section_t * video_outstream_section;
  bg_cfg_section_t * paths_section;
  
  bg_plugin_registry_t * plugin_reg;
  } bg_nle_project_t;

bg_nle_project_t * bg_nle_project_create(bg_plugin_registry_t * plugin_reg);

void bg_nle_project_save(bg_nle_project_t *, const char * file);

const bg_parameter_info_t * bg_nle_project_get_audio_parameters();
const bg_parameter_info_t * bg_nle_project_get_video_parameters();
const bg_parameter_info_t * bg_nle_project_get_paths_parameters();

void bg_nle_project_set_audio_parameter(void * data, const char * name,
                                        const bg_parameter_value_t * val);

void bg_nle_project_set_video_parameter(void * data, const char * name,
                                        const bg_parameter_value_t * val);

void bg_nle_project_destroy(bg_nle_project_t * p);

bg_nle_track_t * bg_nle_project_add_audio_track(bg_nle_project_t * p);
bg_nle_track_t * bg_nle_project_add_video_track(bg_nle_project_t * p);

bg_nle_outstream_t * bg_nle_project_add_audio_outstream(bg_nle_project_t * p);
bg_nle_outstream_t * bg_nle_project_add_video_outstream(bg_nle_project_t * p);


void bg_nle_project_append_track(bg_nle_project_t * p, bg_nle_track_t * t);
void bg_nle_project_append_outstream(bg_nle_project_t * p, bg_nle_outstream_t * t);

/* project_xml.c */

bg_nle_project_t * bg_nle_project_load(const char * filename, bg_plugin_registry_t * plugin_reg);

void bg_nle_project_save(bg_nle_project_t *, const char * filename);

#endif
