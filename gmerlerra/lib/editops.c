#include <string.h>

#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>

#include <track.h>
#include <project.h>
#include <editops.h>

static void edit_add_track(bg_nle_project_t * p, bg_nle_op_track_t * op)
  {
  int i;
  bg_nle_project_insert_track(p, op->track, op->index);

  if(!op->num_outstreams)
    {
    for(i = 0; i < p->num_outstreams; i++)
      {
      if(p->outstreams[i]->type == op->track->type)
        bg_nle_outstream_attach_track(p->outstreams[i],
                                      op->track);
      }
    }
  else if(op->num_outstreams > 0)
    {
    for(i = 0; i < op->num_outstreams; i++)
      bg_nle_outstream_attach_track(op->outstreams[i],
                                    op->track);
    }
  
  p->changed_flags |= BG_NLE_PROJECT_TRACKS_CHANGED;
  }

static void edit_delete_track(bg_nle_project_t * p, bg_nle_op_track_t * op)
  {
  int i;
  /* Detach from source tracks */
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(bg_nle_outstream_has_track(p->outstreams[i], op->track))
      bg_nle_outstream_detach_track(p->outstreams[i], op->track);
    }
  
  /* First call callback */
  if(op->index < p->num_tracks - 1)
    memmove(p->tracks + op->index, p->tracks + op->index + 1,
            (p->num_tracks - 1 - op->index) * sizeof(*p->tracks));


  p->num_tracks--;

  }

static void edit_move_track(bg_nle_project_t * p, bg_nle_op_move_track_t * op)
  {
  bg_nle_track_t * t;

  /* Remove from array */
  t = p->tracks[op->old_index];
  if(op->old_index < p->num_tracks-1)
    memmove(p->tracks + op->old_index, p->tracks + op->old_index + 1,
            (p->num_tracks-1-op->old_index) * sizeof(*p->tracks));

  /* Insert to array */
  if(op->new_index < p->num_tracks-1)
    memmove(p->tracks + op->new_index + 1, p->tracks + op->new_index,
            (p->num_tracks-1-op->new_index) * sizeof(*p->tracks));
  p->tracks[op->new_index] = t;
  }

static void edit_add_outstream(bg_nle_project_t * p, bg_nle_op_outstream_t * op)
  {
  int i;
  bg_nle_project_insert_outstream(p, op->outstream, op->index);
  p->changed_flags |= BG_NLE_PROJECT_OUTSTREAMS_CHANGED;
  
  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i]->type == op->outstream->type)
      bg_nle_outstream_attach_track(op->outstream,
                                    p->tracks[i]);
    }
  }

static void edit_delete_outstream(bg_nle_project_t * p, bg_nle_op_outstream_t * op)
  {
  if(op->outstream->flags & BG_NLE_TRACK_PLAYBACK)
    {
    switch(op->outstream->type)
      {
      case BG_NLE_TRACK_AUDIO:
        p->current_audio_outstream = NULL;
        break;
      case BG_NLE_TRACK_VIDEO:
        p->current_video_outstream = NULL;
        break;
      case BG_NLE_TRACK_NONE:
        break;
      }
    }
  
  if(op->index < p->num_outstreams - 1)
    memmove(p->outstreams + op->index,
            p->outstreams + op->index + 1, (p->num_outstreams - 1 - op->index) *
            sizeof(*p->outstreams));
  p->num_outstreams--;
  }

static void edit_move_outstream(bg_nle_project_t * p, bg_nle_op_move_outstream_t * op)
  {
  bg_nle_outstream_t * t;

  /* Remove from array */
  t = p->outstreams[op->old_index];
  if(op->old_index < p->num_outstreams-1)
    memmove(p->outstreams + op->old_index, p->outstreams + op->old_index + 1,
            (p->num_outstreams-1-op->old_index) * sizeof(*p->outstreams));

  /* Insert to array */
  if(op->new_index < p->num_outstreams-1)
    memmove(p->outstreams + op->new_index + 1, p->outstreams + op->new_index,
            (p->num_outstreams-1-op->new_index) * sizeof(*p->outstreams));
  p->outstreams[op->new_index] = t;
  }

