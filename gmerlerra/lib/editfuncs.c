#include <string.h>

#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>

#include <track.h>
#include <project.h>
#include <editops.h>

static void edited(bg_nle_project_t * p,
                   bg_nle_edit_op_t op,
                   void * op_data, int * id)
  {
  bg_nle_undo_data_t * data;
  data = calloc(1, sizeof(*data));
  data->op = op;
  data->data = op_data;

  if(id)
    data->id = *id;
  else
    data->id = p->undo_id++;
  
  /* Apply the edit operation to the project */
  bg_nle_project_edit(p, data);
  
  /* Notify GUI */
  p->edit_callback(p, op, op_data, p->edit_callback_data);
  
  bg_nle_project_push_undo(p, data);
  }

// BG_NLE_EDIT_ADD_TRACK

static bg_nle_id_t create_track_id(bg_nle_project_t * p)
  {
  bg_nle_id_t ret = 0;
  int keep_going;
  int i;
  
  do{
    ret++;
    keep_going = 0;
    for(i = 0; i < p->num_tracks; i++)
      {
      if(p->tracks[i]->id == ret)
        {
        keep_going = 1;
        break;
        }
      }
    }while(keep_going);
  return ret;
  }

static bg_nle_track_t * create_audio_track(bg_nle_project_t * p)
  {
  int i;
  bg_nle_track_t * track;
  char * tmp_string;
  int num_tracks;
  track = bg_nle_track_create(BG_NLE_TRACK_AUDIO);

  /* Create ID */
  track->id = create_track_id(p);
  
  bg_cfg_section_transfer(p->audio_track_section,
                          track->section);
  /* Create default name */
  num_tracks = 0;
  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i]->type == BG_NLE_TRACK_AUDIO)
      num_tracks++;
    }
  
  tmp_string = bg_sprintf("Audio track %d", num_tracks+1);
  bg_nle_track_set_name(track, tmp_string);
  free(tmp_string);
  return track;
  }

void bg_nle_project_add_audio_track(bg_nle_project_t * p)
  {
  bg_nle_op_track_t * d;
  bg_nle_track_t * track;

  d = calloc(1, sizeof(*d));
  
  track = create_audio_track(p);

  d->track = track;
  d->index = p->num_tracks;
  
  edited(p, BG_NLE_EDIT_ADD_TRACK, d, NULL);
  }

static bg_nle_track_t * create_video_track(bg_nle_project_t * p)
  {
  int i;
  bg_nle_track_t * track;
  char * tmp_string;
  int num_tracks;
  track = bg_nle_track_create(BG_NLE_TRACK_VIDEO);

  /* Create ID */
  track->id = create_track_id(p);
  
  bg_cfg_section_transfer(p->video_track_section,
                          track->section);
  /* Create default name */
  num_tracks = 0;
  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i]->type == BG_NLE_TRACK_VIDEO)
      num_tracks++;
    }
  
  tmp_string = bg_sprintf("Video track %d", num_tracks+1);
  bg_nle_track_set_name(track, tmp_string);
  free(tmp_string);
  return track;
  }

void bg_nle_project_add_video_track(bg_nle_project_t * p)
  {
  bg_nle_op_track_t * d;
  bg_nle_track_t * track;

  d = calloc(1, sizeof(*d));
  
  track = create_video_track(p);

  d->track = track;
  d->index = p->num_tracks;
  
  edited(p, BG_NLE_EDIT_ADD_TRACK, d, NULL);
  }

// BG_NLE_EDIT_DELETE_TRACK

void bg_nle_project_delete_track(bg_nle_project_t * p, bg_nle_track_t * t)
  {
  int num_outstreams, i;
  bg_nle_op_track_t * d;
  
  d = calloc(1, sizeof(*d));
  d->index = bg_nle_project_track_index(p, t);
  d->track = t;

  /* Check to which outstreams this track was attached */
  num_outstreams = 0;

  for(i = 0; i < p->num_outstreams; i++)
    {
    if(bg_nle_outstream_has_track(p->outstreams[i], t))
      num_outstreams++;
    }

  if(!num_outstreams)
    d->num_outstreams = -1;
  else
    {
    d->outstreams = calloc(num_outstreams, sizeof(*d->outstreams));
    for(i = 0; i < p->num_outstreams; i++)
      {
      if(bg_nle_outstream_has_track(p->outstreams[i], t))
        {
        d->outstreams[d->num_outstreams] = p->outstreams[i];
        d->num_outstreams++;
        }
      }
    }
    
  edited(p, BG_NLE_EDIT_DELETE_TRACK, d, NULL);
  }


