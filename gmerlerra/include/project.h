#ifndef PROJECT_H
#define PROJECT_H

#include <medialist.h>
#include <track.h>
#include <outstream.h>

#define BG_NLE_PROJECT_SELECTION_CHANGED  (1<<0)
#define BG_NLE_PROJECT_VISIBLE_CHANGED    (1<<1)
#define BG_NLE_PROJECT_TRACKS_CHANGED     (1<<2)
#define BG_NLE_PROJECT_OUTSTREAMS_CHANGED (1<<3)

typedef void (*bg_nle_edit_callback)(bg_nle_project_t*,
                                     bg_nle_edit_op_t op,
                                     void * op_data,
                                     void * user_data);

struct bg_nle_project_s
  {
  int changed_flags;
  int num_tracks;

  int tracks_alloc;
  
  bg_nle_track_t ** tracks;

  bg_nle_outstream_t ** outstreams;

  int num_outstreams;
  int outstreams_alloc;
  
  bg_nle_media_list_t * media_list;
  
  /* Timeline status */

  bg_nle_time_range_t visible;
  bg_nle_time_range_t selection;
  
  bg_cfg_section_t * audio_track_section;
  bg_cfg_section_t * video_track_section;
  bg_cfg_section_t * audio_outstream_section;
  bg_cfg_section_t * video_outstream_section;
  bg_cfg_section_t * paths_section;
  
  bg_plugin_registry_t * plugin_reg;

  bg_nle_edit_callback edit_callback;
  void * edit_callback_data;

  bg_nle_undo_data_t * undo;
  bg_nle_undo_data_t * redo;
  };

bg_nle_project_t * bg_nle_project_create(bg_plugin_registry_t * plugin_reg);

void bg_nle_project_save(bg_nle_project_t *, const char * file);

void bg_nle_project_resolve_ids(bg_nle_project_t *);

void bg_nle_project_set_edit_callback(bg_nle_project_t *,
                                      bg_nle_edit_callback callback,
                                      void * callback_data);

const bg_parameter_info_t * bg_nle_project_get_audio_parameters();
const bg_parameter_info_t * bg_nle_project_get_video_parameters();
const bg_parameter_info_t * bg_nle_project_get_paths_parameters();

void bg_nle_project_set_audio_parameter(void * data, const char * name,
                                        const bg_parameter_value_t * val);

void bg_nle_project_set_video_parameter(void * data, const char * name,
                                        const bg_parameter_value_t * val);

void bg_nle_project_destroy(bg_nle_project_t * p);

void bg_nle_project_add_audio_track(bg_nle_project_t * p);
void bg_nle_project_add_video_track(bg_nle_project_t * p);

void bg_nle_project_add_audio_outstream(bg_nle_project_t * p);
void bg_nle_project_add_video_outstream(bg_nle_project_t * p);

void bg_nle_project_delete_outstream(bg_nle_project_t * p, bg_nle_outstream_t * t);
void bg_nle_project_delete_track(bg_nle_project_t * p, bg_nle_track_t * t);

void bg_nle_project_append_track(bg_nle_project_t * p, bg_nle_track_t * t);
void bg_nle_project_insert_track(bg_nle_project_t * p, bg_nle_track_t * t, int pos);

void bg_nle_project_move_track(bg_nle_project_t * p, int old_pos, int new_pos);


void bg_nle_project_append_outstream(bg_nle_project_t * p, bg_nle_outstream_t * t);
void bg_nle_project_insert_outstream(bg_nle_project_t * p,
                                     bg_nle_outstream_t * t, int pos);

void bg_nle_project_move_outstream(bg_nle_project_t * p, int old_pos, int new_pos);

int bg_nle_project_outstream_index(bg_nle_project_t * p, bg_nle_outstream_t * outstream);
int bg_nle_project_track_index(bg_nle_project_t * p, bg_nle_track_t * track);

void bg_nle_project_edit(bg_nle_project_t * p,
                         bg_nle_undo_data_t * data);


/* project_xml.c */

bg_nle_project_t * bg_nle_project_load(const char * filename, bg_plugin_registry_t * plugin_reg);

void bg_nle_project_save(bg_nle_project_t *, const char * filename);



#endif