static void edit_change_visible(bg_nle_project_t * p, bg_nle_op_change_range_t * op)
  {
  /* This affects the GUI only */
  bg_nle_time_range_copy(&p->visible, &op->new_range);
  }

static void edit_change_selection(bg_nle_project_t * p, bg_nle_op_change_range_t * op)
  {
  /* This affects the GUI only */
  bg_nle_time_range_copy(&p->selection, &op->new_range);
  p->cursor_pos = op->new_cursor_pos;
  }

static void edit_change_in_out(bg_nle_project_t * p, bg_nle_op_change_range_t * op)
  {
  /* This affects the GUI only */
  bg_nle_time_range_copy(&p->in_out, &op->new_range);
  }

static void edit_change_zoom(bg_nle_project_t * p, bg_nle_op_change_range_t * op)
  {
  /* This affects the GUI only */
  bg_nle_time_range_copy(&p->visible, &op->new_range);
  }

static void edit_track_flags(bg_nle_project_t * p, bg_nle_op_track_flags_t * op)
  {
  op->track->flags = op->new_flags;
  
  }

static void edit_outstream_flags(bg_nle_project_t * p, bg_nle_op_outstream_flags_t * op)
  {
  op->outstream->flags = op->new_flags;
  }

static void edit_outstream_attach_track(bg_nle_project_t * p,
                                        bg_nle_op_outstream_track_t * op)
  {
  bg_nle_outstream_attach_track(op->outstream, op->track);
  }

static void edit_outstream_detach_track(bg_nle_project_t * p,
                                        bg_nle_op_outstream_track_t * op)
  {
  bg_nle_outstream_detach_track(op->outstream, op->track);
  }

static void edit_outstream_make_current(bg_nle_project_t * p,
                                        bg_nle_op_outstream_make_current_t * op)
  {
  switch(op->type)
    {
    case BG_NLE_TRACK_AUDIO:
      if(p->current_audio_outstream)
        p->current_audio_outstream->flags &= ~BG_NLE_TRACK_PLAYBACK;
      p->current_audio_outstream = op->new_outstream;
      if(p->current_audio_outstream)
        p->current_audio_outstream->flags |= BG_NLE_TRACK_PLAYBACK;
      break;
    case BG_NLE_TRACK_VIDEO:
      if(p->current_video_outstream)
        p->current_video_outstream->flags &= ~BG_NLE_TRACK_PLAYBACK;
      p->current_video_outstream = op->new_outstream;
      if(p->current_video_outstream)
        p->current_video_outstream->flags |= BG_NLE_TRACK_PLAYBACK;
      break;
    case BG_NLE_TRACK_NONE:
      break;
    }
  }

static void edit_project_parameters(bg_nle_project_t * p,
                                    bg_nle_op_parameters_t * op)
  {
  bg_cfg_section_transfer(op->new_section, p->section);

  bg_cfg_section_apply(p->cache_section,
                       p->cache_parameters,
                       bg_nle_project_set_cache_parameter,
                       p);
  }

static void edit_track_parameters(bg_nle_project_t * p,
                                  bg_nle_op_parameters_t * op)
  {
  bg_cfg_section_transfer(op->new_section, p->tracks[op->index]->section);
  }

static void edit_outstream_parameters(bg_nle_project_t * p,
                                      bg_nle_op_parameters_t * op)
  {
  bg_cfg_section_transfer(op->new_section, p->outstreams[op->index]->section);
  }

static void edit_add_file(bg_nle_project_t * p,
                          bg_nle_op_file_t * op)
  {
  bg_nle_media_list_insert(p->media_list,
                           op->file, op->index);
  }