// BG_NLE_EDIT_MOVE_TRACK

void bg_nle_project_move_track(bg_nle_project_t * p, int old_pos, int new_pos)
  {
  bg_nle_op_move_track_t * d;
  
  d = calloc(1, sizeof(*d));
  d->old_index = old_pos;
  d->new_index = new_pos;
  edited(p, BG_NLE_EDIT_MOVE_TRACK, d, NULL);
  }


// BG_NLE_EDIT_ADD_OUTSTREAM

static bg_nle_id_t create_outstream_id(bg_nle_outstream_t ** outstreams,
                                       int num_outstreams)
  {
  bg_nle_id_t ret = 0;
  int keep_going;
  int i;
  
  do{
    ret++;
    keep_going = 0;
    for(i = 0; i < num_outstreams; i++)
      {
      if(outstreams[i]->id == ret)
        {
        keep_going = 1;
        break;
        }
      }
    }while(keep_going);
  return ret;
  }

static bg_nle_outstream_t * create_video_outstream(bg_nle_project_t * p)
  {
  bg_nle_outstream_t * outstream;
  char * tmp_string;
  int num, i;
  outstream = bg_nle_outstream_create(BG_NLE_TRACK_VIDEO);
  if(!p->current_video_outstream)
    outstream->flags |= BG_NLE_TRACK_PLAYBACK;

  /* Create ID */
  outstream->id = create_outstream_id(p->outstreams, p->num_outstreams);
  
  bg_cfg_section_transfer(p->video_outstream_section,
                          outstream->section);

  /* Get default label */
  num = 0;
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i]->type == BG_NLE_TRACK_VIDEO)
      num++;
    }
  
  tmp_string = bg_sprintf("Video outstream %d", num+1);
  bg_nle_outstream_set_name(outstream, tmp_string);
  free(tmp_string);
  return outstream;
  }

void bg_nle_project_add_video_outstream(bg_nle_project_t * p)
  {
  bg_nle_outstream_t * outstream;
  bg_nle_op_outstream_t * d;

  outstream = create_video_outstream(p);
  
  d = calloc(1, sizeof(*d));

  d->outstream = outstream;
  d->index = p->num_outstreams;
  
  edited(p, BG_NLE_EDIT_ADD_OUTSTREAM, d, NULL);
  
  }

static bg_nle_outstream_t * create_audio_outstream(bg_nle_project_t * p)
  {
  bg_nle_outstream_t * outstream;
  char * tmp_string;
  int num, i;
  outstream = bg_nle_outstream_create(BG_NLE_TRACK_AUDIO);

  if(!p->current_audio_outstream)
    outstream->flags |= BG_NLE_TRACK_PLAYBACK;
  
  /* Create ID */
  outstream->id = create_outstream_id(p->outstreams, p->num_outstreams);
  
  bg_cfg_section_transfer(p->audio_outstream_section,
                          outstream->section);

  /* Get default label */
  num = 0;
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i]->type == BG_NLE_TRACK_AUDIO)
      num++;
    }
  
  tmp_string = bg_sprintf("Audio outstream %d", num+1);
  bg_nle_outstream_set_name(outstream, tmp_string);
  free(tmp_string);
  return outstream;
  }


void bg_nle_project_add_audio_outstream(bg_nle_project_t * p)
  {
  bg_nle_outstream_t * outstream;
  bg_nle_op_outstream_t * d;

  d = calloc(1, sizeof(*d));
  
  outstream = create_audio_outstream(p);
  d->outstream = outstream;
  d->index = p->num_outstreams;
  
  edited(p, BG_NLE_EDIT_ADD_OUTSTREAM, d, NULL);
  
  }

// BG_NLE_EDIT_DELETE_OUTSTREAM

void bg_nle_project_delete_outstream(bg_nle_project_t * p, bg_nle_outstream_t * t)
  {
  bg_nle_op_outstream_t * d;
  d = calloc(1, sizeof(*d));
  d->index = bg_nle_project_outstream_index(p, t);
  d->outstream = t;
  edited(p, BG_NLE_EDIT_DELETE_OUTSTREAM, d, NULL);
  }

// BG_NLE_EDIT_MOVE_OUTSTREAM



void bg_nle_project_move_outstream(bg_nle_project_t * p, int old_pos, int new_pos)
  {
  bg_nle_op_move_outstream_t * d;
  d = calloc(1, sizeof(*d));
  d->old_index = old_pos;
  d->new_index = new_pos;
  edited(p, BG_NLE_EDIT_MOVE_OUTSTREAM, d, NULL);
  }

