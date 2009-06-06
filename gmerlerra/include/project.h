#ifndef PROJECT_H
#define PROJECT_H


#define BG_NLE_PROJECT_CHANGED (1<<0)

typedef struct
  {
  int flags;

  int num_audio_tracks;
  int num_video_tracks;
  int num_tracks;

  int audio_tracks_alloc;
  int video_tracks_alloc;
  int tracks_alloc;
  
  bg_nle_track_t ** audio_tracks;
  bg_nle_track_t ** video_tracks;
  bg_nle_track_t ** tracks;
  
  //  int num_files;
  //  bg_nle_file_t ** files;

  /* Timeline status */
  gavl_time_t start_visible;
  gavl_time_t end_visible;
  gavl_time_t start_selection;
  gavl_time_t end_selection;
  
  bg_cfg_section_t * audio_section;
  bg_cfg_section_t * video_section;
  bg_cfg_section_t * paths_section;

  gavl_audio_format_t audio_format_preset;
  gavl_audio_format_t audio_format;

  gavl_video_format_t video_format_preset;
  gavl_video_format_t video_format;
  } bg_nle_project_t;

bg_nle_project_t * bg_nle_project_create(const char * file);

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

void bg_nle_project_append_track(bg_nle_project_t * p, bg_nle_track_t * t);

/* project_xml.c */

bg_nle_project_t * bg_nle_project_load(const char * filename);

void bg_nle_project_save(bg_nle_project_t *, const char * filename);

#endif