static void edit_delete_file(bg_nle_project_t * p,
                             bg_nle_op_file_t * op)
  {
  bg_nle_media_list_delete(p->media_list,
                           op->index);
  }

static void
edit_set_cursor_pos(bg_nle_project_t * p, bg_nle_op_cursor_pos_t * op)
  {
  p->cursor_pos = op->new_pos;
  }

static void
edit_set_edit_mode(bg_nle_project_t * p, bg_nle_op_edit_mode_t * op)
  {
  p->edit_mode = op->new_mode;
  }

static void
edit_split_segment(bg_nle_project_t * p, bg_nle_op_split_segment_t * op)
  {
  bg_nle_track_t * t;
  bg_nle_track_segment_t * s1;
  bg_nle_track_segment_t * s2;

  t = op->t;
  
  /* Allocate */
  bg_nle_track_alloc_segments(t, 1);

  /* Move segments */
  if(op->segment < t->num_segments - 1)
    {
    memmove(t->segments + op->segment + 2,
            t->segments + op->segment + 1,
            sizeof(*t->segments) *
            (t->num_segments - 1 - op->segment));
    }
  s1 = &t->segments[op->segment];
  s2 = &t->segments[op->segment+1];

  /* Copy stuff */
  memcpy(s2, s1, sizeof(*s1));
  
  /* Set times */
  s2->dst_pos += op->time;
  s2->src_pos += gavl_time_scale(s1->scale, op->time + 5);
  s2->len -= op->time;
  s1->len -= s2->len;
  t->num_segments++;
  }

static void
edit_combine_segment(bg_nle_project_t * p, bg_nle_op_split_segment_t * op)
  {
  bg_nle_track_t * t;
  bg_nle_track_segment_t * s1;
  bg_nle_track_segment_t * s2;

  t = op->t;
  s1 = &t->segments[op->segment];
  s2 = &t->segments[op->segment+1];

  /* Set length */
  s1->len += s2->len;

  /* Move segments */
  if(op->segment < t->num_segments - 2)
    {
    memmove(t->segments + op->segment + 1,
            t->segments + op->segment + 2,
            sizeof(*t->segments) *
            (t->num_segments - 2 - op->segment));
    }
  t->num_segments--;
  }

static void
edit_insert_segment(bg_nle_project_t * p, bg_nle_op_segment_t * op)
  {
  bg_nle_track_t * t;
  t = op->t;

  /* Allocate */
  bg_nle_track_alloc_segments(t, 1);

  /* Move segments */
  if(op->index < t->num_segments - 1)
    {
    memmove(t->segments + op->index + 1,
            t->segments + op->index,
            sizeof(*t->segments) *
            (t->num_segments - 1 - op->index));
    }

  memcpy(t->segments + op->index,
         &op->seg, sizeof(op->seg));
  
  t->num_segments++;
  }

static void
edit_delete_segment(bg_nle_project_t * p, bg_nle_op_segment_t * op)
  {
  bg_nle_track_t * t;
  t = op->t;

  /* Move segments */
  if(op->index < t->num_segments - 1)
    {
    memmove(t->segments + op->index,
            t->segments + op->index + 1,
            sizeof(*t->segments) *
            (t->num_segments - 1 - op->index));
    }
  t->num_segments--;
  }

static void
edit_move_segment(bg_nle_project_t * p, bg_nle_op_move_segment_t * op)
  {
  bg_nle_track_t * t;
  t = op->t;
  t->segments[op->index].dst_pos = op->new_dst_pos;
  }

static void
edit_change_segment(bg_nle_project_t * p, bg_nle_op_change_segment_t * op)
  {
  bg_nle_track_t * t;
  t = op->t;
  t->segments[op->index].dst_pos = op->new_dst_pos;
  t->segments[op->index].src_pos = op->new_src_pos;
  t->segments[op->index].len      = op->new_len;
  }