// BG_NLE_EDIT_CHANGE_SELECTION

void bg_nle_project_set_selection(bg_nle_project_t * p, bg_nle_time_range_t * selection,
                                  int64_t cursor_pos)
  {
  bg_nle_op_change_range_t * d;

  if((p->selection.start == selection->start) &&
     (p->selection.end == selection->end))
    return;

  
  d = calloc(1, sizeof(*d));

  bg_nle_time_range_copy(&d->old_range, &p->selection);
  bg_nle_time_range_copy(&d->new_range, selection);

  d->old_cursor_pos = p->cursor_pos;
  d->new_cursor_pos = cursor_pos;
  
  edited(p, BG_NLE_EDIT_CHANGE_SELECTION, d, NULL);
  }

// BG_NLE_EDIT_CHANGE_IN_OUT

void bg_nle_project_set_in_out(bg_nle_project_t * p, bg_nle_time_range_t * in_out)
  {
  bg_nle_op_change_range_t * d;
  
  if((p->in_out.start == in_out->start) &&
     (p->in_out.end == in_out->end))
    return;
  
  d = calloc(1, sizeof(*d));

  bg_nle_time_range_copy(&d->old_range, &p->in_out);
  bg_nle_time_range_copy(&d->new_range, in_out);
  
  edited(p, BG_NLE_EDIT_CHANGE_IN_OUT, d, NULL);
  }



// BG_NLE_EDIT_CHANGE_VISIBLE

void bg_nle_project_set_visible(bg_nle_project_t * p, bg_nle_time_range_t * visible)
  {
  bg_nle_op_change_range_t * d;

  if((p->visible.start == visible->start) &&
     (p->visible.end == visible->end))
    return;
  
  d = calloc(1, sizeof(*d));

  bg_nle_time_range_copy(&d->old_range, &p->visible);
  bg_nle_time_range_copy(&d->new_range, visible);
  
  edited(p, BG_NLE_EDIT_CHANGE_VISIBLE, d, NULL);
  }

// BG_NLE_EDIT_CHANGE_ZOOM

void bg_nle_project_set_zoom(bg_nle_project_t * p,
                             bg_nle_time_range_t * visible)
  {
  bg_nle_op_change_range_t * d;

  d = calloc(1, sizeof(*d));

  bg_nle_time_range_copy(&d->old_range, &p->visible);
  bg_nle_time_range_copy(&d->new_range, visible);
  
  edited(p, BG_NLE_EDIT_CHANGE_ZOOM, d, NULL);
  
  }

// BG_NLE_EDIT_TRACK_FLAGS

void bg_nle_project_set_track_flags(bg_nle_project_t * p,
                                    bg_nle_track_t * t, int flags)
  {
  bg_nle_op_track_flags_t * d;

  if(t->flags == flags)
    return;

  if(!(flags & BG_NLE_TRACK_EXPANDED))
    flags &= ~BG_NLE_TRACK_SELECTED;
  
  d = calloc(1, sizeof(*d));
  d->old_flags = t->flags;
  d->new_flags = flags;
  d->track = t;
  edited(p, BG_NLE_EDIT_TRACK_FLAGS, d, NULL);
  
  }

// BG_NLE_EDIT_OUTSTREAM_FLAGS

void bg_nle_project_set_outstream_flags(bg_nle_project_t * p,
                                        bg_nle_outstream_t * t, int flags)
  {
  bg_nle_op_outstream_flags_t * d;
  if(t->flags == flags)
    return;
  d = calloc(1, sizeof(*d));
  d->old_flags = t->flags;
  d->new_flags = flags;
  d->outstream = t;
  edited(p, BG_NLE_EDIT_OUTSTREAM_FLAGS, d, NULL);
  
  }

// BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK

void bg_nle_project_attach_track(bg_nle_project_t * p,
                                 bg_nle_outstream_t * outstream,
                                 bg_nle_track_t * track)
  {
  bg_nle_op_outstream_track_t * d;
  if(bg_nle_outstream_has_track(outstream, track))
    return;
  d = calloc(1, sizeof(*d));
  d->track = track;
  d->outstream = outstream;
  edited(p, BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK, d, NULL);
  }

// BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK

void bg_nle_project_detach_track(bg_nle_project_t * p,
                                 bg_nle_outstream_t * outstream,
                                 bg_nle_track_t * track)
  {
  bg_nle_op_outstream_track_t * d;
  if(!bg_nle_outstream_has_track(outstream, track))
    return;
  d = calloc(1, sizeof(*d));
  d->track = track;
  d->outstream = outstream;
  edited(p, BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK, d, NULL);
  }

