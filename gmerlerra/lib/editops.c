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
    }
  }

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
      swp = d->old_index;
      d->old_index = d->new_index;
      d->new_index = swp;
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
      swp = d->old_index;
      d->old_index = d->new_index;
      d->new_index = swp;
      }
      break;
    case BG_NLE_EDIT_CHANGE_SELECTION:
    case BG_NLE_EDIT_CHANGE_VISIBLE:
    case BG_NLE_EDIT_CHANGE_ZOOM:
      {
      bg_nle_op_change_range_t * d = data->data;
      bg_nle_time_range_swap(&d->old_range, &d->new_range);
      }
      break;
    case BG_NLE_EDIT_TRACK_FLAGS:
      {
      bg_nle_op_track_flags_t * d = data->data;
      int tmp;
      tmp = d->old_flags;
      d->old_flags = d->new_flags;
      d->new_flags = tmp;
      }
      break;
    case BG_NLE_EDIT_OUTSTREAM_FLAGS:
      {
      bg_nle_op_outstream_flags_t * d = data->data;
      int tmp;
      tmp = d->old_flags;
      d->old_flags = d->new_flags;
      d->new_flags = tmp;
      }
      break;
    case BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK:
      data->op = BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK;
      break;
    case BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK:
      data->op = BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK;
      break;
      
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
    case BG_NLE_EDIT_CHANGE_VISIBLE:
    case BG_NLE_EDIT_CHANGE_ZOOM:
    case BG_NLE_EDIT_TRACK_FLAGS:
    case BG_NLE_EDIT_OUTSTREAM_FLAGS:
    case BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK:
    case BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK:
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
        break;
      case BG_NLE_EDIT_CHANGE_SELECTION:
      case BG_NLE_EDIT_CHANGE_VISIBLE:
        {
        bg_nle_op_change_range_t * d1 = p->undo->data;
        bg_nle_op_change_range_t * d2 = data->data;
        bg_nle_time_range_copy(&d1->new_range, &d2->new_range);
        bg_nle_undo_data_destroy(data);
        data = NULL;
        }
        break;
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

  /* Push to redo stack */
  data->next = p->undo;
  p->undo = data;
  }