void bg_nle_project_edit(bg_nle_project_t * p,
                         bg_nle_undo_data_t * data)
  {
  switch(data->op)
    {
    case BG_NLE_EDIT_ADD_TRACK:
      edit_add_track(p, data->data);
      break;
    case BG_NLE_EDIT_DELETE_TRACK:
      edit_delete_track(p, data->data);
      break;
    case BG_NLE_EDIT_MOVE_TRACK:
      edit_move_track(p, data->data);
      break;
    case BG_NLE_EDIT_ADD_OUTSTREAM:
      edit_add_outstream(p, data->data);
      break;
    case BG_NLE_EDIT_DELETE_OUTSTREAM:
      edit_delete_outstream(p, data->data);
      break;
    case BG_NLE_EDIT_MOVE_OUTSTREAM:
      edit_move_outstream(p, data->data);
      break;
    case BG_NLE_EDIT_CHANGE_SELECTION:
      edit_change_selection(p, data->data);
      break;
    case BG_NLE_EDIT_CHANGE_IN_OUT:
      edit_change_in_out(p, data->data);
      break;
    case BG_NLE_EDIT_CHANGE_VISIBLE:
      edit_change_visible(p, data->data);
      break;
    case BG_NLE_EDIT_CHANGE_ZOOM:
      edit_change_zoom(p, data->data);
      break;
    case BG_NLE_EDIT_TRACK_FLAGS:
      edit_track_flags(p, data->data);
      break;
    case BG_NLE_EDIT_OUTSTREAM_FLAGS:
      edit_outstream_flags(p, data->data);
      break;
    case BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK:
      edit_outstream_attach_track(p, data->data);
      break;
    case BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK:
      edit_outstream_detach_track(p, data->data);
      break;
    case BG_NLE_EDIT_OUTSTREAM_MAKE_CURRENT:
      edit_outstream_make_current(p, data->data);
      break;
    case BG_NLE_EDIT_PROJECT_PARAMETERS:
      edit_project_parameters(p, data->data);
      break;
    case BG_NLE_EDIT_TRACK_PARAMETERS:
      edit_track_parameters(p, data->data);
      break;
    case BG_NLE_EDIT_OUTSTREAM_PARAMETERS:
      edit_outstream_parameters(p, data->data);
      break;
    case BG_NLE_EDIT_ADD_FILE:
      edit_add_file(p, data->data);
      break;
    case BG_NLE_EDIT_DELETE_FILE:
      edit_delete_file(p, data->data);
      break;
    case BG_NLE_EDIT_SET_CURSOR_POS:
      edit_set_cursor_pos(p, data->data);
      break;
    case BG_NLE_EDIT_SET_EDIT_MODE:
      edit_set_edit_mode(p, data->data);
      break;
    case BG_NLE_EDIT_SPLIT_SEGMENT:
      edit_split_segment(p, data->data);
      break;
    case BG_NLE_EDIT_COMBINE_SEGMENT:
      edit_combine_segment(p, data->data);
      break;
    case BG_NLE_EDIT_INSERT_SEGMENT:
      edit_insert_segment(p, data->data);
      break;
    case BG_NLE_EDIT_DELETE_SEGMENT:
      edit_delete_segment(p, data->data);
      break;
    case BG_NLE_EDIT_MOVE_SEGMENT:
      edit_move_segment(p, data->data);
      break;
    case BG_NLE_EDIT_CHANGE_SEGMENT:
      edit_change_segment(p, data->data);
      break;
    }
  }

#define SWAP(x1, x2) swp = x1; x1 = x2; x2 = swp;