// BG_NLE_EDIT_OUTSTREAM_MAKE_CURRENT

void bg_nle_project_outstream_make_current(bg_nle_project_t * p,
                                           bg_nle_track_type_t type,
                                           bg_nle_outstream_t * new_outstream)
  {
  bg_nle_op_outstream_make_current_t * d;
  bg_nle_outstream_t * old_outstream = NULL;

  switch(type)
    {
    case BG_NLE_TRACK_AUDIO:
      old_outstream = p->current_audio_outstream;
      break;
    case BG_NLE_TRACK_VIDEO:
      old_outstream = p->current_video_outstream;
      break;
    case BG_NLE_TRACK_NONE:
      break;
    }
  
  if(old_outstream == new_outstream)
    return;

  d = calloc(1, sizeof(*d));
  d->old_outstream = old_outstream;
  d->new_outstream = new_outstream;
  d->type = type;
  edited(p, BG_NLE_EDIT_OUTSTREAM_MAKE_CURRENT, d, NULL);
  }

// BG_NLE_EDIT_PROJECT_PARAMETERS

bg_cfg_section_t * bg_nle_project_set_parameters_start(bg_nle_project_t * p)
  {
  return bg_cfg_section_copy(p->section);
  }

void bg_nle_project_set_parameters_end(bg_nle_project_t * p,
                                       bg_cfg_section_t * s,
                                       int changed)
  {
  if(changed)
    {
    bg_nle_op_parameters_t * d;
    d = calloc(1, sizeof(*d));
    d->old_section = bg_cfg_section_copy(p->section);
    d->new_section = s;
    edited(p, BG_NLE_EDIT_PROJECT_PARAMETERS, d, NULL);
    }
  else
    {
    bg_cfg_section_destroy(s);
    }
    
  }

// BG_NLE_EDIT_TRACK_PARAMETERS

bg_cfg_section_t * bg_nle_project_set_track_parameters_start(bg_nle_project_t * p,
                                               bg_nle_track_t * track)
  {
  return bg_cfg_section_copy(track->section);
  }

void bg_nle_project_set_track_parameters_end(bg_nle_project_t * p,
                                             bg_cfg_section_t * s,
                                             int changed,
                                             bg_nle_track_t * track)
  {
  if(changed)
    {
    bg_nle_op_parameters_t * d;
    d = calloc(1, sizeof(*d));
    d->old_section = bg_cfg_section_copy(track->section);
    d->new_section = s;
    d->index = bg_nle_project_track_index(p, track);
    edited(p, BG_NLE_EDIT_TRACK_PARAMETERS, d, NULL);
    }
  else
    {
    bg_cfg_section_destroy(s);
    }

  }

// BG_NLE_EDIT_OUTSTREAM_PARAMETERS

bg_cfg_section_t * bg_nle_project_set_outstream_parameters_start(bg_nle_project_t * p,
                                                   bg_nle_outstream_t * outstream)
  {
  return bg_cfg_section_copy(outstream->section);
  }

void bg_nle_project_set_outstream_parameters_end(bg_nle_project_t * p,
                                                 bg_cfg_section_t * s,
                                                 int changed,
                                                 bg_nle_outstream_t * outstream)
  {
  if(changed)
    {
    bg_nle_op_parameters_t * d;
    d = calloc(1, sizeof(*d));
    d->old_section = bg_cfg_section_copy(outstream->section);
    d->new_section = s;
    d->index = bg_nle_project_outstream_index(p, outstream);
    edited(p, BG_NLE_EDIT_OUTSTREAM_PARAMETERS, d, NULL);
    }
  else
    {
    bg_cfg_section_destroy(s);
    }
  }

// BG_NLE_EDIT_ADD_FILE,

static void add_file(bg_nle_project_t * p,
                     bg_nle_file_t * file, int * id)
  {
  bg_nle_op_file_t * d;

  d = calloc(1, sizeof(*d));
  d->index = p->media_list->num_files;
  d->file  = file;
  edited(p, BG_NLE_EDIT_ADD_FILE, d, id);
  }

void bg_nle_project_add_file(bg_nle_project_t * p,
                             bg_nle_file_t * file)
  {
  add_file(p, file, NULL);
  }

// BG_NLE_EDIT_DELETE_FILE,