void bg_nle_undo_data_reverse(bg_nle_undo_data_t * data)
  {
  switch(data->op)
    {
    case BG_NLE_EDIT_ADD_TRACK:
      data->op = BG_NLE_EDIT_DELETE_TRACK;
      break;
    case BG_NLE_EDIT_DELETE_TRACK:
      data->op = BG_NLE_EDIT_ADD_TRACK;
      break;
    case BG_NLE_EDIT_MOVE_TRACK:
      {
      bg_nle_op_move_track_t * d = data->data;
      int swp;
      SWAP(d->old_index, d->new_index);
      }
      break;
    case BG_NLE_EDIT_ADD_OUTSTREAM:
      data->op = BG_NLE_EDIT_DELETE_OUTSTREAM;
      break;
    case BG_NLE_EDIT_DELETE_OUTSTREAM:
      data->op = BG_NLE_EDIT_ADD_OUTSTREAM;
      break;
    case BG_NLE_EDIT_MOVE_OUTSTREAM:
      {
      bg_nle_op_move_outstream_t * d = data->data;
      int swp;
      SWAP(d->old_index, d->new_index);
      }
      break;
    case BG_NLE_EDIT_CHANGE_SELECTION:
    case BG_NLE_EDIT_CHANGE_IN_OUT:
    case BG_NLE_EDIT_CHANGE_VISIBLE:
    case BG_NLE_EDIT_CHANGE_ZOOM:
      {
      int64_t swp;
      bg_nle_op_change_range_t * d = data->data;
      bg_nle_time_range_swap(&d->old_range, &d->new_range);
      SWAP(d->old_cursor_pos, d->new_cursor_pos);
      }
      break;
    case BG_NLE_EDIT_TRACK_FLAGS:
      {
      bg_nle_op_track_flags_t * d = data->data;
      int swp;
      SWAP(d->old_flags, d->new_flags);
      }
      break;
    case BG_NLE_EDIT_OUTSTREAM_FLAGS:
      {
      bg_nle_op_outstream_flags_t * d = data->data;
      int swp;
      SWAP(d->old_flags, d->new_flags);
      }
      break;
    case BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK:
      data->op = BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK;
      break;
    case BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK:
      data->op = BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK;
      break;
    case BG_NLE_EDIT_OUTSTREAM_MAKE_CURRENT:
      {
      bg_nle_outstream_t * swp;
      bg_nle_op_outstream_make_current_t * d = data->data;
      SWAP(d->old_outstream, d->new_outstream);
      }
      break;
    case BG_NLE_EDIT_PROJECT_PARAMETERS:
    case BG_NLE_EDIT_TRACK_PARAMETERS:
    case BG_NLE_EDIT_OUTSTREAM_PARAMETERS:
      {
      bg_cfg_section_t * swp;
      bg_nle_op_parameters_t * d = data->data;
      SWAP(d->old_section, d->new_section);
      }
      break;
    case BG_NLE_EDIT_ADD_FILE:
      data->op = BG_NLE_EDIT_DELETE_FILE;
      break;
    case BG_NLE_EDIT_DELETE_FILE:
      data->op = BG_NLE_EDIT_ADD_FILE;
      break;
    case BG_NLE_EDIT_SET_CURSOR_POS:
      {
      int64_t swp;
      bg_nle_op_cursor_pos_t * d = data->data;
      SWAP(d->old_pos, d->new_pos);
      }
      break;
    case BG_NLE_EDIT_SET_EDIT_MODE:
      {
      bg_nle_op_edit_mode_t * d = data->data;
      int swp;
      SWAP(d->old_mode, d->new_mode);
      }
      break;
    case BG_NLE_EDIT_SPLIT_SEGMENT:
      data->op = BG_NLE_EDIT_COMBINE_SEGMENT;
      break;
    case BG_NLE_EDIT_COMBINE_SEGMENT:
      data->op = BG_NLE_EDIT_SPLIT_SEGMENT;
      break;
    case BG_NLE_EDIT_INSERT_SEGMENT:
      data->op = BG_NLE_EDIT_DELETE_SEGMENT;
      break;
    case BG_NLE_EDIT_DELETE_SEGMENT:
      data->op = BG_NLE_EDIT_INSERT_SEGMENT;
      break;
    case BG_NLE_EDIT_MOVE_SEGMENT:
      {
      int64_t swp;
      bg_nle_op_move_segment_t * d = data->data;
      SWAP(d->old_dst_pos, d->new_dst_pos);
      }
    case BG_NLE_EDIT_CHANGE_SEGMENT:
      {
      int64_t swp;
      bg_nle_op_change_segment_t * d = data->data;
      SWAP(d->old_src_pos, d->new_src_pos);
      SWAP(d->old_dst_pos, d->new_dst_pos);
      SWAP(d->old_len, d->new_len);
      }
    }
  }