void bg_nle_project_delete_file(bg_nle_project_t * p,
                                int index)
  {
  bg_nle_op_file_t * d;
  d = calloc(1, sizeof(*d));
  d->index = index;
  d->file = p->media_list->files[index];
  edited(p, BG_NLE_EDIT_DELETE_FILE, d, NULL);
  }

// BG_NLE_EDIT_SET_CURSOR_POS,

void bg_nle_project_set_cursor_pos(bg_nle_project_t * p, int64_t cursor_pos)
  {
  bg_nle_op_cursor_pos_t * d;
  d = calloc(1, sizeof(*d));
  d->old_pos = p->cursor_pos;
  d->new_pos = cursor_pos;
  
  edited(p, BG_NLE_EDIT_SET_CURSOR_POS, d, NULL);
  
  }

void bg_nle_project_set_edit_mode(bg_nle_project_t * p, int mode)
  {
  bg_nle_op_edit_mode_t * d;

  if(p->edit_mode == mode)
    return;

  d = calloc(1, sizeof(*d));
  d->old_mode = p->edit_mode;
  d->new_mode = mode;
  edited(p, BG_NLE_EDIT_SET_EDIT_MODE, d, NULL);
  
  }

// BG_NLE_EDIT_SPLIT_SEGMENT

// BG_NLE_EDIT_COMBINE_SEGMENT

// BG_NLE_EDIT_MOVE_SEGMENT,

static void move_segment(bg_nle_project_t * p,
                         bg_nle_track_t * t, int index,
                         int64_t new_dst_pos, int * id)
  {
  bg_nle_op_move_segment_t * d;
  
  d = calloc(1, sizeof(*d));
  d->old_dst_pos = t->segments[index].dst_pos;
  d->new_dst_pos = new_dst_pos;
  edited(p, BG_NLE_EDIT_MOVE_SEGMENT, d, id);
  }

void bg_nle_project_move_segment(bg_nle_project_t * p,
                                 bg_nle_track_t * t, int index,
                                 int64_t new_dst_pos)
  {
  move_segment(p, t, index, new_dst_pos, NULL);
  }

// BG_NLE_EDIT_DELETE_SEGMENT,

static void delete_segment(bg_nle_project_t * p,
                           bg_nle_track_t * t, int index, int * id)
  {
  bg_nle_op_segment_t * d;
  d = calloc(1, sizeof(*d));
  d->index = index;
  memcpy(&d->seg, t->segments + index, sizeof(d->seg));
  edited(p, BG_NLE_EDIT_DELETE_SEGMENT, d, id);
  }

void bg_nle_project_delete_segment(bg_nle_project_t * p,
                                   bg_nle_track_t * t, int index)
  {
  delete_segment(p, t, index, NULL);
  }

// BG_NLE_EDIT_INSERT_SEGMENT,

static void insert_segment(bg_nle_project_t * p,
                           bg_nle_track_t * t, int index,
                           const bg_nle_track_segment_t * seg,
                           int * id)
  {
  bg_nle_op_segment_t * d;
  d = calloc(1, sizeof(*d));
  d->index = index;
  memcpy(&d->seg, seg, sizeof(d->seg));
  edited(p, BG_NLE_EDIT_INSERT_SEGMENT, d, id);
  }

// BG_NLE_EDIT_CHANGE_SEGMENT,

static void change_segment(bg_nle_project_t * p,
                           bg_nle_track_t * t, int index,
                           int64_t new_src_pos,
                           gavl_time_t new_dst_pos,
                           gavl_time_t new_len,
                           int * id)
  {
  bg_nle_op_change_segment_t * d;
  
  d = calloc(1, sizeof(*d));
  d->old_src_pos = t->segments[index].src_pos;
  d->old_dst_pos = t->segments[index].dst_pos;
  d->old_len     = t->segments[index].len;
  
  d->new_src_pos = new_src_pos;
  d->new_dst_pos = new_dst_pos;
  d->new_len     = new_len;
  edited(p, BG_NLE_EDIT_CHANGE_SEGMENT, d, id);
  }


void bg_nle_project_change_segment(bg_nle_project_t * p,
                                   bg_nle_track_t * t, int index,
                                   int64_t new_src_pos,
                                   int64_t new_dst_pos,
                                   int64_t new_len)
  {
  change_segment(p, t, index, new_src_pos, new_dst_pos,
                 new_len, NULL);
  }

/* Paste */

void bg_nle_project_paste(bg_nle_project_t * p, bg_nle_clipboard_t * c)
  {
  fprintf(stderr, "bg_nle_project_paste\n");

  /* 1. Merge files */
  
  /* 2. Paste tracks */
  
  }