void bg_nle_undo_data_destroy(bg_nle_undo_data_t * data)
  {
  switch(data->op)
    {
    case BG_NLE_EDIT_ADD_TRACK:
    case BG_NLE_EDIT_MOVE_TRACK:
    case BG_NLE_EDIT_ADD_OUTSTREAM:
    case BG_NLE_EDIT_MOVE_OUTSTREAM:
    case BG_NLE_EDIT_CHANGE_SELECTION:
    case BG_NLE_EDIT_CHANGE_IN_OUT:
    case BG_NLE_EDIT_CHANGE_VISIBLE:
    case BG_NLE_EDIT_CHANGE_ZOOM:
    case BG_NLE_EDIT_TRACK_FLAGS:
    case BG_NLE_EDIT_OUTSTREAM_FLAGS:
    case BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK:
    case BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK:
    case BG_NLE_EDIT_OUTSTREAM_MAKE_CURRENT:
    case BG_NLE_EDIT_ADD_FILE:
    case BG_NLE_EDIT_SET_CURSOR_POS:
    case BG_NLE_EDIT_SET_EDIT_MODE:
    case BG_NLE_EDIT_SPLIT_SEGMENT:
    case BG_NLE_EDIT_COMBINE_SEGMENT:
    case BG_NLE_EDIT_INSERT_SEGMENT:
    case BG_NLE_EDIT_DELETE_SEGMENT:
    case BG_NLE_EDIT_MOVE_SEGMENT:
    case BG_NLE_EDIT_CHANGE_SEGMENT:
      break;
    case BG_NLE_EDIT_DELETE_TRACK:
      {
      bg_nle_op_track_t * d = data->data;
      bg_nle_track_destroy(d->track);
      if(d->outstreams)
        free(d->outstreams);
      }
      break;
    case BG_NLE_EDIT_DELETE_OUTSTREAM:
      {
      bg_nle_op_outstream_t * d = data->data;
      bg_nle_outstream_destroy(d->outstream);
      }
      break;
    case BG_NLE_EDIT_PROJECT_PARAMETERS:
    case BG_NLE_EDIT_TRACK_PARAMETERS:
    case BG_NLE_EDIT_OUTSTREAM_PARAMETERS:
      {
      bg_nle_op_parameters_t * d = data->data;
      bg_cfg_section_destroy(d->old_section);
      bg_cfg_section_destroy(d->new_section);
      }
      break;
    case BG_NLE_EDIT_DELETE_FILE:
      {
      bg_nle_op_file_t * d = data->data;
      bg_nle_file_destroy(d->file);
      }
      break;
    }
  if(data->data)
    free(data->data);
  free(data);
  }

void bg_nle_project_push_undo(bg_nle_project_t * p, bg_nle_undo_data_t * data)
  {
  /* Check whether to merge undo data */
  if(p->undo && (p->undo->op == data->op))
    {
    switch(data->op)
      {
      case BG_NLE_EDIT_ADD_TRACK:
      case BG_NLE_EDIT_DELETE_TRACK:
      case BG_NLE_EDIT_MOVE_TRACK:
      case BG_NLE_EDIT_ADD_OUTSTREAM:
      case BG_NLE_EDIT_DELETE_OUTSTREAM:
      case BG_NLE_EDIT_MOVE_OUTSTREAM:
      case BG_NLE_EDIT_CHANGE_ZOOM:
      case BG_NLE_EDIT_TRACK_FLAGS:
      case BG_NLE_EDIT_OUTSTREAM_FLAGS:
      case BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK:
      case BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK:
      case BG_NLE_EDIT_OUTSTREAM_MAKE_CURRENT:
      case BG_NLE_EDIT_PROJECT_PARAMETERS:
      case BG_NLE_EDIT_TRACK_PARAMETERS:
      case BG_NLE_EDIT_OUTSTREAM_PARAMETERS:
      case BG_NLE_EDIT_ADD_FILE:
      case BG_NLE_EDIT_DELETE_FILE:
      case BG_NLE_EDIT_CHANGE_IN_OUT:
      case BG_NLE_EDIT_SET_EDIT_MODE:
      case BG_NLE_EDIT_SPLIT_SEGMENT:
      case BG_NLE_EDIT_COMBINE_SEGMENT:
      case BG_NLE_EDIT_INSERT_SEGMENT:
      case BG_NLE_EDIT_DELETE_SEGMENT:
        break;
      case BG_NLE_EDIT_CHANGE_SELECTION:
      case BG_NLE_EDIT_CHANGE_VISIBLE:
        {
        bg_nle_op_change_range_t * d1 = p->undo->data;
        bg_nle_op_change_range_t * d2 = data->data;
        bg_nle_time_range_copy(&d1->new_range, &d2->new_range);
        d1->new_cursor_pos = d2->new_cursor_pos;
        bg_nle_undo_data_destroy(data);
        data = NULL;
        }
        break;
      case BG_NLE_EDIT_SET_CURSOR_POS:
        {
        bg_nle_op_cursor_pos_t * d1 = p->undo->data;
        bg_nle_op_cursor_pos_t * d2 = data->data;
        d1->new_pos = d2->new_pos;
        bg_nle_undo_data_destroy(data);
        data = NULL;
        }
        break;
      case BG_NLE_EDIT_MOVE_SEGMENT:
        {
        bg_nle_op_move_segment_t * d1 = p->undo->data;
        bg_nle_op_move_segment_t * d2 = data->data;

        if((d1->t == d2->t) && (d1->index == d2->index))
          {
          d1->new_dst_pos = d2->new_dst_pos;
          bg_nle_undo_data_destroy(data);
          data = NULL;
          }
        }
      case BG_NLE_EDIT_CHANGE_SEGMENT:
        {
        bg_nle_op_change_segment_t * d1 = p->undo->data;
        bg_nle_op_change_segment_t * d2 = data->data;

        if((d1->t == d2->t) && (d1->index == d2->index))
          {
          d1->new_dst_pos = d2->new_dst_pos;
          d1->new_src_pos = d2->new_src_pos;
          bg_nle_undo_data_destroy(data);
          data = NULL;
          }
        }
      }
    }
  if(!data)
    return;

  /* TODO: Check maximum undo level */

  /* Push to stack */

  data->next = p->undo;
  p->undo = data;
  
  }

void bg_nle_project_undo(bg_nle_project_t * p)
  {
  bg_nle_undo_data_t * data = p->undo;

  if(!data)
    return;
  
  p->undo = p->undo->next;
  bg_nle_undo_data_reverse(data);
  bg_nle_project_edit(p, data);
  
  /* Notify GUI */
  p->edit_callback(p, data->op, data->data, p->edit_callback_data);

  /* Push to redo stack */
  data->next = p->redo;
  p->redo = data;
  }

void bg_nle_project_redo(bg_nle_project_t * p)
  {
  bg_nle_undo_data_t * data = p->redo;

  if(!data)
    return;
  
  p->redo = p->redo->next;
  bg_nle_undo_data_reverse(data);
  bg_nle_project_edit(p, data);
  
  /* Notify GUI */
  p->edit_callback(p, data->op, data->data, p->edit_callback_data);

  /* Push to undo stack */
  data->next = p->undo;
  p->undo = data;
  }
